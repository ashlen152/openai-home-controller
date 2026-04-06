/**
 * @file AutoDosingManager.cpp
 * @brief Implementation of automated dosing scheduler with weighted day/night distribution.
 * 
 * Phase 1 Implementation:
 * - Singleton pattern for global access
 * - All 6 core functions implemented (generateWeightedSchedule, updateSchedule, checkAndDose, 
 *   performDosing, resetDailyVolume, getRemainingDailyVolume)
 * - Edge case handling (time not synced, pump busy, nullptr checks)
 * - Comprehensive logging with DEBUG flag support
 */

#include "AutoDosingManager.h"
#include "../DisplayManager/DisplayManager.h"
#include <EEPROM.h>
#include <time.h>
#include <WiFi.h>
#include "../../include/Config.h"
#include <NetworkTaskManager.h>
#include <ArduinoJson.h>
#include <ConfigManager.h>

// Singleton instance
static AutoDosingManager* s_instance = nullptr;

// Helper function for time conversion
static time_t toTimeT(uint32_t t) {
    return static_cast<time_t>(t);
}

// ============================================================================
// SINGLETON PATTERN
// ============================================================================

AutoDosingManager& AutoDosingManager::getInstance() {
    if (!s_instance) {
        s_instance = new AutoDosingManager();
    }
    return *s_instance;
}

// Private constructor
AutoDosingManager::AutoDosingManager() 
    : p_pump(nullptr)
    , p_display(nullptr)
    , m_initialized(false)
    , totalDosedVolume(0.0f)
    , lastSyncMillis(0)
    , lastSyncTime(0)
    , paused(false)
    , pauseUntil(0)
    , dosingState(DosingState::IDLE)  // Phase 4: Initialize state machine
    , pendingDoseVolume(0.0f)
    , dosingStartTime(0)
    , doseHistoryCount(0)
    , doseHistoryHead(0)
{
    scheduleMeta.enabled = false;
    scheduleMeta.totalDailyVolume = 0.0f;
    scheduleMeta.dayStartHour = startHour;
    scheduleMeta.dayEndHour = endHour;
    scheduleMeta.lastDosingTime = 0;
    scheduleMeta.nextDosingTime = 0;
    scheduleMeta.lastDoseVolume = 0;
    scheduleMeta.totalDosesDay = 0;
    scheduleMeta.totalDosesNight = 0;
    
    // Initialize dose history array (Phase 3 Sprint 6)
    for (uint8_t i = 0; i < DOSE_HISTORY_SIZE; i++) {
        doseHistory[i].timestamp = 0;
        doseHistory[i].volume = 0.0f;
        doseHistory[i].success = false;
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void AutoDosingManager::initialize(PumpController& pump, DisplayManager& display, const Config& config) {
    if (m_initialized) {
        AUTO_DOSING_LOG("WARN: Already initialized, skipping re-initialization");
        return;
    }
    
    AUTO_DOSING_LOG("Initializing AutoDosingManager...");
    
    // Store references
    p_pump = &pump;
    p_display = &display;
    eepromConfig = config;
    
    // Set defaults
    scheduleMeta.totalDailyVolume = config.defaultVolume;
    scheduleMeta.dayStartHour = startHour;
    scheduleMeta.dayEndHour = endHour;
    
    // Generate initial schedule
    generateWeightedSchedule(slots, scheduleMeta.totalDailyVolume, startHour, endHour, percent1, percent2);
    
    m_initialized = true;
    AUTO_DOSING_LOG("Initialized successfully with %.2f ml daily volume", scheduleMeta.totalDailyVolume);
}

void AutoDosingManager::begin() {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: begin() called before initialize()");
        return;
    }
    
    AUTO_DOSING_LOG("Starting Auto Dosing Manager");
    loadState();
    loadDoseHistory();  // Phase 3 Sprint 6
    updateSchedule();  // Calculate next dose time
}

// ============================================================================
// ENABLE/DISABLE
// ============================================================================

void AutoDosingManager::enable() {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: enable() called before initialize()");
        return;
    }
    
    AUTO_DOSING_LOG("Enabling auto dosing");
    scheduleMeta.enabled = true;
    resetDailyVolume();
    saveState();
}

void AutoDosingManager::disable() {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: disable() called before initialize()");
        return;
    }
    
    AUTO_DOSING_LOG("Disabling auto dosing");
    scheduleMeta.enabled = false;
    saveState();
}

void AutoDosingManager::setDailyVolume(float volume) {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: setDailyVolume() called before initialize()");
        return;
    }
    
    // Validate input
    if (volume < 1.0f || volume > 200.0f) {
        AUTO_DOSING_LOG("ERROR: Invalid daily volume %.2f ml (must be 1-200)", volume);
        return;
    }
    
    AUTO_DOSING_LOG("Setting daily volume to %.2f ml", volume);
    scheduleMeta.totalDailyVolume = volume;
    
    // Regenerate schedule with new volume
    generateWeightedSchedule(slots, scheduleMeta.totalDailyVolume, startHour, endHour, percent1, percent2);
    updateSchedule();
    saveState();
}

// ============================================================================
// DAY/NIGHT CONFIGURATION (Phase 2)
// ============================================================================

void AutoDosingManager::setDayPeriod(uint8_t newStartHour, uint8_t newEndHour) {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: setDayPeriod() called before initialize()");
        return;
    }
    
    // Validate input
    if (newStartHour > 23 || newEndHour > 23) {
        AUTO_DOSING_LOG("ERROR: Invalid hour values (start=%d, end=%d) - must be 0-23", 
                        newStartHour, newEndHour);
        return;
    }
    
    // Allow wrap-around (e.g., 23:00 to 05:00 = 23-5 crosses midnight)
    AUTO_DOSING_LOG("Setting day period: %02d:00 to %02d:00%s", 
                    newStartHour, newEndHour,
                    (newEndHour <= newStartHour) ? " (crosses midnight)" : "");
    
    startHour = newStartHour;
    endHour = newEndHour;
    scheduleMeta.dayStartHour = newStartHour;
    scheduleMeta.dayEndHour = newEndHour;
    
    // Regenerate schedule with new period
    generateWeightedSchedule(slots, scheduleMeta.totalDailyVolume, startHour, endHour, percent1, percent2);
    updateSchedule();
    saveState();
}

void AutoDosingManager::setDayNightSplit(uint8_t dayPercent) {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: setDayNightSplit() called before initialize()");
        return;
    }
    
    // Validate input
    if (dayPercent > 100) {
        AUTO_DOSING_LOG("ERROR: Invalid day percent %d (must be 0-100)", dayPercent);
        return;
    }
    
    uint8_t nightPercent = 100 - dayPercent;
    AUTO_DOSING_LOG("Setting day/night split: %d%% day / %d%% night", dayPercent, nightPercent);
    
    percent1 = dayPercent / 100.0f;
    percent2 = nightPercent / 100.0f;
    
    // Regenerate schedule with new split
    generateWeightedSchedule(slots, scheduleMeta.totalDailyVolume, startHour, endHour, percent1, percent2);
    updateSchedule();
    saveState();
}


// ============================================================================
// CORE FUNCTION 1: generateWeightedSchedule()
// ============================================================================

void AutoDosingManager::generateWeightedSchedule(int slots, float totalMl, 
                                                  int startHour, int endHour, 
                                                  float percent1, float percent2) {
    AUTO_DOSING_LOG("Generating weighted schedule: %d slots, %.2f ml total", slots, totalMl);
    
    schedule.clear();
    
    if (slots <= 0 || totalMl <= 0) {
        AUTO_DOSING_LOG("ERROR: Invalid parameters (slots=%d, totalMl=%.2f)", slots, totalMl);
        return;
    }
    
    int intervalMinutes = 1440 / slots;  // e.g., 48 slots = 30 min each
    
    float total1 = totalMl * percent1;  // Day volume (60%)
    float total2 = totalMl * percent2;  // Night volume (40%)
    
    // Count slots in each period
    int count1 = 0, count2 = 0;
    for (int i = 0; i < slots; i++) {
        int minutes = i * intervalMinutes;
        int h = minutes / 60;
        if (h >= startHour && h < endHour) count1++;
        else count2++;
    }
    
    // Calculate mL per dose for each period
    float mlPerDose1 = (count1 > 0) ? total1 / count1 : 0;
    float mlPerDose2 = (count2 > 0) ? total2 / count2 : 0;
    
    // Generate schedule
    for (int i = 0; i < slots; i++) {
        int minutes = i * intervalMinutes;
        DoseSchedule entry;
        entry.hour = minutes / 60;
        entry.minute = minutes % 60;
        entry.ml = (entry.hour >= startHour && entry.hour < endHour) ? mlPerDose1 : mlPerDose2;
        entry.completed = false;
        schedule.push_back(entry);
    }
    
    AUTO_DOSING_LOG("Schedule generated: Day period (%02d:00-%02d:00): %.3f ml x %d doses", 
                    startHour, endHour, mlPerDose1, count1);
    AUTO_DOSING_LOG("                   Night period: %.3f ml x %d doses", mlPerDose2, count2);
}

// ============================================================================
// CORE FUNCTION 2: updateSchedule()
// ============================================================================

void AutoDosingManager::updateSchedule() {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: updateSchedule() called before initialize()");
        return;
    }
    
    time_t now = time(nullptr);
    if (now <= 0) {
        AUTO_DOSING_LOG("WARN: Time not synced, cannot calculate next dose");
        scheduleMeta.nextDosingTime = 0;
        return;
    }
    
    struct tm *timeinfo = localtime(&now);
    int currentHour = timeinfo->tm_hour;
    int currentMinute = timeinfo->tm_min;
    
    scheduleMeta.nextDosingTime = 0;
    for (const auto &entry : schedule) {
        if (!entry.completed) {
            // Check if this dose is in the future today
            if (entry.hour > currentHour || 
               (entry.hour == currentHour && entry.minute > currentMinute)) {
                struct tm nextTime = *timeinfo;
                nextTime.tm_hour = entry.hour;
                nextTime.tm_min = entry.minute;
                nextTime.tm_sec = 0;
                scheduleMeta.nextDosingTime = mktime(&nextTime);
                AUTO_DOSING_LOG("Next dose scheduled: %02d:%02d (%.2f ml)", 
                               entry.hour, entry.minute, entry.ml);
                break;
            }
        }
    }
    
    if (scheduleMeta.nextDosingTime == 0) {
        AUTO_DOSING_LOG("No more doses scheduled for today");
    }
}

// ============================================================================
// CORE FUNCTION 3: checkAndDose()
// ============================================================================

static bool isTimeReadyForDosing(time_t now, time_t lastSyncTime, uint32_t lastSyncMillis) {
    if (now > 946684800) return true;
    if (lastSyncTime == 0) return false;
    uint32_t elapsed = (millis() - lastSyncMillis) / 1000;
    time_t fallback = lastSyncTime + elapsed;
    return fallback > 1577836800;
}

void AutoDosingManager::checkAndDose() {
    // Early returns for edge cases
    if (!m_initialized) {
        return;  // Silent return - will be spammy in logs
    }
    
    if (!scheduleMeta.enabled) {
        return;  // Silent return - auto-dosing is off
    }
    
    // Check pause state (Phase 3 Sprint 5)
    if (paused) {
        // Check if timed pause has expired
        if (pauseUntil > 0) {
            time_t now = time(nullptr);
            if (now == (time_t)-1) {
                now = lastSyncTime + ((millis() - lastSyncMillis) / 1000);
            }
            
            if (now >= pauseUntil) {
                // Auto-resume
                AUTO_DOSING_LOG("Timed pause expired - auto-resuming");
                resume();
                // Continue with dose check
            } else {
                return;  // Still paused
            }
        } else {
            return;  // Indefinite pause
        }
    }
    
    time_t now = time(nullptr);
    if (!isTimeReadyForDosing(now, lastSyncTime, lastSyncMillis)) {
        static unsigned long lastWarnTime = 0;
        if (millis() - lastWarnTime > 60000) {
            AUTO_DOSING_LOG("WARN: Time not ready for dosing - waiting for NTP sync");
            lastWarnTime = millis();
        }
        return;
    }
    
    if (now <= 0) {
        // Fallback: Use millis() offset + last known time
        if (lastSyncTime > 0 && lastSyncMillis > 0) {
            uint32_t elapsedSeconds = (millis() - lastSyncMillis) / 1000;
            now = lastSyncTime + elapsedSeconds;
            
            static unsigned long lastFallbackLog = 0;
            if (millis() - lastFallbackLog > 300000) {  // Log every 5 minutes
                AUTO_DOSING_LOG("Using fallback time (WiFi down): %ld", now);
                lastFallbackLog = millis();
            }
        } else {
            // No fallback available
            static unsigned long lastWarnTime = 0;
            if (millis() - lastWarnTime > 60000) {  // Warn once per minute
                AUTO_DOSING_LOG("WARN: Cannot check doses - time not synced and no fallback");
                lastWarnTime = millis();
            }
            return;
        }
    } else {
        // Time is synced - update fallback baseline
        lastSyncTime = now;
        lastSyncMillis = millis();
    }
    
    struct tm *timeinfo = localtime(&now);
    int currentHour = timeinfo->tm_hour;
    int currentMinute = timeinfo->tm_min;
    
    // Check if current time matches any schedule entry
    for (auto &entry : schedule) {
        if (entry.hour == currentHour && 
            entry.minute == currentMinute && 
            !entry.completed &&
            dosingState == DosingState::IDLE) {  // Phase 4: Only start if not already dosing
            
            AUTO_DOSING_LOG("✓ Dose time matched: %02d:%02d - %.2f ml", 
                           entry.hour, entry.minute, entry.ml);
            
            if (performDosing(entry.ml)) {
                // Dose started successfully - state machine now IN_PROGRESS
                entry.completed = true;  // Mark as completed so we don't retry next second
            } else {
                AUTO_DOSING_LOG("✗ Dosing failed to start");
                // Don't log event if it didn't even start
            }
            
            // Only dose once per minute (avoid multiple triggers)
            break;
        }
    }
}

// ============================================================================
// CORE FUNCTION 4: performDosing()
// ============================================================================

bool AutoDosingManager::performDosing(float volume) {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: performDosing() called before initialize()");
        return false;
    }
    
    // Validate volume
    if (volume <= 0 || volume > 50.0f) {  // Safety limit: max 50mL per dose
        AUTO_DOSING_LOG("ERROR: Invalid dose volume %.2f ml", volume);
        return false;
    }
    
    // Safety check: don't interrupt manual dosing
    if (p_pump->getMode() == PumpMode::DOSING && p_pump->isRunning()) {
        AUTO_DOSING_LOG("Skipping auto-dose: manual dosing in progress");
        return false;
    }
    
    // Safety check: ensure pump is calibrated
    if (p_pump->getDosingStepsPerML() <= 0) {
        AUTO_DOSING_LOG("ERROR: Pump not calibrated (stepsPerML = 0)");
        return false;
    }
    
    AUTO_DOSING_LOG(">>> Starting auto-dose: %.2f ml", volume);
    
    // Phase 4: Log dosing START event before executing
    logDosingEvent(volume, false, true);  // volume, success=false, isStart=true
    
    // Execute dosing
    p_pump->setMode(PumpMode::DOSING);
    p_pump->moveML(volume);
    
    // Phase 4: Set state machine to IN_PROGRESS (do NOT update totals yet!)
    dosingState = DosingState::IN_PROGRESS;
    pendingDoseVolume = volume;
    dosingStartTime = millis();
    
    time_t now = time(nullptr);
    if (now == (time_t)-1) {
      now = lastSyncTime + ((millis() - lastSyncMillis) / 1000);
    }
    dosingTimestamp = (uint32_t)now;
    
    // Track day/night counts (for logging purposes)
    struct tm *timeinfo = localtime(&now);
    if (timeinfo->tm_hour >= scheduleMeta.dayStartHour && 
        timeinfo->tm_hour < scheduleMeta.dayEndHour) {
        scheduleMeta.totalDosesDay++;
        AUTO_DOSING_LOG("Day dose #%d", scheduleMeta.totalDosesDay);
    } else {
        scheduleMeta.totalDosesNight++;
        AUTO_DOSING_LOG("Night dose #%d", scheduleMeta.totalDosesNight);
    }
    
    scheduleMeta.lastDosingTime = now;
    scheduleMeta.lastDoseVolume = volume;
    saveState();
    
    AUTO_DOSING_LOG(">>> Auto-dose started, waiting for completion...");
    return true;
}

// ============================================================================
// CORE FUNCTION 4B: updateDosingProgress() - NEW (Phase 4)
// ============================================================================

void AutoDosingManager::updateDosingProgress() {
    if (!m_initialized) {
        return;  // Silent return
    }
    
    // Check if we're currently dosing
    if (dosingState != DosingState::IN_PROGRESS) {
        return;  // Nothing to update
    }
    
    // Check if pump has finished (mode might already be HOLDING if runDosing() called stop())
    if (!p_pump->isRunning()) {
        // Dose complete!
        unsigned long duration = millis() - dosingStartTime;
        
        AUTO_DOSING_LOG(">>> Auto-dose COMPLETED: %.2f ml in %lu ms", 
                       pendingDoseVolume, duration);
        
        // Now update the total
        totalDosedVolume += pendingDoseVolume;
        saveState();
        
        // Log COMPLETE event to server
        logDosingEvent(pendingDoseVolume, true, false);  // volume, success=true, isStart=false
        
        AUTO_DOSING_LOG("Total dosed today: %.2f ml (%.1f%% of daily target)", 
                       totalDosedVolume,
                       (totalDosedVolume / scheduleMeta.totalDailyVolume) * 100.0f);
        
        // Reset state machine
        dosingState = DosingState::IDLE;
        pendingDoseVolume = 0.0f;
        dosingStartTime = 0;
        dosingTimestamp = 0;
        return;  // Don't run timeout check after completion
    }
    
    // Timeout check (safety: if dose takes > 5 minutes, something is wrong)
    if (millis() - dosingStartTime > 300000) {  // 5 minutes
        AUTO_DOSING_LOG("ERROR: Dose timeout! Aborting after 5 minutes");
        p_pump->stop();
        
        // Log FAILED event
        logDosingEvent(pendingDoseVolume, false, false);  // volume, success=false, isStart=false
        
        // Reset state without updating total (dose failed)
        dosingState = DosingState::IDLE;
        pendingDoseVolume = 0.0f;
        dosingStartTime = 0;
        dosingTimestamp = 0;
    }
}

// ============================================================================
// CORE FUNCTION 5: resetDailyVolume()
// ============================================================================

void AutoDosingManager::resetDailyVolume() {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: resetDailyVolume() called before initialize()");
        return;
    }
    
    AUTO_DOSING_LOG("=== MIDNIGHT RESET ===");
    AUTO_DOSING_LOG("Resetting daily volume counters");
    AUTO_DOSING_LOG("Previous totals - Day: %d doses, Night: %d doses, Total: %.2f ml",
                    scheduleMeta.totalDosesDay, scheduleMeta.totalDosesNight, totalDosedVolume);
    
    // Reset all completed flags
    for (auto &entry : schedule) {
        entry.completed = false;
    }
    
    // Reset counters
    totalDosedVolume = 0.0f;
    scheduleMeta.totalDosesDay = 0;
    scheduleMeta.totalDosesNight = 0;
    
    saveState();
    updateSchedule();  // Recalculate next dosing time
    
    AUTO_DOSING_LOG("Daily reset complete - Ready for new day");
}

// ============================================================================
// CORE FUNCTION 6: getRemainingDailyVolume()
// ============================================================================

float AutoDosingManager::getRemainingDailyVolume() const {
    if (!m_initialized) {
        return 0.0f;
    }
    
    float remaining = scheduleMeta.totalDailyVolume - totalDosedVolume;
    return (remaining > 0) ? remaining : 0.0f;
}

// ============================================================================
// STATE PERSISTENCE (EEPROM)
// ============================================================================

void AutoDosingManager::loadState() {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: loadState() called before initialize()");
        return;
    }
    
    AUTO_DOSING_LOG("Loading state from EEPROM");
    
    bool enabled = false;
    float storedDailyVolume = eepromConfig.defaultVolume;
    uint32_t lastDoseTime = 0;
    float storedTotalDosed = 0;
    uint8_t storedDayStartHour = 11;  // defaults
    uint8_t storedDayEndHour = 23;
    uint8_t storedDayPercent = 60;
    
    EEPROM.get(eepromConfig.enabledAddr, enabled);
    EEPROM.get(eepromConfig.volumeAddr, storedDailyVolume);
    EEPROM.get(eepromConfig.lastTimeAddr, lastDoseTime);
    EEPROM.get(eepromConfig.totalDosedAddr, storedTotalDosed);
    EEPROM.get(eepromConfig.dayStartHourAddr, storedDayStartHour);
    EEPROM.get(eepromConfig.dayEndHourAddr, storedDayEndHour);
    EEPROM.get(eepromConfig.dayPercentAddr, storedDayPercent);
    
    if (storedDayPercent > 100) {
        AUTO_DOSING_LOG("  WARN: dayPercent out of range (%d), clamping to 70", storedDayPercent);
        storedDayPercent = 70;
    }
    
    // Set values with validation
    scheduleMeta.enabled = enabled;
    scheduleMeta.totalDailyVolume = (!isnan(storedDailyVolume) && storedDailyVolume > 0) ? storedDailyVolume : eepromConfig.defaultVolume;
    scheduleMeta.lastDosingTime = lastDoseTime;
    totalDosedVolume = (!isnan(storedTotalDosed) && storedTotalDosed >= 0) ? storedTotalDosed : 0;
    
    // Validate and set day/night config
    if (storedDayStartHour <= 23 && storedDayEndHour <= 23) {
        startHour = storedDayStartHour;
        endHour = storedDayEndHour;
        scheduleMeta.dayStartHour = storedDayStartHour;
        scheduleMeta.dayEndHour = storedDayEndHour;
    }
    if (storedDayPercent <= 100) {
        percent1 = storedDayPercent / 100.0f;
        percent2 = (100 - storedDayPercent) / 100.0f;
    }
    
    AUTO_DOSING_LOG("Loaded from EEPROM:");
    AUTO_DOSING_LOG("  Enabled: %s", scheduleMeta.enabled ? "YES" : "NO");
    AUTO_DOSING_LOG("  Daily Volume: %.2f ml", scheduleMeta.totalDailyVolume);
    AUTO_DOSING_LOG("  Last Dose Time: %lu", scheduleMeta.lastDosingTime);
    AUTO_DOSING_LOG("  Total Dosed: %.2f ml", totalDosedVolume);
    AUTO_DOSING_LOG("  Day Period: %02d:00 to %02d:00", startHour, endHour);
    AUTO_DOSING_LOG("  Day/Night Split: %d%% / %d%%", storedDayPercent, 100 - storedDayPercent);
    
    // Load pause state (Phase 3 Sprint 5)
    EEPROM.get(::Config::EEPROM_PAUSE_STATE_ADDR, paused);
    EEPROM.get(::Config::EEPROM_PAUSE_STATE_ADDR + 1, pauseUntil);
    
    if (paused && pauseUntil > 0) {
        time_t now = time(nullptr);
        if (now != (time_t)-1 && now >= pauseUntil) {
            paused = false;
            pauseUntil = 0;
        }
        if (pauseUntil > 0 && pauseUntil != 0xFFFFFFFF) {
            time_t now = time(nullptr);
            if (now != (time_t)-1 && pauseUntil > now + 86400 * 365) {
                Serial.printf("[AutoDosing] Invalid pauseUntil: %lu, resetting\n", pauseUntil);
                paused = false;
                pauseUntil = 0;
            }
        }
        AUTO_DOSING_LOG("  Paused until: %lu", pauseUntil);
    } else if (paused) {
        AUTO_DOSING_LOG("  Paused indefinitely");
    }
    
    // Regenerate schedule with loaded settings
    generateWeightedSchedule(slots, scheduleMeta.totalDailyVolume, startHour, endHour, percent1, percent2);
}

void AutoDosingManager::saveState() {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: saveState() called before initialize()");
        return;
    }
    
    AUTO_DOSING_LOG("Saving state to EEPROM");
    
    uint8_t dayPercent = (uint8_t)(percent1 * 100);
    
    EEPROM.put(eepromConfig.enabledAddr, scheduleMeta.enabled);
    EEPROM.put(eepromConfig.volumeAddr, scheduleMeta.totalDailyVolume);
    EEPROM.put(eepromConfig.lastTimeAddr, scheduleMeta.lastDosingTime);
    EEPROM.put(eepromConfig.totalDosedAddr, totalDosedVolume);
    EEPROM.put(eepromConfig.dayStartHourAddr, (uint8_t)startHour);
    EEPROM.put(eepromConfig.dayEndHourAddr, (uint8_t)endHour);
    EEPROM.put(eepromConfig.dayPercentAddr, dayPercent);
    
    // Save pause state (Phase 3 Sprint 5)
    EEPROM.put(::Config::EEPROM_PAUSE_STATE_ADDR, paused);
    EEPROM.put(::Config::EEPROM_PAUSE_STATE_ADDR + 1, pauseUntil);
    
    EEPROM.commit();
}

// ============================================================================
// SETTINGS SYNC (Phase 4)
// ============================================================================

void AutoDosingManager::syncSettings() {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: syncSettings() called before initialize()");
        return;
    }
    
    AUTO_DOSING_LOG("Syncing settings to server...");
    
    JsonDocument doc;
    
    ConfigManager& configMgr = ConfigManager::getInstance();
    const char* pumpId = configMgr.getPumpId();
    
    doc["pumpId"] = pumpId;
    doc["enabled"] = scheduleMeta.enabled;
    doc["dailyVolume"] = scheduleMeta.totalDailyVolume;
    doc["dayStartHour"] = startHour;
    doc["dayEndHour"] = endHour;
    doc["dayPercent"] = constrain((uint8_t)(percent1 * 100), (uint8_t)0, (uint8_t)100);
    doc["stepsPerML"] = p_pump->getDosingStepsPerML();
    doc["activeProfile"] = p_pump->getActiveProfile();
    doc["pausedUntil"] = pauseUntil;
    
    doc["online"] = true;
    doc["lastHeartbeat"] = (uint32_t)time(nullptr);
    doc["wifiRssi"] = WiFi.RSSI();
    doc["ipAddress"] = WiFi.localIP().toString().c_str();
    doc["isDosing"] = (dosingState == DosingState::IN_PROGRESS);
    doc["totalDosedToday"] = totalDosedVolume;
    doc["uptimeSeconds"] = (uint32_t)(millis() / 1000);
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["lastSettingsSync"] = (uint32_t)time(nullptr);
    
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    
    AUTO_DOSING_LOG("Settings payload: %s", jsonBuffer);
    
    NetworkTaskManager& netMgr = NetworkTaskManager::getInstance();
    NetworkCommandMessage cmd;
    cmd.command = NetworkCommand::HTTP_POST_SETTINGS;
    cmd.param1 = 0;
    cmd.param2 = 0;
    strncpy(cmd.data, jsonBuffer, sizeof(cmd.data) - 1);
    cmd.data[sizeof(cmd.data) - 1] = '\0';
    
    if (netMgr.sendCommand(cmd)) {
        AUTO_DOSING_LOG("Settings sync queued successfully");
    } else {
        AUTO_DOSING_LOG("WARNING: Failed to queue settings sync (queue may be full)");
    }
}


// ============================================================================
// DEBUG/LOGGING FUNCTIONS
// ============================================================================

void AutoDosingManager::printStatus() const {
    if (!m_initialized) {
        Serial.println("[AutoDosing] Not initialized");
        return;
    }
    
    AUTO_DOSING_LOG("=== Auto Dosing Status ===");
    AUTO_DOSING_LOG("Enabled: %s", scheduleMeta.enabled ? "YES" : "NO");
    AUTO_DOSING_LOG("Daily Volume: %.2f ml", scheduleMeta.totalDailyVolume);
    AUTO_DOSING_LOG("Total Dosed Today: %.2f ml (%.1f%%)", 
                    totalDosedVolume, 
                    (totalDosedVolume / scheduleMeta.totalDailyVolume) * 100.0f);
    AUTO_DOSING_LOG("Remaining Today: %.2f ml", getRemainingDailyVolume());
    AUTO_DOSING_LOG("Last Dose: %.2f ml", scheduleMeta.lastDoseVolume);
    AUTO_DOSING_LOG("Day Doses: %d, Night Doses: %d", 
                    scheduleMeta.totalDosesDay, scheduleMeta.totalDosesNight);
    
    if (scheduleMeta.nextDosingTime > 0) {
        time_t t = scheduleMeta.nextDosingTime;
        struct tm* timeinfo = localtime(&t);
        AUTO_DOSING_LOG("Next Dose: %02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
    } else {
        AUTO_DOSING_LOG("Next Dose: Not scheduled");
    }
}

void AutoDosingManager::printSchedule() const {
    if (!m_initialized) {
        Serial.println("[AutoDosing] Not initialized");
        return;
    }
    
    AUTO_DOSING_LOG("=== Dosing Schedule ===");
    AUTO_DOSING_LOG("Total Daily: %.2f ml across %d doses", scheduleMeta.totalDailyVolume, schedule.size());
    AUTO_DOSING_LOG("Day Period: %02d:00 - %02d:00 (%.0f%% of volume)", 
                    startHour, endHour, percent1 * 100);
    AUTO_DOSING_LOG("Night Period: %02d:00 - %02d:00 (%.0f%% of volume)", 
                    endHour, startHour, percent2 * 100);
    
    // Print first 5 and last 5 doses as sample
    AUTO_DOSING_LOG("Sample doses:");
    int count = 0;
    for (const auto &entry : schedule) {
        if (count < 3 || count >= (int)schedule.size() - 3) {
            AUTO_DOSING_LOG("  %02d:%02d - %.3f ml %s", 
                           entry.hour, entry.minute, entry.ml,
                           entry.completed ? "[DONE]" : "");
        } else if (count == 3) {
            AUTO_DOSING_LOG("  ... (%d more doses) ...", (int)schedule.size() - 6);
        }
        count++;
    }
}

void AutoDosingManager::logDosingEvent(float volume, bool success, bool isStart) {
    const char* statusStr = isStart ? "STARTED" : (success ? "COMPLETED" : "FAILED");
    
    AUTO_DOSING_LOG("=== Dosing Event: %s ===", statusStr);
    AUTO_DOSING_LOG("Time: %lu", time(nullptr));
    AUTO_DOSING_LOG("Volume: %.2f ml", volume);
    if (!isStart) {
        AUTO_DOSING_LOG("Success: %s", success ? "YES" : "NO");
    }
    AUTO_DOSING_LOG("Total Today: %.2f ml", totalDosedVolume);
    AUTO_DOSING_LOG("Remaining: %.2f ml", getRemainingDailyVolume());
    
    // Phase 3 Sprint 6: Add to dose history (only on complete, not on start)
    time_t now = time(nullptr);
    if (now == (time_t)-1) {
        now = lastSyncTime + ((millis() - lastSyncMillis) / 1000);
    }
    
    if (!isStart) {
        addDoseToHistory((uint32_t)now, volume, success);
    }
    
    // Phase 4: POST dose event to server via Core 0
    NetworkTaskManager& networkTask = NetworkTaskManager::getInstance();
    
    // Build JSON payload
    ConfigManager& configMgr = ConfigManager::getInstance();
    JsonDocument doc;
    
    // Create unique event ID using timestamp in milliseconds
    unsigned long eventId = (unsigned long)now * 1000 + (millis() % 1000);
    
    doc["pumpId"] = configMgr.getPumpId();
    doc["eventId"] = String(eventId);
    doc["timestamp"] = (unsigned long)now;
    doc["dosingTimestamp"] = dosingTimestamp;
    doc["volume"] = volume;
    doc["status"] = isStart ? "started" : (success ? "completed" : "failed");
    
    // Add metadata
    JsonObject metadata = doc["metadata"].to<JsonObject>();
    metadata["totalToday"] = totalDosedVolume;
    metadata["remaining"] = getRemainingDailyVolume();
    metadata["isAuto"] = true;
    
    // Only set success field for completed/failed events
    if (!isStart) {
        doc["success"] = success;
    } else {
        doc["success"] = nullptr;  // null for started events
    }
    
    char jsonBuffer[256];  // Increased buffer size for metadata
    serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    
    // Queue POST command to Core 0 (non-blocking)
    NetworkCommandMessage cmd;
    cmd.command = NetworkCommand::HTTP_POST_DOSE_LOG;
    cmd.param1 = 0;
    cmd.param2 = 0;
    strncpy(cmd.data, jsonBuffer, sizeof(cmd.data) - 1);
    cmd.data[sizeof(cmd.data) - 1] = '\0';
    
    if (networkTask.sendCommand(cmd, 0)) {
        AUTO_DOSING_LOG("Dose event (%s) queued for POST to server", statusStr);
    } else {
        AUTO_DOSING_LOG("WARN: Failed to queue dose event (network queue full)");
    }
}

// ============================================================================
// PAUSE/RESUME (Phase 3 Sprint 5)
// ============================================================================

void AutoDosingManager::pause(uint32_t durationSeconds) {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: pause() called before initialize()");
        return;
    }
    
    paused = true;
    
    if (durationSeconds == 0) {
        pauseUntil = 0;  // Indefinite pause
        AUTO_DOSING_LOG("Auto-dosing paused indefinitely");
    } else {
        time_t now = time(nullptr);
        if (now == (time_t)-1) {
            // Time not synced - use millis offset
            now = lastSyncTime + ((millis() - lastSyncMillis) / 1000);
        }
        pauseUntil = now + durationSeconds;
        AUTO_DOSING_LOG("Auto-dosing paused for %lu seconds (until %lu)", durationSeconds, pauseUntil);
    }
    
    // Persist pause state to EEPROM
    EEPROM.put(::Config::EEPROM_PAUSE_STATE_ADDR, paused);
    EEPROM.put(::Config::EEPROM_PAUSE_STATE_ADDR + 1, pauseUntil);
    EEPROM.commit();
}

void AutoDosingManager::resume() {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: resume() called before initialize()");
        return;
    }
    
    if (!paused) {
        AUTO_DOSING_LOG("WARN: resume() called but not paused");
        return;
    }
    
    paused = false;
    pauseUntil = 0;
    AUTO_DOSING_LOG("Auto-dosing resumed");
    
    // Persist resume state to EEPROM
    EEPROM.put(::Config::EEPROM_PAUSE_STATE_ADDR, paused);
    EEPROM.put(::Config::EEPROM_PAUSE_STATE_ADDR + 1, pauseUntil);
    EEPROM.commit();
    
    // Regenerate schedule since time has passed
    updateSchedule();
}

bool AutoDosingManager::isPaused() const {
    if (!paused) {
        return false;
    }
    
    // Check if timed pause has expired
    if (pauseUntil == 0) {
        return true;  // Indefinite pause
    }
    
    time_t now = time(nullptr);
    if (now == (time_t)-1) {
        // Time not synced - use millis offset
        now = lastSyncTime + ((millis() - lastSyncMillis) / 1000);
    }
    
    return now < pauseUntil;
}

uint32_t AutoDosingManager::getPauseRemaining() const {
    if (!paused) {
        return 0;
    }
    
    if (pauseUntil == 0) {
        return 0xFFFFFFFF;
    }
    
    time_t now = time(nullptr);
    if (now == (time_t)-1) {
        now = lastSyncTime + ((millis() - lastSyncMillis) / 1000);
    }
    
    if (now >= pauseUntil) {
        return 0;
    }
    
    uint32_t remaining = pauseUntil - now;
    if (remaining > 86400 * 365) {
        Serial.printf("[AutoDosing] Invalid pause remaining: %u seconds, clearing pause state\n", remaining);
        const_cast<AutoDosingManager*>(this)->paused = false;
        const_cast<AutoDosingManager*>(this)->pauseUntil = 0;
        EEPROM.put(::Config::EEPROM_PAUSE_STATE_ADDR, false);
        EEPROM.put(::Config::EEPROM_PAUSE_STATE_ADDR + 1, (uint32_t)0);
        EEPROM.commit();
        return 0;
    }
    
    return remaining;
}

// ============================================================================
// DOSE HISTORY (Phase 3 Sprint 6)
// ============================================================================

void AutoDosingManager::addDoseToHistory(uint32_t timestamp, float volume, bool success) {
    if (!m_initialized) {
        return;
    }
    
    // Ring buffer: overwrite oldest entry when full
    uint8_t writeIndex;
    if (doseHistoryCount < DOSE_HISTORY_SIZE) {
        writeIndex = doseHistoryCount;
        doseHistoryCount++;
    } else {
        // Buffer full, overwrite oldest (at head)
        writeIndex = doseHistoryHead;
        doseHistoryHead = (doseHistoryHead + 1) % DOSE_HISTORY_SIZE;
    }
    
    doseHistory[writeIndex].timestamp = timestamp;
    doseHistory[writeIndex].volume = volume;
    doseHistory[writeIndex].success = success;
    
    AUTO_DOSING_LOG("Added to history [%d]: time=%lu vol=%.2f success=%d", 
                    writeIndex, timestamp, volume, success);
    
    // Persist to EEPROM
    saveDoseHistory();
}

const DoseHistoryEntry* AutoDosingManager::getDoseHistory(uint8_t& count) const {
    count = doseHistoryCount;
    return doseHistory;
}

void AutoDosingManager::clearDoseHistory() {
    doseHistoryCount = 0;
    doseHistoryHead = 0;
    
    for (uint8_t i = 0; i < DOSE_HISTORY_SIZE; i++) {
        doseHistory[i].timestamp = 0;
        doseHistory[i].volume = 0.0f;
        doseHistory[i].success = false;
    }
    
    saveDoseHistory();
    AUTO_DOSING_LOG("Dose history cleared");
}

void AutoDosingManager::loadDoseHistory() {
    if (!m_initialized) {
        AUTO_DOSING_LOG("ERROR: loadDoseHistory() called before initialize()");
        return;
    }
    
    AUTO_DOSING_LOG("Loading dose history from EEPROM");
    
    // Load count and head index
    EEPROM.get(::Config::EEPROM_DOSE_HISTORY_ADDR, doseHistoryCount);
    EEPROM.get(::Config::EEPROM_DOSE_HISTORY_ADDR + 1, doseHistoryHead);
    
    // Validate
    if (doseHistoryCount > DOSE_HISTORY_SIZE) {
        AUTO_DOSING_LOG("WARN: Invalid history count %d, resetting", doseHistoryCount);
        doseHistoryCount = 0;
        doseHistoryHead = 0;
        return;
    }
    
    if (doseHistoryHead >= DOSE_HISTORY_SIZE) {
        AUTO_DOSING_LOG("WARN: Invalid history head %d, resetting", doseHistoryHead);
        doseHistoryHead = 0;
    }
    
    // Load history entries (starting at offset 2 for count+head)
    uint16_t addr = ::Config::EEPROM_DOSE_HISTORY_ADDR + 2;
    for (uint8_t i = 0; i < DOSE_HISTORY_SIZE; i++) {
        EEPROM.get(addr, doseHistory[i]);
        addr += sizeof(DoseHistoryEntry);
    }
    
    AUTO_DOSING_LOG("Loaded %d dose history entries (head=%d)", doseHistoryCount, doseHistoryHead);
}

void AutoDosingManager::saveDoseHistory() {
    if (!m_initialized) {
        return;
    }
    
    // Save count and head index
    EEPROM.put(::Config::EEPROM_DOSE_HISTORY_ADDR, doseHistoryCount);
    EEPROM.put(::Config::EEPROM_DOSE_HISTORY_ADDR + 1, doseHistoryHead);
    
    // Save all history entries (starting at offset 2)
    uint16_t addr = ::Config::EEPROM_DOSE_HISTORY_ADDR + 2;
    for (uint8_t i = 0; i < DOSE_HISTORY_SIZE; i++) {
        EEPROM.put(addr, doseHistory[i]);
        addr += sizeof(DoseHistoryEntry);
    }
    
    EEPROM.commit();
}
