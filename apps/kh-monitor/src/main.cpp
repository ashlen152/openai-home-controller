#include <Arduino.h>

#include "PumpTaskManager/PumpTaskManager.h"

static unsigned long loopCount = 0;
static unsigned long testStartTime = 0;
static uint8_t testPhase = 0;

static constexpr uint32_t PHASE_DURATION = 2000;
static constexpr uint32_t IDLE_DURATION = 1000;

static constexpr uint8_t RGB_LED_PIN = 48;

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  rgbLedWrite(RGB_LED_PIN, r, g, b);
}

void runTestSequence() {
  unsigned long elapsed = millis() - testStartTime;
  uint8_t phase = elapsed / (PHASE_DURATION + IDLE_DURATION);

  if (phase != testPhase) {
    testPhase = phase;
    PumpTaskManager::getInstance().stop(1);
    PumpTaskManager::getInstance().stop(2);
    Serial.printf("[TEST] Phase %d\n", testPhase);
  }

  uint32_t phaseTime = elapsed % (PHASE_DURATION + IDLE_DURATION);

  if (phaseTime < PHASE_DURATION) {
    switch (testPhase % 4) {
    case 0:
      Serial.printf("[TEST] REF FORWARD (phaseTime=%lu)\n", phaseTime);
      setLEDColor(0, 255, 0);
      PumpTaskManager::getInstance().forward(1);
      break;
    case 1:
      Serial.printf("[TEST] REF REVERSE (phaseTime=%lu)\n", phaseTime);
      setLEDColor(0, 0, 255);
      PumpTaskManager::getInstance().reverse(1);
      break;
    case 2:
      Serial.printf("[TEST] TANK FORWARD (phaseTime=%lu)\n", phaseTime);
      setLEDColor(255, 255, 0);
      PumpTaskManager::getInstance().forward(2);
      break;
    case 3:
      Serial.printf("[TEST] TANK REVERSE (phaseTime=%lu)\n", phaseTime);
      setLEDColor(255, 0, 0);
      PumpTaskManager::getInstance().reverse(2);
      break;
    }
  } else {
    setLEDColor(0, 0, 0);
  }
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("");
  Serial.println("=== KH Monitor Starting ===");
  Serial.printf("File: %s\n", __FILE__);
  Serial.printf("Date: %s\n", __DATE__);
  Serial.printf("Time: %s\n", __TIME__);

  setLEDColor(255, 255, 255);
  delay(500);
  setLEDColor(0, 0, 0);

  PumpTaskManager::getInstance().begin();

  Serial.println("=== KH Monitor Ready ===");
  Serial.println("");
  Serial.println("[TEST] Starting sequence test...");
  Serial.println("[TEST] 0=REF FWD=GREEN, 1=REF REV=BLUE, 2=TANK FWD=YELLOW, "
                 "3=TANK REV=RED");

  testStartTime = millis();
}

void loop() {
  loopCount++;
  PumpTaskManager::getInstance().update();

  runTestSequence();

  if (loopCount % 25 == 0) {
    Serial.printf("[LOOP %lu] Phase: %d\n", loopCount, testPhase);
  }

  delay(20);
}
