---
name: state-machine
description: KH State Machine implementation (states, transitions, timing, LED colors)
license: MIT
compatibility: opencode
metadata:
  project: kh-monitor
  layer: control-flow
---

# State Machine Skill

## Overview
KH State Machine drives the entire measurement cycle. Uses non-blocking millis() timing.

## States

### Normal Mode Flow
```
IDLE → FILL_REFERENCE → STABILIZE_REFERENCE → MEASURE_REFERENCE_INITIAL
    → AERATE_REFERENCE → WAIT_AFTER_AERATION_REF → MEASURE_REFERENCE_FINAL
    → DRAIN → FLUSH → FILL_TANK → STABILIZE_TANK → MEASURE_TANK_INITIAL
    → AERATE_TANK → WAIT_AFTER_AERATION_TANK → MEASURE_TANK_FINAL
    → CALCULATE_KH → DOSE → CLEAN_TUBE → IDLE
```

### Calibration Mode
```
CALIB_IDLE → CALIB_MEASURE → (normal flow) → CALIB_STORE → CALIB_DONE → IDLE
```

## Implementation

### State Enum (KHStateMachine.h)
```cpp
enum class KHState {
  IDLE,
  FILL_REFERENCE,
  STABILIZE_REFERENCE,
  MEASURE_REFERENCE_INITIAL,
  AERATE_REFERENCE,
  WAIT_AFTER_AERATION_REF,
  MEASURE_REFERENCE_FINAL,
  DRAIN,
  FLUSH,
  FILL_TANK,
  STABILIZE_TANK,
  MEASURE_TANK_INITIAL,
  AERATE_TANK,
  WAIT_AFTER_AERATION_TANK,
  MEASURE_TANK_FINAL,
  CALCULATE_KH,
  DOSE,
  CLEAN_TUBE,
  ERROR,
  CALIB_IDLE,
  CALIB_MEASURE,
  CALIB_STORE,
  CALIB_DONE
};
```

### Timing Config (KHStateConfig)
```cpp
struct KHStateConfig {
  unsigned long fillTimeMs = 5000UL;
  unsigned long stabilizeTimeMs = 3000UL;
  unsigned long aerationTimeMs = 60000UL;
  unsigned long waitAfterAerationMs = 5000UL;
  unsigned long drainTimeMs = 8000UL;
  unsigned long flushTimeMs = 10000UL;
  unsigned long doseTimeMs = 3000UL;
  unsigned long cleanTubeTimeMs = 5000UL;
};
```

## LED Colors

| State | RGB |
|-------|-----|
| IDLE | Off |
| FILL_* | White |
| STABILIZE_* | Yellow |
| MEASURE_* | Cyan |
| AERATE_* | Blue |
| DRAIN, FLUSH | Orange |
| CALCULATE_KH | Yellow |
| CALIB_* | Purple |
| DOSE | Green |
| CLEAN_TUBE | Light Blue |
| ERROR | Red |

## Non-Blocking Pattern

```cpp
void loop() {
  unsigned long now = millis();
  
  switch (m_currentState) {
    case KHState::FILL_REFERENCE:
      if (now - m_stateStartTime >= m_config.fillTimeMs) {
        transitionTo(KHState::STABILIZE_REFERENCE);
      }
      break;
    // ... other states
  }
}
```

## Key Points

- NEVER use delay() - use millis() with stateStartTime
- Each state has onEnter, onUpdate, onExit (implicit via transition)
- Log every state transition via Serial
- Use transitionTo() for all state changes
- Check isStable() before measurement states