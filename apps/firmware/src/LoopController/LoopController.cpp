#include "LoopController.h"
#include "../DisplayController/DisplayUpdater.h"
#include "../ViewController/Home/HomeHandler.h"
#include "../ViewController/Manual/ManualHandler.h"
#include <AutoDosingManager.h>
#include <ConfigManager.h>
#include <DisplayManager.h>
#include <EEPROM.h>
#include <NetworkTaskManager.h>
#include <PumpController.h>
#include <RemoteCommandManager.h>

void handleButtons() {
  HomeHandler();
  DisplayManager &display = DisplayManager::getInstance();
  DisplayManager::DisplayState currentState = display.getCurrentState();
  if (currentState == DisplayManager::DisplayState::DOSING_MANUAL_BEGIN ||
      currentState == DisplayManager::DisplayState::DOSING_MANUAL_START ||
      currentState == DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS ||
      currentState == DisplayManager::DisplayState::DOSING_MANUAL_COMPLETE) {
    ManualHandler();
  }
}

void handleDisplay() {
  if (!RemoteCommandManager::getInstance().isCommandActive()) {
    updateDisplayStatus();
  }
}

void handlePump() {
  PumpController::getInstance().runDosing();
}

void handleNetworkResponses() {
  NetworkTaskManager &networkTask = NetworkTaskManager::getInstance();
  PumpController &pump = PumpController::getInstance();
  unsigned long currentTime = millis();
  static unsigned long lastCommandPollTime = 0;

  NetworkResponseMessage response;
  while (networkTask.getResponse(response, 0)) {
    switch (response.command) {
    case NetworkCommand::CONNECT_WIFI:
      if (response.status == NetworkStatus::WIFI_CONNECTED) {
        Serial.printf("[Main] WiFi connected: %s\n", response.data);
        DisplayManager::getInstance().setSignalStrength(response.value);

        NetworkCommandMessage syncCmd;
        syncCmd.command = NetworkCommand::SYNC_TIME;
        memset(syncCmd.data, 0, sizeof(syncCmd.data));
        networkTask.sendCommand(syncCmd, 0);

        NetworkCommandMessage settingsCmd;
        settingsCmd.command = NetworkCommand::HTTP_GET_SETTINGS;
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

            if (data["stepsPerML"].is<float>()) {
              float stepsPerML = data["stepsPerML"].as<float>();
              if (stepsPerML > 0) {
                pump.setStepsPerML(stepsPerML);
                pump.saveStepsPerML(stepsPerML);
              }
            }

            if (data["activeProfile"].is<uint8_t>()) {
              uint8_t profile = data["activeProfile"].as<uint8_t>();
              if (profile <= 2) {
                pump.setSpeedProfile(profile);
                pump.saveSpeedProfile(profile);
              }
            }

            AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
            if (data["enabled"].is<bool>()) {
              bool serverEnabled = data["enabled"].as<bool>();
              if (serverEnabled != autoDosing.isEnabled()) {
                serverEnabled ? autoDosing.enable() : autoDosing.disable();
              }
            }

            if (data["dailyVolume"].is<float>()) {
              float dailyVol = data["dailyVolume"].as<float>();
              if (dailyVol > 0 && dailyVol <= 200) {
                autoDosing.setDailyVolume(dailyVol);
              }
            }

            if (data["dayStartHour"].is<uint8_t>()) {
              uint8_t dayStart = data["dayStartHour"].as<uint8_t>();
              if (dayStart <= 23) {
                autoDosing.setDayPeriod(dayStart, autoDosing.getDayEndHour());
              }
            }

            if (data["dayEndHour"].is<uint8_t>()) {
              uint8_t dayEnd = data["dayEndHour"].as<uint8_t>();
              if (dayEnd <= 23) {
                autoDosing.setDayPeriod(autoDosing.getDayStartHour(), dayEnd);
              }
            }

            if (data["dayPercent"].is<uint8_t>()) {
              uint8_t dayPercent = data["dayPercent"].as<uint8_t>();
              if (dayPercent <= 100) {
                autoDosing.setDayNightSplit(dayPercent);
              }
            }

            NetworkCommandMessage reportCmd;
            reportCmd.command = NetworkCommand::HTTP_POST_SETTINGS;
            memset(reportCmd.data, 0, sizeof(reportCmd.data));
            snprintf(reportCmd.data, sizeof(reportCmd.data),
                     "{\"pumpId\":\"%s\",\"enabled\":%s,\"dailyVolume\":%.1f,"
                     "\"dayStartHour\":%d,\"dayEndHour\":%d,\"dayPercent\":%d,"
                     "\"stepsPerML\":%.0f,\"activeProfile\":%d}",
                     ConfigManager::getInstance().getPumpId(),
                     autoDosing.isEnabled() ? "true" : "false",
                     autoDosing.getDailyVolume(), autoDosing.getDayStartHour(),
                     autoDosing.getDayEndHour(),
                     (int)autoDosing.getDayPercent(), pump.getStepsPerML(),
                     pump.getActiveProfile());
            networkTask.sendCommand(reportCmd, 0);
          }
        }
      } else {
        Serial.printf("[Main] HTTP settings failed: %s\n", response.data);
      }
      break;

    case NetworkCommand::HEALTH_CHECK:
      Serial.println(response.status == NetworkStatus::SUCCESS
                         ? "[Main] Server health check OK"
                         : "[Main] Server health check failed");
      break;

    case NetworkCommand::AUTO_DOSING_RESET:
      if (response.status == NetworkStatus::SUCCESS) {
        Serial.println("[Main] Midnight reset - resetting auto-dosing");
        AutoDosingManager::getInstance().resetDailyVolume();
      }
      break;

    case NetworkCommand::HTTP_GET_COMMANDS:
      if (response.status == NetworkStatus::HTTP_OK) {
        Serial.printf("[Main] Commands received: %s\n", response.data);
        RemoteCommandManager::getInstance().parseCommands(String(response.data));
      }
      break;

    case NetworkCommand::HTTP_POST_COMMAND_COMPLETE:
      Serial.printf("[Main] Command completed: %s\n", response.data);
      break;

    default:
      break;
    }
  }

  if (!RemoteCommandManager::getInstance().isCommandActive() &&
      currentTime - lastCommandPollTime >= 30000) {
    if (WiFi.status() == WL_CONNECTED) {
      NetworkCommandMessage pollCmd;
      pollCmd.command = NetworkCommand::HTTP_GET_COMMANDS;
      memset(pollCmd.data, 0, sizeof(pollCmd.data));
      networkTask.sendCommand(pollCmd, 0);
    }
    lastCommandPollTime = currentTime;
  }
}

void handleAutoDosing(unsigned long currentTime) {
  static int lastDay = -1;
  time_t now = time(nullptr);

  if (now > 0) {
    struct tm *timeInfo = localtime(&now);
    int currentDay = timeInfo->tm_mday;
    if (lastDay == -1) {
      lastDay = currentDay;
    } else if (currentDay != lastDay) {
      Serial.printf("[Main] Midnight detected! Day %d -> %d\n", lastDay,
                    currentDay);
      AutoDosingManager::getInstance().resetDailyVolume();
      lastDay = currentDay;
    }
  }

  AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
  if (autoDosing.isEnabled()) {
    static unsigned long lastScheduleUpdate = 0;
    static unsigned long lastDoseCheck = 0;

    if (currentTime - lastScheduleUpdate >= 300000) {
      autoDosing.updateSchedule();
      lastScheduleUpdate = currentTime;
    }

    if (currentTime - lastDoseCheck >= 1000) {
      autoDosing.checkAndDose();
      lastDoseCheck = currentTime;
    }

    autoDosing.updateDosingProgress();
  }
}