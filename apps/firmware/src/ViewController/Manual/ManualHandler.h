/**
 * @file ManualHandler.h
 * @brief Manual dosing state machine orchestrator.
 *
 * Routes to the appropriate ManualDosingController function based on
 * the current DisplayState:
 *   !isInManual()       -> beginManualDosingController() (enter volume setup)
 *   DOSING_MANUAL_BEGIN -> startManualDosingController() (enter duration setup)
 *   DOSING_MANUAL_START -> progressManualDosingController() (start pumping)
 *   DOSING_MANUAL_PROGRESS -> completeManualDosingController() (check completion)
 *   DOSING_MANUAL_COMPLETE -> (idle, waiting for user action)
 *
 * @note BUG: When !isInManual(), calls beginManualDosingController(isInManualBegin())
 *       which passes false (since we're not in manual yet), triggering state initialization
 *       on every loop iteration when in NORMAL state. See AGENTS.md Known Issues #9.
 */

#ifndef MANUAL_HANDLER_H
#define MANUAL_HANDLER_H

void ManualHandler();

bool isInManual();

bool isInManualBegin();

bool isInManualProgress();

bool isInManualStart();

bool isInManualComplete();

#endif