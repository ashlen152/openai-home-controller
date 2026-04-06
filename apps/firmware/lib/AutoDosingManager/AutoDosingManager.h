/**
 * @file AutoDosingManager.h
 * @brief Automated dosing scheduler with weighted day/night distribution (WIP).
 *
 * Implements a time-based dosing schedule that splits daily volume between
 * day and night periods with configurable weighting (default: 60/40).
 *
 * Schedule generation:
 *   - Divides 24h into N slots (default 48 = every 30 minutes)
 *   - Day period (11:00-23:00): receives 60% of daily volume
 *   - Night period (23:00-11:00): receives 40% of daily volume
 *   - Each dose within a period is equally sized
 *
 * State is persisted to EEPROM (enabled, daily volume, last dose time, total dosed).
 *
 * @note WIP: generateWeightedSchedule(), checkAndDose(), resetDailyVolume(),
 *       performDosing(), getRemainingDailyVolume() are declared but incomplete.
 *       See AGENTS.md Known Issues #4.
 * @note Not currently instantiated in main.cpp (commented out).
 *
 * @see autoschedule.cpp for standalone simulation of the weighted schedule algorithm.
 */

#include <vector>
#ifndef AUTO_DOSING_MANAGER_H
#define AUTO_DOSING_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "../PumpController/PumpController.h"
#include <time.h>
#include "../../include/Config.h"

// Forward declarations to avoid circular dependency
class DisplayManager;

#define DEBUG_AUTO_DOSING 1
#if DEBUG_AUTO_DOSING
#define AUTO_DOSING_LOG(format, ...) Serial.printf("[AutoDosing] " format "\n", ##__VA_ARGS__)
#else
#define AUTO_DOSING_LOG(format, ...)
#endif

/**
 * @struct DoseSchedule
 * @brief A single scheduled dose entry with time and volume.
 */
struct DoseSchedule {
    int hour;        ///< Hour of day (0-23)
    int minute;      ///< Minute of hour (0-59)
    float ml;        ///< Volume to dispense (mL)
    bool completed;  ///< Whether this dose has been dispensed today
};

/**
 * @struct DosingSchedule
 * @brief Metadata for the daily dosing schedule configuration and state.
 */
struct DosingSchedule {
    float totalDailyVolume;      ///< Total volume for the day (e.g. 30mL)
    float dayVolume;             ///< Portion allocated to day period (e.g. 60%)
    float nightVolume;           ///< Portion allocated to night period (e.g. 40%)
    uint8_t dayStartHour;        ///< Day period start hour (default: 11)
    uint8_t dayEndHour;          ///< Day period end hour (default: 23)
    uint32_t lastDosingTime;     ///< Unix timestamp of last dose
    uint32_t nextDosingTime;     ///< Unix timestamp of next scheduled dose
    bool enabled;                ///< Auto-dosing enabled state
    float lastDoseVolume;        ///< Volume of last dose for logging
    uint32_t totalDosesDay;      ///< Count of doses dispensed during day period
    uint32_t totalDosesNight;    ///< Count of doses dispensed during night period
};

/**
 * @struct DoseHistoryEntry
 * @brief A single dose history record (Phase 3 Sprint 6).
 * 
 * Size: 12 bytes (4 + 4 + 1 + 3 padding)
 * EEPROM layout: 5 entries × 12 bytes = 60 bytes total
 */
struct DoseHistoryEntry {
    uint32_t timestamp;  ///< Unix time when dose occurred
    float volume;        ///< Volume dispensed in mL
    bool success;        ///< Whether dose completed successfully
    // 3 bytes padding for alignment
};

class AutoDosingManager {
public:
    // Singleton access
    static AutoDosingManager& getInstance();
    
    struct Config {
        uint16_t enabledAddr;     // EEPROM address for enabled state
        uint16_t volumeAddr;      // EEPROM address for daily volume
        uint16_t lastTimeAddr;    // EEPROM address for last dosing time
        uint16_t totalDosedAddr;  // EEPROM address for total dosed volume
        uint16_t dayStartHourAddr; // EEPROM address for day start hour
        uint16_t dayEndHourAddr;   // EEPROM address for day end hour
        uint16_t dayPercentAddr;   // EEPROM address for day percent
        float defaultVolume;      // Default daily volume in mL
    };

    // Initialization (call after getInstance, before other methods)
    void initialize(PumpController& pump, DisplayManager& display, const Config& config);
    void begin();
    void enable();
    void disable();
    void setDailyVolume(float volume);
    float getDailyVolume() const { return scheduleMeta.totalDailyVolume; }
    void setDayPeriod(uint8_t startHour, uint8_t endHour);
    void setDayNightSplit(uint8_t dayPercent);
    uint8_t getDayStartHour() const { return startHour; }
    uint8_t getDayEndHour() const { return endHour; }
    uint8_t getDayPercent() const { return (uint8_t)(percent1 * 100); }
    void updateSchedule();
    void checkAndDose();
    void updateDosingProgress();  // Phase 4: Check and complete in-progress doses
    bool isEnabled() const { return scheduleMeta.enabled; }
    uint32_t getNextDosingTime() const { return scheduleMeta.nextDosingTime; }
    float getRemainingDailyVolume() const;
    void loadState();
    void saveState();
    void resetDailyVolume();  // Called from main.cpp midnight reset handler
    
    // Pause/Resume functionality (Phase 3 Sprint 5)
    void pause(uint32_t durationSeconds);  // Pause for specified duration (0 = indefinite)
    void resume();                          // Resume auto-dosing
    bool isPaused() const;                  // Check if currently paused
    uint32_t getPauseRemaining() const;     // Seconds remaining in pause (0 = not paused or indefinite)
    
    // Dose History (Phase 3 Sprint 6)
    const DoseHistoryEntry* getDoseHistory(uint8_t& count) const;  // Returns array of history entries and count
    void clearDoseHistory();                 // Clear all history entries
    
    // Getter methods for time fallback and totals (Phase 3 Sprint 10)
    uint32_t getLastSyncMillis() const { return lastSyncMillis; }
    time_t getLastSyncTime() const { return lastSyncTime; }
    float getTotalDosedVolume() const { return totalDosedVolume; }
    
    // Settings sync (Phase 4)
    void syncSettings();  // POST current settings to server
    
    // Debug functions
    void printStatus() const;
    void printSchedule() const;

private:
    // Singleton pattern - private constructor
    AutoDosingManager();
    AutoDosingManager(const AutoDosingManager&) = delete;
    AutoDosingManager& operator=(const AutoDosingManager&) = delete;
    
    void generateWeightedSchedule(int slots, float totalMl, int startHour, int endHour, float percent1, float percent2);
    bool performDosing(float volume);
    void logDosingEvent(float volume, bool success, bool isStart = false);  // Phase 4: Added isStart parameter
    
    // Dose History helpers (Phase 3 Sprint 6)
    void addDoseToHistory(uint32_t timestamp, float volume, bool success);
    void loadDoseHistory();
    void saveDoseHistory();
    
    // Member variables
    PumpController* p_pump;
    DisplayManager* p_display;
    bool m_initialized;
    DosingSchedule scheduleMeta;
    float totalDosedVolume;
    Config eepromConfig;
    std::vector<DoseSchedule> schedule;
    int slots = ::Config::DEFAULT_SCHEDULE_SLOTS;
    int startHour = ::Config::DEFAULT_DAY_START_HOUR;
    int endHour = ::Config::DEFAULT_DAY_END_HOUR;
    float percent1 = ::Config::DEFAULT_DAY_PERCENT / 100.0f;
    float percent2 = (100 - ::Config::DEFAULT_DAY_PERCENT) / 100.0f;
    
    // Time sync fallback (Phase 3 Sprint 4)
    uint32_t lastSyncMillis;  ///< millis() when time was last synced
    time_t lastSyncTime;      ///< Epoch time from last successful sync
    
    // Pause state (Phase 3 Sprint 5)
    bool paused;              ///< Whether auto-dosing is currently paused
    uint32_t pauseUntil;      ///< Unix timestamp when pause expires (0 = indefinite)
    
    // Dosing state machine (Phase 4 Sprint 1 - Fix)
    enum class DosingState { IDLE, IN_PROGRESS };
    DosingState dosingState;       ///< Current dosing operation state
    float pendingDoseVolume;       ///< Volume of dose currently in progress
    unsigned long dosingStartTime; ///< millis() when current dose started
    uint32_t dosingTimestamp;     ///< Unix timestamp when dose started (for server storage)
    
    // Dose History (Phase 3 Sprint 6)
    static constexpr uint8_t DOSE_HISTORY_SIZE = 5;
    DoseHistoryEntry doseHistory[DOSE_HISTORY_SIZE];
    uint8_t doseHistoryCount;     ///< Number of valid entries (0-5)
    uint8_t doseHistoryHead;      ///< Index of oldest entry (ring buffer head)
};

#endif