/**
 * @file main.cpp
 * @brief SmartPump entry point - setup() and loop() for ESP32 Arduino.
 *
 * Initialization sequence (setup):
 *   1. Serial (115200 baud) and EEPROM (512 bytes)
 *   2. Serial2 for TMC2209 UART (RX=16, TX=17)
 *   3. Button GPIO pins with internal pull-ups
 *   4. OLED display initialization
 *   5. PumpController init and begin
 *   6. WiFi connection attempt (shown on display)
 *
 * Main loop priority order:
 *   1. HomeHandler()        - Handle Home screen button inputs
 *   2. ManualHandler()      - Handle manual dosing state machine
 *   3. updateDisplayStatus() - Refresh OLED based on current DisplayState
 *   4. pump.runDosing()     - Execute stepper steps (time-critical)
 *   5. Early return if DOSING mode (skip WiFi for timing accuracy)
 *   6. handleWiFi()         - WiFi reconnect and health check
 *
 * @note Auto-dosing and data sync are currently commented out (WIP).
 */

#include "LoopController/LoopController.h"
#include <ApiClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AutoDosingManager.h>
#include <ButtonConfig.h>
#include <Config.h>
#include <ConfigManager.h>
#include <DisplayManager.h>
#include <EEPROM.h>
#include <NetworkTaskManager.h>
#include <PumpController.h>
#include <RemoteCommandManager.h>
#include <WiFiManager.h>
#include <esp_task_wdt.h>

unsigned long lastTimeDisplayUpdate = 0;
unsigned long lastWiFiRetryTime = 0;

void setup() {
  DisplayManager &display = DisplayManager::getInstance();
  PumpController &pump = PumpController::getInstance();
  NetworkTaskManager &networkTask = NetworkTaskManager::getInstance();
  ConfigManager &config = ConfigManager::getInstance();

  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Starting SmartPump...");

  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = 30000,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);
  Serial.println("Watchdog timer initialized (30s timeout)");

  EEPROM.begin(512);
  Serial2.begin(115200, SERIAL_8N1, Config::RX_PIN, Config::TX_PIN);

  pinMode(BUTTON_ENABLE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SPEED_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SPEED_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_MENU_PIN, INPUT_PULLUP);

  // Initialize ConfigManager (loads pump ID from EEPROM)
  config.begin();
  Serial.print("Pump ID: ");
  Serial.println(config.getPumpId());

  display.begin();
  pump.init(&Serial2, Config::STEP_PIN, Config::DIR_PIN, Config::STEPPER_EN_PIN,
            Config::R_SENSE, Config::DRIVER_ADDR);
  pump.begin();

  // Initialize AutoDosingManager
  Serial.println("Initializing auto-dosing...");
  AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
  AutoDosingManager::Config dosingConfig = {
      .enabledAddr = Config::EEPROM_AUTO_DOSING_ENABLED_ADDR,
      .volumeAddr = Config::EEPROM_DAILY_VOLUME_ADDR,
      .lastTimeAddr = Config::EEPROM_LAST_DOSING_TIME_ADDR,
      .totalDosedAddr = Config::EEPROM_TOTAL_DOSED_ADDR,
      .dayStartHourAddr = Config::EEPROM_DAY_START_HOUR_ADDR,
      .dayEndHourAddr = Config::EEPROM_DAY_END_HOUR_ADDR,
      .dayPercentAddr = Config::EEPROM_DAY_PERCENT_ADDR,
      .defaultVolume = Config::DEFAULT_DAILY_VOLUME};
  autoDosing.initialize(pump, display, dosingConfig);
  autoDosing.begin();
  Serial.println("Auto-dosing initialized");

  // Initialize and start network task on Core 0
  Serial.println("Initializing network task...");
  if (networkTask.initialize(10, 10)) {
    Serial.println("Network task initialized successfully");
    if (networkTask.start(8192, 1)) {
      Serial.println("Network task started on Core 0");

      // Send initial WiFi connection command to Core 0
      NetworkCommandMessage cmd;
      cmd.command = NetworkCommand::CONNECT_WIFI;
      cmd.param1 = 0;
      cmd.param2 = 0;
      memset(cmd.data, 0, sizeof(cmd.data));

      if (networkTask.sendCommand(cmd, 0)) {
        Serial.println("WiFi connection command queued");
        display.showText("WiFi Connecting...");
      } else {
        Serial.println("Failed to queue WiFi connection command");
        display.showText("WiFi Init Failed");
      }
    } else {
      Serial.println("Failed to start network task");
      display.showText("Network Task Failed");
    }
  } else {
    Serial.println("Failed to initialize network task");
    display.showText("Network Init Failed");
  }

  lastWiFiRetryTime = millis();
}

void loop() {
  // Reset watchdog timer every loop iteration
  esp_task_wdt_reset();

  PumpController &pump = PumpController::getInstance();
  unsigned long currentTime = millis();
  static bool firstLoop = true;
  if (firstLoop) {
    lastTimeDisplayUpdate = currentTime;
    firstLoop = false;
  }

  // Handle button inputs and manual dosing states
  handleButtons();

  // Refresh OLED display (skip when remote command active)
  handleDisplay();

  // Execute stepper steps (time-critical)
  handlePump();

  // Early return if DOSING mode to reduce latency
  if (pump.getMode() == PumpMode::DOSING)
    return;

  // Process network responses from Core 0
  handleNetworkResponses();

  // Midnight detection and auto-dosing
  handleAutoDosing(currentTime);

  // Remote command update
  RemoteCommandManager::getInstance().update();
}
