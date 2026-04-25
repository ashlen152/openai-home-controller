/**
 * @file ManualDosingController.cpp
 * @brief Implementation of manual dosing 4-step flow.
 *
 * Step 1 - beginManualDosingController (DOSING_MANUAL_BEGIN):
 *   User adjusts target volume (default 10mL, +/- 0.1mL per press).
 *   Enable=cancel, Menu=proceed to Step 2.
 *
 * Step 2 - startManualDosingController (DOSING_MANUAL_START):
 *   User adjusts duration (default 1 min, +/- 1 min per press, min=1).
 *   Enable=cancel, Menu=proceed to Step 3.
 *
 * Step 3 - progressManualDosingController (DOSING_MANUAL_PROGRESS):
 *   Calls pump.moveML(volume) to start dosing.
 *   Monitors pump.getDistanceToGo() for completion.
 *   Shows real-time remaining volume on display.
 *   Enable=cancel dosing.
 *
 * Step 4 - completeManualDosingController (DOSING_MANUAL_COMPLETE):
 *   Shows completion screen with total volume dispensed.
 *
 * @note Duration is mapped to target dosing speed:
 *       speed (steps/s) = (targetSteps) / (durationSeconds), clamped for safety.
 */

#include "ManualDosingController.h"
#include <ButtonConfig.h>
#include <Config.h>
#include "ButtonController/ButtonController.h"
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <PumpController.h>

static float volume = 10.0; // Default volume for manual dosing
static int duration = 1;

static void restoreActiveProfileSpeed(PumpController &pump)
{
    pump.setSpeedProfile(pump.getActiveProfile());
}

void beginManualDosingController(bool isInManualBegin)
{
    DisplayManager &display = DisplayManager::getInstance();
    if (isInManualBegin)
    {
        if (pressButtonDown())
        {
            volume -= 0.1;
            display.setContextDosingManualBegin(volume);
        }
        if (pressButtonUp())
        {
            volume += 0.1;
            display.setContextDosingManualBegin(volume);
        }
        if (pressButtonEnable())
        {
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
        if (pressButtonMenu())
        {
            startManualDosingController(false);
        }
    }
    else
    {
        volume = 10; // Initial volume for setup
        display.setContextDosingManualBegin(volume);
        display.setState(DisplayManager::DisplayState::DOSING_MANUAL_BEGIN);
    }
}

void startManualDosingController(bool isInManualStart)
{
    DisplayManager &display = DisplayManager::getInstance();
    if (isInManualStart)
    {
        if (pressButtonDown())
        {
            duration = max(duration - 1.0, 1.0);
            display.setContextDosingManualStart(duration);
        }
        if (pressButtonUp())
        {
            duration += 1.0;
            display.setContextDosingManualStart(duration);
        }
        if (pressButtonEnable())
        {
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
        if (pressButtonMenu())
        {
            progressManualDosingController(false);
        }
    }
    else
    {
        duration = 1;
        display.setContextDosingManualStart(duration);
        display.setState(DisplayManager::DisplayState::DOSING_MANUAL_START);
    }
}

void progressManualDosingController(bool isInManualProgress)
{
    DisplayManager &display = DisplayManager::getInstance();
    PumpController &pump = PumpController::getInstance();
    WiFiManager &wifi = WiFiManager::getInstance();

    if (isInManualProgress)
    {
        if (pressButtonEnable())
        {
            // Stop the pump and cancel dosing
            pump.stop();
            restoreActiveProfileSpeed(pump);
            display.showText("Dosing\nCancelled");
            delay(1000);
            display.setState(DisplayManager::DisplayState::NORMAL);
            return; // Prevent fallthrough into completion/progress logic after cancel
        }
        float stepsPerML = pump.getDosingStepsPerML();
        if (stepsPerML <= 0.0f) {
            pump.stop();
            restoreActiveProfileSpeed(pump);
            display.showText("Invalid\nCalibration");
            delay(1000);
            display.setState(DisplayManager::DisplayState::NORMAL);
            return;
        }
        const long totalSteps = volume * stepsPerML;
        if (pump.getDistanceToGo() <= 0)
        {
            completeManualDosingController(false);
        }
        else
        {
            long currentSteps = abs(pump.getCurrentPosition());
            float remainingVolume = (totalSteps - currentSteps) / stepsPerML;
            if (remainingVolume < 0) {
                remainingVolume = 0;
            }
            display.setContextDosingManualProgress(volume, remainingVolume, wifi.getCurrentTime());
        }
    }
    else
    {
        float stepsPerML = pump.getDosingStepsPerML();
        if (stepsPerML <= 0.0f) {
            display.showText("Invalid\nCalibration");
            delay(1000);
            display.setState(DisplayManager::DisplayState::NORMAL);
            return;
        }
        const long targetSteps = volume * stepsPerML;
        const float targetSeconds = max(duration, 1) * 60.0f;
        const float targetSpeed = constrain(targetSteps / targetSeconds,
                                            ::Config::MANUAL_MIN_SPEED,
                                            ::Config::PROFILE_MAX_SPEED);

        // Manual mode now honors selected duration by mapping it to stepper max speed.
        pump.setMaxSpeed(targetSpeed);
        pump.setAcceleration(targetSpeed * ::Config::MANUAL_ACCEL_MULTIPLIER);

        pump.moveML(volume);

        display.setContextDosingManualProgress(volume, volume, "00:00:00");
        display.setState(DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS);
    }
}

void completeManualDosingController(bool isInManualComplete)
{
    DisplayManager &display = DisplayManager::getInstance();
    PumpController &pump = PumpController::getInstance();
    restoreActiveProfileSpeed(pump);
    display.setState(DisplayManager::DisplayState::DOSING_MANUAL_COMPLETE);
}
