---
name: kh-calculation
description: KH calculation formula (ratio method, coefficients, calibration)
license: MIT
compatibility: opencode
metadata:
  project: kh-monitor
  layer: algorithm
---

# KH Calculation Skill

## Overview
KH calculation using ratio method from pH delta before/after aeration.

## Formula

```
KH_tank = KH_ref × (ΔpH_tank / ΔpH_ref)
```

Where:
- KH_ref = 7.5 dKH (reference water)
- ΔpH_ref = pH_after_aeration - pH_before_aeration (reference water)
- ΔpH_tank = pH_after_aeration - pH_before_aeration (tank water)

## Implementation (KHSolver.h)

```cpp
enum class KHCalibrationType {
  LINEAR = 0,
  QUADRATIC = 1,
  RATIO_LINEAR = 2,
  RATIO_QUADRATIC = 3
};

enum class KHInputType {
  RATIO = 0,
  DELTA_PH = 1
};

KHResult computeFromPair(float deltaPH_tank, float deltaPH_ref);
```

## computeFromPair() Implementation

```cpp
KHResult KHSolver::computeFromPair(float deltaPH_tank, float deltaPH_ref) {
  // Validate inputs
  if (!isValidInput(deltaPH_tank) || !isValidInput(deltaPH_ref)) {
    return invalidResult();
  }
  
  // Require minimum delta for reference
  if (deltaPH_ref < EPSILON) {
    return invalidResult();
  }
  
  // Calculate ratio
  float ratio = deltaPH_tank / deltaPH_ref;
  
  // Apply calibration (default: a = 7.5)
  float kh = m_a * ratio + m_b;
  
  // Apply clamping
  kh = constrain(kh, KH_MIN, KH_MAX);
  
  return { kh, true, 1.0f };
}
```

## Default Coefficients

| Parameter | Default | Description |
|-----------|---------|-------------|
| m_a | 7.5 | Reference KH (KH_ref) |
| m_b | 0.0 | Offset |
| m_c | 0.0 | Quadratic coefficient |

## Calibration Types

| Type | Formula |
|------|---------|
| LINEAR | KH = a × x + b |
| QUADRATIC | KH = a × x² + b × x + c |
| RATIO_LINEAR | KH = a × ratio + b |
| RATIO_QUADRATIC | KH = a × ratio² + b × ratio + c |

Where x = deltaPH (direct) or ratio = deltaPH_tank / deltaPH_ref

## Input Smoothing

Optional input smoothing to reduce noise:

```cpp
enum class KHSmoothingMode {
  NONE = 0,
  SMOOTH_INPUT = 1,  // Smooth deltaPH or ratio
  SMOOTH_OUTPUT = 2  // Smooth final KH
};

float applyInputSmoothing(float input) {
  m_inputSmoothingBuffer[m_inputSmoothingIndex] = input;
  m_inputSmoothingIndex = (m_inputSmoothingIndex + 1) % SMOOTHING_WINDOW;
  
  // Calculate average
  float sum = 0;
  for (uint8_t i = 0; i < m_inputSmoothingCount; i++) {
    sum += m_inputSmoothingBuffer[i];
  }
  return sum / m_inputSmoothingCount;
}
```

## Output Clamping

```cpp
static constexpr float KH_MIN = 0.0f;
static constexpr float KH_MAX = 20.0f;

float applyClamp(float kh) {
  if (!m_outputClampEnabled) return kh;
  return constrain(kh, m_clampMin, m_clampMax);
}
```

## Confidence Calculation

```cpp
float computeConfidence(float deltaPH_ref, float ratio) const {
  // Higher deltaPH_ref = more confidence
  // Lower ratio variance = more confidence
  float refConfidence = min(deltaPH_ref / MIN_DELTA_PH_FOR_CONFIDENCE, 1.0f);
  return refConfidence * 100.0f; // 0-100%
}
```

## EEPROM Storage

- Address: 64
- Size: 64 bytes
- Stores: calibration type, coefficients, checksum

## Usage in State Machine

```cpp
case KHState::CALCULATE_KH:
  KHResult result = m_khSolver->computeFromPair(
    m_tankPH_final - m_tankPH_initial,
    m_refPH_final - m_refPH_initial
  );
  if (result.valid) {
    m_calculatedKH = result.value;
    Serial.printf("[KH] Calculated: %.2f dKH (confidence: %.0f%%)\n",
      m_calculatedKH, result.confidence);
  }
  transitionTo(KHState::DOSE);
  break;
```

## Constants

```cpp
static constexpr float EPSILON = 0.001f;
static constexpr float KH_MIN = 0.0f;
static constexpr float KH_MAX = 20.0f;
static constexpr float MIN_DELTA_PH_FOR_CONFIDENCE = 0.05f;
static constexpr unsigned long HYSTERESIS_STABILITY_MS = 1000UL;
```