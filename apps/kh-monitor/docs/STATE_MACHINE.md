# State Machine

## States

### Normal Mode

```
IDLE → FILL_REFERENCE → STABILIZE_REFERENCE → MEASURE_REFERENCE_INITIAL
    → AERATE_REFERENCE → WAIT_AFTER_AERATION_REF → MEASURE_REFERENCE_FINAL
    → DRAIN → FLUSH → FILL_TANK → STABILIZE_TANK → MEASURE_TANK_INITIAL
    → AERATE_TANK → WAIT_AFTER_AERATION_TANK → MEASURE_TANK_FINAL
    → CALCULATE_KH → DOSE → CLEAN_TUBE → IDLE
```

### Calibration Mode

```
CALIB_IDLE → CALIB_MEASURE → (normal measurement) → CALIB_STORE → CALIB_DONE → IDLE
```

## LED Colors

| State | LED Color |
|-------|----------|
| IDLE | Off |
| FILL_REFERENCE, FILL_TANK | White |
| STABILIZE_REFERENCE, STABILIZE_TANK | Yellow |
| MEASURE_* | Cyan |
| AERATE_* | Blue |
| DRAIN, FLUSH | Orange |
| CALCULATE_KH | Yellow |
| CALIB_* | Purple |
| DOSE | Green |
| CLEAN_TUBE | Light Blue |
| ERROR | Red |

## State Details

### IDLE
- Pumps: OFF
- pH: readings valid
- Next: auto-trigger or manual start

### FILL_REFERENCE
- Pump: REFERENCE on
- Time: fillTimeMs
- Next: STABILIZE_REFERENCE

### STABILIZE_REFERENCE
- Pump: OFF
- Time: stabilizeTimeMs
- Next: MEASURE_REFERENCE_INITIAL

### MEASURE_REFERENCE_INITIAL
- pH: capture initial
- Next: AERATE_REFERENCE

### AERATE_REFERENCE
- Air pump: ON
- Time: aerationTimeMs
- Next: WAIT_AFTER_AERATION_REF

### WAIT_AFTER_AERATION_REF
- Air pump: OFF
- Time: waitAfterAerationMs
- Next: MEASURE_REFERENCE_FINAL

### MEASURE_REFERENCE_FINAL
- pH: capture final → ΔpH_ref
- Next: DRAIN

### DRAIN
- Pump: REFERENCE reverse
- Time: drainTimeMs
- Next: FLUSH

### FLUSH
- Pump: REFERENCE forward
- Time: flushTimeMs
- Next: FILL_TANK

### FILL_TANK
- Pump: TANK on
- Time: fillTimeMs
- Next: STABILIZE_TANK

### STABILIZE_TANK
- Pump: OFF
- Time: stabilizeTimeMs
- Next: MEASURE_TANK_INITIAL

### MEASURE_TANK_INITIAL
- pH: capture initial
- Next: AERATE_TANK

### AERATE_TANK
- Air pump: ON
- Time: aerationTimeMs
- Next: WAIT_AFTER_AERATION_TANK

### WAIT_AFTER_AERATION_TANK
- Air pump: OFF
- Time: waitAfterAerationMs
- Next: MEASURE_TANK_FINAL

### MEASURE_TANK_FINAL
- pH: capture final → ΔpH_tank
- Next: CALCULATE_KH

### CALCULATE_KH
- KH = KH_ref × (ΔpH_tank / ΔpH_ref)
- Next: DOSE

### DOSE
- Pump: TANK on (dose)
- Time: doseTimeMs
- Next: CLEAN_TUBE

### CLEAN_TUBE
- Pump: TANK reverse
- Time: cleanTubeTimeMs
- Next: IDLE