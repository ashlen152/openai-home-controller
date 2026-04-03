#include "ButtonHandler.h"
#include <ButtonConfig.h>
#include <DisplayManager.h>

static unsigned long lastButtonPressTime = 0;
static unsigned long holdStartTime[4] = {0};
static unsigned long lastActionTime[4] = {0};

static DisplayManager &display = DisplayManager::getInstance();

bool checkButtonPress(int pin)
{
  if (digitalRead(pin) == LOW)
  {
    lastButtonPressTime = millis();
    if (display.isSleeping())
    {
      display.wakeDisplay();
      while (digitalRead(pin) == LOW)
      {
        // pump.runDosing();
      }
      return false;
    }
    while (digitalRead(pin) == LOW)
    {
      // pump.runDosing();
    }
    return true;
  }
  return false;
}

bool checkButtonPressOrHold(int pin)
{
  DisplayManager &display = DisplayManager::getInstance();
  int index;
  if (pin == BUTTON_SPEED_UP_PIN)
  {
    index = 0;
  }
  else if (pin == BUTTON_SPEED_DOWN_PIN)
  {
    index = 1;
  }
  else if (pin == BUTTON_MENU_PIN)
  {
    index = 2;
  }
  else
  {
    index = 3;
  }
  if (digitalRead(pin) == LOW)
  {
    if (display.isSleeping())
    {
      lastButtonPressTime = millis();
      display.wakeDisplay();
      while (digitalRead(pin) == LOW)
      {
        return false;
      }
      // pump.runDosing();
    }
    if (holdStartTime[index] == 0)
    {
      holdStartTime[index] = millis();
      lastActionTime[index] = millis();
      return true;
    }
    else
    {
      unsigned long totalHoldTime = millis() - holdStartTime[index];
      unsigned long interval = (totalHoldTime > 2000) ? 100 : 500;
      if (millis() - lastActionTime[index] >= interval)
      {
        lastActionTime[index] = millis();
        return true;
      }
    }
  }
  else
  {
    holdStartTime[index] = 0;
    lastActionTime[index] = 0;
  }
  return false;
}
