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
Displacement-based fluid system - uses volume (ml) instead of time for pump control.

## States

### Normal Mode Flow (Volume-Based)
```
IDLE → PRE_FLUSH → FILL_REFERENCE → FLUSH_LINE → STABILIZE_REFERENCE → MEASURE_REFERENCE_INITIAL
    → AERATE_REFERENCE → WAIT_AFTER_AERATION_REF → MEASURE_REFERENCE_FINAL
    → PARTIAL_DRAIN → FLUSH_CHAMBER → FILL_TANK → FLUSH_LINE_TANK
    → STABILIZE_TANK → MEASURE_TANK_INITIAL → AERATE_TANK
    → WAIT_AFTER_AERATION_TANK → MEASURE_TANK_FINAL
    → CALCULATE_KH → DOSE → FINALIZE_CHAMBER → IDLE
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
  PRE_FLUSH,
  FILL_REFERENCE,
  FLUSH_LINE,
  STABILIZE_REFERENCE,
  MEASURE_REFERENCE_INITIAL,
  AERATE_REFERENCE,
  WAIT_AFTER_AERATION_REF,
  MEASURE_REFERENCE_FINAL,
  PARTIAL_DRAIN,
  FLUSH_CHAMBER,
  FILL_TANK,
  FLUSH_LINE_TANK,
  STABILIZE_TANK,
  MEASURE_TANK_INITIAL,
  AERATE_TANK,
  WAIT_AFTER_AERATION_TANK,
  MEASURE_TANK_FINAL,
  CALCULATE_KH,
  DOSE,
  FINALIZE_CHAMBER,  // ensures probe chamber stays wet
  ERROR,
  // Calibration mode states
  CALIB_IDLE,
  CALIB_MEASURE,
  CALIB_STORE,
  CALIB_DONE
};
```

### Fluid System Config (FluidSystemConfig)
```cpp
struct FluidSystemConfig {
  float tubeVolumeMl = 2.0f;
  float pumpHeadVolumeMl = 1.5f;
  float chamberVolumeMl = 3.0f;
  float referenceVolumeMl = 20.0f;
  float tankVolumeMl = 20.0f;
  float doseVolumeMl = 5.0f;      // TODO: not yet implemented
  float flushMultiplier = 2.5f;   // flush = dead * 2.5

  float getDeadVolumeMl() const { return tube + pumpHead + chamber; }
  float getFlushVolumeMl() const { return deadVolume * flushMultiplier; }
};
```

### Timing Config (KHStateConfig)
```cpp
struct KHStateConfig {
  unsigned long fillTimeMs = 5000UL;
  unsigned long stabilizeTimeMs = 3000UL;
  unsigned long aerationTimeMs = 900000UL;
  unsigned long waitAfterAerationMs = 5000UL;
  unsigned long doseTimeMs = 3000UL;

  float referenceVolumeMl = 20.0f;
  float tankVolumeMl = 20.0f;
  float stepsPerMl = 100.0f;   // used for volume→time conversion
};
```

## Volume-Based Pump Control (DC Pump)

DC pump uses time-based approximation:
```cpp
// PWMPumpController defaults
static constexpr float DEFAULT_FLOW_RATE_ML_PER_SEC = 2.0f;  // 2 ml/sec

void runVolume(PumpDirection direction, float volumeMl) {
  float rate = m_flowRateMlPerSec;  // default 2.0 ml/sec
  uint32_t durationMs = (volumeMl / rate) * 1000;
  runTimed(direction, durationMs);
}
```

Calibrate flow rate: `pump.setFlowRateMlPerSec(your_rate)` (e.g., 1.5 ml/sec)

## LED Colors

| State | RGB |
|-------|-----|
| IDLE | Off |
| PRE_FLUSH, FLUSH_* | White |
| STABILIZE_* | Yellow |
| MEASURE_* | Cyan |
| AERATE_* | Blue |
| PARTIAL_DRAIN | Orange |
| CALCULATE_KH | Yellow |
| CALIB_* | Purple |
| DOSE | Green |
| FINALIZE_CHAMBER | Light Blue |
| ERROR | Red |

## Non-Blocking Pattern

```cpp
void loop() {
  unsigned long now = millis();

  switch (m_currentState) {
    case KHState::FILL_REFERENCE:
      if (now - m_stateEntryTime >= m_config.fillTimeMs) {
        transitionTo(KHState::STABILIZE_REFERENCE);
      }
      break;
    // ... other states
  }
}
```

## Key Points

- NEVER use delay() - use millis() with stateEntryTime
- Each state has onEnter, onUpdate, onExit (implicit via transition)
- Log every state transition via Serial
- Use transitionTo() for all state changes
- Check isStable() before measurement states
- Volume-based: flush ≥ 2-3x dead volume to ensure clean displacement
- Probe chamber MUST always remain wet
- DOSE handler: TODO - not yet implemented