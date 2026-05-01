#include "PumpTaskManager.h"

PumpTaskManager::PumpTaskManager()
  : m_head(0)
  , m_tail(0)
  , m_full(false)
  , m_refPump()
  , m_tankPump()
{
  for (uint8_t i = 0; i < MAX_PUMP_TASKS; i++)
  {
    m_queue[i].command = PumpCommand::NONE;
    m_queue[i].duration = 0;
  }
}

PumpTaskManager &PumpTaskManager::getInstance()
{
  static PumpTaskManager instance;
  return instance;
}

void PumpTaskManager::begin()
{
  m_refPump.init(REF_IN1_PIN, REF_IN2_PIN);
  m_refPump.begin();
  Serial.println("[PumpTask] Initialized");
}

void PumpTaskManager::update()
{
  m_refPump.update();
  m_tankPump.update();
  processTask();
}

void PumpTaskManager::forward(uint8_t pumpId)
{
  if (m_full)
    return;

  m_queue[m_tail].command = PumpCommand::FORWARD;
  m_queue[m_tail].duration = 0;
  m_tail = (m_tail + 1) % MAX_PUMP_TASKS;
  m_full = m_tail == m_head;
}

void PumpTaskManager::reverse(uint8_t pumpId)
{
  if (m_full)
    return;

  m_queue[m_tail].command = PumpCommand::REVERSE;
  m_queue[m_tail].duration = 0;
  m_tail = (m_tail + 1) % MAX_PUMP_TASKS;
  m_full = m_tail == m_head;
}

void PumpTaskManager::stop(uint8_t pumpId)
{
  if (m_full)
    return;

  m_queue[m_tail].command = PumpCommand::STOP;
  m_queue[m_tail].duration = 0;
  m_tail = (m_tail + 1) % MAX_PUMP_TASKS;
  m_full = m_tail == m_head;
}

void PumpTaskManager::forwardTimed(uint8_t pumpId, uint32_t durationMs)
{
  if (m_full)
    return;

  m_queue[m_tail].command = PumpCommand::FORWARD_TIMED;
  m_queue[m_tail].duration = durationMs;
  m_tail = (m_tail + 1) % MAX_PUMP_TASKS;
  m_full = m_tail == m_head;
}

void PumpTaskManager::reverseTimed(uint8_t pumpId, uint32_t durationMs)
{
  if (m_full)
    return;

  m_queue[m_tail].command = PumpCommand::REVERSE_TIMED;
  m_queue[m_tail].duration = durationMs;
  m_tail = (m_tail + 1) % MAX_PUMP_TASKS;
  m_full = m_tail == m_head;
}

bool PumpTaskManager::isIdle()
{
  return !m_refPump.isRunning() && !m_tankPump.isRunning() && (m_head == m_tail && !m_full);
}

bool PumpTaskManager::isRunning() const
{
  return m_refPump.isRunning() || m_tankPump.isRunning();
}

PWMPumpController *PumpTaskManager::getPump(uint8_t pumpId)
{
  if (pumpId == 1)
    return &m_refPump;
  if (pumpId == 2)
    return &m_tankPump;
  return nullptr;
}

void PumpTaskManager::processTask()
{
  if (m_head == m_tail && !m_full) {
    return;
  }

  PumpTask &task = m_queue[m_head];
  PWMPumpController *pump = getPump(1);

  if (!pump) {
    Serial.println("[ERROR] Invalid pump ID");
    return;
  }

  if (task.command == PumpCommand::NONE) {
    return;
  }

  Serial.printf("[PROCESS] Command: %d, Duration: %lu\n", (int)task.command, task.duration);

  switch (task.command)
  {
    case PumpCommand::FORWARD:
      Serial.println("[TASK] Forward");
      pump->forward();
      break;

    case PumpCommand::REVERSE:
      Serial.println("[TASK] Reverse");
      pump->reverse();
      break;

    case PumpCommand::STOP:
      Serial.println("[TASK] Stop");
      pump->stop();
      break;

    case PumpCommand::FORWARD_TIMED:
      Serial.printf("[TASK] Forward Timed: %lu ms\n", task.duration);
      pump->runTimed(PumpDirection::FORWARD, task.duration);
      Serial.printf("[TASK] Pump isRunning: %s\n", pump->isRunning() ? "YES" : "NO");
      break;

    case PumpCommand::REVERSE_TIMED:
      Serial.printf("[TASK] Reverse Timed: %lu ms\n", task.duration);
      pump->runTimed(PumpDirection::REVERSE, task.duration);
      break;

    default:
      break;
  }

  m_head = (m_head + 1) % MAX_PUMP_TASKS;
  m_full = false;
}