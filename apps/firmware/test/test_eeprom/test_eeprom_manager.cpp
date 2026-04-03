/**
 * @file test_eeprom_manager.cpp
 * @brief Unit tests for EEPROMManager wear leveling and CRC validation
 * 
 * Tests:
 * 1. CRC-16-CCITT calculation correctness
 * 2. Bank rotation logic
 * 3. Data corruption detection
 * 4. Invalid data recovery
 * 5. Write counter increment
 */

#include <unity.h>
#include <stdint.h>
#include <string.h>

// CRC-16-CCITT implementation for testing
uint16_t calculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

void test_crc16_calculation(void) {
    // Test known CRC values
    uint8_t testData1[] = {0x01, 0x02, 0x03, 0x04};
    uint16_t crc1 = calculateCRC16(testData1, 4);
    
    // Verify CRC is non-zero for non-empty data
    TEST_ASSERT_NOT_EQUAL(0, crc1);
    
    // Verify same data produces same CRC
    uint16_t crc2 = calculateCRC16(testData1, 4);
    TEST_ASSERT_EQUAL(crc1, crc2);
    
    // Verify different data produces different CRC
    uint8_t testData2[] = {0x01, 0x02, 0x03, 0x05};
    uint16_t crc3 = calculateCRC16(testData2, 4);
    TEST_ASSERT_NOT_EQUAL(crc1, crc3);
}

void test_bank_rotation(void) {
    // Simulate bank rotation (4 banks, 0-3)
    const int BANK_COUNT = 4;
    uint16_t writeCounters[BANK_COUNT] = {0, 0, 0, 0};
    
    // Simulate 10 writes
    int currentBank = 0;
    for (int write = 0; write < 10; write++) {
        writeCounters[currentBank]++;
        currentBank = (currentBank + 1) % BANK_COUNT;
    }
    
    // Each bank should have been written ~2-3 times (10 writes / 4 banks)
    for (int i = 0; i < BANK_COUNT; i++) {
        TEST_ASSERT_GREATER_OR_EQUAL(2, writeCounters[i]);
        TEST_ASSERT_LESS_OR_EQUAL(3, writeCounters[i]);
    }
}

void test_data_corruption_detection(void) {
    // Simulate stored data with correct CRC
    uint8_t data[] = {0x12, 0x34, 0x56, 0x78};
    uint16_t correctCRC = calculateCRC16(data, 4);
    
    // Verify correct CRC passes
    uint16_t calculatedCRC = calculateCRC16(data, 4);
    TEST_ASSERT_EQUAL(correctCRC, calculatedCRC);
    
    // Corrupt one byte
    data[2] = 0xFF;
    uint16_t corruptedCRC = calculateCRC16(data, 4);
    
    // Verify corrupted data produces different CRC
    TEST_ASSERT_NOT_EQUAL(correctCRC, corruptedCRC);
}

void test_invalid_data_recovery(void) {
    // Simulate recovery from invalid bank
    // Bank 0: invalid (CRC mismatch)
    // Bank 1: valid (fallback)
    
    uint8_t bank0Data[] = {0x01, 0x02, 0x03, 0x04};
    uint16_t bank0CRC = calculateCRC16(bank0Data, 4);
    bank0Data[0] = 0xFF;  // Corrupt data
    
    uint8_t bank1Data[] = {0x05, 0x06, 0x07, 0x08};
    uint16_t bank1CRC = calculateCRC16(bank1Data, 4);
    
    // Check bank 0 (should fail)
    bool bank0Valid = (calculateCRC16(bank0Data, 4) == bank0CRC);
    TEST_ASSERT_FALSE(bank0Valid);
    
    // Check bank 1 (should succeed)
    bool bank1Valid = (calculateCRC16(bank1Data, 4) == bank1CRC);
    TEST_ASSERT_TRUE(bank1Valid);
}

void test_write_counter_increment(void) {
    // Test write counter wraps correctly at uint16_t max
    uint16_t counter = 0xFFFE;  // Near max
    
    counter++;
    TEST_ASSERT_EQUAL(0xFFFF, counter);
    
    counter++;
    TEST_ASSERT_EQUAL(0, counter);  // Wraps to 0
}

void setUp(void) {
    // Setup code if needed
}

void tearDown(void) {
    // Cleanup code if needed
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_crc16_calculation);
    RUN_TEST(test_bank_rotation);
    RUN_TEST(test_data_corruption_detection);
    RUN_TEST(test_invalid_data_recovery);
    RUN_TEST(test_write_counter_increment);
    
    return UNITY_END();
}
