# Architecture

## System Overview

KH Monitor is an automated system that measures carbonate hardness (KH) of aquarium water using pH delta method.

## Hardware

- ESP32-S3 (ESP32-S3-DevKitM-1)
- pH probe (analog)
- 2x peristaltic pumps (DRV8871)
- 1x air pump (MOSFET)

## Workflow

1. Fill reference container with known KH water (7.5 dKH default)
2. Measure pH before aeration
3. Aerate to remove CO2
4. Measure pH after aeration → ΔpH_ref
5. Drain and flush
6. Fill tank water
7. Same process for tank → ΔpH_tank
8. Calculate: KH_tank = KH_ref × (ΔpH_tank / ΔpH_ref)

## Timing (configurable)

| Phase | Default |
|-------|--------|
| Fill | 5s |
| Stabilize | 3s |
| Aerate | 60s |
| Wait after aeration | 5s |
| Drain | 8s |
| Flush | 10s |
| Dose | 3s |
| Clean tube | 5s |

## Auto Cycle

- Default interval: 1 hour
- Uses `millis()` - non-blocking