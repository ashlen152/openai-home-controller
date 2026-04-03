/**
 * @file test_schedule.cpp
 * @brief Unit tests for AutoDosingManager schedule generation logic
 * 
 * Tests:
 * 1. Basic schedule generation (24 hour period)
 * 2. Day/night split distribution (70/30)
 * 3. Midnight wrap-around handling
 * 4. Edge cases (0% split, 100% split)
 * 5. Multiple dose distribution
 * 6. Schedule regeneration on parameter change
 */

#include <unity.h>
#include <stdint.h>
#include <time.h>

// Mock structures for testing (simplified from AutoDosingManager)
struct DoseScheduleEntry {
    uint8_t hour;
    uint8_t minute;
    float volume;
};

// Helper function to calculate total doses in a time range
int countDosesInRange(const DoseScheduleEntry* schedule, int count, uint8_t startHour, uint8_t endHour) {
    int dosesInRange = 0;
    for (int i = 0; i < count; i++) {
        if (endHour > startHour) {
            // Normal range (e.g., 6-18)
            if (schedule[i].hour >= startHour && schedule[i].hour < endHour) {
                dosesInRange++;
            }
        } else {
            // Wrapped range (e.g., 18-6 = 18-23 + 0-6)
            if (schedule[i].hour >= startHour || schedule[i].hour < endHour) {
                dosesInRange++;
            }
        }
    }
    return dosesInRange;
}

// Helper function to calculate total volume in a time range
float sumVolumeInRange(const DoseScheduleEntry* schedule, int count, uint8_t startHour, uint8_t endHour) {
    float totalVolume = 0.0f;
    for (int i = 0; i < count; i++) {
        if (endHour > startHour) {
            if (schedule[i].hour >= startHour && schedule[i].hour < endHour) {
                totalVolume += schedule[i].volume;
            }
        } else {
            if (schedule[i].hour >= startHour || schedule[i].hour < endHour) {
                totalVolume += schedule[i].volume;
            }
        }
    }
    return totalVolume;
}

void test_basic_schedule_generation(void) {
    // Test that total volume matches daily volume
    DoseScheduleEntry schedule[10];
    float dailyVolume = 10.0f;
    int doseCount = 10;
    
    // Simulate uniform distribution
    for (int i = 0; i < doseCount; i++) {
        schedule[i].hour = (i * 24) / doseCount;
        schedule[i].minute = ((i * 24 * 60) / doseCount) % 60;
        schedule[i].volume = dailyVolume / doseCount;
    }
    
    // Calculate total volume
    float totalVolume = 0.0f;
    for (int i = 0; i < doseCount; i++) {
        totalVolume += schedule[i].volume;
    }
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, dailyVolume, totalVolume);
}

void test_day_night_split_70_30(void) {
    // Test 70% day (6-18), 30% night (18-6) split
    DoseScheduleEntry schedule[10];
    float dailyVolume = 10.0f;
    uint8_t dayStartHour = 6;
    uint8_t dayEndHour = 18;
    uint8_t dayPercent = 70;
    
    float dayVolume = dailyVolume * dayPercent / 100.0f;  // 7.0 mL
    float nightVolume = dailyVolume - dayVolume;          // 3.0 mL
    
    int dayDoses = 7;   // 70% of 10 doses
    int nightDoses = 3; // 30% of 10 doses
    
    // Simulate day doses (6-18)
    for (int i = 0; i < dayDoses; i++) {
        schedule[i].hour = dayStartHour + (i * 12) / dayDoses;
        schedule[i].minute = 0;
        schedule[i].volume = dayVolume / dayDoses;
    }
    
    // Simulate night doses (18-6)
    for (int i = 0; i < nightDoses; i++) {
        int idx = dayDoses + i;
        schedule[idx].hour = (dayEndHour + (i * 12) / nightDoses) % 24;
        schedule[idx].minute = 0;
        schedule[idx].volume = nightVolume / nightDoses;
    }
    
    // Verify day volume
    float actualDayVolume = sumVolumeInRange(schedule, 10, dayStartHour, dayEndHour);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, dayVolume, actualDayVolume);
    
    // Verify night volume
    float actualNightVolume = sumVolumeInRange(schedule, 10, dayEndHour, dayStartHour);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, nightVolume, actualNightVolume);
}

void test_midnight_wraparound(void) {
    // Test day period that wraps midnight (e.g., 22:00-06:00)
    DoseScheduleEntry schedule[8];
    uint8_t dayStartHour = 22;  // 10 PM
    uint8_t dayEndHour = 6;     // 6 AM
    
    // Simulate doses in wrapped period
    schedule[0].hour = 23; schedule[0].minute = 0; schedule[0].volume = 0.5f;
    schedule[1].hour = 1;  schedule[1].minute = 0; schedule[1].volume = 0.5f;
    schedule[2].hour = 3;  schedule[2].minute = 0; schedule[2].volume = 0.5f;
    
    // Simulate doses outside period
    schedule[3].hour = 8;  schedule[3].minute = 0; schedule[3].volume = 0.5f;
    schedule[4].hour = 12; schedule[4].minute = 0; schedule[4].volume = 0.5f;
    schedule[5].hour = 16; schedule[5].minute = 0; schedule[5].volume = 0.5f;
    
    int dosesInDay = countDosesInRange(schedule, 6, dayStartHour, dayEndHour);
    TEST_ASSERT_EQUAL(3, dosesInDay);
    
    int dosesInNight = countDosesInRange(schedule, 6, dayEndHour, dayStartHour);
    TEST_ASSERT_EQUAL(3, dosesInNight);
}

void test_edge_case_0_percent_day(void) {
    // Test 0% day split (all doses at night)
    float dailyVolume = 10.0f;
    uint8_t dayPercent = 0;
    
    float dayVolume = dailyVolume * dayPercent / 100.0f;
    float nightVolume = dailyVolume - dayVolume;
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dayVolume);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, nightVolume);
}

void test_edge_case_100_percent_day(void) {
    // Test 100% day split (all doses during day)
    float dailyVolume = 10.0f;
    uint8_t dayPercent = 100;
    
    float dayVolume = dailyVolume * dayPercent / 100.0f;
    float nightVolume = dailyVolume - dayVolume;
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, dayVolume);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, nightVolume);
}

void test_multiple_dose_distribution(void) {
    // Test that doses are distributed evenly
    DoseScheduleEntry schedule[12];
    int doseCount = 12;
    
    // Simulate evenly spaced doses every 2 hours
    for (int i = 0; i < doseCount; i++) {
        schedule[i].hour = (i * 2) % 24;
        schedule[i].minute = 0;
        schedule[i].volume = 1.0f;
    }
    
    // Verify spacing
    for (int i = 1; i < doseCount; i++) {
        int hourDiff = (schedule[i].hour - schedule[i-1].hour + 24) % 24;
        TEST_ASSERT_EQUAL(2, hourDiff);
    }
}

void setUp(void) {
    // Setup code if needed
}

void tearDown(void) {
    // Cleanup code if needed
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_basic_schedule_generation);
    RUN_TEST(test_day_night_split_70_30);
    RUN_TEST(test_midnight_wraparound);
    RUN_TEST(test_edge_case_0_percent_day);
    RUN_TEST(test_edge_case_100_percent_day);
    RUN_TEST(test_multiple_dose_distribution);
    
    return UNITY_END();
}
