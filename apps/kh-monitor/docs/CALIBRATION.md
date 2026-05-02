# Calibration

## KH Formula

Default (RATIO_LINEAR):
```
KH_tank = KH_ref × (ΔpH_tank / ΔpH_ref)
```
Where:
- KH_ref = 7.5 dKH (default, stored in KHSolver m_a)
- ΔpH_ref = pH_after_aeration - pH_before_aeration (reference water)
- ΔpH_tank = pH_after_aeration - pH_before_aeration (tank water)

## Coefficients

| Parameter | Default | Description |
|-----------|---------|-------------|
| m_a | 7.5 | Reference KH (KH_ref) |
| m_b | 0.0 | Offset (not used) |
| m_c | 0.0 | Quadratic coefficient |

## Calibration Types

| Type | Formula |
|------|---------|
| LINEAR | KH = a × x + b |
| QUADRATIC | KH = a × x² + b × x + c |
| RATIO_LINEAR | KH = a × ratio + b |
| RATIO_QUADRATIC | KH = a × ratio² + b × ratio + c |

Where ratio = ΔpH_tank / ΔpH_ref

## Default Coefficients

Default values in KHSolver.cpp:
```cpp
m_a = 7.5f   // KH_ref
m_b = 0.0f
m_c = 0.0f
```

## pH Sensor

Default calibration in PHProbe.h:
- Slope: -5.6548
- Offset: 21.7543

Formula: pH = slope × voltage + offset

## EEPROM

- KHSolver: Address 64, 64 bytes
- PHProbe: Address 0, 16 bytes