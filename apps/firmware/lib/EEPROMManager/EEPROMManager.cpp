/**
 * @file EEPROMManager.cpp
 * @brief Implementation of EEPROM wear leveling manager.
 */

#include "EEPROMManager.h"
#include <EEPROM.h>

// Singleton instance
EEPROMManager &EEPROMManager::getInstance()
{
    static EEPROMManager instance;
    return instance;
}

// Private constructor
EEPROMManager::EEPROMManager()
    : activeBankIndex(0), writeCounter(0)
{
    memset(bankData, 0, DATA_SIZE);
}

// Initialize and find active bank
void EEPROMManager::begin()
{
    Serial.println("[EEPROM] Initializing wear leveling system...");

    // Find bank with highest valid counter
    activeBankIndex = findActiveBankIndex();

    if (activeBankIndex == 0xFF)
    {
        // No valid banks found - this is first boot or corruption
        Serial.println("[EEPROM] No valid banks found, migrating legacy data...");
        migrateLegacyData();
        activeBankIndex = 0;
        writeCounter = 1;
    }
    else
    {
        // Load active bank data
        uint16_t crc;
        uint8_t valid;
        loadBank(activeBankIndex, writeCounter, crc, valid, bankData);

        Serial.printf("[EEPROM] Active bank: %d, Write counter: %d\n",
                      activeBankIndex, writeCounter);
    }
}

// Calculate CRC-16-CCITT
uint16_t EEPROMManager::calculateCRC(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF; // CRC-16-CCITT initial value
    const uint16_t polynomial = 0x1021;

    for (size_t i = 0; i < len; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ polynomial;
            else
                crc = crc << 1;
        }
    }

    return crc;
}

// Load bank from EEPROM
void EEPROMManager::loadBank(uint8_t bankIndex, uint16_t &outCounter,
                             uint16_t &outCrc, uint8_t &outValid, uint8_t *outData)
{
    int baseAddr = BANK_BASE_ADDR + (bankIndex * BANK_SIZE);

    // Read bank header
    outCounter = (EEPROM.read(baseAddr) << 8) | EEPROM.read(baseAddr + 1);
    outCrc = (EEPROM.read(baseAddr + 2) << 8) | EEPROM.read(baseAddr + 3);
    outValid = EEPROM.read(baseAddr + 4);

    // Read data payload
    for (int i = 0; i < DATA_SIZE; i++)
    {
        outData[i] = EEPROM.read(baseAddr + 5 + i);
    }
}

// Save current data to specified bank
void EEPROMManager::saveBank(uint8_t bankIndex, uint16_t counter)
{
    int baseAddr = BANK_BASE_ADDR + (bankIndex * BANK_SIZE);

    // Calculate CRC of data
    uint16_t crc = calculateCRC(bankData, DATA_SIZE);

    // Write bank header
    EEPROM.write(baseAddr, (counter >> 8) & 0xFF);       // Counter high byte
    EEPROM.write(baseAddr + 1, counter & 0xFF);          // Counter low byte
    EEPROM.write(baseAddr + 2, (crc >> 8) & 0xFF);       // CRC high byte
    EEPROM.write(baseAddr + 3, crc & 0xFF);              // CRC low byte
    EEPROM.write(baseAddr + 4, 0xFF);                    // Valid flag

    // Write data payload
    for (int i = 0; i < DATA_SIZE; i++)
    {
        EEPROM.write(baseAddr + 5 + i, bankData[i]);
    }

    EEPROM.commit();

    Serial.printf("[EEPROM] Saved to bank %d (counter: %d, CRC: 0x%04X)\n",
                  bankIndex, counter, crc);
}

// Find bank with highest valid counter
uint8_t EEPROMManager::findActiveBankIndex()
{
    uint16_t maxCounter = 0;
    uint8_t maxBankIndex = 0xFF; // 0xFF means no valid bank
    bool foundValid = false;

    for (uint8_t i = 0; i < BANK_COUNT; i++)
    {
        uint16_t counter, crc, valid;
        uint8_t tempData[DATA_SIZE];
        loadBank(i, counter, crc, valid, tempData);

        // Check if bank is marked valid
        if (valid != 0xFF)
        {
            Serial.printf("[EEPROM] Bank %d invalid (flag: 0x%02X)\n", i, valid);
            continue;
        }

        // Validate CRC
        uint16_t calculatedCrc = calculateCRC(tempData, DATA_SIZE);
        if (crc != calculatedCrc)
        {
            Serial.printf("[EEPROM] Bank %d CRC mismatch (stored: 0x%04X, calc: 0x%04X)\n",
                          i, crc, calculatedCrc);
            continue;
        }

        // Bank is valid
        Serial.printf("[EEPROM] Bank %d valid (counter: %d)\n", i, counter);
        foundValid = true;

        if (counter > maxCounter || maxBankIndex == 0xFF)
        {
            maxCounter = counter;
            maxBankIndex = i;
        }
    }

    return foundValid ? maxBankIndex : 0xFF;
}

// Migrate legacy EEPROM data to bank system
void EEPROMManager::migrateLegacyData()
{
    Serial.println("[EEPROM] Migrating legacy data (addresses 0-28) to bank 0...");

    // Read legacy data from addresses 0-28
    for (int i = 0; i < 29 && i < DATA_SIZE; i++)
    {
        bankData[i] = EEPROM.read(i);
    }

    // Save to bank 0 with counter = 1
    saveBank(0, 1);

    // Invalidate other banks
    for (uint8_t i = 1; i < BANK_COUNT; i++)
    {
        int baseAddr = BANK_BASE_ADDR + (i * BANK_SIZE);
        EEPROM.write(baseAddr + 4, 0x00); // Mark invalid
    }
    EEPROM.commit();

    Serial.println("[EEPROM] Migration complete");
}

// Commit changes to EEPROM with bank rotation
void EEPROMManager::commit()
{
    // Rotate to next bank
    uint8_t nextBankIndex = (activeBankIndex + 1) % BANK_COUNT;
    uint16_t nextCounter = writeCounter + 1;

    // Save to next bank
    saveBank(nextBankIndex, nextCounter);

    // Invalidate previous bank
    int prevBaseAddr = BANK_BASE_ADDR + (activeBankIndex * BANK_SIZE);
    EEPROM.write(prevBaseAddr + 4, 0x00); // Mark invalid
    EEPROM.commit();

    // Update active bank and counter
    activeBankIndex = nextBankIndex;
    writeCounter = nextCounter;

    Serial.printf("[EEPROM] Rotated to bank %d (total writes: %d)\n",
                  activeBankIndex, writeCounter);
}
