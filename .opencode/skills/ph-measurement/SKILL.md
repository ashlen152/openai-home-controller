---
name: ph-measurement
description: pH sensor measurement (ADC, filtering, stability detection, mock mode)
license: MIT
compatibility: opencode
metadata:
  project: kh-monitor
  layer: sensor
---

# pH Measurement Skill

## Overview
pH sensor reading with ADC, two-stage filtering, and stability detection.

## Hardware

- **GPIO**: 1 (ADC)
- **ADC**: ESP32-S3 12-bit, 0-4095
- **Attenuation**: 11dB (0-3.3V → 0-4095)

## Class (PHProbe.h)

```cpp
class PHProbe {
public:
  void begin(uint8_t adcPin);
  void update(); // non-blocking, call in loop
  
  float readRawVoltage();
  float readRawPH();
  float readFilteredPH();
  bool isStable(float threshold = 0.02f, unsigned long duration_ms = 3000);
  bool isReady() const;
};
```

## Filtering Pipeline

```
Raw ADC → Median Filter (5 samples) → Moving Average (10 samples) → Filtered pH
```

### Median Filter (5)
Removes spikes/noise.

```cpp
float applyMedianFilter(float value) {
  m_medianBuffer[m_medianIndex] = value;
  m_medianIndex = (m_medianIndex + 1) % MEDIAN_FILTER_SIZE;
  return computeMedian(m_medianBuffer, m_medianFillCount);
}
```

### Moving Average (10)
Smooth final output.

```cpp
float applyMovingAverage(float value) {
  m_movingAvgBuffer[m_movingAvgIndex] = value;
  m_movingAvgIndex = (m_movingAvgIndex + 1) % MOVING_AVG_SIZE;
  // Calculate running average
}
```

## Voltage to pH Conversion

```cpp
float voltageToPH(float voltage) const {
  // Default: slope = -5.6548, offset = 21.7543
  return (m_slope * voltage) + m_offset;
}
```

## Stability Detection

```cpp
bool isStable(float threshold, unsigned long duration_ms) {
  // Check range of last N moving avg values
  float minVal = m_movingAvgBuffer[0];
  float maxVal = m_movingAvgBuffer[0];
  for (uint8_t i = 1; i < m_movingAvgFillCount; i++) {
    minVal = min(minVal, m_movingAvgBuffer[i]);
    maxVal = max(maxVal, m_movingAvgBuffer[i]);
  }
  float range = maxVal - minVal;
  return (range <= threshold);
}
```

## Mock Mode (USE_MOCK_PH)

Enable in platformio.ini: `-DUSE_MOCK_PH=1`

```cpp
#if USE_MOCK_PH
float readRawVoltage() {
  if (m_mockEnabled) {
    float voltage = m_mockPHValue / m_slope - m_offset / m_slope;
    return voltage;
  }
#endif
  // Real ADC reading
}
```

### Mock Configuration
```cpp
#if USE_MOCK_PH
PHProbe::getInstance().enableMock(true);
PHProbe::getInstance().setMockPH(6.8f);
#endif
```

## Usage in State Machine

```cpp
case KHState::MEASURE_REFERENCE_INITIAL:
  if (PHProbe::getInstance().isReady() && PHProbe::getInstance().isStable()) {
    m_refPH_initial = PHProbe::getInstance().readFilteredPH();
    transitionTo(KHState::AERATE_REFERENCE);
  }
  break;
```

## EEPROM Calibration

- Base address: 0
- Magic byte: 0x50 ('P')
- Stores: slope, offset, checksum

## Constants

```cpp
static constexpr uint8_t MEDIAN_FILTER_SIZE = 5;
static constexpr uint8_t MOVING_AVG_SIZE = 10;
static constexpr uint8_t ADC_SAMPLES_PER_READ = 100;
static constexpr float DEFAULT_SLOPE = -5.6548f;
static constexpr float DEFAULT_OFFSET = 21.7543f;
static constexpr float DEFAULT_STABILITY_THRESHOLD = 0.02f;
```