#include "ManualHandler.h"
#include "ViewController/Manual/ManualHandler.h"
#include <DisplayManager.h>
#include "ButtonController/ButtonController.h"
#include "ManualDosingController/ManualDosingController.h"

static DisplayManager &display = DisplayManager::getInstance();

void ManualHandler()
{
    if (!isInManual())
    {
        beginManualDosingController(false);  // Initialize state (not already in state)
    }
    else if (isInManualBegin())
    {
        startManualDosingController(isInManualStart());
    }
    else if (isInManualStart())
    {
        progressManualDosingController(isInManualProgress());
    }
    else if (isInManualProgress())
    {
        completeManualDosingController(isInManualComplete());
    }
    else if (isInManualComplete())
    {
    }
}

bool isInManual()
{
    if (display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_BEGIN || display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_START || display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS || display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_COMPLETE)
        return true;
    return false;
}

bool isInManualBegin()
{
    if (display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_BEGIN)
        return true;
    return false;
}

bool isInManualProgress()
{
    if (display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS)
        return true;
    return false;
}

bool isInManualStart()
{
    if (display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_START)
        return true;
    return false;
}

bool isInManualComplete()
{
    if (display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_COMPLETE)
        return true;
    return false;
}