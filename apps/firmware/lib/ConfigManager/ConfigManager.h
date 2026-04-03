/**
 * @file ConfigManager.h
 * @brief Singleton configuration manager for runtime-editable settings.
 *
 * Manages device-specific configuration that can be modified via OLED menu:
 *   - Pump ID (max 15 characters, alphanumeric + underscore)
 *   - Factory reset functionality
 *
 * All settings persist to EEPROM and survive reboots.
 *
 * Usage:
 *   ConfigManager& config = ConfigManager::getInstance();
 *   config.begin();  // Load from EEPROM
 *   const char* id = config.getPumpId();
 *   config.setPumpId("SmartPump_02");
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>

class ConfigManager
{
public:
    /**
     * @brief Get singleton instance
     */
    static ConfigManager &getInstance();

    /**
     * @brief Initialize and load settings from EEPROM
     */
    void begin();

    /**
     * @brief Get current pump ID
     * @return Null-terminated pump ID string (max 15 chars)
     */
    const char *getPumpId() const;

    /**
     * @brief Set pump ID with validation
     * @param id New pump ID (alphanumeric + underscore only, max 15 chars)
     * @return true if valid and saved, false if validation failed
     */
    bool setPumpId(const char *id);

    /**
     * @brief Reset all EEPROM settings to factory defaults
     * @warning This clears ALL pump settings (calibration, auto-dosing, etc.)
     */
    void resetToDefaults();

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;

    char pumpId[16]; ///< 15 chars + null terminator

    static constexpr const char *DEFAULT_PUMP_ID = "SmartPump_01";

    /**
     * @brief Validate pump ID string
     * @param id String to validate
     * @return true if valid (alphanumeric + underscore, length <= 15)
     */
    bool isValidPumpId(const char *id) const;

    /**
     * @brief Load pump ID from EEPROM
     */
    void loadFromEEPROM();

    /**
     * @brief Save pump ID to EEPROM
     */
    void saveToEEPROM();
};

#endif // CONFIG_MANAGER_H
