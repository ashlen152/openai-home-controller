#include "ButtonController.h"
#include "ButtonHandler.h"
#include <ButtonConfig.h>

bool pressButtonUp()
{
  return checkButtonPress(BUTTON_SPEED_UP_PIN);
}

bool pressButtonDown()
{
  return checkButtonPress(BUTTON_SPEED_DOWN_PIN);
}

bool pressButtonMenu()
{
  return checkButtonPress(BUTTON_MENU_PIN);
}

bool pressButtonEnable()
{
  return checkButtonPress(BUTTON_ENABLE_PIN);
}

bool holdButtonUp()
{
  return checkButtonPressOrHold(BUTTON_SPEED_UP_PIN);
}

bool holdButtonDown()
{
  return checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN);
}

bool holdButtonMenu()
{
  return checkButtonPressOrHold(BUTTON_MENU_PIN);
}

bool holdButtonEnable()
{
  return checkButtonPressOrHold(BUTTON_ENABLE_PIN);
}
