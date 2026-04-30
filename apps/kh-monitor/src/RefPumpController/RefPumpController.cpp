#include "RefPumpController.h"

RefPumpController::RefPumpController()
  : m_pump()
{
}

RefPumpController &RefPumpController::getInstance()
{
  static RefPumpController instance;
  return instance;
}

void RefPumpController::init()
{
  m_pump.init(REF_PUMP_IN1, REF_PUMP_IN2);
}

void RefPumpController::begin()
{
  m_pump.begin();
  Serial.println("[INFO] REF pump initialized");
}

void RefPumpController::forward()
{
  m_pump.forward();
  Serial.println("[REF] Forward");
}

void RefPumpController::reverse()
{
  m_pump.reverse();
  Serial.println("[REF] Reverse");
}

void RefPumpController::stop()
{
  m_pump.stop();
  Serial.println("[REF] Stop");
}

void RefPumpController::run(PumpDirection direction)
{
  m_pump.run(direction);
  Serial.printf("[REF] Run: %d\n", static_cast<int>(direction));
}

void RefPumpController::safeDrive(PumpDirection direction)
{
  m_pump.safeDrive(direction);
  Serial.printf("[REF] SafeDrive: %d\n", static_cast<int>(direction));
}

void RefPumpController::runTimed(uint32_t durationMs)
{
  m_pump.runTimed(durationMs);
  Serial.printf("[REF] Timed run: %lu ms\n", durationMs);
}

void RefPumpController::runTimed(PumpDirection direction, uint32_t durationMs)
{
  m_pump.runTimed(direction, durationMs);
  Serial.printf("[REF] Timed run: dir=%d, %lu ms\n", static_cast<int>(direction), durationMs);
}

bool RefPumpController::updateTimedRun()
{
  return m_pump.updateTimedRun();
}

uint32_t RefPumpController::getRemainingTime() const
{
  return m_pump.getRemainingTime();
}

bool RefPumpController::isRunning() const
{
  return m_pump.isRunning();
}