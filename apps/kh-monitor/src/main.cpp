#include <Arduino.h>

#include "PWMPumpController/PWMPumpController.h"
#include "RefPumpController/RefPumpController.h"
#include "TankPumpController/TankPumpController.h"
#include "AerationPump/AerationPump.h"
#include "PHProbe/PHProbe.h"
#include "KHSolver/KHSolver.h"
#include "KHStateMachine/KHStateMachine.h"

static constexpr uint8_t RGB_LED_PIN = 48;
static constexpr uint8_t PH_ADC_PIN = 1;
static constexpr uint8_t AIR_PUMP_PIN = 15;

static constexpr unsigned long KH_INTERVAL_MS = 4UL * 60UL * 60UL * 1000UL;
static constexpr unsigned long DEBUG_LOG_INTERVAL_MS = 5000UL;
static constexpr unsigned long SERIAL_CHECK_INTERVAL_MS = 100UL;

static unsigned long g_lastKHRun = 0;
static unsigned long g_lastDebugLog = 0;
static unsigned long g_lastSerialCheck = 0;
static bool g_verboseMode = false;

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  ::rgbLedWrite(RGB_LED_PIN, r, g, b);
}

void updateLEDForState(KHState state) {
  switch (state) {
    case KHState::IDLE:
      setLEDColor(0, 0, 0);
      break;
    case KHState::FILL_REFERENCE:
    case KHState::FILL_TANK:
      setLEDColor(255, 255, 255);
      break;
    case KHState::STABILIZE_REFERENCE:
    case KHState::STABILIZE_TANK:
      setLEDColor(128, 128, 0);
      break;
    case KHState::MEASURE_REFERENCE_INITIAL:
    case KHState::MEASURE_REFERENCE_FINAL:
    case KHState::MEASURE_TANK_INITIAL:
    case KHState::MEASURE_TANK_FINAL:
      setLEDColor(0, 255, 255);
      break;
    case KHState::AERATE_REFERENCE:
    case KHState::AERATE_TANK:
      setLEDColor(0, 128, 255);
      break;
    case KHState::DRAIN:
    case KHState::FLUSH:
      setLEDColor(255, 128, 0);
      break;
    case KHState::CALCULATE_KH:
      setLEDColor(255, 255, 0);
      break;
    case KHState::CALIB_IDLE:
    case KHState::CALIB_MEASURE:
    case KHState::CALIB_STORE:
    case KHState::CALIB_DONE:
      setLEDColor(128, 0, 255);
      break;
    case KHState::DOSE:
      setLEDColor(0, 200, 100);
      break;
    case KHState::CLEAN_TUBE:
      setLEDColor(100, 100, 255);
      break;
    case KHState::ERROR:
      setLEDColor(255, 0, 0);
      break;
  }
}

void processSerialInput() {
  if (Serial.available() == 0) {
    return;
  }

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0) {
    return;
  }

  Serial.print("> ");
  Serial.println(cmd);

  if (cmd == "start") {
    RefPumpController::getInstance().begin();
    TankPumpController::getInstance().begin();
    AerationPump::getInstance().begin(AIR_PUMP_PIN);
    KHStateMachine::getInstance().start();
    Serial.println("[MAIN] KH cycle started");
  }
  else if (cmd == "stop") {
    KHStateMachine::getInstance().stop();
    Serial.println("[MAIN] KH cycle stopped");
  }
  else if (cmd == "status") {
    Serial.println("========== System Status ==========");
    Serial.printf("Uptime: %lu seconds\n", millis() / 1000UL);
    Serial.printf("State: %s\n", KHStateMachine::getInstance().getStateName());
    Serial.printf("Last KH: %.2f dKH\n", KHSolver::getInstance().getLastValidKH());
    Serial.printf("Calibration points: %d\n", KHStateMachine::getInstance().getCalibPointCount());
    Serial.printf("Verbose: %s\n", g_verboseMode ? "ON" : "OFF");
    Serial.println("====================================");
  }
  else if (cmd == "verbose") {
    g_verboseMode = !g_verboseMode;
    KHStateMachine::getInstance().setVerbose(g_verboseMode);
    KHSolver::getInstance().setVerbose(g_verboseMode);
    Serial.printf("[MAIN] Verbose mode: %s\n", g_verboseMode ? "ON" : "OFF");
  }
  else if (cmd.startsWith("kh ")) {
    if (!KHStateMachine::getInstance().processCommand(cmd)) {
      KHSolver::getInstance().processSerialCommand(cmd);
    }
  }
  else if (cmd == "help") {
    Serial.println("========== Available Commands ==========");
    Serial.println("start          - Start KH measurement cycle");
    Serial.println("stop           - Stop KH measurement cycle");
    Serial.println("status         - Print system status");
    Serial.println("verbose        - Toggle debug output");
    Serial.println("kh calib ...   - Calibration commands");
    Serial.println("kh ...         - Forward to KHSolver");
    Serial.println("help           - Show this help");
    Serial.println("========================================");
  }
  else {
    Serial.println("[MAIN] Unknown command. Type 'help' for list.");
  }
}

void logDebugInfo() {
  unsigned long now = millis();

  if (now - g_lastDebugLog < DEBUG_LOG_INTERVAL_MS) {
    return;
  }

  g_lastDebugLog = now;

  Serial.printf("[%lu s] State: %-20s | Last KH: %.2f dKH | pH: %.2f\n",
               now / 1000UL,
               KHStateMachine::getInstance().getStateName(),
               KHSolver::getInstance().getLastValidKH(),
               PHProbe::getInstance().getLastFilteredPH());
}

void checkAutoKHTrigger() {
  unsigned long now = millis();

  if (g_lastKHRun == 0) {
    g_lastKHRun = now;
    return;
  }

  if (now - g_lastKHRun >= KH_INTERVAL_MS) {
    if (!KHStateMachine::getInstance().isRunning()) {
      Serial.println("[MAIN] Auto-triggering KH measurement cycle");
      KHStateMachine::getInstance().start();
      g_lastKHRun = now;
    }
  }
}

void handleKHCompletion() {
  if (KHStateMachine::getInstance().isComplete()) {
    KHState state = KHStateMachine::getInstance().getState();

    if (state == KHState::ERROR) {
      Serial.println("[MAIN] KH cycle ended with ERROR");
    } else {
      Serial.println("[MAIN] KH cycle completed");
    }

    KHStateMachine::getInstance().reset();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("");
  Serial.println("============ KH Monitor ESP32-S3 ============");
  Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
  Serial.printf("KH Interval: %lu hours\n", KH_INTERVAL_MS / 3600000UL);
  Serial.println("============================================");
  Serial.println("");

  setLEDColor(255, 255, 255);
  delay(200);
  setLEDColor(0, 0, 0);

  PHProbe::getInstance().begin(PH_ADC_PIN);

  KHSolver::getInstance().begin();

  KHStateConfig config;
  config.fillTimeMs = 5000UL;
  config.stabilizeTimeMs = 3000UL;
  config.aerationTimeMs = 60000UL;
  config.waitAfterAerationMs = 5000UL;
  config.drainTimeMs = 8000UL;
  config.flushTimeMs = 10000UL;
  config.doseTimeMs = 3000UL;
  config.cleanTubeTimeMs = 5000UL;

  KHStateMachine::getInstance().begin(
    &RefPumpController::getInstance(),
    &TankPumpController::getInstance(),
    &AerationPump::getInstance(),
    &PHProbe::getInstance(),
    &KHSolver::getInstance()
  );
  KHStateMachine::getInstance().setConfig(config);
  KHStateMachine::getInstance().setVerbose(g_verboseMode);

  Serial.println("");
  Serial.println("============ System Ready =================");
  Serial.println("Type 'help' for available commands");
  Serial.println("============================================");
  Serial.println("");

  g_lastKHRun = millis();
}

void loop() {
  RefPumpController::getInstance().update();
  TankPumpController::getInstance().update();
  AerationPump::getInstance().update();
  PHProbe::getInstance().update();
  KHStateMachine::getInstance().update();

  unsigned long now = millis();
  if (now - g_lastSerialCheck >= SERIAL_CHECK_INTERVAL_MS) {
    g_lastSerialCheck = now;
    processSerialInput();
  }

  checkAutoKHTrigger();
  handleKHCompletion();
  logDebugInfo();
  updateLEDForState(KHStateMachine::getInstance().getState());
}