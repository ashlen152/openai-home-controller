/**
 * @file HomeHandler.h
 * @brief Home screen controller - entry point for user navigation.
 *
 * Active only when DisplayState == NORMAL.
 * Button actions from Home:
 *   - Menu button: Opens MenuHandler (MENU state)
 *   - Enable button: Enters ManualHandler (DOSING_MANUAL_BEGIN state)
 */

#ifndef HOME_HANDLER_H
#define HOME_HANDLER_H

void HomeHandler();
bool isInHome();

#endif
