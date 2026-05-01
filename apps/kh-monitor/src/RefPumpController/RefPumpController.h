#ifndef REF_PUMP_CONTROLLER_H
#define REF_PUMP_CONTROLLER_H

#include <Arduino.h>
#include "../PWMPumpController/PWMPumpController.h"

#ifdef REF_IN1
static constexpr uint8_t REF_PUMP_IN1 = REF_IN1;
#else
static constexpr uint8_t REF_PUMP_IN1 = 4;
#endif

#ifdef REF_IN2
static constexpr uint8_t REF_PUMP_IN2 = REF_IN2;
#else
static constexpr uint8_t REF_PUMP_IN2 = 5;
#endif

class RefPumpController
{
public:
  static RefPumpController &getInstance();

  void init();
  void begin();

  void forward();
  void reverse();
  void stop();
  void run(PumpDirection direction);
  void safeDrive(PumpDirection direction);

  void runTimed(uint32_t durationMs);
  void runTimed(PumpDirection direction, uint32_t durationMs);
  bool update();
  bool updateTimedRun();
  uint32_t getRemainingTime() const;

  bool isRunning() const;
  PumpDirection getDirection() const { return m_pump.getDirection(); }
  bool isTimedRunActive() const { return m_pump.isTimedRunActive(); }
  PumpState getState() const { return m_pump.getState(); }

  void setReverseDelay(uint16_t delayMs) { m_pump.setReverseDelay(delayMs); }
  void setTimeout(uint32_t timeoutMs) { m_pump.setTimeout(timeoutMs); }

private:
  RefPumpController();
  RefPumpController(const RefPumpController &) = delete;
  RefPumpController &operator=(const RefPumpController &) = delete;

  PWMPumpController m_pump;
};

#endif