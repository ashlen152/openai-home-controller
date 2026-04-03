#include "HomeHandler.h"
#include <DisplayManager.h>
#include <WiFi.h>
#include <NetworkTaskManager.h>
#include <PumpController.h>
#include <RemoteCommandManager.h>
#include "ButtonController/ButtonController.h"
#include "ViewController/Menu/MenuHandler.h"
#include "ViewController/Manual/ManualHandler.h"
#include "CalibrateDosingController/CalibrateDosingController.h"

static DisplayManager &display = DisplayManager::getInstance();
static NetworkTaskManager &networkTask = NetworkTaskManager::getInstance();
static PumpController &pump = PumpController::getInstance();

void HomeHandler()
{
  if (!isInHome())
    return;
    
  if (pressButtonUp())
  {
    Serial.println("[Home] Speed Up pressed");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[Home] WiFi connected - polling commands...");
      NetworkCommandMessage pollCmd;
      pollCmd.command = NetworkCommand::HTTP_GET_COMMANDS;
      memset(pollCmd.data, 0, sizeof(pollCmd.data));
      networkTask.sendCommand(pollCmd, 0);
    } else {
      Serial.println("[Home] WiFi NOT connected!");
    }
  }
  else if (pressButtonDown())
  {
    Serial.println("[Home] Speed Down pressed - cancelling remote calibration");
    cancelRemoteCalibration();
    RemoteCommandManager::getInstance().update();
  }
  else if (pressButtonMenu())
  {
    MenuHandler();
  }
  else if (pressButtonEnable())
  {
    ManualHandler();
  }
}

bool isInHome()
{
  if (display.getCurrentState() == DisplayManager::DisplayState::NORMAL)
    return true;
  return false;
}