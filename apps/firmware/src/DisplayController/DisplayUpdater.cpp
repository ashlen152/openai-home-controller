/**
 * @file DisplayUpdater.cpp
 * @brief Populates DisplayContext with system state and triggers display
 * refresh.
 *
 * Feeds NORMAL state context with real system values:
 *   - pumpEnabled: from PumpController::getIsEnable()
 *   - value: from PumpController::getSpeed()
 *   - currentTime: from WiFiManager NTP time
 *   - autodosingEnabled: from AutoDosingManager::isEnabled() ✓ IMPLEMENTED
 *   - nextSchedule: from AutoDosingManager::getNextDosingTime() ✓ IMPLEMENTED
 */

#include "DisplayUpdater.h"
#include <AutoDosingManager.h>
#include <DisplayManager.h>
#include <PumpController.h>
#include <WiFiManager.h>
#include <time.h>

static DisplayManager &display = DisplayManager::getInstance();
static WiFiManager &wifi = WiFiManager::getInstance();
static PumpController &pump = PumpController::getInstance();

void updateDisplayStatus() {
  AutoDosingManager &autoDosing = AutoDosingManager::getInstance();

  DisplayContext ctx;
  ctx.pumpEnabled = pump.getIsEnable();
  ctx.value = pump.getSpeed();
  ctx.currentTime = wifi.getCurrentTime();
  ctx.autodosingEnabled = autoDosing.isEnabled();

  static unsigned long lastDebugLog = 0;
  if (millis() - lastDebugLog > 10000) {
    Serial.printf("[Display] Time: '%s', AutoDosing: %d, Paused: %d\n",
                  ctx.currentTime ? ctx.currentTime : "NULL",
                  ctx.autodosingEnabled, autoDosing.isPaused());
    if (autoDosing.isPaused()) {
      Serial.printf("[Display] Pause remaining: %u seconds\n",
                    autoDosing.getPauseRemaining());
    }
    lastDebugLog = millis();
  }

  static char nextScheduleStr[16] = "-----";
  if (autoDosing.isPaused()) {
    uint32_t remaining = autoDosing.getPauseRemaining();
    if (remaining == 0xFFFFFFFF) {
      snprintf(nextScheduleStr, sizeof(nextScheduleStr), "PAUSED");
    } else if (remaining > 3600) {
      snprintf(nextScheduleStr, sizeof(nextScheduleStr), "P:%dh",
               remaining / 3600);
    } else if (remaining > 60) {
      snprintf(nextScheduleStr, sizeof(nextScheduleStr), "P:%dm",
               remaining / 60);
    } else {
      snprintf(nextScheduleStr, sizeof(nextScheduleStr), "P:%ds", remaining);
    }
    ctx.nextSchedule = nextScheduleStr;
  } else {
    uint32_t nextTime = autoDosing.getNextDosingTime();
    if (nextTime > 0 && autoDosing.isEnabled()) {
      time_t t = nextTime;
      struct tm *timeinfo = localtime(&t);
      snprintf(nextScheduleStr, sizeof(nextScheduleStr), "%02d:%02d",
               timeinfo->tm_hour, timeinfo->tm_min);
      ctx.nextSchedule = nextScheduleStr;
    } else {
      ctx.nextSchedule = "-----";
    }
  }

  ctx.totalVolume = autoDosing.getTotalDosedVolume();
  ctx.serverConnected = wifi.isConnected();
  ctx.stepsPerML = pump.getDosingStepsPerML();
  ctx.activeProfile = pump.getActiveProfile();
  display.setContextNormal(ctx.pumpEnabled, ctx.value, ctx.currentTime,
                           ctx.autodosingEnabled, ctx.nextSchedule,
                           ctx.totalVolume, ctx.stepsPerML, ctx.activeProfile);
  display.setServerConnected(ctx.serverConnected);
  display.updateDisplayState();
}
