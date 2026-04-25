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

#include <Arduino.h>
#include <EEPROM.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <Config.h>
#include <ButtonConfig.h>
#include <AutoDosingManager.h>
#include <ConfigManager.h>
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <NetworkTaskManager.h>
#include <ApiClient.h>
#include "WifiController/WiFiSync.h"
#include "DisplayController/DisplayUpdater.h"
#include "PumpController.h"
#include "ViewController/Home/HomeHandler.h"
#include "ViewController/Menu/MenuHandler.h"
#include "ViewController/Manual/ManualHandler.h"
#include "CalibrateDosingController/CalibrateDosingController.h"
#include "TestDosingController/TestDosingController.h"
#include <RemoteCommandManager.h>
// #include "WiFiSync.h"

// // Only keep global variable declarations needed for modules
// DisplayManager::PumpMode currentMode = DisplayManager::PumpMode::DOSING;
// DisplayManager::DosingState dosingState = DisplayManager::DosingState::IDLE;
float targetVolume = 0.0;
float remainingVolume = 0.0;
unsigned long lastTimeDisplayUpdate = 0;
float currentStepsPerML = 0;
int stepsPerSecond = 2000;
unsigned long lastWiFiRetryTime = 0;
unsigned long lastSyncTime = 0;
unsigned long lastCalibrationResultTime = 0;
bool showingCalibrationResult = false;
unsigned long lastCommandPollTime = 0;

// // Auto-dosing configuration
// AutoDosingManager::Config dosingConfig = {
//     .enabledAddr = EEPROM_AUTO_DOSING_ENABLED_ADDR,
//     .volumeAddr = EEPROM_DAILY_VOLUME_ADDR,
//     .lastTimeAddr = EEPROM_LAST_DOSING_TIME_ADDR,
//     .totalDosedAddr = EEPROM_TOTAL_DOSED_ADDR,
//     .defaultVolume = DEFAULT_DAILY_VOLUME
// };

// // Create instances

// AutoDosingManager autoDosing(pump, display, dosingConfig);

void setup()
{
  DisplayManager &display = DisplayManager::getInstance();
  PumpController &pump = PumpController::getInstance();
  NetworkTaskManager &networkTask = NetworkTaskManager::getInstance();
  ConfigManager &config = ConfigManager::getInstance();

  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Starting SmartPump...");
  
  esp_task_wdt_init(30, true);
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
  pump.init(&Serial2, Config::STEP_PIN, Config::DIR_PIN, Config::STEPPER_EN_PIN, Config::R_SENSE, Config::DRIVER_ADDR);
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
      .defaultVolume = Config::DEFAULT_DAILY_VOLUME
  };
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

void loop()
{
  // Reset watchdog timer every loop iteration
  esp_task_wdt_reset();
  
  PumpController &pump = PumpController::getInstance();
  NetworkTaskManager &networkTask = NetworkTaskManager::getInstance();
  unsigned long currentTime = millis();
  static bool firstLoop = true;
  if (firstLoop)
  {
    lastTimeDisplayUpdate = currentTime;
    firstLoop = false;
  }

  // Button and menu handling
  HomeHandler();
  
  DisplayManager &display = DisplayManager::getInstance();
  DisplayManager::DisplayState currentState = display.getCurrentState();
  if (currentState == DisplayManager::DisplayState::DOSING_MANUAL_BEGIN ||
      currentState == DisplayManager::DisplayState::DOSING_MANUAL_START ||
      currentState == DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS ||
      currentState == DisplayManager::DisplayState::DOSING_MANUAL_COMPLETE) {
    ManualHandler();
  }

  if (!RemoteCommandManager::getInstance().isCommandActive()) {
    updateDisplayStatus();
  }

  pump.runDosing();
  
  RemoteCommandManager::getInstance().update();

  // Prevent further processing if in dosing mode
  // for reduced latency and responsiveness
  if (pump.getMode() == PumpMode::DOSING)
    return;

  // Check for network responses from Core 0 (non-blocking)
  NetworkResponseMessage response;
  while (networkTask.getResponse(response, 0)) {
    // Handle network responses
    switch (response.command) {
      case NetworkCommand::CONNECT_WIFI:
        if (response.status == NetworkStatus::WIFI_CONNECTED) {
          Serial.printf("[Main] WiFi connected: %s\n", response.data);
          DisplayManager::getInstance().setSignalStrength(response.value);
          
          // Request time sync after WiFi connection
          NetworkCommandMessage syncCmd;
          syncCmd.command = NetworkCommand::SYNC_TIME;
          syncCmd.param1 = 0;
          syncCmd.param2 = 0;
          memset(syncCmd.data, 0, sizeof(syncCmd.data));
          networkTask.sendCommand(syncCmd, 0);
          
          // Request settings sync after WiFi connection
          NetworkCommandMessage settingsCmd;
          settingsCmd.command = NetworkCommand::HTTP_GET_SETTINGS;
          settingsCmd.param1 = 0;
          settingsCmd.param2 = 0;
          memset(settingsCmd.data, 0, sizeof(settingsCmd.data));
          networkTask.sendCommand(settingsCmd, 0);
          
          NetworkCommandMessage pollCmd;
          pollCmd.command = NetworkCommand::HTTP_GET_COMMANDS;
          memset(pollCmd.data, 0, sizeof(pollCmd.data));
          networkTask.sendCommand(pollCmd, 0);
        } else {
          Serial.printf("[Main] WiFi connection failed: %s\n", response.data);
        }
        break;
        
      case NetworkCommand::SYNC_TIME:
        if (response.status == NetworkStatus::TIME_SYNCED) {
          Serial.printf("[Main] Time synced: %s\n", response.data);
        }
        break;
        
      case NetworkCommand::HTTP_GET_SETTINGS:
      case NetworkCommand::HTTP_POST_SETTINGS:
        if (response.status == NetworkStatus::HTTP_OK) {
          Serial.printf("[Main] HTTP settings success: %s\n", response.data);
          
          if (response.command == NetworkCommand::HTTP_GET_SETTINGS) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, response.data);
            
            if (!error) {
              JsonObject data = doc.as<JsonObject>();
              
              // Server values override EEPROM defaults
              if (data["stepsPerML"].is<float>()) {
                float stepsPerML = data["stepsPerML"].as<float>();
                Serial.printf("[Main] Server stepsPerML: %.2f\n", stepsPerML);
                if (stepsPerML > 0) {
                  pump.setStepsPerML(stepsPerML);
                  pump.saveStepsPerML(stepsPerML);
                  Serial.printf("[Main] Applied server stepsPerML: %.2f\n", stepsPerML);
                }
              }
              
              if (data["activeProfile"].is<uint8_t>()) {
                uint8_t profile = data["activeProfile"].as<uint8_t>();
                Serial.printf("[Main] Server activeProfile: %d\n", profile);
                if (profile <= 2) {
                  pump.setSpeedProfile(profile);
                  pump.saveSpeedProfile(profile);
                  Serial.printf("[Main] Applied server activeProfile: %d\n", profile);
                }
              }
              
              // Apply auto-dosing settings from server
              AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
              
              if (data["enabled"].is<bool>()) {
                bool serverEnabled = data["enabled"].as<bool>();
                bool currentEnabled = autoDosing.isEnabled();
                Serial.printf("[Main] Server enabled: %d, current: %d\n", serverEnabled, currentEnabled);
                if (serverEnabled != currentEnabled) {
                  if (serverEnabled) {
                    autoDosing.enable();
                  } else {
                    autoDosing.disable();
                  }
                  Serial.printf("[Main] Applied server enabled: %d\n", serverEnabled);
                }
              }
              
              if (data["dailyVolume"].is<float>()) {
                float dailyVol = data["dailyVolume"].as<float>();
                Serial.printf("[Main] Server dailyVolume: %.2f\n", dailyVol);
                if (dailyVol > 0 && dailyVol <= 200) {
                  autoDosing.setDailyVolume(dailyVol);
                  Serial.printf("[Main] Applied server dailyVolume: %.2f\n", dailyVol);
                }
              }
              
              // Apply day period atomically so wrap-around updates (e.g., 18->6)
              // don't get rejected by transient start==end intermediate states.
              uint8_t mergedDayStart = autoDosing.getDayStartHour();
              uint8_t mergedDayEnd = autoDosing.getDayEndHour();
              bool hasDayStartUpdate = false;
              bool hasDayEndUpdate = false;

              if (data["dayStartHour"].is<uint8_t>()) {
                uint8_t dayStart = data["dayStartHour"].as<uint8_t>();
                Serial.printf("[Main] Server dayStartHour: %d\n", dayStart);
                if (dayStart <= 23) {
                  mergedDayStart = dayStart;
                  hasDayStartUpdate = true;
                }
              }

              if (data["dayEndHour"].is<uint8_t>()) {
                uint8_t dayEnd = data["dayEndHour"].as<uint8_t>();
                Serial.printf("[Main] Server dayEndHour: %d\n", dayEnd);
                if (dayEnd <= 23) {
                  mergedDayEnd = dayEnd;
                  hasDayEndUpdate = true;
                }
              }

              if (hasDayStartUpdate || hasDayEndUpdate) {
                if (mergedDayStart != mergedDayEnd) {
                  autoDosing.setDayPeriod(mergedDayStart, mergedDayEnd);
                  Serial.printf("[Main] Applied server day period: %02d:00-%02d:00\n",
                                mergedDayStart, mergedDayEnd);
                } else {
                  Serial.printf("[Main] Skipping invalid server day period: %02d:00-%02d:00\n",
                                mergedDayStart, mergedDayEnd);
                }
              }
              
              if (data["dayPercent"].is<uint8_t>()) {
                uint8_t dayPercent = data["dayPercent"].as<uint8_t>();
                Serial.printf("[Main] Server dayPercent: %d\n", dayPercent);
                if (dayPercent <= 100) {
                  autoDosing.setDayNightSplit(dayPercent);
                  Serial.printf("[Main] Applied server dayPercent: %d\n", dayPercent);
                }
              }
              
              // Report device settings back to server for comparison
              NetworkCommandMessage reportCmd;
              reportCmd.command = NetworkCommand::HTTP_POST_SETTINGS;
              reportCmd.param1 = 0;
              reportCmd.param2 = 0;
              memset(reportCmd.data, 0, sizeof(reportCmd.data));
              
              // Build JSON with device's current settings
              snprintf(reportCmd.data, sizeof(reportCmd.data),
                "{\"pumpId\":\"%s\",\"enabled\":%s,\"dailyVolume\":%.1f,\"dayStartHour\":%d,\"dayEndHour\":%d,\"dayPercent\":%d,\"stepsPerML\":%.0f,\"activeProfile\":%d}",
                ConfigManager::getInstance().getPumpId(),
                autoDosing.isEnabled() ? "true" : "false",
                autoDosing.getDailyVolume(),
                autoDosing.getDayStartHour(),
                autoDosing.getDayEndHour(),
                (int)autoDosing.getDayPercent(),
                pump.getStepsPerML(),
                pump.getActiveProfile()
              );
              networkTask.sendCommand(reportCmd, 0);
              Serial.println("[Main] Device settings reported to server");
              
            } else {
              Serial.printf("[Main] JSON parse error: %s\n", error.c_str());
            }
          }
        } else {
          Serial.printf("[Main] HTTP settings failed: %s\n", response.data);
        }
        break;
        
      case NetworkCommand::HEALTH_CHECK:
        if (response.status == NetworkStatus::SUCCESS) {
          Serial.println("[Main] Server health check OK");
        } else {
          Serial.println("[Main] Server health check failed");
        }
        break;
        
      case NetworkCommand::AUTO_DOSING_RESET:
        if (response.status == NetworkStatus::SUCCESS) {
          Serial.println("[Main] ⏰ Midnight reset detected - resetting auto-dosing");
          AutoDosingManager::getInstance().resetDailyVolume();
        }
        break;
        
      case NetworkCommand::HTTP_GET_COMMANDS:
        if (response.status == NetworkStatus::HTTP_OK) {
          Serial.printf("[Main] Commands received: %s\n", response.data);
          String responseStr = String(response.data);
          RemoteCommandManager::getInstance().parseCommands(responseStr);
        } else {
          Serial.printf("[Main] Command poll failed: %s\n", response.data);
        }
        break;
        
      case NetworkCommand::HTTP_POST_COMMAND_COMPLETE:
        if (response.status == NetworkStatus::HTTP_OK) {
          Serial.printf("[Main] Command completed: %s\n", response.data);
        } else {
          Serial.printf("[Main] Command completion failed: %s\n", response.data);
        }
        break;
        
      default:
        break;
    }
  }
  
  if (!RemoteCommandManager::getInstance().isCommandActive() && currentTime - lastCommandPollTime >= 30000) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[Main] Polling for commands...");
      NetworkCommandMessage pollCmd;
      pollCmd.command = NetworkCommand::HTTP_GET_COMMANDS;
      memset(pollCmd.data, 0, sizeof(pollCmd.data));
      networkTask.sendCommand(pollCmd, 0);
    } else {
      Serial.println("[Main] WiFi not connected, skipping poll");
    }
    lastCommandPollTime = currentTime;
  }

  // Midnight detection and daily volume reset (Phase 3 Sprint 10)
  {
    static int lastDay = -1;
    time_t now = time(nullptr);
    
    if (now > 0) {  // Only if time is synced
      struct tm* timeInfo = localtime(&now);
      int currentDay = timeInfo->tm_mday;
      
      if (lastDay == -1) {
        // First run - initialize
        lastDay = currentDay;
        Serial.printf("[Main] Day tracking initialized: Day %d\n", currentDay);
      } else if (currentDay != lastDay) {
        // Day changed - midnight crossed
        Serial.printf("[Main] ⏰ Midnight detected! Day %d -> %d\n", lastDay, currentDay);
        AutoDosingManager::getInstance().resetDailyVolume();
        lastDay = currentDay;
      }
    }
  }

  // Auto-dosing check and execution (non-blocking)
  AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
  if (autoDosing.isEnabled()) {
    static unsigned long lastScheduleUpdate = 0;
    static unsigned long lastDoseCheck = 0;
    
    // Update schedule every 5 minutes to recalculate next dose
    if (currentTime - lastScheduleUpdate >= 300000) {
      autoDosing.updateSchedule();
      lastScheduleUpdate = currentTime;
    }
    
    // Check for scheduled doses every second
    if (currentTime - lastDoseCheck >= 1000) {
      autoDosing.checkAndDose();
      lastDoseCheck = currentTime;
    }
    
    // Monitor dosing progress every loop iteration (Phase 4 Sprint 1)
    // Detects pump completion and logs COMPLETE event to server
    autoDosing.updateDosingProgress();
  }

  // WiFi connection is now handled by Core 0 background tasks
  // No need for handleWiFi() - Core 0 auto-reconnects every 5 seconds

  // // Data sync
  // if (wifi.isConnected() && currentTime - lastSyncTime >= SYNC_INTERVAL) {
  //   syncData();
  //   lastSyncTime = currentTime;
  // }

  // // Settings display timeout
  // if (showingSettings && currentTime - lastSettingsDisplayTime >= SETTINGS_DISPLAY_DURATION) {
  //   showingSettings = false;
  //   updateDisplayStatus();
  // }

  // // Calibration result timeout
  // if (showingCalibrationResult && currentTime - lastCalibrationResultTime >= CALIBRATION_RESULT_DURATION) {
  //   showingCalibrationResult = false;
  //   updateDisplayStatus();
  // }

  // // Dosing progress update
  // if (!display.isSleeping() && !inMenu && !showingSettings && !showingCalibrationResult && currentMode == DisplayManager::PumpMode::DOSING) {
  //   if (currentTime - lastTimeDisplayUpdate >= 1000) {
  //     if (dosingState == DisplayManager::DosingState::RUNNING) {
  //       const long totalStepsNeeded = targetVolume * pump.getStepsPerML();
  //       const long currentPosition = pump.getCurrentPosition();
  //       const long elapsedSteps = abs(currentPosition);
  //       Serial.printf("Dosing Progress: Steps %ld/%ld, Moving: %d\n", elapsedSteps, totalStepsNeeded, pump.isMoving());
  //       if (elapsedSteps >= totalStepsNeeded || !pump.isMoving()) {
  //         Serial.println("Dosing Complete - Target reached or stopped moving");
  //         pump.stop();
  //         dosingState = DisplayManager::DosingState::COMPLETED;
  //         display.showDosingComplete(targetVolume);
  //       } else {
  //         remainingVolume = (totalStepsNeeded - elapsedSteps) / pump.getStepsPerML();
  //         Serial.printf("Remaining volume: %.2f mL\n", remainingVolume);
  //         display.showDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
  //       }
  //     } else if ((dosingState == DisplayManager::DosingState::IDLE || dosingState == DisplayManager::DosingState::COMPLETED) && !inMenu && !showingSettings && !showingCalibrationResult) {
  //       char nextScheduleStr[6];
  //       uint32_t nextTime = autoDosing.getNextDosingTime();
  //       time_t t = nextTime;
  //       struct tm* timeinfo = localtime(&t);
  //       snprintf(nextScheduleStr, sizeof(nextScheduleStr), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  //       display.updateStatus(pump.isEnabled(), autoDosing.getRemainingDailyVolume(), currentMode, wifi.getCurrentTime(), autoDosing.isEnabled(), nextScheduleStr);
  //     }
  //     lastTimeDisplayUpdate = currentTime;
  //   }
  // }

  // Auto-dosing
  // if (autoDosing.isEnabled()) {
  //   autoDosing.updateSchedule();
  //   autoDosing.checkAndDose();
  // }
}
