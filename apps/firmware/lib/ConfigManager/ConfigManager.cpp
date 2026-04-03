/**
 * @file ConfigManager.cpp
 * @brief Implementation of ConfigManager singleton.
 */

#include "ConfigManager.h"
#include <EEPROM.h>
#include "../../include/Config.h"
#include <cstring>

// Singleton instance
ConfigManager &ConfigManager::getInstance()
{
    static ConfigManager instance;
    return instance;
}

// Private constructor
ConfigManager::ConfigManager()
{
    strncpy(pumpId, DEFAULT_PUMP_ID, sizeof(pumpId) - 1);
    pumpId[15] = '\0'; // Ensure null termination
}

// Initialize and load from EEPROM
void ConfigManager::begin()
{
    loadFromEEPROM();
}

// Get pump ID
const char *ConfigManager::getPumpId() const
{
    return pumpId;
}

// Set pump ID with validation
bool ConfigManager::setPumpId(const char *id)
{
    if (!isValidPumpId(id))
    {
        Serial.println("[CONFIG] Invalid pump ID - must be alphanumeric/underscore, max 15 chars");
        return false;
    }

    strncpy(pumpId, id, sizeof(pumpId) - 1);
    pumpId[15] = '\0'; // Ensure null termination

    saveToEEPROM();

    Serial.print("[CONFIG] Pump ID set to: ");
    Serial.println(pumpId);

    return true;
}

// Validate pump ID
bool ConfigManager::isValidPumpId(const char *id) const
{
    if (id == nullptr || id[0] == '\0')
        return false;

    size_t len = strlen(id);
    if (len > 15)
        return false;

    // Check each character: alphanumeric or underscore only
    for (size_t i = 0; i < len; i++)
    {
        char c = id[i];
        bool valid = (c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') ||
                     (c == '_');
        if (!valid)
            return false;
    }

    return true;
}

// Load from EEPROM
void ConfigManager::loadFromEEPROM()
{
    // Read 16 bytes from EEPROM
    for (int i = 0; i < 16; i++)
    {
        pumpId[i] = EEPROM.read(Config::EEPROM_PUMP_ID_ADDR + i);
    }
    pumpId[15] = '\0'; // Ensure null termination

    // Validate loaded ID - if invalid, use default
    if (!isValidPumpId(pumpId))
    {
        Serial.println("[CONFIG] Invalid pump ID in EEPROM, using default");
        strncpy(pumpId, DEFAULT_PUMP_ID, sizeof(pumpId) - 1);
        pumpId[15] = '\0';
        saveToEEPROM(); // Save default
    }
    else
    {
        Serial.print("[CONFIG] Loaded pump ID: ");
        Serial.println(pumpId);
    }
}

// Save to EEPROM
void ConfigManager::saveToEEPROM()
{
    // Write 16 bytes to EEPROM
    for (int i = 0; i < 16; i++)
    {
        EEPROM.write(Config::EEPROM_PUMP_ID_ADDR + i, pumpId[i]);
    }
    EEPROM.commit();

    Serial.println("[CONFIG] Pump ID saved to EEPROM");
}

// Factory reset
void ConfigManager::resetToDefaults()
{
    Serial.println("[CONFIG] *** FACTORY RESET - Clearing all EEPROM ***");

    // Clear all EEPROM (write 0xFF to all 512 bytes)
    for (int i = 0; i < 512; i++)
    {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();

    // Reload defaults
    strncpy(pumpId, DEFAULT_PUMP_ID, sizeof(pumpId) - 1);
    pumpId[15] = '\0';
    saveToEEPROM();

    Serial.println("[CONFIG] Factory reset complete. Please restart device.");
}
