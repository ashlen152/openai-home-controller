# Hardware

## Pin Map

| Component | GPIO | Notes |
|----------|------|-------|
| REF PUMP IN1 | 4 | DRV8871 #1 |
| REF PUMP IN2 | 5 | DRV8871 #1 |
| TANK PUMP IN1 | 6 | DRV8871 #2 |
| TANK PUMP IN2 | 7 | DRV8871 #2 |
| AIR PUMP | 15 | MOSFET (IRLZ34N) |
| pH SENSOR | 1 | ADC |
| RGB LED | 48 | Built-in NeoPixel |

## Build Flags

```
-DARDUINO_USB_CDC_ON_BOOT=1
-DBOARD_HAS_PSRAM=1
-DREF_IN1=4
-DREF_IN2=5
-DTANK_IN1=6
-DTANK_IN2=7
-DAIR_PIN=15
-DPH_PIN=1
-DREVERSE_DELAY=50
-DCORE_DEBUG_LEVEL=0
-DUSE_MOCK_PH=0    ; 1 = mock pH sensor
```

## Build Commands

```bash
pio run              # Build
pio run --target upload  # Upload
pio device monitor   # Serial monitor
```

## Configuration (platformio.ini)

| Flag | Default | Description |
|------|---------|-------------|
| USE_MOCK_PH | 0 | Enable mock pH (6.8 default) |

## ADC

- ESP32-S3 ADC: 12-bit (0-4095)
- Reference: ~3.3V
- Attenuation: 11dB (0-3.3V → 0-4095)