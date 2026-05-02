---
name: esp32-optimization
description: ESP32-S3 optimization (memory, timing, power, non-blocking patterns)
license: MIT
compatibility: opencode
metadata:
  project: kh-monitor
  layer: optimization
---

# ESP32 Optimization Skill

## Overview
ESP32-S3 specific optimizations for memory, timing, and power.

## Memory (ESP32-S3-DevKitM-1)

| Resource | Size |
|----------|------|
| Flash | 16MB |
| SRAM | 327KB |
| PSRAM | 8MB (optional) |

## Build Flags (platformio.ini)

```
-DARDUINO_USB_CDC_ON_BOOT=1    # Enable USB CDC
-DBOARD_HAS_PSRAM=1           # Enable PSRAM
-DCORE_DEBUG_LEVEL=0          # Disable debug output
```

## Non-Blocking Timing

### WRONG (blocking)
```cpp
delay(5000);  // Blocks entire system
```

### CORRECT (non-blocking)
```cpp
static unsigned long stateStartTime = 0;

void update() {
  unsigned long now = millis();
  if (stateStartTime == 0) {
    stateStartTime = now;  // First entry
  }
  if (now - stateStartTime >= 5000UL) {
    // State complete
    stateStartTime = 0;
    transitionTo(nextState);
  }
}
```

## Memory Optimization

### Static Allocation
Prefer static over dynamic:

```cpp
// WRONG
String* buffer = new String[10];

// CORRECT
static char buffer[256];
static float smoothingBuffer[10];
```

### Constexpr
Use compile-time constants:

```cpp
static constexpr uint8_t BUFFER_SIZE = 10;
static constexpr unsigned long TIMEOUT_MS = 5000UL;
```

### Member Initialization
Initialize in constructor:

```cpp
ClassName() 
  : m_buffer{0}
  , m_index(0)
  , m_count(0)
{
  // No dynamic allocation
}
```

## Timing Optimization

### Rate Limiting
```cpp
static constexpr unsigned long MIN_UPDATE_INTERVAL_MS = 100;
unsigned long lastUpdate = 0;

void update() {
  unsigned long now = millis();
  if (now - lastUpdate < MIN_UPDATE_INTERVAL_MS) {
    return;
  }
  lastUpdate = now;
  // Do work
}
```

### Batched I/O
```cpp
// Collect samples, process in batch
static constexpr uint8_t BATCH_SIZE = 100;
uint8_t sampleCount = 0;
uint32_t adcSum = 0;

void sample() {
  adcSum += analogRead(pin);
  sampleCount++;
  if (sampleCount >= BATCH_SIZE) {
    uint32_t average = adcSum / BATCH_SIZE;
    process(average);
    adcSum = 0;
    sampleCount = 0;
  }
}
```

## Power Optimization

### WiFi Off (not used in kh-monitor)
```

### Dynamic Frequency
```cpp
setCpuFrequencyMhz(80);  // Lower for idle, 240 for active
```

### Sleep Modes
```cpp
// Light sleep between readings
if (deepSleep) {
  esp_sleep_enable_timer_task(wakeupTime);
  esp_deep_sleep_start();
}
```

## Serial Optimization

### Buffered Output
```cpp
// Use Serial.write for binary, printf for text
// Batch log messages
static char logBuffer[128];
```

### Debug Control
```cpp
void verboseLog(const char* msg) {
  if (m_verbose) {
    Serial.println(msg);
  }
}
```

## Best Practices

1. **Never use delay()** - use millis() pattern
2. **Never allocate in loop** - pre-allocate buffers
3. **Use constexpr** - compile-time constants
4. **Rate limit updates** - avoid CPU saturation
5. **Batch ADC samples** - reduce noise, improve accuracy
6. **Disable unused peripherals** - save power

## Common Patterns

### State Machine Timing
```cpp
class State {
  unsigned long m_stateStartTime;
  unsigned long m_stateDuration;
  
  bool isComplete() {
    return (millis() - m_stateStartTime >= m_stateDuration);
  }
  
  void enter(unsigned long duration) {
    m_stateStartTime = millis();
    m_stateDuration = duration;
  }
};
```

### Ring Buffer
```cpp
template<typename T, uint8_t N>
class RingBuffer {
  T buffer[N];
  uint8_t head = 0;
  uint8_t count = 0;
  
  void write(T value) {
    buffer[head] = value;
    head = (head + 1) % N;
    if (count < N) count++;
  }
};
```

## Memory Usage (typical)

| Component | RAM | Flash |
|----------|-----|-------|
| State Machine | ~1KB | ~50KB |
| KHSolver | ~1KB | ~30KB |
| PHProbe | ~500B | ~20KB |
| Pumps | ~500B | ~10KB |
| **Total** | ~3KB | ~110KB |