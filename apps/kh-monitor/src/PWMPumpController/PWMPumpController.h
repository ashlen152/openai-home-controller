#ifndef PWM_PUMP_CONTROLLER_H
#define PWM_PUMP_CONTROLLER_H

#include <Arduino.h>

enum class PumpDirection
{
  STOP = 0,
  FORWARD = 1,
  REVERSE = -1
};

class PWMPumpController
{
public:
  PWMPumpController();

  void init(uint8_t in1Pin, uint8_t in2Pin);
  void begin();

  void forward();
  void reverse();
  void stop();
  void run(PumpDirection direction);
  void safeDrive(PumpDirection direction);

  void runTimed(uint32_t durationMs);
  void runTimed(PumpDirection direction, uint32_t durationMs);
  bool updateTimedRun();
  uint32_t getRemainingTime() const;

  bool isRunning() const;
  PumpDirection getDirection() const { return m_currentDirection; }
  bool isTimedRunActive() const { return m_timedRunActive; }

  void setReverseDelay(uint16_t delayMs) { m_reverseDelay = delayMs; }
  uint8_t getIn1Pin() const { return m_in1Pin; }
  uint8_t getIn2Pin() const { return m_in2Pin; }

private:
  uint8_t m_in1Pin;
  uint8_t m_in2Pin;
  uint16_t m_reverseDelay;

  PumpDirection m_currentDirection;
  bool m_timedRunActive;
  unsigned long m_runStartTime;
  uint32_t m_runDuration;
  PumpDirection m_timedDirection;
};

#endif