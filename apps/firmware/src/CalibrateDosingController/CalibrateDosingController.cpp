/**
 * @file CalibrateDosingController.cpp
 * @brief Calibration flow implementation for determining steps-per-mL ratio.
 *
 * Calibration process:
 *   1. Begin: Show prompt, wait for Enable button to start
 *   2. Progress: Run motor for DOSING_CAL_STEPS (200,000 steps) at 2000 steps/sec
 *      - Shows progress on display with step count
 *      - Detects completion, stall (no movement for 2s), or timeout (2 min)
 *      - Enable button aborts calibration
 *   3. Complete: User measures actual mL dispensed, adjusts with Up/Down (+/- 0.1mL)
 *      - Enable button confirms: calculates newStepsPerML = 200000 / measuredML
 *      - Saves new calibration to EEPROM at EEPROM_DOSING_STEPS_ADDR
 *      - Menu button cancels without saving
 *
 * @note Functions are defined here but NOT declared in the header file.
 *       See AGENTS.md Known Issues #8.
 */

#include "CalibrateDosingController.h"
#include <ButtonConfig.h>
#include "ButtonController/ButtonController.h"
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <PumpController.h>
#include <EEPROM.h>
#include <Config.h>

// Constants
const float CALIBRATION_VOLUME = 5.0f;
const unsigned long TIMEOUT = 300000;         // 5 minutes timeout

// Internal variables
static unsigned long startTime = 0;
static unsigned long displayUpdate = 0;
static unsigned long lastMoveTime = 0;
static long lastPosition = 0;
static float ml = CALIBRATION_VOLUME;
static bool showingCalibrationResult = false;
static unsigned long lastCalibrationResultTime = 0;

static DisplayManager &display = DisplayManager::getInstance();
static PumpController &pump = PumpController::getInstance();
static WiFiManager &wifi = WiFiManager::getInstance();

// --- BEGIN STATE ---
void beginCalibrateDosingController(bool isInBegin)
{
    if (isInBegin)
    {
        // User confirmation before calibration
        if (pressButtonEnable())
        {
            progressCalibrateDosingController(false);
        }
        if (pressButtonMenu())
        {
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
    }
    else
    {
        // Prepare system
        pump.stop();
        pump.setCurrentPosition(0);
        pump.setMode(PumpMode::DOSING);

            display.showText("Ready to calibrate.\nPress Enable to start.");
        display.setState(DisplayManager::DisplayState::CALIBRATE_BEGIN);
    }
}

// --- PROGRESS STATE ---
void progressCalibrateDosingController(bool isInProgress)
{
    if (isInProgress)
    {
        pump.runDosing();

        long currentPosition = pump.getCurrentPosition();
        long calibrationTotalSteps = (long)(CALIBRATION_VOLUME * pump.getDosingStepsPerML());

        // Movement check
        if (currentPosition != lastPosition)
        {
            lastPosition = currentPosition;
            lastMoveTime = millis();
        }

        // Periodic display update
        if (millis() - displayUpdate >= 1000)
        {
            char progressText[64];
            snprintf(progressText, sizeof(progressText), "Calibrating...\n%ld / %ld steps\n%d%%", 
                     currentPosition, calibrationTotalSteps, (int)((currentPosition * 100) / calibrationTotalSteps));
            display.showText(progressText);
            displayUpdate = millis();
        }

        // Completion or timeout
        if (currentPosition >= calibrationTotalSteps ||
            (!pump.isRunning() && millis() - lastMoveTime > 2000) ||
            (millis() - startTime > TIMEOUT))
        {
            completeCalibrateDosingController(false);
        }

        // Abort
        if (pressButtonEnable())
        {
            pump.stop();
            display.showText("Calibration aborted.");
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
    }
    else
    {
        // Start calibration movement
        pump.moveML(CALIBRATION_VOLUME);
        startTime = millis();
        displayUpdate = millis();
        lastMoveTime = millis();
        lastPosition = 0;

        display.showText("Starting calibration...");
        display.setState(DisplayManager::DisplayState::CALIBRATE_PROGRESS);
    }
}

// --- COMPLETE STATE ---
void completeCalibrateDosingController(bool isInComplete)
{
    if (isInComplete)
    {
        // Adjust measured mL
        if (pressButtonUp())
        {
            ml += 0.1f;
            display.showCalibrationInput(ml);
        }
        if (pressButtonDown())
        {
            ml = max(ml - 0.1f, 0.0f);
            display.showCalibrationInput(ml);
        }

        // Confirm calibration
        if (pressButtonEnable())
        {
            long actualSteps = pump.getCurrentPosition();
            float newStepsPerML = (ml > 0) ? ((float)actualSteps / ml) : pump.getDosingStepsPerML();
            pump.setDosingStepsPerML(newStepsPerML);
            EEPROM.put(Config::EEPROM_DOSING_STEPS_ADDR, newStepsPerML);
            EEPROM.commit();

            display.showCalibrationResult(newStepsPerML, pump.getSpeedStep());
            showingCalibrationResult = true;
            lastCalibrationResultTime = millis();
            display.setState(DisplayManager::DisplayState::CALIBRATE_COMPLETE);
        }

        if (pressButtonMenu())
        {
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
    }
    else
    {
        // Calibration movement complete
        pump.stop();
        long actualSteps = pump.getCurrentPosition();
        if (actualSteps < (long)(CALIBRATION_VOLUME * pump.getDosingStepsPerML()))
        {
            display.showText("Calibration failed!");
            display.setState(DisplayManager::DisplayState::NORMAL);
            return;
        }

        ml = CALIBRATION_VOLUME;
        display.showCalibrationInput(ml);
        display.setState(DisplayManager::DisplayState::CALIBRATE_COMPLETE);
    }
}

static bool remoteCalibrationInProgress = false;
static unsigned long remoteCalibrationStartTime = 0;
static long remoteCalibrationLastPosition = 0;
static long remoteCalibrationTotalSteps = 0;

void startRemoteCalibration() {
    Serial.println("[RemoteCalibration] Starting remote calibration...");
    
    pump.stop();
    pump.setCurrentPosition(0);
    pump.setMode(PumpMode::DOSING);
    pump.enablePump();
    
    float stepsPerML = pump.getDosingStepsPerML();
    Serial.printf("[RemoteCalibration] stepsPerML: %.2f\n", stepsPerML);
    
    remoteCalibrationTotalSteps = (long)(CALIBRATION_VOLUME * stepsPerML);
    Serial.printf("[RemoteCalibration] target steps: %ld\n", remoteCalibrationTotalSteps);
    
    pump.moveML(CALIBRATION_VOLUME);
    
    remoteCalibrationInProgress = true;
    remoteCalibrationStartTime = millis();
    remoteCalibrationLastPosition = 0;
    
    display.setState(DisplayManager::DisplayState::CALIBRATE_PROGRESS);
    char startText[64];
    snprintf(startText, sizeof(startText), "Remote Cal...\n%ld / %ld steps\n0%%", remoteCalibrationTotalSteps, remoteCalibrationTotalSteps);
    display.showText(startText);
    
    Serial.printf("[RemoteCalibration] Running %ld steps at %.0f steps/sec\n", remoteCalibrationTotalSteps, 2000.0f);
    Serial.printf("[RemoteCalibration] isEnable=%d, isRunning=%d\n", pump.getIsEnable(), pump.isRunning());
}

bool updateRemoteCalibration() {
    if (!remoteCalibrationInProgress) {
        return false;
    }
    
    pump.runDosing();
    
    long currentPosition = pump.getCurrentPosition();
    
    if (currentPosition != remoteCalibrationLastPosition) {
        remoteCalibrationLastPosition = currentPosition;
    }
    
    if (millis() - remoteCalibrationStartTime >= 1000) {
        Serial.printf("[RemoteCalibration] Progress: %ld / %ld steps, running=%d\n", 
                     currentPosition, remoteCalibrationTotalSteps, pump.isRunning());
        
        char progressText[64];
        int percent = (int)((currentPosition * 100) / remoteCalibrationTotalSteps);
        snprintf(progressText, sizeof(progressText), "Remote Cal...\n%ld / %ld steps\n%d%%", 
                 currentPosition, remoteCalibrationTotalSteps, percent);
        display.showText(progressText);
        
        remoteCalibrationStartTime = millis();
    }
    
    unsigned long elapsed = millis() - remoteCalibrationStartTime;
    if (elapsed > 180000) {
        Serial.printf("[RemoteCalibration] TIMEOUT after %lu seconds, forcing complete\n", elapsed/1000);
        remoteCalibrationInProgress = false;
        return true;
    }
    
    if (!pump.isRunning() || currentPosition >= remoteCalibrationTotalSteps) {
        Serial.printf("[RemoteCalibration] Complete - %ld steps\n", currentPosition);
        remoteCalibrationInProgress = false;
        return true;
    }
    
    return false;
}

void cancelRemoteCalibration() {
    remoteCalibrationInProgress = false;
    pump.stop();
    display.setState(DisplayManager::DisplayState::NORMAL);
    Serial.println("[RemoteCalibration] Cancelled by user");
}

void saveRemoteCalibration(float measuredML) {
    PumpController &pump = PumpController::getInstance();
    long actualSteps = pump.getCurrentPosition();
    float newStepsPerML = (float)actualSteps / measuredML;
    pump.saveStepsPerML(newStepsPerML);
    Serial.printf("[Calibration] Saved: %.2f steps/ml (from %ld steps / %.2f ml)\n", newStepsPerML, actualSteps, measuredML);
}
