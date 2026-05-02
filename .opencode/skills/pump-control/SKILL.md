---
name: pump-control
description: Pump control (DRV8871, PWM, timing, safety)
license: MIT
compatibility: opencode
metadata:
  project: kh-monitor
  layer: hardware
---

# Pump Control Skill

## Overview
Peristaltic pump control with DRV8871 driver, PWM speed control, and safety features.

## Hardware

| Pump | GPIO | Driver |
|------|------|--------|
| REF IN1 | 4 | DRV8871 #1 |
| REF IN2 | 5 | DRV8871 #1 |
| TANK IN1 | 6 | DRV8871 #2 |
| TANK IN2 | 7 | DRV8871 #2 |

## Class Hierarchy

```
PWMPumpController (base)
├── RefPumpController
└── TankPumpController
```

## Base Class (PWMPumpController.h)

```cpp
class PWMPumpController {
public:
  void begin(uint8_t in1, uint8_t in2);
  void forward();
  void reverse();
  void stop();
  void setSpeed(uint8_t speed); // 0-255
  void update(); // non-blocking
  
  bool isRunning() const;
  unsigned long getRunTime() const;
};
```

## Key Methods

### begin()
Initialize GPIO pins, set as OUTPUT.

```cpp
void begin(uint8_t in1, uint8_t in2) {
  m_in1 = in1;
  m_in2 = in2;
  pinMode(m_in1, OUTPUT);
  pinMode(m_in2, OUTPUT);
  stop();
}
```

### forward() / reverse()
Direction control. DRV8871: IN1=HIGH, IN2=LOW = forward.

```cpp
void forward() {
  digitalWrite(m_in1, HIGH);
  digitalWrite(m_in2, LOW);
  m_direction = Direction::FORWARD;
}

void reverse() {
  digitalWrite(m_in1, LOW);
  digitalWrite(m_in2, HIGH);
  m_direction = Direction::REVERSE;
}
```

### setSpeed()
PWM speed control via ledc (ESP32 HAL).

```cpp
void setSpeed(uint8_t speed) {
  m_speed = constrain(speed, 0, 255);
  if (m_speed > 0) {
    ledcWrite(m_channel, m_speed);
  }
}
```

## Timing (non-blocking)

```cpp
void update() {
  if (!m_isRunning) return;
  
  unsigned long now = millis();
  if (m_targetTime > 0 && now - m_startTime >= m_targetTime) {
    stop();
  }
}
```

## Safety Features

1. **Max run time**: Prevent dry run
2. **Reverse delay**: Wait before reversing direction
3. **Speed ramp**: Gradual speed change

```cpp
static constexpr unsigned long MAX_RUN_TIME_MS = 30000UL;
static constexpr unsigned long REVERSE_DELAY_MS = 50UL;
```

## Usage in State Machine

```cpp
case KHState::FILL_REFERENCE:
  RefPumpController::getInstance().forward();
  RefPumpController::getInstance().setTargetTime(config.fillTimeMs);
  break;

case KHState::DRAIN:
  RefPumpController::getInstance().reverse();
  RefPumpController::getInstance().setTargetTime(config.drainTimeMs);
  break;
```

## Debug Output

```cpp
if (m_verbose) {
  Serial.printf("[PUMP] %s: %s, speed=%d, time=%lu\n",
    name, isRunning() ? "RUN" : "STOP", m_speed, getRunTime());
}
```