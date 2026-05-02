---
name: kh-monitor
description: KH Monitor ESP32-S3 project context (KH measurement, state machine, pH sensor)
license: MIT
compatibility: opencode
metadata:
  project: kh-monitor
  type: esp32-firmware
---

# KH Monitor Skill

## About This Skill
This skill provides context for the KH Monitor ESP32-S3 project.

## Project Location
- **Path**: `apps/kh-monitor/`
- **Type**: PlatformIO ESP32-S3 firmware
- **Purpose**: Automated KH (carbonate hardness) monitoring

## Project Structure

```
apps/kh-monitor/
├── platformio.ini           # PlatformIO config
├── src/
│   ├── main.cpp           # Entry point (auto-starts KH cycle)
│   ├── KHStateMachine/    # State machine (20 states)
│   ├── KHSolver/       # KH calculation
│   ├── PHProbe/        # pH sensor (ADC)
│   ├── AerationPump/   # Air pump control
│   ├── RefPumpController/   # Reference pump (DRV8871)
│   ├── TankPumpController/    # Tank pump (DRV8871)
│   └── PWMPumpController/   # Base pump class
├── docs/
│   ├── ARCHITECTURE.md   # System overview
│   ├── STATE_MACHINE.md  # States & transitions
│   ├── HARDWARE.md      # Pin map, build flags
│   └── CALIBRATION.md   # KH formula
└── AGENTS.md           # Agent directives

vs apps/firmware/ (SmartPump)

apps/firmware/                 → apps/kh-monitor/
├── lib/WiFiManager/         → (not needed - no WiFi)
├── lib/DisplayManager/        → (not needed - RGB LED only)
├── lib/NetworkTaskManager/    → (not needed - dual core not used)
├── src/main.cpp             → src/main.cpp (simpler)
├── include/Config.h         → platformio.ini
└── src/Calibrate*/        → src/KHStateMachine/ (state machine)
```

## Key Differences (kh-monitor vs firmware)

| Aspect | SmartPump (firmware) | KH Monitor |
|--------|-------------------|----------|
| Connectivity | WiFi, HTTP API | None (standalone) |
| Display | OLED SSD1306 | RGB LED only |
| Core | Dual-core (FreeRTOS) | Single core |
| Input | Buttons, menu | Serial commands |
| Control | Server API polling | Auto cycle |
| Output | HTTP to server | Serial log only |

## Documentation Structure

When working on kh-monitor, load:
- State logic → @docs/STATE_MACHINE.md
- Hardware / pins / pumps → @docs/HARDWARE.md
- KH math / formulas → @docs/CALIBRATION.md
- System design → @docs/ARCHITECTURE.md
- Agent directives → @AGENTS.md

## Runtime Configuration

| Setting | Value | File |
|---------|-------|------|
| pH mock | 6.8 | src/main.cpp |
| Reference KH | 7.5 dKH | src/KHSolver/KHSolver.cpp |
| KH interval | 1 hour | src/main.cpp |
| Mock mode | disabled | platformio.ini (USE_MOCK_PH) |

## Key Files

| File | Purpose |
|------|---------|
| src/main.cpp | Entry, auto-start, loop |
| src/KHStateMachine/KHStateMachine.cpp | State machine logic |
| src/KHSolver/KHSolver.cpp | KH calculation (7.5 × ratio) |
| src/PHProbe/PHProbe.cpp | pH sensor reading |
| platformio.ini | Build config, pins |

## Build Commands

```bash
cd apps/kh-monitor
pio run              # Build
pio run --target upload  # Upload
pio device monitor  # Serial monitor
```

## Important Notes

- NO WiFi / network - standalone system
- NO server API calls - serial log only
- NO dual-core - single core ESP32-S3
- Auto-start on boot - no manual start needed
- Non-blocking - all timing via millis()
- State machine drives entire system