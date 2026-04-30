#include <Arduino.h>

#include "RefPumpController/RefPumpController.h"
#include "TankPumpController/TankPumpController.h"

#ifdef AIR_PIN
static constexpr uint8_t AIR_PUMP_PIN = AIR_PIN;
#else
static constexpr uint8_t AIR_PUMP_PIN = 15;
#endif

static constexpr uint8_t LED_PIN = 2;

void setup() {
  Serial.begin(115200);

  Serial.println("\nPH-4502C Meter Configuration:");
  Serial.println("ADC Resolution: 12-bit");
  Serial.println("ADC Attenuation: 11dB (0-3.9V range)");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  delay(2000);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.begin(115200);
  delay(500);
  Serial.println("");
  Serial.println("=== KH Monitor Starting ===");
  Serial.println(__FILE__);
  Serial.println(__DATE__);
  Serial.println(__TIME__);

  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);

  RefPumpController::getInstance().init();
  RefPumpController::getInstance().begin();

  TankPumpController::getInstance().init();
  TankPumpController::getInstance().begin();

#ifdef AIR_PIN
  pinMode(AIR_PUMP_PIN, OUTPUT);
  digitalWrite(AIR_PUMP_PIN, LOW);
  Serial.println("Air pump pin configured");
#endif

  Serial.println("=== KH Monitor Ready ===");
  Serial.println("Commands:");
  Serial.println("1f -> REF forward");
  Serial.println("1r -> REF reverse");
  Serial.println("1s -> REF stop");
  Serial.println("2f -> TANK forward");
  Serial.println("2r -> TANK reverse");
  Serial.println("2s -> TANK stop");
  Serial.println("1t3000 -> REF run 3 seconds");
  Serial.println("2t5000 -> TANK run 5 seconds");

  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  RefPumpController::getInstance().updateTimedRun();
  TankPumpController::getInstance().updateTimedRun();

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.length() < 2)
      return;

    int pumpId = cmd.charAt(0) - '0';
    char action = cmd.charAt(1);

    if (pumpId == 1) {
      if (action == 'f') {
        RefPumpController::getInstance().forward();
      } else if (action == 'r') {
        RefPumpController::getInstance().reverse();
      } else if (action == 's') {
        RefPumpController::getInstance().stop();
      } else if (action == 't') {
        int duration = cmd.substring(2).toInt();
        RefPumpController::getInstance().runTimed(duration);
      }
    } else if (pumpId == 2) {
      if (action == 'f') {
        TankPumpController::getInstance().forward();
      } else if (action == 'r') {
        TankPumpController::getInstance().reverse();
      } else if (action == 's') {
        TankPumpController::getInstance().stop();
      } else if (action == 't') {
        int duration = cmd.substring(2).toInt();
        TankPumpController::getInstance().runTimed(duration);
      }
    }
  }

  delay(10);
}
