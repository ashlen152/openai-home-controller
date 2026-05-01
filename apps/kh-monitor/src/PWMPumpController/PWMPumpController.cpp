#include "PWMPumpController.h"

#ifdef REVERSE_DELAY
static constexpr uint16_t DEFAULT_REVERSE_DELAY = REVERSE_DELAY;
#else
static constexpr uint16_t DEFAULT_REVERSE_DELAY = 50;
#endif

PWMPumpController::PWMPumpController()
  : m_in1Pin(0)
  , m_in2Pin(0)
  , m_reverseDelay(DEFAULT_REVERSE_DELAY)
  , m_timeoutMs(DEFAULT_TIMEOUT_MS)
  , m_lastDirection(PumpDirection::STOP)
  , m_currentDirection(PumpDirection::STOP)
  , m_state(PumpState::IDLE)
  , m_timedRunActive(false)
  , m_runStartTime(0)
  , m_runDuration(0)
  , m_timedDirection(PumpDirection::STOP)
{
}

void PWMPumpController::init(uint8_t in1Pin, uint8_t in2Pin)
{
  m_in1Pin = in1Pin;
  m_in2Pin = in2Pin;
}

void PWMPumpController::begin()
{
  pinMode(m_in1Pin, OUTPUT);
  pinMode(m_in2Pin, OUTPUT);
  stop();
}

void PWMPumpController::forward()
{
  Serial.printf("[PUMP] forward() called. Current: dir=%d, state=%d\n", 
    (int)m_currentDirection, (int)m_state);

  if (m_currentDirection == PumpDirection::FORWARD && m_state == PumpState::RUNNING)
  {
    Serial.println("[PUMP] Already running forward, skipping");
    return;
  }

  stop();

  digitalWrite(m_in1Pin, HIGH);
  digitalWrite(m_in2Pin, LOW);
  m_lastDirection = m_currentDirection;
  m_currentDirection = PumpDirection::FORWARD;
  m_state = PumpState::RUNNING;
  m_runStartTime = millis();

  Serial.printf("[PUMP] Forward started. state=%d, startTime=%lu\n", 
    (int)m_state, m_runStartTime);
}

void PWMPumpController::reverse()
{
  if (m_currentDirection == PumpDirection::REVERSE && m_state == PumpState::RUNNING)
  {
    return;
  }

  stop();
  delay(m_reverseDelay);

  digitalWrite(m_in1Pin, LOW);
  digitalWrite(m_in2Pin, HIGH);
  m_lastDirection = m_currentDirection;
  m_currentDirection = PumpDirection::REVERSE;
  m_state = PumpState::RUNNING;
  m_runStartTime = millis();
}

void PWMPumpController::stop()
{
  digitalWrite(m_in1Pin, LOW);
  digitalWrite(m_in2Pin, LOW);
  m_lastDirection = m_currentDirection;
  m_currentDirection = PumpDirection::STOP;
  m_state = PumpState::IDLE;
  m_timedRunActive = false;
}

void PWMPumpController::run(PumpDirection direction)
{
  if (direction == PumpDirection::FORWARD)
  {
    forward();
  }
  else if (direction == PumpDirection::REVERSE)
  {
    reverse();
  }
  else
  {
    stop();
  }
}

void PWMPumpController::safeDrive(PumpDirection direction)
{
  run(direction);
}

void PWMPumpController::runTimed(uint32_t durationMs)
{
  runTimed(PumpDirection::FORWARD, durationMs);
}

void PWMPumpController::runTimed(PumpDirection direction, uint32_t durationMs)
{
  Serial.printf("[PUMP] runTimed(dir=%d, dur=%lu)\n", (int)direction, durationMs);

  if (direction == PumpDirection::STOP)
  {
    stop();
    return;
  }

  run(direction);

  m_timedRunActive = true;
  m_runStartTime = millis();
  m_runDuration = durationMs;
  m_timedDirection = direction;

  Serial.printf("[PUMP] Timed run set. active=%d, start=%lu, dur=%lu\n",
    m_timedRunActive, m_runStartTime, m_runDuration);
}

bool PWMPumpController::update()
{
  unsigned long now = millis();

  if (m_state == PumpState::TIMEOUT)
  {
    Serial.println("[PUMP] State: TIMEOUT");
    stop();
    return false;
  }

  if (m_timedRunActive)
  {
    unsigned long elapsed = now - m_runStartTime;
    if (elapsed >= m_runDuration)
    {
      Serial.printf("[PUMP] Timed run done. elapsed=%lu >= duration=%lu\n", elapsed, m_runDuration);
      stop();
      return false;
    }
  }

  if (m_state == PumpState::RUNNING && !m_timedRunActive)
  {
    unsigned long elapsed = now - m_runStartTime;
    if (elapsed >= m_timeoutMs)
    {
      Serial.println("[PUMP] Timeout reached");
      stop();
      m_state = PumpState::TIMEOUT;
      return false;
    }
  }

  bool running = (m_state == PumpState::RUNNING);
  return running;
}

uint32_t PWMPumpController::getRemainingTime() const
{
  if (!m_timedRunActive)
  {
    return 0;
  }

  unsigned long elapsed = millis() - m_runStartTime;
  if (elapsed >= m_runDuration)
  {
    return 0;
  }

  return m_runDuration - elapsed;
}

bool PWMPumpController::isRunning() const
{
  return m_state == PumpState::RUNNING;
}