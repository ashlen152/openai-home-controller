# AGENTS.md

## Hardware Pin Map

```
==================== KH CONTROLLER PIN MAP ====================

PUMP REF (DRV8871 #1)
--------------------------------
GPIO4   → IN1
GPIO5   → IN2

PUMP TANK (DRV8871 #2)
--------------------------------
GPIO6   → IN1
GPIO7   → IN2

AIR PUMP (MOSFET - IRLZ34N)
--------------------------------
GPIO15  → Gate (via 220Ω resistor)

pH SENSOR (ADC)
--------------------------------
GPIO1   → ADC input (with RC filter)

RGB LED
--------------------------------
GPIO48  → Built-in NeoPixel RGB LED

==============================================================
```

## Build Commands

- **Build**: `platformio run`
- **Upload**: `platformio run --target upload`
- **Monitor Serial**: `platformio device monitor`
- **Clean**: `platformio run --target clean`

## Code Style Guidelines

- **Imports**:
  - Use `#include` for standard and library headers.
  - Group standard headers first, followed by library headers.

- **Formatting**:
  - Follow Arduino-style indentation (2 spaces per level).
  - Use braces `{}` for all control structures, even single-line blocks.

- **Types**:
  - Prefer `int`, `float`, and `bool` for simplicity.
  - Use `size_t` for sizes and indices.

- **Naming Conventions**:
  - Use camelCase for functions and variables.
  - Use PascalCase for classes.

- **Error Handling**:
  - Use `Serial.print` for debugging.
  - Implement error codes or flags for critical failures.

## Project Architecture

```
src/
├── main.cpp                    - Entry point, orchestration via getInstance()
├── KHStateMachine/             - State machine (21 states including calibration)
├── KHSolver/                   - KH calculation with linear/quadratic fitting
├── PHProbe/                    - pH sensor with Median(5) + MovingAvg(10) filters
├── AerationPump/               - Air pump MOSFET control
├── RefPumpController/          - Reference pump (DRV8871)
├── TankPumpController/         - Tank pump (DRV8871)
└── PWMPumpController/          - Base GPIO pump class with safety features
```

## Singleton Pattern

All modules use `getInstance()` for access:

```
RefPumpController::getInstance()
TankPumpController::getInstance()
AerationPump::getInstance()
PHProbe::getInstance()
KHSolver::getInstance()
KHStateMachine::getInstance()
```

## Normal KH State Machine

```
IDLE → FILL_REFERENCE → STABILIZE_REFERENCE → MEASURE_REFERENCE_INITIAL
    → AERATE_REFERENCE → WAIT_AFTER_AERATION_REF → MEASURE_REFERENCE_FINAL
    → DRAIN → FLUSH → FILL_TANK → STABILIZE_TANK → MEASURE_TANK_INITIAL
    → AERATE_TANK → WAIT_AFTER_AERATION_TANK → MEASURE_TANK_FINAL
    → CALCULATE_KH → DOSE → CLEAN_TUBE → IDLE
```

## Calibration Mode State Machine

```
CALIB_IDLE → CALIB_MEASURE → (normal measurement) → CALIB_STORE → CALIB_DONE
```

- Calibration reuses normal measurement states (FILL_REFERENCE → CALCULATE_KH)
- After CALCULATE_KH, transitions to CALIB_STORE instead of DOSE

## Calibration Commands

| Command | Description |
|---------|-------------|
| `kh calib start <kh>` | Start calibration with known KH value |
| `kh calib add <kh>` | Add another calibration point |
| `kh calib finish` | Fit curve (2pts=linear, 3+=quadratic) and apply |
| `kh calib clear` | Reset calibration to factory |
| `kh calib list` | Show stored calibration points |

## Serial Commands

| Command | Description |
|---------|-------------|
| `start` | Start KH measurement cycle |
| `stop` | Stop current cycle |
| `status` | Print state, KH value, uptime |
| `verbose` | Toggle debug output |
| `kh ...` | Forward to KHSolver |
| `help` | Show available commands |

## Auto KH Cycle

- Configurable interval (default: 4 hours)
- Uses `millis()` - non-blocking

## KH Formula

```
KH = a * (ΔpH_tank / ΔpH_ref) + b        (linear)
KH = a + b*x + c*x^2                     (quadratic, x = ratio)
```

## Notes for Agents

- All modules use non-blocking `millis()` timing - never use `delay()` in loop
- Use `rgbLedWrite()` for RGB LED (GPIO 48) - no external library needed
- KHStateMachine receives injected module references via `begin()`
- Calibration mode does not modify normal KH flow
- All pump classes inherit from PWMPumpController with safety features