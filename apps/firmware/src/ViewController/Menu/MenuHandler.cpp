#include "MenuHandler.h"
#include <ButtonConfig.h>
#include "ButtonController/ButtonController.h"
#include <DisplayManager.h>
#include <PumpController.h>
#include <AutoDosingManager.h>
#include <ConfigManager.h>
#include <time.h>

static int menuIndex = 0;
static bool showingSettings = false;
static const char *menuItems[] = {"Dosing Cal", "Settings Info", "Auto Dosing", "Set Daily Vol", "Day Period", "Day/Night %", "Set Pump ID", "Reset Config", "Pause Dosing", "Resume Dosing", "Dose History", "Speed Profile", "Edit Profiles"};

void runMenuSelection()
{
  DisplayManager &display = DisplayManager::getInstance();
  PumpController &pump = PumpController::getInstance();

  switch (menuIndex)
  {
  case 0: // Dosing Calibration
    // calibrateDosing();
    display.setState(DisplayManager::DisplayState::CALIBRATE_BEGIN);
    break;
  case 1: // Settings Info
    display.setContextSettings(pump.getSpeed(), pump.getDosingStepsPerML(), pump.getSpeedStep());
    display.setState(DisplayManager::DisplayState::SETTINGS);
    break;
  case 2: // Auto Dosing
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    if (autoDosing.isEnabled()) {
      autoDosing.disable();
      display.showText("Auto Dosing\nDisabled");
    } else {
      autoDosing.enable();
      display.showText("Auto Dosing\nEnabled");
    }
    autoDosing.syncSettings();  // Phase 4: Sync to server
    delay(800);
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 3: // Set Daily Volume
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    float volume = autoDosing.getDailyVolume();
    bool setting = true;
    
    while (setting) {
      display.showValue("Daily Vol (mL)", volume);
      
      if (pressButtonUp()) {
        volume += 1.0f;
        if (volume > 200.0f) volume = 200.0f;  // Max 200mL per day
      }
      if (pressButtonDown()) {
        volume = constrain(volume - 1.0f, 1.0f, 200.0f);  // Min 1mL, max 200mL
      }
      if (pressButtonEnable()) {
        autoDosing.setDailyVolume(volume);
        autoDosing.syncSettings();  // Phase 4: Sync to server
        char msg[32];
        snprintf(msg, sizeof(msg), "Volume Saved\n%.0f mL", volume);
        display.showText(msg);
        delay(1000);
        setting = false;
      }
      if (pressButtonMenu()) {
        display.showText("Cancelled");
        delay(500);
        setting = false;  // Cancel without saving
      }
      
      delay(100);  // Debounce
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 4: // Day Period (start/end hour)
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    uint8_t startH = autoDosing.getDayStartHour();
    uint8_t endH = autoDosing.getDayEndHour();
    bool settingStart = true;  // true = setting start, false = setting end
    bool setting = true;
    
    while (setting) {
      char label[32];
      if (settingStart) {
        snprintf(label, sizeof(label), "Day Start Hour\nEn=-> M=X");
        display.showValue(label, (float)startH);
      } else {
        snprintf(label, sizeof(label), "Day End Hour\nEn=-> M=X");
        display.showValue(label, (float)endH);
      }
      
      if (pressButtonUp()) {
        if (settingStart) {
          startH = (startH + 1) % 24;
        } else {
          endH = (endH + 1) % 24;
        }
      }
      if (pressButtonDown()) {
        if (settingStart) {
          startH = (startH == 0) ? 23 : startH - 1;
        } else {
          endH = (endH == 0) ? 23 : endH - 1;
        }
      }
      if (pressButtonEnable()) {  // SWAPPED: Enable now advances/saves
        if (settingStart) {
          settingStart = false;  // Switch to setting end hour
          display.showText("Now set end hour");
          delay(500);
        } else {
          // Validate and save
          if (startH == endH) {
            display.showText("Error: Start/End\ncan't be same hour");
            delay(2000);
            settingStart = true;  // Go back to start hour
          } else {
            autoDosing.setDayPeriod(startH, endH);
            autoDosing.syncSettings();  // Phase 4: Sync to server
            char msg[32];
            snprintf(msg, sizeof(msg), "Period Saved\n%02d:00-%02d:00", startH, endH);
            display.showText(msg);
            delay(800);
            setting = false;
          }
        }
      }
      if (pressButtonMenu()) {  // SWAPPED: Menu now cancels
        display.showText("Cancelled");
        delay(500);
        setting = false;  // Cancel without saving
      }
      
      delay(100);  // Debounce
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 5: // Day/Night Split %
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    uint8_t dayPercent = autoDosing.getDayPercent();
    bool setting = true;
    
    while (setting) {
      char label[32];
      uint8_t nightPercent = 100 - dayPercent;
      snprintf(label, sizeof(label), "Day:%d%% Night:%d%%", dayPercent, nightPercent);
      display.showText(label);
      
      if (pressButtonUp()) {
        dayPercent += 5;
        if (dayPercent > 100) dayPercent = 100;
      }
      if (pressButtonDown()) {
        if (dayPercent >= 5) dayPercent -= 5;
        else dayPercent = 0;
      }
      if (pressButtonEnable()) {
        // Warn on extreme splits
        if (dayPercent == 0 || dayPercent == 100) {
          display.showText("Warning: 100% in\none period only");
          delay(2000);
        }
        autoDosing.setDayNightSplit(dayPercent);
        autoDosing.syncSettings();  // Phase 4: Sync to server
        char msg[32];
        snprintf(msg, sizeof(msg), "Split Saved\n%d%% / %d%%", dayPercent, 100 - dayPercent);
        display.showText(msg);
        delay(500);
        setting = false;
      }
      if (pressButtonMenu()) {
        display.showText("Cancelled");
        delay(500);
        setting = false;  // Cancel without saving
      }
      
      delay(100);  // Debounce
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 6: // Set Pump ID
  {
    ConfigManager &config = ConfigManager::getInstance();
    char newId[16];
    strncpy(newId, config.getPumpId(), sizeof(newId) - 1);
    newId[15] = '\0';
    
    int cursorPos = 0;
    int idLen = strlen(newId);
    bool editing = true;
    const char validChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    
    while (editing) {
      // Show current ID with cursor
      char displayBuf[32];
      snprintf(displayBuf, sizeof(displayBuf), "ID:%s\nPos:%d En=OK/-> M=X", newId, cursorPos + 1);
      display.showText(displayBuf);
      
      if (pressButtonUp()) {
        // Cycle through valid characters
        if (cursorPos < idLen) {
          char current = newId[cursorPos];
          const char *pos = strchr(validChars, current);
          if (pos && *(pos + 1) != '\0') {
            newId[cursorPos] = *(pos + 1);
          } else {
            newId[cursorPos] = validChars[0];  // Wrap to 'A'
          }
        } else if (idLen < 15) {
          // Add new character
          newId[idLen] = 'A';
          newId[idLen + 1] = '\0';
          idLen++;
        }
      }
      
      if (pressButtonDown()) {
        // Delete last character (keep as-is per Q2: Option A)
        if (idLen > 0 && cursorPos == idLen - 1) {
          // Delete last character
          newId[idLen - 1] = '\0';
          idLen--;
          if (cursorPos > 0) cursorPos--;
        }
      }
      
      if (pressButtonEnable()) {  // SWAPPED: Enable now advances/saves
        // Move cursor right (or save if at end)
        if (cursorPos < idLen - 1) {
          cursorPos++;
        } else {
          // Validate before saving
          if (idLen == 0) {
            display.showText("Error: ID cannot\nbe empty");
            delay(1500);
          } else if (config.setPumpId(newId)) {
            display.showText("Pump ID Saved");
            delay(500);
            editing = false;
          } else {
            display.showText("Invalid ID");
            delay(500);
            editing = false;
          }
        }
      }
      
      if (pressButtonMenu()) {  // SWAPPED: Menu now cancels
        display.showText("Cancelled");
        delay(500);
        editing = false;
      }
      
      delay(150);  // Debounce
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 7: // Reset Config (Factory Reset)
  {
    display.showText("Factory Reset?\nEnable=YES\nMenu=NO");
    delay(100);
    
    bool waiting = true;
    while (waiting) {
      if (pressButtonEnable()) {  // SWAPPED: Enable confirms
        display.showText("Resetting...");
        delay(300);
        
        ConfigManager &config = ConfigManager::getInstance();
        config.resetToDefaults();
        
        display.showText("Reset Complete\nRestarting...");
        delay(1500);
        ESP.restart();  // Restart ESP32
        waiting = false;
      }
      if (pressButtonMenu()) {  // SWAPPED: Menu cancels
        display.showText("Cancelled");
        delay(500);
        waiting = false;
      }
      delay(100);
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 8: // Pause Dosing (Phase 3 Sprint 5)
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    
    const char* pauseOptions[] = {"1 Hour", "6 Hours", "12 Hours", "24 Hours", "Indefinite"};
    const uint32_t pauseDurations[] = {3600, 21600, 43200, 86400, 0}; // seconds (0 = indefinite)
    int pauseIndex = 0;
    const int pauseOptionCount = 5;
    
    bool selecting = true;
    while (selecting) {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "Pause:\n%s\nEnable=OK Menu=Cancel", pauseOptions[pauseIndex]);
      display.showText(buffer);
      delay(100);
      
      if (pressButtonUp()) {
        pauseIndex = (pauseIndex == 0) ? pauseOptionCount - 1 : pauseIndex - 1;
      }
      if (pressButtonDown()) {
        pauseIndex = (pauseIndex + 1) % pauseOptionCount;
      }
      if (pressButtonEnable()) {  // SWAPPED: Enable confirms
        autoDosing.pause(pauseDurations[pauseIndex]);
        display.showText("Paused");
        delay(800);
        selecting = false;
      }
      if (pressButtonMenu()) {  // SWAPPED: Menu cancels
        display.showText("Cancelled");
        delay(500);
        selecting = false;
      }
      delay(100);
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 9: // Resume Dosing (Phase 3 Sprint 5)
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    
    if (!autoDosing.isPaused()) {
      display.showText("Not Paused");
      delay(800);
    } else {
      display.showText("Resume?\nEnable=YES\nMenu=NO");
      delay(100);
      
      bool waiting = true;
      while (waiting) {
        if (pressButtonEnable()) {  // SWAPPED: Enable confirms
          autoDosing.resume();
          display.showText("Resumed");
          delay(800);
          waiting = false;
        }
        if (pressButtonMenu()) {  // SWAPPED: Menu cancels
          display.showText("Cancelled");
          delay(500);
          waiting = false;
        }
        delay(100);
      }
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 10: // Dose History - Today's Summary (Phase 3 Sprint 10)
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    
    // Use millis() fallback if time not synced (Decision 5=B)
    time_t now = time(nullptr);
    if (now == 0) {
      // Time not synced - use fallback
      if (autoDosing.getLastSyncTime() > 0 && autoDosing.getLastSyncMillis() > 0) {
        uint32_t elapsedSeconds = (millis() - autoDosing.getLastSyncMillis()) / 1000;
        now = autoDosing.getLastSyncTime() + elapsedSeconds;
      } else {
        // No fallback available
        display.showText("Time not synced\nCannot show history");
        delay(2000);
        display.setState(DisplayManager::DisplayState::NORMAL);
        break;
      }
    }
    
    // Calculate today's start time (midnight 00:00:00)
    struct tm* nowInfo = localtime(&now);
    struct tm dayStart = *nowInfo;
    dayStart.tm_hour = 0;
    dayStart.tm_min = 0;
    dayStart.tm_sec = 0;
    time_t dayStartTime = mktime(&dayStart);
    
    // Get ring buffer history
    uint8_t count = 0;
    const DoseHistoryEntry* history = autoDosing.getDoseHistory(count);
    
    // Filter doses from today and count them
    uint8_t todayCount = 0;
    DoseHistoryEntry todayDoses[5];  // Max 5 from ring buffer
    
    for (uint8_t i = 0; i < count; i++) {
      if (history[i].timestamp >= dayStartTime) {
        todayDoses[todayCount++] = history[i];
      }
    }
    
    // Use totalDosedVolume for accurate total (Decision 1=A)
    // Note: This assumes midnight reset has run for current day
    float accurateTotalMl = autoDosing.getTotalDosedVolume();
    
    // Format display
    char historyText[256];
    int offset = snprintf(historyText, sizeof(historyText), 
                          "=Today's Doses=\nTotal: %.1fmL (%d doses)", 
                          accurateTotalMl, todayCount);
    
    if (todayCount == 0) {
      offset += snprintf(historyText + offset, sizeof(historyText) - offset, 
                         "\n\nNo doses today");
    } else {
      // Show up to 3 most recent doses (to fit on screen)
      for (uint8_t i = 0; i < todayCount && i < 3; i++) {
        time_t t = todayDoses[i].timestamp;  // Convert uint32_t to time_t
        struct tm* timeinfo = localtime(&t);
        offset += snprintf(historyText + offset, sizeof(historyText) - offset,
                           "\n%02d:%02d %.1fmL %s",
                           timeinfo->tm_hour, timeinfo->tm_min,
                           todayDoses[i].volume,
                           todayDoses[i].success ? "OK" : "X");
      }
      
      if (todayCount > 3) {
        offset += snprintf(historyText + offset, sizeof(historyText) - offset,
                           "\n...+%d more", todayCount - 3);
      }
    }
    
    display.showText(historyText);
    delay(100);
    
    bool viewing = true;
    while (viewing) {
      if (pressButtonEnable() || pressButtonMenu()) {
        viewing = false;
      }
      delay(100);
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 11: // Speed Profile (Phase 3 Sprint 7)
  {
    PumpController &pump = PumpController::getInstance();
    uint8_t profile = pump.getActiveProfile();
    const char* profileNames[] = {"Slow", "Medium", "Fast"};
    
    bool selecting = true;
    while (selecting) {
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "Profile: %s\n%.0f steps/sec\nEnable=OK Menu=Cancel", 
               profileNames[profile], pump.getProfileSpeed(profile));
      display.showText(buffer);
      delay(100);
      
      if (pressButtonUp()) {
        profile = (profile + 1) % 3;
      }
      if (pressButtonDown()) {
        profile = (profile == 0) ? 2 : profile - 1;
      }
      if (pressButtonEnable()) {  // SWAPPED: Enable confirms
        pump.setSpeedProfile(profile);
        AutoDosingManager::getInstance().syncSettings();  // Phase 4: Sync to server
        display.showText("Profile Set");
        delay(800);
        selecting = false;
      }
      if (pressButtonMenu()) {  // SWAPPED: Menu cancels
        display.showText("Cancelled");
        delay(500);
        selecting = false;
      }
      delay(100);
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 12: // Edit Profiles (Phase 3 Sprint 7)
  {
    PumpController &pump = PumpController::getInstance();
    uint8_t profile = 0;
    const char* profileNames[] = {"Slow", "Medium", "Fast"};
    
    bool editing = true;
    while (editing) {
      float speed = pump.getProfileSpeed(profile);
      
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "Edit %s:\n%.0f steps/sec\nMenu=Next Enable=Done", 
               profileNames[profile], speed);
      display.showText(buffer);
      delay(100);
      
      if (pressButtonUp()) {
        speed += 1000.0f;
        if (speed > 50000.0f) speed = 50000.0f;
        pump.setProfileSpeed(profile, speed);
      }
      if (pressButtonDown()) {
        speed = constrain(speed - 1000.0f, 1000.0f, 50000.0f);
        pump.setProfileSpeed(profile, speed);
      }
      if (pressButtonMenu()) {
        // Move to next profile (don't auto-exit on wrap)
        profile = (profile + 1) % 3;
        display.showText("Next profile");
        delay(300);
      }
      if (pressButtonEnable()) {
        display.showText("Profiles Saved");
        delay(500);
        editing = false;
      }
      delay(100);
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  }
}

const int menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);

bool isInMenu()
{
  DisplayManager &display = DisplayManager::getInstance();
  if (display.getCurrentState() == DisplayManager::DisplayState::MENU)
    return true;
  return false;
}

void MenuHandler()
{
  DisplayManager &display = DisplayManager::getInstance();
  // Button/menu handling
  if (pressButtonMenu())
  {
    if (isInMenu())
    {
      printf("Menu selection: %d\n", menuIndex);
      runMenuSelection();
    }
    else
    {
      printf("Entering menu\n");
      menuIndex = 0;
      display.setContextMenu(menuIndex, menuItems, menuItemCount);
      display.setState(DisplayManager::DisplayState::MENU);
    }
  }
  if (isInMenu())
  {
    if (holdButtonUp())
    {
      printf("Menu up\n");
      menuIndex = (menuIndex + 1) % menuItemCount;
      display.setContextMenu(menuIndex, menuItems, menuItemCount);
    }
    if (holdButtonDown())
    {
      printf("Menu down\n");
      menuIndex = (menuIndex - 1 + menuItemCount) % menuItemCount;
      display.setContextMenu(menuIndex, menuItems, menuItemCount);
    }
  }
}
