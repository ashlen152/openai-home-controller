/**
 * @file CalibrateDosingController.h
 * @brief Calibration flow controller for dosing pump step/mL calibration.
 *
 * Calibration process:
 *   1. beginCalibrateDosingController()    - Show "Ready to calibrate" prompt
 *   2. progressCalibrateDosingController() - Run motor for DOSING_CAL_STEPS (200000 steps),
 *                                            display progress, detect completion/timeout
 *   3. completeCalibrateDosingController() - User enters measured mL, calculates new
 *                                            stepsPerML, saves to EEPROM
 *
 * Constants:
 *   DOSING_CAL_STEPS = 200000 steps
 *   DOSING_CAL_SPEED = 2000 steps/sec
 *   TIMEOUT = 120000ms (2 minutes)
 *
 * @note BUG: This header has no function declarations despite the .cpp defining
 *       3 functions. See AGENTS.md Known Issues #8.
 */

#ifndef CALIBRATE_DOSING_CONTROLLER_H
#define CALIBRATE_DOSING_CONTROLLER_H

void beginCalibrateDosingController(bool isInCalibrateBegin);
void progressCalibrateDosingController(bool isInCalibrateProgress);
void completeCalibrateDosingController(bool isInCalibrateComplete);

void startRemoteCalibration();
bool updateRemoteCalibration();
void cancelRemoteCalibration();

void saveRemoteCalibration(float measuredML);
#endif
