/**
 * @file ManualDosingController.h
 * @brief Manual dosing flow controller - handles volume setup, timing, pumping, and completion.
 *
 * State machine flow (each function handles one state):
 *   1. beginManualDosingController()    - Volume input (UP/DOWN to adjust, MENU to proceed)
 *   2. startManualDosingController()    - Duration input (UP/DOWN to adjust, MENU to start)
 *   3. progressManualDosingController() - Active pumping (monitors steps, shows progress)
 *   4. completeManualDosingController() - Dosing complete screen
 *
 * Each function takes a bool indicating whether the state is already active:
 *   - true:  Handle button inputs and update display within that state
 *   - false: Initialize the state (set defaults, transition DisplayState)
 *
 * Button mapping during manual dosing:
 *   - Enable: Cancel / go back to NORMAL
 *   - Up/Down: Adjust volume or duration
 *   - Menu: Proceed to next step
 */

#ifndef MANUAL_DOSING_CONTROLLER_H
#define MANUAL_DOSING_CONTROLLER_H

void beginManualDosingController(bool isInManualBegin);

void startManualDosingController(bool isInManualStart);

void progressManualDosingController(bool isInManualProgress);

void completeManualDosingController(bool isInManualComplete);

#endif