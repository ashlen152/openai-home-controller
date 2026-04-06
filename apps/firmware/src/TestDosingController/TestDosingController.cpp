#include "TestDosingController.h"
#include <PumpController.h>
#include <DisplayManager.h>

static bool testDoseInProgress = false;
static unsigned long testDoseStartTime = 0;
static long testDoseTargetSteps = 0;

void startRemoteTestDose(long steps, int speed) {
    PumpController &pump = PumpController::getInstance();
    DisplayManager &display = DisplayManager::getInstance();
    
    testDoseTargetSteps = steps;
    
    Serial.printf("[TestDose] Starting: %ld steps (speed: %d)\n", steps, speed);
    
    pump.setMode(PumpMode::DOSING);
    pump.enablePump();
    pump.moveRelative(steps);
    
    testDoseInProgress = true;
    testDoseStartTime = millis();
    
    display.setState(DisplayManager::DisplayState::CALIBRATE_PROGRESS);
    char startText[64];
    snprintf(startText, sizeof(startText), "Test Dose...\n%ld steps", steps);
    display.showText(startText);
}

bool updateRemoteTestDose() {
    if (!testDoseInProgress) {
        return false;
    }
    
    PumpController &pump = PumpController::getInstance();
    DisplayManager &display = DisplayManager::getInstance();
    
    pump.runDosing();
    
    long currentPosition = pump.getCurrentPosition();
    long distanceToGo = pump.getDistanceToGo();
    
    if (distanceToGo <= 0) {
        testDoseInProgress = false;
        pump.stop();
        display.setState(DisplayManager::DisplayState::NORMAL);
        
        float dispensedML = (float)currentPosition / pump.getStepsPerML();
        Serial.printf("[TestDose] DONE: %.2f ml\n", dispensedML);
        
        char completeText[64];
        snprintf(completeText, sizeof(completeText), "Test Dose Done!\n%.2f mL", dispensedML);
        display.showText(completeText);
        
        return true;
    }
    
    unsigned long elapsed = millis() - testDoseStartTime;
    if (elapsed > 300000) {
        testDoseInProgress = false;
        pump.stop();
        display.setState(DisplayManager::DisplayState::NORMAL);
        Serial.println("[TestDose] TIMEOUT");
        return false;
    }
    
    return false;
}

void cancelRemoteTestDose() {
    testDoseInProgress = false;
    PumpController &pump = PumpController::getInstance();
    DisplayManager &display = DisplayManager::getInstance();
    pump.stop();
    display.setState(DisplayManager::DisplayState::NORMAL);
    Serial.println("[TestDose] Cancelled by user");
}
