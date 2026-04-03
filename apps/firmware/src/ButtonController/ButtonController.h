/**
 * @file ButtonController.h
 * @brief High-level button API mapping physical buttons to named actions.
 *
 * Wraps ButtonHandler's low-level GPIO functions with semantic names:
 *   - pressButtonUp/Down/Menu/Enable: Single press detection
 *   - holdButtonUp/Down/Menu/Enable: Press with auto-repeat on hold
 *
 * Pin mapping (from ButtonConfig.h):
 *   Enable=25, SpeedUp=35, SpeedDown=34, Menu=14
 */

#ifndef BUTTON_CONTROLLER_H
#define BUTTON_CONTROLLER_H

bool pressButtonUp();
bool pressButtonDown();
bool pressButtonMenu();
bool pressButtonEnable();

bool holdButtonUp();
bool holdButtonDown();
bool holdButtonMenu();
bool holdButtonEnable();

#endif
