# State Machine

## Overview
Displacement-based fluid system. Track volumes, flush using dead volume calculations.
Probe chamber MUST always remain wet.

## States

### Normal Mode (Volume-Based)
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
CALIB_IDLE → CALIB_MEASURE → (normal measurement) → CALIB_STORE → CALIB_DONE → IDLE
```

## Fluid System

### Dead Volume Calculation
```
tubeVolumeMl      = 2.0ml   (tubing from pump to chamber)
pumpHeadVolumeMl = 1.5ml   (inside pump head)
chamberVolumeMl  = 3.0ml   (pH probe chamber)

deadVolumeMl = tube + pumpHead + chamber  // ~6.5ml
flushVolumeMl = deadVolumeMl * 2.5         // ~16ml (2.5x flush)
```

### Volume Rules
- NEW fluid → PUSH → OLD fluid out (displacement, not drain-and-fill)
- Flush volume ≥ 2-3x dead volume for complete displacement
- Probe chamber must ALWAYS remain wet
- FINALIZE_CHAMBER at cycle end ensures probe stays wet

## LED Colors

| State | LED Color |
|-------|----------|
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

## State Details

### IDLE
- Pumps: OFF
- pH: readings valid
- Next: auto-trigger or manual start

### FILL_REFERENCE
- Pump: REFERENCE on
- Volume: referenceVolumeMl (20ml default)
- Notes: Fill reference chamber with known KH solution
- Next: FLUSH_LINE

### FLUSH_LINE
- Pump: REFERENCE forward
- Volume: getFlushVolumeMl()
- Notes: Push new fluid through line, displacing old
- Next: STABILIZE_REFERENCE

### STABILIZE_REFERENCE
- Pump: OFF
- Time: stabilizeTimeMs (3s)
- Notes: Wait for pH to stabilize
- Next: MEASURE_REFERENCE_INITIAL

### MEASURE_REFERENCE_INITIAL
- pH: capture initial reading
- Next: AERATE_REFERENCE

### AERATE_REFERENCE
- Air pump: ON
- Time: aerationTimeMs (15min default)
- Next: WAIT_AFTER_AERATION_REF

### WAIT_AFTER_AERATION_REF
- Air pump: OFF
- Time: waitAfterAerationMs (5s)
- Notes: Let bubbles dissipate
- Next: MEASURE_REFERENCE_FINAL

### MEASURE_REFERENCE_FINAL
- pH: capture final reading after aeration
- Notes: Use this for KH calculation
- Next: PARTIAL_DRAIN

### PARTIAL_DRAIN
- Pump: TANK reverse
- Volume: chamberVolumeMl
- Notes: Pull tank water into chamber (for mixing measurement)
- Next: FLUSH_CHAMBER

### FLUSH_CHAMBER
- Pump: TANK forward
- Volume: getFlushVolumeMl()
- Notes: Push tank water through chamber, displacing reference solution
- Next: FILL_TANK

### FILL_TANK
- Pump: TANK forward
- Volume: tankVolumeMl (20ml)
- Notes: Fill tank with replacement water
- Next: FLUSH_LINE_TANK

### FLUSH_LINE_TANK
- Pump: TANK forward  
- Volume: getDeadVolumeMl()
- Notes: Flush dead volume from pump/tubing
- Next: STABILIZE_TANK

### STABILIZE_TANK
- Pump: OFF
- Time: stabilizeTimeMs (3s)
- Next: MEASURE_TANK_INITIAL

### MEASURE_TANK_INITIAL, AERATE_TANK, WAIT_AFTER_AERATION_TANK, MEASURE_TANK_FINAL
- Same pattern as reference side
- Next: CALCULATE_KH

### CALCULATE_KH
- Compute: KH from reference and tank pH difference
- Uses KHSolver with calibration curve
- Next: DOSE

### DOSE
- TODO: Not yet implemented
- Expected: Dispense doseVolumeMl of KH buffer
- Next: FINALIZE_CHAMBER

### FINALIZE_CHAMBER (Important!)
- Pump: TANK reverse
- Volume: getFlushVolumeMl()
- Notes: Ensure pH probe chamber stays wet after cycle
- CRITICAL: Probe must never go dry
- Next: IDLE

### ERROR
- Pumps: all OFF
- Air pump: OFF
- Display error message
- Next: IDLE (on reset/clear)

##Timing Config

| Parameter | Default | Description |
|-----------|---------|-------------|
| fillTimeMs | 5000 | Fill duration |
| stabilizeTimeMs | 3000 | pH stabilization wait |
| aerationTimeMs | 900000 | 15 minutes |
| waitAfterAerationMs | 5000 | Post-aeration wait |
| doseTimeMs | 3000 | Dose operation |

## Volume Config

| Parameter | Default | Description |
|-----------|---------|-------------|
| tubeVolumeMl | 2.0 | Tubing volume |
| pumpHeadVolumeMl | 1.5 | Pump head volume |
| chamberVolumeMl | 3.0 | Probe chamber volume |
| referenceVolumeMl | 20.0 | Reference fill volume |
| tankVolumeMl | 20.0 | Tank fill volume |
| doseVolumeMl | 5.0 | Dose amount (TODO) |
| flushMultiplier | 2.5 | Flush = dead × multiplier |

## DC Pump Flow Rate Calibration

Default: 2.0 ml/sec (500ml per second)
- Applies to both REFERENCE and TANK pumps

Calibrate:
```cpp
// In setup or via serial command
refPump.setFlowRateMlPerSec(1.5);   // your measured rate
tankPump.setFlowRateMlPerSec(1.5);
```

Volume → time conversion:
```
time_ms = (volume_ml / flow_rate_ml_per_sec) * 1000
```