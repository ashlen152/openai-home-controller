/**
 * @file ButtonConfig.h
 * @brief Button GPIO pin assignments.
 *
 * Defines the 4 physical button pins used for user input.
 * All buttons use INPUT_PULLUP mode (active-LOW).
 *
 * Button mapping:
 *   - ENABLE (25): Enter manual mode, confirm/cancel actions
 *   - SPEED_UP (35): Navigate up in menus, increase values
 *   - SPEED_DOWN (34): Navigate down in menus, decrease values
 *   - MENU (14): Open menu, select menu items, proceed to next step
 */

#ifndef BUTTONCONFIG_H  // FIXED: was WIFICONFIG_H
#define BUTTONCONFIG_H

// Button Pins
constexpr int BUTTON_ENABLE_PIN = 25;      ///< Enable/confirm button
constexpr int BUTTON_SPEED_UP_PIN = 35;    ///< Up/increase button
constexpr int BUTTON_SPEED_DOWN_PIN = 34;  ///< Down/decrease button
constexpr int BUTTON_MENU_PIN = 14;        ///< Menu/select button

#endif
