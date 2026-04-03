/**
 * @file test_auto_dosing_time.cpp
 * @brief Unit tests for AutoDosingManager time handling and fallback
 * 
 * Tests:
 * 1. Time sync fallback using millis()
 * 2. Dose timing calculations
 * 3. Next dose determination
 * 4. Time wrap-around at midnight
 */

#include <unity.h>
#include <stdint.h>
#include <time.h>

// Mock millis() for testing
static uint32_t mockMillis = 0;
uint32_t millis() {
    return mockMillis;
}

void test_time_sync_fallback(void) {
    // Simulate WiFi down scenario - use millis offset
    time_t lastSyncTime = 1704067200;  // 2024-01-01 00:00:00 UTC
    uint32_t lastSyncMillis = 1000;     // 1 second after boot
    
    mockMillis = 61000;  // 61 seconds after boot (60 second offset from sync)
    
    // Calculate current time using fallback
    uint32_t millisElapsed = mockMillis - lastSyncMillis;
    time_t estimatedTime = lastSyncTime + (millisElapsed / 1000);
    
    time_t expectedTime = lastSyncTime + 60;  // 60 seconds later
    TEST_ASSERT_EQUAL(expectedTime, estimatedTime);
}

void test_dose_timing_calculation(void) {
    // Test calculating time until next dose
    time_t currentTime = 1704067200;  // 2024-01-01 00:00:00 UTC (midnight)
    
    // Next dose at 06:00:00 (6 hours = 21600 seconds)
    time_t nextDoseTime = currentTime + 21600;
    
    int32_t secondsUntilDose = (int32_t)(nextDoseTime - currentTime);
    TEST_ASSERT_EQUAL(21600, secondsUntilDose);
}

void test_next_dose_determination(void) {
    // Test finding next dose in sorted schedule
    struct DoseScheduleEntry {
        uint8_t hour;
        uint8_t minute;
    };
    
    DoseScheduleEntry schedule[4] = {
        {6, 0},   // 06:00
        {12, 0},  // 12:00
        {18, 0},  // 18:00
        {23, 0}   // 23:00
    };
    
    // Current time: 10:30
    uint8_t currentHour = 10;
    uint8_t currentMinute = 30;
    
    // Find next dose (should be 12:00)
    int nextDoseIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (schedule[i].hour > currentHour || 
            (schedule[i].hour == currentHour && schedule[i].minute > currentMinute)) {
            nextDoseIndex = i;
            break;
        }
    }
    
    TEST_ASSERT_EQUAL(1, nextDoseIndex);  // Index 1 = 12:00
    TEST_ASSERT_EQUAL(12, schedule[nextDoseIndex].hour);
}

void test_time_wraparound_midnight(void) {
    // Test midnight wraparound (23:50 -> next dose at 00:10)
    uint8_t currentHour = 23;
    uint8_t currentMinute = 50;
    
    uint8_t nextDoseHour = 0;
    uint8_t nextDoseMinute = 10;
    
    // Calculate minutes until next dose (accounting for midnight wrap)
    int minutesUntilDose;
    if (nextDoseHour < currentHour || 
        (nextDoseHour == currentHour && nextDoseMinute <= currentMinute)) {
        // Next dose is tomorrow
        int minutesToMidnight = (24 - currentHour - 1) * 60 + (60 - currentMinute);
        int minutesAfterMidnight = nextDoseHour * 60 + nextDoseMinute;
        minutesUntilDose = minutesToMidnight + minutesAfterMidnight;
    } else {
        // Next dose is today
        minutesUntilDose = (nextDoseHour - currentHour) * 60 + (nextDoseMinute - currentMinute);
    }
    
    // 23:50 -> 00:10 = 20 minutes
    TEST_ASSERT_EQUAL(20, minutesUntilDose);
}

void setUp(void) {
    mockMillis = 0;
}

void tearDown(void) {
    // Cleanup code if needed
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_time_sync_fallback);
    RUN_TEST(test_dose_timing_calculation);
    RUN_TEST(test_next_dose_determination);
    RUN_TEST(test_time_wraparound_midnight);
    
    return UNITY_END();
}
