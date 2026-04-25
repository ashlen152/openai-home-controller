/**
 * @file Config.h
 * @brief Central configuration constants for SmartPump hardware and software.
 *
 * Contains all pin assignments, EEPROM memory map, timing constants,
 * calibration defaults, and debug flags.
 *
 * All constants are in a Config namespace using constexpr for compile-time evaluation.
 *
 * Key sections:
 *   - Stepper Motor Settings (pins, TMC2209 parameters)
 *   - Display & Timing (timeouts, update intervals)
 *   - EEPROM Address Map (persistent storage layout)
 *   - Debug Settings (logging flags)
 *   - Calibration Settings (default values for calibration process)
 *   - Auto Dosing Defaults (default daily volume)
 *   - Serial / Communication Pins (UART for TMC2209)
 */

#pragma once

namespace Config
{
    // ==========================
    // Stepper Motor Settings
    // ==========================
    constexpr int STEPPER_EN_PIN = 26; // Stepper enable pin
    constexpr int DIR_PIN = 2;         // Direction pin
    constexpr int STEP_PIN = 5;        // Step pin

    constexpr float R_SENSE = 0.11f;      // Sense resistor for TMC2209
    constexpr uint8_t DRIVER_ADDR = 0b00; // Default address for TMC2209 driver

    constexpr float MAX_SPEED = 100000.0f;  // steps/sec
    constexpr float ACCELERATION = 1000.0f; // steps/sec²
    constexpr float PROFILE_MIN_SPEED = 1000.0f;      // steps/sec
    constexpr float PROFILE_MAX_SPEED = 50000.0f;     // steps/sec
    constexpr float MANUAL_MIN_SPEED = 500.0f;        // steps/sec
    constexpr float MANUAL_ACCEL_MULTIPLIER = 2.0f;   // accel = speed * multiplier

    // ==========================
    // Display & Timing
    // ==========================
    constexpr unsigned long DISPLAY_TIMEOUT = 100000UL;           // ms
    constexpr unsigned long SETTINGS_DISPLAY_DURATION = 5000UL;   // ms
    constexpr unsigned long CALIBRATION_RESULT_DURATION = 3000UL; // ms

    // ==========================
    // EEPROM Address Map
    // ==========================
    constexpr int EEPROM_PERISTALTIC_STEPS_ADDR = 0;
    constexpr int EEPROM_DOSING_STEPS_ADDR = EEPROM_PERISTALTIC_STEPS_ADDR + sizeof(float);
    constexpr int EEPROM_SAVED_SPEED_ADDR = EEPROM_DOSING_STEPS_ADDR + sizeof(float);
    constexpr int EEPROM_MODE_ADDR = EEPROM_SAVED_SPEED_ADDR + sizeof(float);
    constexpr int EEPROM_AUTO_DOSING_ENABLED_ADDR = EEPROM_MODE_ADDR + sizeof(uint8_t);
    constexpr int EEPROM_DAILY_VOLUME_ADDR = EEPROM_AUTO_DOSING_ENABLED_ADDR + sizeof(bool);
    constexpr int EEPROM_LAST_DOSING_TIME_ADDR = EEPROM_DAILY_VOLUME_ADDR + sizeof(float);
    constexpr int EEPROM_TOTAL_DOSED_ADDR = EEPROM_LAST_DOSING_TIME_ADDR + sizeof(uint32_t);
    constexpr int EEPROM_DAY_START_HOUR_ADDR = EEPROM_TOTAL_DOSED_ADDR + sizeof(float);
    constexpr int EEPROM_DAY_END_HOUR_ADDR = EEPROM_DAY_START_HOUR_ADDR + sizeof(uint8_t);
    constexpr int EEPROM_DAY_PERCENT_ADDR = EEPROM_DAY_END_HOUR_ADDR + sizeof(uint8_t);
    
    // Wear Leveling System (EEPROMManager) - 4 banks × 34 bytes = 136 bytes
    // Bank layout: 2B counter + 2B CRC + 1B valid + 29B data = 34 bytes
    // Banks occupy addresses 30-165 (managed automatically by EEPROMManager)
    constexpr int EEPROM_BANK_BASE_ADDR = 30;        // Starting address of bank 0
    constexpr int EEPROM_BANK_SIZE = 34;             // Bytes per bank
    constexpr int EEPROM_BANK_COUNT = 4;             // Number of banks
    
    // Phase 3 addresses (non-banked)
    constexpr int EEPROM_PUMP_ID_ADDR = 170;         // 16 bytes (15 chars + null)
    constexpr int EEPROM_PAUSE_STATE_ADDR = 186;     // 5 bytes (bool + uint32_t)
    constexpr int EEPROM_DOSE_HISTORY_ADDR = 191;    // 62 bytes (1B count + 1B head + 5×12B entries)
    constexpr int EEPROM_SPEED_PROFILES_ADDR = 253;  // 13 bytes (3×float + uint8_t)
    constexpr int EEPROM_SERVER_STEPS_PERML_ADDR = 266;  // 4 bytes (float)
    constexpr int EEPROM_SERVER_PROFILE_ADDR = 270;     // 1 byte (uint8_t)
    constexpr int EEPROM_SERVER_SYNC_TIME_ADDR = 271;   // 4 bytes (uint32_t)
    
    constexpr int EEPROM_ADDR = 0; // base EEPROM address (legacy)

    // ==========================
    // Debug Flags
    // ==========================
    // Note: DEBUG_AUTO_DOSING is defined in AutoDosingManager.h
    // constexpr bool DEBUG_AUTO_DOSING = true; // Enable auto-dosing debug logs (moved to AutoDosingManager.h)
    constexpr bool DEBUG_TIMESTAMP = true;   // Include timestamps in debug logs

    // ==========================
    // Calibration Settings
    // ==========================
    constexpr unsigned int CALIBRATE_PERISTALTIC_TIME = 60; // seconds
    constexpr float CALIBRATE_DOSING_VOLUME = 10.0f;        // mL
    constexpr float CALIBRATE_SPEED = 20000.0f;             // steps/sec
    constexpr unsigned int CALIBRATE_TIME = 60;             // seconds (legacy)

    // ==========================
    // Auto Dosing Defaults
    // ==========================
    constexpr float DEFAULT_DAILY_VOLUME = 30.0f; // Default daily volume in mL
    constexpr int DEFAULT_SCHEDULE_SLOTS = 48;
    constexpr uint8_t DEFAULT_DAY_START_HOUR = 8;
    constexpr uint8_t DEFAULT_DAY_END_HOUR = 20;
    constexpr uint8_t DEFAULT_DAY_PERCENT = 70;

    // ==========================
    // Serial / Communication Pins
    // ==========================
    constexpr int RX_PIN = 16;
    constexpr int TX_PIN = 17;
}
