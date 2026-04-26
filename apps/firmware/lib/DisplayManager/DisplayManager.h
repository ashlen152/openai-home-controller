/**
 * @file DisplayManager.h
 * @brief SSD1306 OLED display manager for SmartPump UI.
 *
 * Manages a 128x64 OLED display via I2C. Uses a state machine (DisplayState enum)
 * to determine which screen to render. Display data is passed through a DisplayContext
 * struct, which is populated by controllers before calling updateDisplayState().
 *
 * Singleton pattern - access via DisplayManager::getInstance().
 *
 * Screen hierarchy:
 *   NORMAL -> MENU -> CALIBRATE_BEGIN/SETTINGS/DOSING_SETUP
 *   DOSING_MANUAL_BEGIN -> DOSING_MANUAL_START -> DOSING_MANUAL_PROGRESS -> DOSING_MANUAL_COMPLETE
 *   CALIBRATE_BEGIN -> CALIBRATE_PROGRESS -> CALIBRATE_COMPLETE
 *
 * @note Known bug: updateDisplayState() resets lastUpdate=0 on every call,
 *       defeating the 200ms throttle. See AGENTS.md Known Issues #7.
 * @note Known bug: DisplayManager.cpp references CALIBRATION_START/INPUT/RESULT
 *       which don't exist in the enum. See AGENTS.md Known Issues #3.
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <vector>

/**
 * @struct DisplayContext
 * @brief Data container passed to display rendering functions.
 * Populated by controllers (DisplayUpdater, ManualDosingController, etc.)
 * before calling updateDisplayState().
 */
struct DisplayContext
{
    bool pumpEnabled = false;
    float value = 0.0f;
    const char *currentTime = nullptr;
    bool autodosingEnabled = false;
    const char *nextSchedule = nullptr;
    int menuIndex = 0;
    const char **m_menuItems = nullptr;
    int m_menuItemCount = 0;
    int currentSpeed = 0;
    float stepsPerML = 0.0f;
    int speedStep = 0;
    int activeProfile = 1;
    int timeLeft = 0;
    float ml = 0.0f;
    float remainingVolume = 0.0f;
    const char *remainingTime = nullptr;
    float totalVolume = 0.0f;
    int duration = 1;
    bool serverConnected = false;
};

class DisplayManager
{
public:
    // Enums
    enum class PumpMode
    {
        PERISTALTIC,
        DOSING
    };

    enum class DisplayState
    {
        NORMAL,
        MENU,
        SETTINGS,
        CALIBRATE_BEGIN,
        CALIBRATE_PROGRESS,
        CALIBRATE_COMPLETE,
        DOSING_SETUP,
        DOSING_BEGIN,
        DOSING_PROGRESS,
        DOSING_COMPLETE,
        DOSING_MANUAL_START,
        DOSING_MANUAL_BEGIN,
        DOSING_MANUAL_PROGRESS,
        DOSING_MANUAL_COMPLETE,
        DOSE_HISTORY,      // Phase 3 Sprint 6
        STATUS,
        INFO,
        ERROR,
        WARNING,
        SUCCESS,
    };

    enum class DosingState
    {
        IDLE,
        SETUP_VOLUME,
        SETUP_TIME,
        RUNNING,
        PAUSED,
        COMPLETED
    };

    // Singleton access
    static DisplayManager &getInstance();

    // Initialization
    void begin();

    // State management
    void setState(DisplayManager::DisplayState state) { m_currentState = state; m_dirty = true; }
    DisplayState getCurrentState() const { return m_currentState; }
    bool isSleeping() const { return m_displaySleeping; }

    // Status & Info
    void updateStatus(bool pumpEnabled, float value, const char *currentTime = nullptr, bool autodosingEnabled = false, const char *nextSchedule = nullptr, float totalVolume = 0.0f, float stepsPerML = 0.0f, int activeProfile = 1);
    void setSignalStrength(int strength);

    // Sleep/Wake
    void sleepDisplay();
    void wakeDisplay();

    // Menu/UI
    void showMenu(int menuIndex, const char *menuItems[], int itemCount);
    void showSettingsInfo(int currentSpeed, float stepsPerML, int speedStep);
    void showHomeStatus(float stepsPerML, int activeProfile);

    // Calibration UI
    void showCalibrationStart(int timeLeft);
    void showCalibrationInput(float ml);
    void showCalibrationResult(float stepsPerML, int speedStep);

    // Text display
    void showText(const char *text);
    void showText(const std::vector<String> &textArray);
    void showValue(const char *label, float value);

    // Main loop display updater
    void updateDisplayState();

    // Manual dosing UI
    void showDosingManualSetup(float volume);
    void showDosingManualBegin(int du);
    void showDosingManualProgress(float volume, float remainingVolume, const char *remainingTime);
    void showDosingManualComplete(float totalVolume);
    void showDoseHistory();  // Phase 3 Sprint 6

    // Display constants
    static const int SCREEN_WIDTH = 128;
    static const int SCREEN_HEIGHT = 64;
    static const int OLED_RESET = -1;

    // Context setter functions for each state
    void setContextNormal(bool pumpEnabled, float value, const char *currentTime, bool autodosingEnabled, const char *nextSchedule, float totalVolume = 0.0f, float stepsPerML = 0.0f, int activeProfile = 1)
    {
        m_ctx.pumpEnabled = pumpEnabled;
        m_ctx.value = value;
        m_ctx.currentTime = currentTime;
        m_ctx.autodosingEnabled = autodosingEnabled;
        m_ctx.nextSchedule = nextSchedule;
        m_ctx.totalVolume = totalVolume;
        m_ctx.stepsPerML = stepsPerML;
        m_ctx.activeProfile = activeProfile;
    }
    void setServerConnected(bool connected)
    {
        m_ctx.serverConnected = connected;
    }
    void setContextMenu(int menuIndex, const char **menuItems, int itemCount)
    {
        m_ctx.menuIndex = menuIndex;
        m_ctx.m_menuItems = menuItems;
        m_ctx.m_menuItemCount = itemCount;
    }
    void setContextSettings(int currentSpeed, float stepsPerML, int speedStep)
    {
        m_ctx.currentSpeed = currentSpeed;
        m_ctx.stepsPerML = stepsPerML;
        m_ctx.speedStep = speedStep;
    }
    void setContextCalibrationStart(int timeLeft)
    {
        m_ctx.timeLeft = timeLeft;
    }
    void setContextCalibrationInput(float ml)
    {
        m_ctx.ml = ml;
    }
    void setContextCalibrationResult(float stepsPerML, int speedStep)
    {
        m_ctx.stepsPerML = stepsPerML;
        m_ctx.speedStep = speedStep;
    }
    void setContextDosingManualBegin(float volume)
    {
        m_ctx.value = volume;
    }
    void setContextDosingManualStart(int duration)
    {
        m_ctx.duration = duration;
    }
    void setContextDosingManualProgress(float volume, float remainingVolume, const char *remainingTime)
    {
        m_ctx.value = volume;
        m_ctx.remainingVolume = remainingVolume;
        m_ctx.remainingTime = remainingTime;
    }
    void setContextDosingManualComplete(float totalVolume)
    {
        m_ctx.totalVolume = totalVolume;
    }

private:
    // Singleton pattern
    DisplayManager();
    DisplayManager(const DisplayManager &) = delete;
    DisplayManager &operator=(const DisplayManager &) = delete;

    // Internal helpers
    bool isDisplayInUse(DisplayState state);
    void displaySignalStrength();
    void drawWiFiSignal(int strength);

    Adafruit_SSD1306 m_display;
    bool m_displaySleeping = false;
    unsigned long m_lastUpdate = 0;
    bool m_dirty = true;
    DisplayState m_currentState = DisplayState::NORMAL;
    DisplayState lastState = DisplayState::NORMAL;
    unsigned long stateChangeTime = 0;
    int rssi = 0;
    DisplayContext m_ctx;
    DisplayContext m_prevCtx;
};

#endif
