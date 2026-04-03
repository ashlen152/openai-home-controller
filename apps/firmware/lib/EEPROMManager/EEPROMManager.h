/**
 * @file EEPROMManager.h
 * @brief EEPROM wear leveling manager with CRC validation and bank rotation.
 *
 * Implements a 4-bank rotation system to extend EEPROM lifetime by distributing
 * write cycles across multiple memory locations. Each bank contains:
 *   - Write counter (2 bytes) - increments on each save
 *   - CRC-16 checksum (2 bytes) - validates data integrity
 *   - Valid flag (1 byte) - 0xFF = valid, 0x00 = invalid
 *   - Data payload (29 bytes) - actual settings
 *
 * On boot, the manager finds the bank with the highest write counter and valid CRC.
 * On save, it rotates to the next bank, increments counter, calculates CRC, and marks valid.
 *
 * Total EEPROM usage: 4 banks × 34 bytes = 136 bytes (addresses 30-165)
 *
 * Usage:
 *   EEPROMManager& eeprom = EEPROMManager::getInstance();
 *   eeprom.begin();  // Find active bank, validate CRC
 *   float value = eeprom.read<float>(0);  // Read from logical address
 *   eeprom.write(0, value);  // Write to logical address (in-memory cache)
 *   eeprom.commit();  // Rotate bank and persist to EEPROM
 */

#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include <stdint.h>

class EEPROMManager
{
public:
    /**
     * @brief Get singleton instance
     */
    static EEPROMManager &getInstance();

    /**
     * @brief Initialize and find active bank
     * Scans all 4 banks, validates CRC, selects bank with highest counter
     */
    void begin();

    /**
     * @brief Read value from logical address (in-memory cache)
     * @tparam T Data type to read
     * @param logicalAddr Logical address (0-28)
     * @return Value of type T
     */
    template <typename T>
    T read(int logicalAddr);

    /**
     * @brief Write value to logical address (in-memory cache)
     * @tparam T Data type to write
     * @param logicalAddr Logical address (0-28)
     * @param value Value to write
     */
    template <typename T>
    void write(int logicalAddr, T value);

    /**
     * @brief Commit changes to EEPROM with bank rotation
     * Rotates to next bank, increments counter, calculates CRC, marks valid
     */
    void commit();

    /**
     * @brief Get current write counter
     * @return Number of writes performed
     */
    uint16_t getWriteCounter() const { return writeCounter; }

    /**
     * @brief Get active bank index
     * @return 0-3
     */
    uint8_t getActiveBankIndex() const { return activeBankIndex; }

private:
    EEPROMManager();
    ~EEPROMManager() = default;
    EEPROMManager(const EEPROMManager &) = delete;
    EEPROMManager &operator=(const EEPROMManager &) = delete;

    static constexpr int BANK_COUNT = 4;         ///< Number of banks
    static constexpr int BANK_SIZE = 34;         ///< Bytes per bank
    static constexpr int DATA_SIZE = 29;         ///< Payload size (29 bytes)
    static constexpr int BANK_BASE_ADDR = 30;    ///< Starting address of bank 0

    uint8_t activeBankIndex;                     ///< Current active bank (0-3)
    uint16_t writeCounter;                       ///< Write counter for active bank
    uint8_t bankData[DATA_SIZE];                 ///< In-memory cache of data

    /**
     * @brief Calculate CRC-16-CCITT checksum
     * @param data Data to checksum
     * @param len Length in bytes
     * @return CRC-16 checksum
     */
    uint16_t calculateCRC(const uint8_t *data, size_t len);

    /**
     * @brief Load bank from EEPROM
     * @param bankIndex Bank to load (0-3)
     * @param outCounter Output write counter
     * @param outCrc Output CRC value
     * @param outValid Output valid flag
     * @param outData Output data payload (must be DATA_SIZE bytes)
     */
    void loadBank(uint8_t bankIndex, uint16_t &outCounter, uint16_t &outCrc,
                  uint8_t &outValid, uint8_t *outData);

    /**
     * @brief Save current data to specified bank
     * @param bankIndex Bank to save to (0-3)
     * @param counter Write counter to use
     */
    void saveBank(uint8_t bankIndex, uint16_t counter);

    /**
     * @brief Find bank with highest valid counter
     * @return Bank index (0-3) or 0 if none valid
     */
    uint8_t findActiveBankIndex();

    /**
     * @brief Migrate legacy EEPROM data to bank system
     * Called on first boot if no valid banks found
     */
    void migrateLegacyData();
};

// Template implementations (must be in header)
template <typename T>
T EEPROMManager::read(int logicalAddr)
{
    if (logicalAddr < 0 || logicalAddr + sizeof(T) > DATA_SIZE)
    {
        Serial.printf("[EEPROM] ERROR: Read out of bounds at %d (size %d)\n", logicalAddr, sizeof(T));
        return T();
    }

    T value;
    memcpy(&value, &bankData[logicalAddr], sizeof(T));
    return value;
}

template <typename T>
void EEPROMManager::write(int logicalAddr, T value)
{
    if (logicalAddr < 0 || logicalAddr + sizeof(T) > DATA_SIZE)
    {
        Serial.printf("[EEPROM] ERROR: Write out of bounds at %d (size %d)\n", logicalAddr, sizeof(T));
        return;
    }

    memcpy(&bankData[logicalAddr], &value, sizeof(T));
}

#endif // EEPROM_MANAGER_H
