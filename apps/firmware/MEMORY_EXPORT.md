# SmartPump Memory Export

This file contains all critical project knowledge stored in MCP memory. Use this to reconstruct the knowledge graph on another computer or to review project context.

## How to Use This File

If you need to migrate to another computer or restore memory:
1. Share this file with the AI agent
2. Ask the agent to "restore memory from MEMORY_EXPORT.md"
3. The agent will recreate all entities and relations

---

## Entities

### Project: SmartPump
- ESP32-based peristaltic pump controller for aquarium fertilizer dosing
- Hardware: ESP32 DevKit v1, TMC2209 stepper driver, SSD1306 OLED, 4 buttons
- Framework: Arduino (PlatformIO), uses FreeRTOS underneath
- Key libraries: TMCStepper, AccelStepper, Adafruit_SSD1306, ArduinoJson, ArduinoHttpClient
- WiFi connectivity for NTP time sync and HTTP API (192.168.68.108:3000)
- Bluetooth Classic support (BluetoothManager - currently unused)
- Default calibration: 709.22 steps/mL
- EEPROM storage for calibration and settings (512 bytes)
- Project location: /home/tpthinh/learn/SmartPump

### Class: PumpController
- Singleton pattern - controls TMC2209 + AccelStepper
- Three modes: PERISTALTIC (continuous), DOSING (position control), HOLDING (idle)
- moveML(float ml) - dispenses precise volume using calibrated steps/mL
- runDosing() must be called in loop() for position-based movement
- Default TMC2209 config: toff=5, 500mA RMS, stealthChop enabled
- Default motion: maxSpeed=4000, acceleration=2000, speedStep=2000
- Enable pin is active-LOW (HIGH=off, LOW=on)
- File: lib/PumpController/PumpController.h/.cpp

### Class: DisplayManager
- Singleton pattern - manages SSD1306 128x64 OLED via I2C (0x3C)
- State machine with DisplayState enum (NORMAL, MENU, CALIBRATE_*, DOSING_MANUAL_*, etc.)
- Uses DisplayContext struct to pass data to rendering functions
- updateDisplayState() renders current state (BUG: resets lastUpdate=0 every call)
- Shows WiFi signal strength indicator in bottom-left corner
- Display sleep/wake functionality
- File: lib/DisplayManager/DisplayManager.h/.cpp

### Class: WiFiManager
- Singleton pattern - handles WiFi, HTTP client, NTP time sync
- Server: 192.168.68.108:3000 (hardcoded)
- NTP: Asia pool servers, timezone ICT-7 (Vietnam UTC+7)
- HTTP timeout: 1 second, WiFi retry: 5 seconds, Sync interval: 3 minutes
- BUG: connect() is blocking (delay(500) loop + sleep(2))
- Methods: GET, POST, PUT, DELETE with JSON support
- Time re-sync: every hour automatically
- File: lib/WiFiManager/WiFiManager.h/.cpp

### Class: AutoDosingManager
- WIP - automated dosing scheduler with weighted day/night distribution
- Default: 48 slots/day, 60% day (11:00-23:00), 40% night (23:00-11:00)
- Persists state to EEPROM (enabled, daily volume, last dose time, total dosed)
- BUG: generateWeightedSchedule(), checkAndDose(), resetDailyVolume(), performDosing(), getRemainingDailyVolume() are incomplete
- Currently commented out in main.cpp (not instantiated)
- Reference: autoschedule.cpp for standalone simulation
- File: lib/AutoDosingManager/AutoDosingManager.h/.cpp

### Feature: ManualDosingFlow
- 4-step user flow: Begin (volume) -> Start (duration) -> Progress (pumping) -> Complete
- Default volume: 10mL, adjustable +/- 0.1mL per button press
- Default duration: 1 min, adjustable +/- 1 min (min=1) - NOTE: duration not actually used
- Progress: monitors pump.getDistanceToGo(), shows remaining volume on display
- Enable button cancels at any step, Menu button proceeds to next step
- Files: src/ManualDosingController/, src/ViewController/Manual/

### Feature: CalibrationFlow
- 3-step calibration: Begin (prompt) -> Progress (run motor) -> Complete (measure + calculate)
- Runs motor for 200,000 steps at 2000 steps/sec
- User measures actual mL dispensed, enters with Up/Down buttons (+/- 0.1mL)
- Calculates: newStepsPerML = 200000 / measuredML
- Saves to EEPROM at EEPROM_DOSING_STEPS_ADDR (address 4)
- BUG: Functions not declared in .h file despite being defined in .cpp
- Files: src/CalibrateDosingController/

### Feature: ButtonSystem
- 4 physical buttons: Enable (25), SpeedUp (35), SpeedDown (34), Menu (14)
- All buttons use INPUT_PULLUP (active-LOW)
- Two detection modes: press (single) and hold (auto-repeat)
- Hold behavior: first press immediate, then 500ms intervals, accelerates to 100ms after 2s
- Automatically wakes display from sleep on button press
- ButtonHandler (low-level GPIO) -> ButtonController (high-level API) -> ViewControllers
- Files: src/ButtonController/ButtonHandler.h/.cpp, ButtonController.h/.cpp

### Architecture: DisplayStateFlow
- State hierarchy: NORMAL (home) -> MENU -> sub-states (CALIBRATE_*, DOSING_*, SETTINGS)
- Home (NORMAL): Menu button opens menu, Enable enters manual dosing
- Menu: 4 items (Dosing Cal, Settings Info, Auto Dosing, Set Daily Vol)
- Manual flow: DOSING_MANUAL_BEGIN -> START -> PROGRESS -> COMPLETE
- Calibration flow: CALIBRATE_BEGIN -> PROGRESS -> COMPLETE
- ViewControllers: HomeHandler, MenuHandler, ManualHandler
- BUG: ManualHandler logic issue - calls beginManualDosingController when !isInManual()

### Data: EEPROMMap
- Total size: 512 bytes allocated
- Address 0 (4 bytes): Peristaltic steps/mL (float)
- Address 4 (4 bytes): Dosing steps/mL (float)
- Address 8 (4 bytes): Saved speed (float)
- Address 12 (1 byte): Mode (uint8_t)
- Address 13 (1 byte): Auto dosing enabled (bool)
- Address 14 (4 bytes): Daily volume (float)
- Address 18 (4 bytes): Last dosing time (uint32_t)
- Address 22 (4 bytes): Total dosed volume (float)
- Defined in: include/Config.h

### Technical Debt: KnownBugs
- BUG #1: ButtonConfig.h uses WIFICONFIG_H include guard (conflicts with WifiConfig.h) - should be BUTTONCONFIG_H (FIXED in this session)
- BUG #2: Hardcoded WiFi credentials in WifiConfig.h alongside build-flag approach
- BUG #3: DisplayManager.cpp references CALIBRATION_START/INPUT/RESULT which don't exist in enum
- BUG #4: AutoDosingManager incomplete - generateWeightedSchedule, checkAndDose, etc. not implemented
- BUG #5: BluetoothManager included but never instantiated in main.cpp
- BUG #6: WiFiManager::connect() is blocking (delay loops, freezes pump control)
- BUG #7: DisplayManager::updateDisplayState() resets lastUpdate=0 every call (defeats 200ms throttle)
- BUG #8: CalibrateDosingController.h empty (no function declarations despite .cpp having 3 functions)
- BUG #9: ManualHandler state logic calls beginManualDosingController(isInManualBegin()) when !isInManual()

### Recommendation: ArchitectureRecommendations
- RECOMMENDED: Implement multi-core separation using FreeRTOS (already available in Arduino ESP32)
- Core 0: WiFi, HTTP, MQTT, NTP sync (network tasks)
- Core 1: Pump control, display, buttons (time-critical tasks)
- Use xTaskCreatePinnedToCore() to create tasks pinned to specific cores
- Use xQueueSend/Receive for inter-core communication
- DO NOT migrate to ESP-IDF yet - Arduino works fine, libraries are Arduino-native
- Migration would require rewriting all hardware drivers (TMCStepper, AccelStepper, Adafruit_SSD1306)
- Consider multi-core only when experiencing timing issues during WiFi operations

### User Context: UserQuestions
- User tpthinh asked about multi-core separation (Core 0: network, Core 1: pump)
- User asked about migrating from Arduino to ESP-IDF
- Recommendation given: YES to multi-core with Arduino+FreeRTOS, NO to ESP-IDF migration
- User wants comprehensive documentation and memory storage for future reference
- User wants ability to migrate memory to another computer
- Project path: /home/tpthinh/learn/SmartPump

### Tooling: BuildSystem
- Build: platformio run
- Test: platformio test (or platformio test -f <test_name>)
- Platform: ESP32 (espressif32), Board: esp32dev
- Framework: Arduino
- Serial monitor: 115200 baud
- Pre-build script: load_env.py (loads WiFi credentials from .env)
- Build flags: WIFI_SSID and WIFI_PASSWORD from environment variables
- No linting configured - follow style guidelines in AGENTS.md

---

## Relations

- SmartPump **uses** PumpController
- SmartPump **uses** DisplayManager
- SmartPump **uses** WiFiManager
- SmartPump **plans_to_use** AutoDosingManager
- SmartPump **implements** ManualDosingFlow
- SmartPump **implements** CalibrationFlow
- SmartPump **implements** ButtonSystem
- SmartPump **implements** DisplayStateFlow
- SmartPump **uses** EEPROMMap
- PumpController **stores_data_in** EEPROMMap
- AutoDosingManager **stores_data_in** EEPROMMap
- ManualDosingFlow **controls** PumpController
- ManualDosingFlow **updates** DisplayManager
- CalibrationFlow **calibrates** PumpController
- CalibrationFlow **saves_to** EEPROMMap
- DisplayStateFlow **drives** DisplayManager
- ButtonSystem **triggers_transitions** DisplayStateFlow
- KnownBugs **affects** SmartPump
- ArchitectureRecommendations **suggests_improvements_for** SmartPump
- UserQuestions **triggered** ArchitectureRecommendations

---

## Quick Reference Commands

```bash
# Build the project
platformio run

# Upload to ESP32
platformio run --target upload

# Open serial monitor
platformio device monitor

# Run tests
platformio test

# Clean build
platformio run --target clean
```

## Key Files

- `AGENTS.md` - Comprehensive project documentation
- `platformio.ini` - Build configuration
- `include/Config.h` - Hardware/software constants
- `include/ButtonConfig.h` - Button pin definitions
- `include/WifiConfig.h` - WiFi credentials and API config
- `lib/PumpController/` - Motor control
- `lib/DisplayManager/` - OLED display management
- `lib/WiFiManager/` - Network connectivity
- `lib/AutoDosingManager/` - Automated dosing (WIP)
- `src/main.cpp` - Entry point
- `src/ManualDosingController/` - Manual dosing flow
- `src/CalibrateDosingController/` - Calibration flow
- `src/ViewController/` - UI state handlers
