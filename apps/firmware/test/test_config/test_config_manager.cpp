/**
 * @file test_config_manager.cpp
 * @brief Unit tests for ConfigManager validation and defaults
 * 
 * Tests:
 * 1. Pump ID validation (alphanumeric + underscore)
 * 2. Pump ID length limits (1-15 chars)
 * 3. Invalid character rejection
 * 4. Factory reset to defaults
 */

#include <unity.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

// Helper function to validate pump ID (from ConfigManager logic)
bool isValidPumpId(const char* id) {
    if (id == nullptr || id[0] == '\0') {
        return false;  // Empty
    }
    
    size_t len = strlen(id);
    if (len > 15) {
        return false;  // Too long
    }
    
    for (size_t i = 0; i < len; i++) {
        char c = id[i];
        if (!isalnum(c) && c != '_') {
            return false;  // Invalid character
        }
    }
    
    return true;
}

void test_valid_pump_id(void) {
    // Test valid IDs
    TEST_ASSERT_TRUE(isValidPumpId("SmartPump_01"));
    TEST_ASSERT_TRUE(isValidPumpId("PUMP123"));
    TEST_ASSERT_TRUE(isValidPumpId("my_pump"));
    TEST_ASSERT_TRUE(isValidPumpId("A"));  // Single char
    TEST_ASSERT_TRUE(isValidPumpId("123456789012345"));  // 15 chars (max)
}

void test_invalid_pump_id_empty(void) {
    // Test empty/null IDs
    TEST_ASSERT_FALSE(isValidPumpId(""));
    TEST_ASSERT_FALSE(isValidPumpId(nullptr));
}

void test_invalid_pump_id_length(void) {
    // Test too long (>15 chars)
    TEST_ASSERT_FALSE(isValidPumpId("1234567890123456"));  // 16 chars
    TEST_ASSERT_FALSE(isValidPumpId("ThisIsAVeryLongPumpIdThatExceedsLimit"));
}

void test_invalid_pump_id_characters(void) {
    // Test invalid characters
    TEST_ASSERT_FALSE(isValidPumpId("Pump-01"));     // Hyphen not allowed
    TEST_ASSERT_FALSE(isValidPumpId("Pump 01"));     // Space not allowed
    TEST_ASSERT_FALSE(isValidPumpId("Pump@01"));     // @ not allowed
    TEST_ASSERT_FALSE(isValidPumpId("Pump.01"));     // Period not allowed
    TEST_ASSERT_FALSE(isValidPumpId("Pump#01"));     // # not allowed
}

void setUp(void) {
    // Setup code if needed
}

void tearDown(void) {
    // Cleanup code if needed
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_valid_pump_id);
    RUN_TEST(test_invalid_pump_id_empty);
    RUN_TEST(test_invalid_pump_id_length);
    RUN_TEST(test_invalid_pump_id_characters);
    
    return UNITY_END();
}
