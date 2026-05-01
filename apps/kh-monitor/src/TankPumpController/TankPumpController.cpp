#include "TankPumpController.h"

TankPumpController::TankPumpController()
  : m_pump()
{
}

TankPumpController &TankPumpController::getInstance()
{
  static TankPumpController instance;
  return instance;
}

void TankPumpController::init()
{
  m_pump.init(TANK_PUMP_IN1, TANK_PUMP_IN2);
}

void TankPumpController::begin()
{
  m_pump.begin();
  Serial.println("[INFO] TANK pump initialized");
}

void TankPumpController::forward()
{
  m_pump.forward();
  Serial.println("[TANK] Forward");
}

void TankPumpController::reverse()
{
  m_pump.reverse();
  Serial.println("[TANK] Reverse");
}

void TankPumpController::stop()
{
  m_pump.stop();
  Serial.println("[TANK] Stop");
}

void TankPumpController::run(PumpDirection direction)
{
  m_pump.run(direction);
  Serial.printf("[TANK] Run: %d\n", static_cast<int>(direction));
}

void TankPumpController::safeDrive(PumpDirection direction)
{
  m_pump.safeDrive(direction);
  Serial.printf("[TANK] SafeDrive: %d\n", static_cast<int>(direction));
}

void TankPumpController::runTimed(uint32_t durationMs)
{
  m_pump.runTimed(durationMs);
  Serial.printf("[TANK] Timed run: %lu ms\n", durationMs);
}

void TankPumpController::runTimed(PumpDirection direction, uint32_t durationMs)
{
  m_pump.runTimed(direction, durationMs);
  Serial.printf("[TANK] Timed run: dir=%d, %lu ms\n", static_cast<int>(direction), durationMs);
}

bool TankPumpController::update()
{
  return m_pump.update();
}

bool TankPumpController::updateTimedRun()
{
  return m_pump.update();
}

uint32_t TankPumpController::getRemainingTime() const
{
  return m_pump.getRemainingTime();
}

bool TankPumpController::isRunning() const
{
  return m_pump.isRunning();
}