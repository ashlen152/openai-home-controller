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
  , m_currentDirection(PumpDirection::STOP)
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
  digitalWrite(m_in1Pin, HIGH);
  digitalWrite(m_in2Pin, LOW);
  m_currentDirection = PumpDirection::FORWARD;
}

void PWMPumpController::reverse()
{
  digitalWrite(m_in1Pin, LOW);
  digitalWrite(m_in2Pin, HIGH);
  m_currentDirection = PumpDirection::REVERSE;
}

void PWMPumpController::stop()
{
  digitalWrite(m_in1Pin, LOW);
  digitalWrite(m_in2Pin, LOW);
  m_currentDirection = PumpDirection::STOP;
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
  stop();
  delay(m_reverseDelay);

  if (direction == PumpDirection::FORWARD)
  {
    forward();
  }
  else if (direction == PumpDirection::REVERSE)
  {
    reverse();
  }
}

void PWMPumpController::runTimed(uint32_t durationMs)
{
  runTimed(PumpDirection::FORWARD, durationMs);
}

void PWMPumpController::runTimed(PumpDirection direction, uint32_t durationMs)
{
  if (direction == PumpDirection::STOP)
  {
    stop();
    m_timedRunActive = false;
    return;
  }

  safeDrive(direction);

  m_timedRunActive = true;
  m_runStartTime = millis();
  m_runDuration = durationMs;
  m_timedDirection = direction;
}

bool PWMPumpController::updateTimedRun()
{
  if (!m_timedRunActive)
  {
    return false;
  }

  unsigned long elapsed = millis() - m_runStartTime;
  if (elapsed >= m_runDuration)
  {
    stop();
    m_timedRunActive = false;
    return false;
  }

  return true;
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
  return m_currentDirection != PumpDirection::STOP;
}