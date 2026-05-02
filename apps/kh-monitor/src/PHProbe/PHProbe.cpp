#include "PHProbe/PHProbe.h"
#include <EEPROM.h>

// EEPROM storage structure (must be packed to avoid alignment issues)
#pragma pack(push, 1)
struct CalibrationData
{
  uint8_t magic;           // Magic byte for validation
  uint8_t version;         // Structure version
  float slope;             // Calibration slope
  float offset;            // Calibration offset
  uint8_t checksum;        // Simple XOR checksum
};
#pragma pack(pop)

// Wear leveling: store in 4 slots and use the one with highest valid count
static constexpr uint8_t NUM_EEPROM_SLOTS = 4;
static constexpr uint8_t CALIBRATION_SIZE = sizeof(CalibrationData);

PHProbe::PHProbe()
  : m_adcPin(0)
  , m_adcChannel(0)
  , m_slope(DEFAULT_SLOPE)
  , m_offset(DEFAULT_OFFSET)
  , m_medianIndex(0)
  , m_movingAvgIndex(0)
  , m_medianFillCount(0)
  , m_movingAvgFillCount(0)
  , m_rawVoltage(0.0f)
  , m_lastRawPH(0.0f)
  , m_lastFilteredPH(0.0f)
  , m_lastReadTime(0)
  , m_minReadIntervalMs(MIN_READ_INTERVAL_MS)
  , m_lastStableTime(0)
  , m_wasStable(false)
  , m_stabilityThreshold(DEFAULT_STABILITY_THRESHOLD)
  , m_stabilityDurationMs(DEFAULT_STABILITY_DURATION_MS)
  , m_debugEnabled(false)
  , m_lastDebugTime(0)
  , m_sampleCount(0)
#if USE_MOCK_PH
  , m_mockPHValue(7.0f)
  , m_mockDelta(0.5f)
  , m_mockEnabled(false)
  , m_mockPostAeration(false)
#endif
  {
  }

PHProbe& PHProbe::getInstance() {
  static PHProbe instance;
  return instance;
}

void PHProbe::begin(uint8_t adcPin)
{
  m_adcPin = adcPin;
  
  // Map GPIO to ADC channel for ESP32
  if (adcPin >= 1 && adcPin <= 10) {
    m_adcChannel = static_cast<uint8_t>(adcPin);
  } else {
    m_adcChannel = 1;  // Default to GPIO1
  }
  
  analogReadResolution(12);
  
  // Initialize filter buffers
  for (uint8_t i = 0; i < MEDIAN_FILTER_SIZE; i++) {
    m_medianBuffer[i] = 0.0f;
  }
  for (uint8_t i = 0; i < MOVING_AVG_SIZE; i++) {
    m_movingAvgBuffer[i] = 0.0f;
  }
  
  // Load calibration from EEPROM
  if (!loadCalibration()) {
    resetCalibration();
  }
  
  m_lastReadTime = millis();
  
  if (m_debugEnabled) {
    Serial.printf("[PH] Initialized on GPIO%d (channel %d)\n", adcPin, m_adcChannel);
    Serial.printf("[PH] Calibration: slope=%.4f, offset=%.4f\n", m_slope, m_offset);
  }
}

void PHProbe::update()
{
  unsigned long now = millis();
  
  // Rate limiting
  if (now - m_lastReadTime < m_minReadIntervalMs) {
    return;
  }
  m_lastReadTime = now;
  
  // Read raw voltage from ADC
  m_rawVoltage = readRawVoltage();
  m_lastRawPH = voltageToPH(m_rawVoltage);
  m_sampleCount++;
  
  // Apply two-stage filter pipeline
  float medianFiltered = applyMedianFilter(m_lastRawPH);
  m_lastFilteredPH = applyMovingAverage(medianFiltered);
  
  // Debug output
  if (m_debugEnabled && (now - m_lastDebugTime >= DEBUG_PRINT_INTERVAL_MS)) {
    m_lastDebugTime = now;
    Serial.printf("[PH] Raw: %.3f pH, Filtered: %.3f pH, Voltage: %.3f V\n",
                 m_lastRawPH, m_lastFilteredPH, m_rawVoltage);
  }
}

float PHProbe::readRawVoltage()
{
#if USE_MOCK_PH
  if (m_mockEnabled) {
    float phValue = m_mockPHValue;
    if (m_mockPostAeration) {
      phValue += m_mockDelta;
    }
    float voltage = phValue / m_slope - m_offset / m_slope;
    return voltage;
  }
#endif

  // Take multiple samples and average for better accuracy
  uint32_t adcSum = 0;
  
  for (uint8_t i = 0; i < ADC_SAMPLES_PER_READ; i++) {
    adcSum += analogRead(m_adcPin);
  }
  
  // Convert to voltage
  // ESP32 ADC: 12-bit (0-4095), reference voltage ~3.3V
  // With 11dB attenuation: 0-3.3V maps to 0-4095
  float voltage = (static_cast<float>(adcSum) / ADC_SAMPLES_PER_READ) * 3.3f / 4095.0f;
  
  return voltage;
}

float PHProbe::readRawPH()
{
  return m_lastRawPH;
}

float PHProbe::readFilteredPH()
{
  return m_lastFilteredPH;
}

bool PHProbe::isStable(float threshold, unsigned long duration_ms)
{
  unsigned long now = millis();
  
  // Need enough samples to determine stability
  if (m_movingAvgFillCount < MOVING_AVG_SIZE) {
    return false;
  }
  
  // Calculate range of recent readings
  float minVal = m_movingAvgBuffer[0];
  float maxVal = m_movingAvgBuffer[0];
  
  for (uint8_t i = 1; i < m_movingAvgFillCount; i++) {
    if (m_movingAvgBuffer[i] < minVal) minVal = m_movingAvgBuffer[i];
    if (m_movingAvgBuffer[i] > maxVal) maxVal = m_movingAvgBuffer[i];
  }
  
  float range = maxVal - minVal;
  bool currentlyStable = (range <= threshold);
  
  // Update stability tracking
  if (currentlyStable) {
    if (!m_wasStable) {
      m_lastStableTime = now;
      m_wasStable = true;
    }
    return (now - m_lastStableTime >= duration_ms);
  } else {
    m_wasStable = false;
    return false;
  }
}

bool PHProbe::isReady() const
{
  return (m_medianFillCount > 0);
}

void PHProbe::setSlope(float slope)
{
  m_slope = slope;
  if (m_debugEnabled) {
    Serial.printf("[PH] Slope set to %.4f\n", slope);
  }
}

void PHProbe::setOffset(float offset)
{
  m_offset = offset;
  if (m_debugEnabled) {
    Serial.printf("[PH] Offset set to %.4f\n", offset);
  }
}

void PHProbe::saveCalibration()
{
  CalibrationData calData;
  calData.magic = EEPROM_MAGIC;
  calData.version = 1;
  calData.slope = m_slope;
  calData.offset = m_offset;
  calData.checksum = computeChecksum(reinterpret_cast<const uint8_t*>(&calData), 
                                     offsetof(CalibrationData, checksum));
  
  // Find the slot with lowest write count (simple wear leveling)
  uint16_t baseAddr = EEPROM_BASE_ADDR;
  uint8_t bestSlot = 0;
  uint8_t minWrites = 0xFF;
  
  for (uint8_t slot = 0; slot < NUM_EEPROM_SLOTS; slot++) {
    uint16_t slotAddr = baseAddr + (slot * CALIBRATION_SIZE);
    CalibrationData existing;
    EEPROM.get(slotAddr, existing);
    
    // Simple wear tracking: use slot with lowest magic value as proxy
    // (lower = older write = more worn)
    if (existing.magic < minWrites) {
      minWrites = existing.magic;
      bestSlot = slot;
    }
  }
  
  // Write to best slot
  uint16_t writeAddr = baseAddr + (bestSlot * CALIBRATION_SIZE);
  EEPROM.put(writeAddr, calData);
  EEPROM.commit();
  
  if (m_debugEnabled) {
    Serial.printf("[PH] Calibration saved to slot %d\n", bestSlot);
  }
}

bool PHProbe::loadCalibration()
{
  uint16_t baseAddr = EEPROM_BASE_ADDR;
  
  // Scan all slots and find valid one with highest magic (most recent)
  CalibrationData bestData;
  bool foundValid = false;
  uint8_t bestMagic = 0;
  
  for (uint8_t slot = 0; slot < NUM_EEPROM_SLOTS; slot++) {
    uint16_t slotAddr = baseAddr + (slot * CALIBRATION_SIZE);
    CalibrationData data;
    EEPROM.get(slotAddr, data);
    
    if (data.magic == EEPROM_MAGIC && 
        validateEEPROMData(reinterpret_cast<const uint8_t*>(&data), 
                          offsetof(CalibrationData, checksum), data.checksum)) {
      if (data.magic > bestMagic) {
        bestData = data;
        bestMagic = data.magic;
        foundValid = true;
      }
    }
  }
  
  if (foundValid) {
    m_slope = bestData.slope;
    m_offset = bestData.offset;
    
    if (m_debugEnabled) {
      Serial.printf("[PH] Calibration loaded: slope=%.4f, offset=%.4f\n", 
                    m_slope, m_offset);
    }
    return true;
  }
  
  return false;
}

void PHProbe::resetCalibration()
{
  m_slope = DEFAULT_SLOPE;
  m_offset = DEFAULT_OFFSET;
  
  if (m_debugEnabled) {
    Serial.printf("[PH] Calibration reset to defaults\n");
  }
}

float PHProbe::voltageToPH(float voltage) const
{
  // Standard pH electrode formula: pH = slope * voltage + offset
  // Typical slope: -5.6548 (mV/pH at 25°C) -> -0.0056548 (V/pH)
  // Typical offset: ~21.7543 (based on standard electrode potential)
  return (m_slope * voltage) + m_offset;
}

float PHProbe::applyMedianFilter(float value)
{
  m_medianBuffer[m_medianIndex] = value;
  m_medianIndex = (m_medianIndex + 1) % MEDIAN_FILTER_SIZE;
  
  if (m_medianFillCount < MEDIAN_FILTER_SIZE) {
    m_medianFillCount++;
  }
  
  return computeMedian(m_medianBuffer, m_medianFillCount);
}

float PHProbe::applyMovingAverage(float value)
{
  m_movingAvgBuffer[m_movingAvgIndex] = value;
  m_movingAvgIndex = (m_movingAvgIndex + 1) % MOVING_AVG_SIZE;
  
  if (m_movingAvgFillCount < MOVING_AVG_SIZE) {
    m_movingAvgFillCount++;
  }
  
  // Calculate running average
  float sum = 0.0f;
  for (uint8_t i = 0; i < m_movingAvgFillCount; i++) {
    sum += m_movingAvgBuffer[i];
  }
  return sum / m_movingAvgFillCount;
}

float PHProbe::computeMedian(float* buffer, uint8_t size)
{
  if (size == 0) return 0.0f;
  if (size == 1) return buffer[0];
  
  // Copy to temporary buffer (don't modify original)
  float temp[MEDIAN_FILTER_SIZE];
  for (uint8_t i = 0; i < size; i++) {
    temp[i] = buffer[i];
  }
  
  sortArray(temp, size);
  
  // Return middle element
  return temp[size / 2];
}

void PHProbe::sortArray(float* array, uint8_t size)
{
  // Simple bubble sort - adequate for small arrays
  for (uint8_t i = 0; i < size - 1; i++) {
    for (uint8_t j = 0; j < size - 1 - i; j++) {
      if (array[j] > array[j + 1]) {
        float temp = array[j];
        array[j] = array[j + 1];
        array[j + 1] = temp;
      }
    }
  }
}

uint8_t PHProbe::computeChecksum(const uint8_t* data, uint8_t length)
{
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

bool PHProbe::validateEEPROMData(const uint8_t* data, uint8_t length, uint8_t checksum)
{
  return (computeChecksum(data, length) == checksum);
}