/**
 * @file ButtonHandler.h
 * @brief Low-level button press and hold detection for GPIO-connected buttons.
 *
 * Provides two detection modes:
 *   - checkButtonPress(): Simple press detection with display wake-on-press.
 *     Blocks until button is released (busy-wait).
 *   - checkButtonPressOrHold(): Press + auto-repeat on hold.
 *     First press fires immediately, then repeats at 500ms intervals,
 *     accelerating to 100ms after 2 seconds of continuous hold.
 *
 * Both functions automatically wake the display from sleep on first press.
 * Buttons are active-LOW with internal pull-ups (INPUT_PULLUP).
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

bool checkButtonPress(int pin);
bool checkButtonPressOrHold(int pin);

#endif
