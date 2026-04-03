#include <iostream>
#include <vector>
#include <iomanip>
#include <thread>
#include <chrono>

struct DoseSchedule {
    int hour;
    int minute;
    float ml;
};

std::vector<DoseSchedule> schedule;

// ---- Generate Weighted Schedule ----
void generateWeightedSchedule(int slots, float totalMl,
                              int startHour, int endHour,
                              float percent1, float percent2) {
    schedule.clear();
    int intervalMinutes = 1440 / slots;  // minutes per dose

    float total1 = totalMl * percent1;
    float total2 = totalMl * percent2;

    int count1 = 0, count2 = 0;

    // Count slots in each window
    for (int i = 0; i < slots; i++) {
        int minutes = i * intervalMinutes;
        int h = minutes / 60;
        if (h >= startHour && h < endHour) count1++;
        else count2++;
    }

    float mlPerDose1 = (count1 > 0) ? total1 / count1 : 0;
    float mlPerDose2 = (count2 > 0) ? total2 / count2 : 0;

    // Fill schedule
    for (int i = 0; i < slots; i++) {
        int minutes = i * intervalMinutes;
        DoseSchedule entry;
        entry.hour = minutes / 60;
        entry.minute = minutes % 60;

        if (entry.hour >= startHour && entry.hour < endHour)
            entry.ml = mlPerDose1;
        else
            entry.ml = mlPerDose2;

        schedule.push_back(entry);
    }
}

// ---- Function to run when dose time is reached ----
void runDose(const DoseSchedule &entry) {
    std::cout << "[DOSE] "
              << std::setfill('0') << std::setw(2) << entry.hour << ":"
              << std::setfill('0') << std::setw(2) << entry.minute
              << " -> Dispense " << std::fixed << std::setprecision(2)
              << entry.ml << " ml" << std::endl;
}

// ---- Simulation of time ticking ----
void simulateDay() {
    for (int h = 0; h < 24; h++) {
        for (int m = 0; m < 60; m++) {
            // Print current time
            std::cout << "[TIME] "
                      << std::setfill('0') << std::setw(2) << h << ":"
                      << std::setfill('0') << std::setw(2) << m << std::endl;

            // Check if this time matches a dose slot
            for (auto &entry : schedule) {
                if (entry.hour == h && entry.minute == m) {
                    runDose(entry);
                }
            }

            // Fast-forward simulation (1 second = 1 minute)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
}

// ---- Main ----
int main() {
    float totalMl = 100.0;  // total daily dose
    int slots = 48;         // 24, 48, 96...
    int startHour = 11;     // heavy window start
    int endHour   = 23;     // heavy window end
    float percent1 = 0.6;   // 60% in window 1
    float percent2 = 0.4;   // 40% in window 2

    generateWeightedSchedule(slots, totalMl, startHour, endHour, percent1, percent2);

    // Simulate a full day
    simulateDay();

    return 0;
}