#ifndef AERATION_PUMP_H
#define AERATION_PUMP_H

#include <Arduino.h>

class AerationPump
{
public:
  static constexpr unsigned long DEFAULT_MIN_OFF_TIME_MS = 2000;
  static constexpr unsigned long DEFAULT_TIMEOUT_MS = 3600000;
  static constexpr unsigned long DEFAULT_SOFT_START_DELAY_MS = 500;

  static AerationPump& getInstance();

  AerationPump();
  AerationPump(const AerationPump&) = delete;
  AerationPump& operator=(const AerationPump&) = delete;

  void begin(uint8_t gatePin);

  void turnOn();
  void turnOff();
  void toggle();
  bool isRunning() const;

  void runForDuration(unsigned long duration_ms);
  void update();

  void setMinOffTime(unsigned long ms) { m_minOffTimeMs = ms; }
  void setTimeout(unsigned long ms) { m_timeoutMs = ms; }
  void setSoftStartDelay(unsigned long ms) { m_softStartDelayMs = ms; }
  void enableDebug(bool enable) { m_debugEnabled = enable; }

  uint8_t getGatePin() const { return m_gatePin; }
  unsigned long getOnTimeMs() const { return m_onTimeMs; }
  unsigned long getTotalOnTimeMs() const { return m_totalOnTimeMs; }

private:
  uint8_t m_gatePin;
  bool m_isRunning;
  bool m_debugEnabled;

  unsigned long m_minOffTimeMs;
  unsigned long m_timeoutMs;
  unsigned long m_softStartDelayMs;

  unsigned long m_lastToggleTime;
  unsigned long m_lastOnTime;
  unsigned long m_onStartTime;
  unsigned long m_scheduledDuration;
  bool m_timedRunActive;

  unsigned long m_onTimeMs;
  unsigned long m_totalOnTimeMs;
};

#endif