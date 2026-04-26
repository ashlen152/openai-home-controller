# Wave 2 Refactor Candidates — Magic Numbers

## Summary
- 54 delay() calls found across 3 files
- 69 magic number occurrences (1000, 2000, 3000, 5000) across 14 files

## Magic Numbers by Category

### Delays (delay(ms))
| File | Count | Values |
|------|-------|-------|
| MenuHandler.cpp | 40 | 100, 300, 500, 800, 1000, 1500, 2000 |
| WiFiManager.cpp | 10 | 10, 500, 1000, 2000, 3000 |
| ManualDosingController.cpp | 1 | 1000 |

### Timeout Intervals (ms)
| File | Line | Value | Suggested Constant |
|------|------|-------|------------------|
| main.cpp | 393 | 30000 | kCommandPollIntervalMs |
| main.cpp | 436 | 300000 | kScheduleUpdateIntervalMs |
| main.cpp | 442 | 1000 | kDoseCheckIntervalMs |
| AutoDosingManager.cpp | 378 | 300000 | kFallbackLogIntervalMs |
| AutoDosingManager.cpp | 534 | 300000 | kDosingTimeoutMs |

### Speed Values (steps/sec)
| File | Line | Value | Suggested Constant |
|------|------|-------|------------------|
| PumpController.cpp | 50-51 | 2000 | kDefaultAcceleration |
| PumpController.cpp | 251-252 | 1000-50000 | kMinSpeedSteps/kMaxSpeedSteps |
| PumpController.cpp | 293-294 | 10000/20000/40000 | kSpeedProfileSlow/Medium/Fast |

### Mutex Timeouts (pdMS_TO_TICKS)
| File | Count | Value |
|------|-------|-------|
| NetworkTaskManager.cpp | 11 | 1000 |

## Constants Recommendation

Create `include/Config.h` or add to existing:
```cpp
// Timing constants
constexpr uint32_t kDebounceDelayMs = 100;
constexpr uint32_t kMenuTransitionDelayMs = 500;
constexpr uint32_t kWiFiConnectTimeoutMs = 3000;
constexpr uint32_t kCommandPollIntervalMs = 30000;
constexpr uint32_t kScheduleUpdateIntervalMs = 300000;

// Speed constants  
constexpr float kMinSpeedStepsPerSec = 1000.0f;
constexpr float kMaxSpeedStepsPerSec = 50000.0f;
constexpr float kSpeedProfileSlow = 10000.0f;
constexpr float kSpeedProfileMedium = 20000.0f;
constexpr float kSpeedProfileFast = 40000.0f;
```

## Status
- **Scan complete**: No changes made
- **Recommendation**: Low priority - delays are readable as-is; only extract if needed for configuration