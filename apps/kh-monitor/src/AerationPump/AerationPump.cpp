#include "AerationPump.h"

AerationPump::AerationPump()
  : m_gatePin(0)
  , m_isRunning(false)
  , m_debugEnabled(false)
  , m_minOffTimeMs(DEFAULT_MIN_OFF_TIME_MS)
  , m_timeoutMs(DEFAULT_TIMEOUT_MS)
  , m_softStartDelayMs(DEFAULT_SOFT_START_DELAY_MS)
  , m_lastToggleTime(0)
  , m_lastOnTime(0)
  , m_onStartTime(0)
  , m_scheduledDuration(0)
  , m_timedRunActive(false)
  , m_onTimeMs(0)
  , m_totalOnTimeMs(0)
{
}

AerationPump& AerationPump::getInstance() {
  static AerationPump instance;
  return instance;
}

void AerationPump::begin(uint8_t gatePin)
{
  m_gatePin = gatePin;
  pinMode(m_gatePin, OUTPUT);
  digitalWrite(m_gatePin, LOW);
  m_isRunning = false;

  if (m_debugEnabled) {
    Serial.printf("[AerationPump] Initialized on GPIO %d\n", gatePin);
    Serial.printf("[AerationPump] Min OFF time: %lu ms\n", m_minOffTimeMs);
    Serial.printf("[AerationPump] Timeout: %lu ms\n", m_timeoutMs);
  }
}

void AerationPump::turnOn()
{
  unsigned long now = millis();

  if (m_isRunning) {
    if (m_debugEnabled) {
      Serial.println("[AerationPump] Already running, ignoring turnOn()");
    }
    return;
  }

  if (now - m_lastOnTime < m_minOffTimeMs) {
    if (m_debugEnabled) {
      Serial.printf("[AerationPump] Blocked - minimum OFF time not met (%lu ms remaining)\n",
        m_minOffTimeMs - (now - m_lastOnTime));
    }
    return;
  }

  if (m_softStartDelayMs > 0) {
    if (m_debugEnabled) {
      Serial.printf("[AerationPump] Soft start delay: %lu ms\n", m_softStartDelayMs);
    }
    delay(m_softStartDelayMs);
  }

  digitalWrite(m_gatePin, HIGH);
  m_isRunning = true;
  m_onStartTime = now;
  m_lastToggleTime = now;

  if (m_debugEnabled) {
    Serial.println("[AerationPump] ON");
  }
}

void AerationPump::turnOff()
{
  if (!m_isRunning) {
    if (m_debugEnabled) {
      Serial.println("[AerationPump] Already off, ignoring turnOff()");
    }
    return;
  }

  digitalWrite(m_gatePin, LOW);
  m_isRunning = false;
  m_lastOnTime = millis();
  m_timedRunActive = false;

  m_onTimeMs += millis() - m_onStartTime;
  m_totalOnTimeMs += millis() - m_onStartTime;

  if (m_debugEnabled) {
    Serial.printf("[AerationPump] OFF (session: %lu ms, total: %lu ms)\n",
      millis() - m_onStartTime, m_totalOnTimeMs);
  }
}

void AerationPump::toggle()
{
  if (m_isRunning) {
    turnOff();
  } else {
    turnOn();
  }
}

bool AerationPump::isRunning() const
{
  return m_isRunning;
}

void AerationPump::runForDuration(unsigned long duration_ms)
{
  if (m_isRunning) {
    if (m_debugEnabled) {
      Serial.println("[AerationPump] Already running, ignoring runForDuration()");
    }
    return;
  }

  m_scheduledDuration = duration_ms;
  m_timedRunActive = true;
  turnOn();

  if (m_debugEnabled) {
    Serial.printf("[AerationPump] Timed run scheduled: %lu ms\n", duration_ms);
  }
}

void AerationPump::update()
{
  if (!m_isRunning) {
    return;
  }

  unsigned long now = millis();

  if (m_timedRunActive) {
    unsigned long elapsed = now - m_onStartTime;
    if (elapsed >= m_scheduledDuration) {
      if (m_debugEnabled) {
        Serial.printf("[AerationPump] Timed run complete (%lu ms)\n", elapsed);
      }
      turnOff();
      return;
    }
  }

  unsigned long elapsed = now - m_onStartTime;
  if (elapsed >= m_timeoutMs) {
    if (m_debugEnabled) {
      Serial.printf("[AerationPump] Timeout reached (%lu ms), auto shutoff\n", elapsed);
    }
    turnOff();
  }
}