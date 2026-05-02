#ifndef PH_PROBE_H
#define PH_PROBE_H

#include <Arduino.h>

// Build flag to enable mock mode (set USE_MOCK_PH=1 in platformio.ini)
#ifndef USE_MOCK_PH
#define USE_MOCK_PH 0
#endif

/**
 * PHProbe - Advanced pH Sensor Module for ESP32
 * 
 * Features:
 * - Non-blocking ADC reading with hardware averaging
 * - Two-stage filter pipeline: Median (5) → Moving Average (10)
 * - Stability detection for calibration/control decisions
 * - EEPROM calibration storage with wear leveling
 * - Configurable slope/offset for different probes
 * - Debug logging (raw vs filtered)
 * - Mock mode for testing without real sensor
 */
class PHProbe
{
public:
  // Configuration constants
  static constexpr uint8_t MEDIAN_FILTER_SIZE = 5;
  static constexpr uint8_t MOVING_AVG_SIZE = 10;
  static constexpr uint8_t ADC_SAMPLES_PER_READ = 100;
  
  // Default calibration values (typical for pH probes)
  static constexpr float DEFAULT_SLOPE = -5.6548f;  // pH = slope * V + offset
  static constexpr float DEFAULT_OFFSET = 21.7543f;
  
  // Stability detection defaults
  static constexpr float DEFAULT_STABILITY_THRESHOLD = 0.02f;  // pH units
  static constexpr unsigned long DEFAULT_STABILITY_DURATION_MS = 3000UL;
  
  // Timing
  static constexpr unsigned long MIN_READ_INTERVAL_MS = 100;
  static constexpr unsigned long DEBUG_PRINT_INTERVAL_MS = 1000;
  
  // EEPROM configuration
  static constexpr uint16_t EEPROM_BASE_ADDR = 0;
  static constexpr uint8_t EEPROM_MAGIC = 0x50;  // 'P' for PH
  
  PHProbe();

  static PHProbe& getInstance();

  PHProbe(const PHProbe&) = delete;
  PHProbe& operator=(const PHProbe&) = delete;
  
  /**
   * Initialize the pH sensor
   * @param adcPin ADC pin connected to pH probe (e.g., GPIO1)
   */
  void begin(uint8_t adcPin);
  
  /**
   * Update function - call in main loop (non-blocking)
   * Takes multiple ADC samples and processes through filter pipeline
   */
  void update();
  
  /**
   * Read raw voltage (in volts) from ADC
   * Takes multiple samples and returns averaged value
   * @return Raw voltage in volts
   */
  float readRawVoltage();
  
  /**
   * Read raw pH value (unfiltered)
   * @return Raw pH value (before filtering)
   */
  float readRawPH();
  
  /**
   * Read filtered pH value (after median + moving average)
   * @return Filtered pH value
   */
  float readFilteredPH();
  
  /**
   * Check if pH reading is stable
   * @param threshold Maximum pH change allowed (e.g., 0.02)
   * @param duration_ms Duration to monitor for stability
   * @return true if stable for the specified duration
   */
  bool isStable(float threshold = DEFAULT_STABILITY_THRESHOLD, 
                unsigned long duration_ms = DEFAULT_STABILITY_DURATION_MS);
  
  /**
   * Check if we have valid readings yet
   * @return true if at least one reading has been processed
   */
  bool isReady() const;
  
  // ===== Calibration Functions =====
  
  /**
   * Set the slope for voltage-to-pH conversion
   * Typical value: -5.6548 (for 25°C)
   * @param slope Slope value (mV/pH or V/pH depending on units)
   */
  void setSlope(float slope);
  
  /**
   * Get current slope
   * @return Current slope value
   */
  float getSlope() const { return m_slope; }
  
  /**
   * Set the offset for voltage-to-pH conversion
   * Typical value: 21.7543
   * @param offset Offset value
   */
  void setOffset(float offset);
  
  /**
   * Get current offset
   * @return Current offset value
   */
  float getOffset() const { return m_offset; }
  
  /**
   * Save calibration to EEPROM
   * Uses wear leveling to extend EEPROM life
   */
  void saveCalibration();
  
  /**
   * Load calibration from EEPROM
   * @return true if valid calibration was loaded
   */
  bool loadCalibration();
  
  /**
   * Reset calibration to factory defaults
   */
  void resetCalibration();
  
// ===== Configuration =====

  void setDebugEnabled(bool enable) { m_debugEnabled = enable; }
  void setStabilityThreshold(float threshold) { m_stabilityThreshold = threshold; }
  void setStabilityDuration(unsigned long duration_ms) { m_stabilityDurationMs = duration_ms; }
  void setMinReadInterval(unsigned long interval_ms) { m_minReadIntervalMs = interval_ms; }

#if USE_MOCK_PH
  void setMockPH(float value) { m_mockPHValue = value; }
  void setMockDelta(float delta) { m_mockDelta = delta; }
  void setMockRefDelta(float delta) { m_mockRefDelta = delta; }
  void setMockTankDelta(float delta) { m_mockTankDelta = delta; }
  void setMockEquilibrium(float eq) { m_mockEquilibrium = eq; }
  void setMockPostAeration(bool post) { m_mockPostAeration = post; }
  void setMockIsTank(bool isTank) { m_mockIsTank = isTank; }
  void setMockAerationStart(unsigned long time) { m_mockAerationStart = time; }
  void enableMock(bool enable) { m_mockEnabled = enable; }
  bool isMockEnabled() const { return m_mockEnabled; }
#endif
  
  // ===== Getters =====
  
  uint8_t getAdcPin() const { return m_adcPin; }
  unsigned long getLastReadTime() const { return m_lastReadTime; }
  unsigned long getSampleCount() const { return m_sampleCount; }
  float getRawVoltage() const { return m_rawVoltage; }
  float getLastRawPH() const { return m_lastRawPH; }
  float getLastFilteredPH() const { return m_lastFilteredPH; }
  
private:
  // ADC configuration
  uint8_t m_adcPin;
  uint8_t m_adcChannel;

  // Calibration parameters
  float m_slope;
  float m_offset;

#if USE_MOCK_PH
  float m_mockPHValue;
  float m_mockDelta;
  float m_mockRefDelta;
  float m_mockTankDelta;
  float m_mockEquilibrium;
  unsigned long m_mockAerationStart;
  bool m_mockEnabled;
  bool m_mockPostAeration;
  bool m_mockIsTank;
#endif
  
  // Filter state
  float m_medianBuffer[MEDIAN_FILTER_SIZE];
  float m_movingAvgBuffer[MOVING_AVG_SIZE];
  uint8_t m_medianIndex;
  uint8_t m_movingAvgIndex;
  uint8_t m_medianFillCount;
  uint8_t m_movingAvgFillCount;
  
  // Latest readings
  float m_rawVoltage;
  float m_lastRawPH;
  float m_lastFilteredPH;
  
  // Timing
  unsigned long m_lastReadTime;
  unsigned long m_minReadIntervalMs;
  
  // Stability tracking
  unsigned long m_lastStableTime;
  bool m_wasStable;
  float m_stabilityThreshold;
  unsigned long m_stabilityDurationMs;
  
  // Debug
  bool m_debugEnabled;
  unsigned long m_lastDebugTime;
  unsigned long m_sampleCount;
  
  // Internal methods
  float voltageToPH(float voltage) const;
  float applyMedianFilter(float value);
  float applyMovingAverage(float value);
  float computeMedian(float* buffer, uint8_t size);
  void sortArray(float* array, uint8_t size);
  
  // EEPROM helpers
  uint8_t computeChecksum(const uint8_t* data, uint8_t length);
  bool validateEEPROMData(const uint8_t* data, uint8_t length, uint8_t checksum);
};

#endif // PH_PROBE_H