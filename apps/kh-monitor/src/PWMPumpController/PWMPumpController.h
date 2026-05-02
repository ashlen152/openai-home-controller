#ifndef PWM_PUMP_CONTROLLER_H
#define PWM_PUMP_CONTROLLER_H

#include <Arduino.h>

enum class PumpDirection
{
  STOP = 0,
  FORWARD = 1,
  REVERSE = -1
};

enum class PumpState
{
  IDLE,
  RUNNING,
  TIMEOUT
};

class PWMPumpController
{
public:
  static constexpr uint32_t DEFAULT_TIMEOUT_MS = 30000;
  static constexpr uint16_t DEFAULT_REVERSE_DELAY = 50;
  static constexpr float DEFAULT_FLOW_RATE_ML_PER_SEC = 2.0f;

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
  bool update();
  uint32_t getRemainingTime() const;

  void runVolume(PumpDirection direction, float volumeMl);
  float getFlowRateMlPerSec() const { return m_flowRateMlPerSec; }

  bool isRunning() const;
  PumpDirection getDirection() const { return m_currentDirection; }
  bool isTimedRunActive() const { return m_timedRunActive; }
  PumpState getState() const { return m_state; }

  void setReverseDelay(uint16_t delayMs) { m_reverseDelay = delayMs; }
  void setTimeout(uint32_t timeoutMs) { m_timeoutMs = timeoutMs; }
  void setFlowRateMlPerSec(float rate) { m_flowRateMlPerSec = rate; }
  uint8_t getIn1Pin() const { return m_in1Pin; }
  uint8_t getIn2Pin() const { return m_in2Pin; }

private:
  uint8_t m_in1Pin;
  uint8_t m_in2Pin;
  uint16_t m_reverseDelay;
  uint32_t m_timeoutMs;
  float m_flowRateMlPerSec;

  PumpDirection m_lastDirection;
  PumpDirection m_currentDirection;
  PumpState m_state;
  bool m_timedRunActive;
  unsigned long m_runStartTime;
  uint32_t m_runDuration;
  PumpDirection m_timedDirection;
};

#endif