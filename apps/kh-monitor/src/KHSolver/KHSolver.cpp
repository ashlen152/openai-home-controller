#include "KHSolver/KHSolver.h"

#include <string.h>

KHSolver::KHSolver()
  : m_type(KHCalibrationType::RATIO_LINEAR)
  , m_inputType(KHInputType::RATIO)
  , m_a(100.0f)
  , m_b(0.0f)
  , m_c(0.0f)
  , m_hasValidData(false)
  , m_lastValidKH(0.0f)
  , m_verbose(false)
  , m_outputClampEnabled(true)
  , m_clampMin(KH_MIN)
  , m_clampMax(KH_MAX)
  , m_smoothingMode(KHSmoothingMode::SMOOTH_INPUT)
  , m_smoothingWindow(SMOOTHING_WINDOW)
  , m_inputSmoothingIndex(0)
  , m_inputSmoothingCount(0)
  , m_outputSmoothingIndex(0)
  , m_outputSmoothingCount(0)
  , m_lastEEPROMWriteTime(0)
  , m_lastChangeTime(0)
  , m_eepromDirty(false)
  , m_lastValidTime(0)
  , m_wasInvalid(false)
  , m_invalidStartTime(0)
{
  for (uint8_t i = 0; i < SMOOTHING_WINDOW; i++) {
    m_inputSmoothingBuffer[i] = 0.0f;
    m_outputSmoothingBuffer[i] = 0.0f;
  }

  memset(&m_lastSavedConfig, 0, sizeof(m_lastSavedConfig));
}

KHSolver& KHSolver::getInstance() {
  static KHSolver instance;
  return instance;
}

void KHSolver::begin()
{
  EEPROM.begin(EEPROM_SIZE);

  if (!loadFromEEPROM()) {
    resetToFactory();
  }

  if (m_verbose) {
    Serial.println("[KHSolver] Initialized (production mode v2)");
    printCalibrationInfo();
  }
}

void KHSolver::setLinear(float a, float b)
{
  if (isnan(a) || isnan(b) || isinf(a) || isinf(b)) {
    if (m_verbose) Serial.println("[KHSolver] ERROR: Invalid coefficients for linear");
    return;
  }

  m_a = a;
  m_b = b;
  m_c = 0.0f;
  m_type = KHCalibrationType::LINEAR;
  m_hasValidData = validateCoefficients();

  if (m_hasValidData) {
    m_eepromDirty = true;
    m_lastChangeTime = millis();
    scheduleAutoSave();
  }

  if (m_verbose) {
    Serial.printf("[KHSolver] Linear set: a=%.4f, b=%.4f\n", m_a, m_b);
  }
}

void KHSolver::setQuadratic(float a, float b, float c)
{
  if (isnan(a) || isnan(b) || isnan(c) || isinf(a) || isinf(b) || isinf(c)) {
    if (m_verbose) Serial.println("[KHSolver] ERROR: Invalid coefficients for quadratic");
    return;
  }

  m_a = a;
  m_b = b;
  m_c = c;
  m_type = KHCalibrationType::QUADRATIC;
  m_hasValidData = validateCoefficients();

  if (m_hasValidData) {
    m_eepromDirty = true;
    m_lastChangeTime = millis();
    scheduleAutoSave();
  }

  if (m_verbose) {
    Serial.printf("[KHSolver] Quadratic set: a=%.4f, b=%.4f, c=%.4f\n", m_a, m_b, m_c);
  }
}

void KHSolver::setCalibrationType(KHCalibrationType type)
{
  m_type = type;
  m_eepromDirty = true;
  m_lastChangeTime = millis();
}

void KHSolver::setInputType(KHInputType type)
{
  m_inputType = type;
  m_eepromDirty = true;
  m_lastChangeTime = millis();

  if (m_verbose) {
    Serial.printf("[KHSolver] Input type set to: %s\n",
                  type == KHInputType::RATIO ? "RATIO" : "DELTA_PH");
  }
}

KHResult KHSolver::computeFromDeltaPH(float deltaPH)
{
  KHResult result;

  if (!isValidInput(deltaPH)) {
    result.valid = false;
    result.fallbackUsed = true;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    return result;
  }

  if (!checkHysteresis(true)) {
    result.valid = false;
    result.fallbackUsed = true;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    if (m_verbose) Serial.println("[KHSolver] Hysteresis: still stabilizing");
    return result;
  }

  float input = deltaPH;
  if (m_smoothingMode == KHSmoothingMode::SMOOTH_INPUT) {
    input = applyInputSmoothing(deltaPH);
  }

  float kh = safeCompute(input);

  if (m_smoothingMode == KHSmoothingMode::SMOOTH_OUTPUT) {
    kh = applyOutputSmoothing(kh);
  }

  kh = applyClamp(kh);

  if (!isValidInput(kh)) {
    result.valid = false;
    result.fallbackUsed = true;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    return result;
  }

  result.valid = true;
  result.fallbackUsed = false;
  result.value = kh;
  result.confidence = computeConfidence(deltaPH, 0.0f);

  m_lastValidKH = kh;
  m_lastValidTime = millis();
  m_wasInvalid = false;

  return result;
}

KHResult KHSolver::computeFromRatio(float ratio)
{
  KHResult result;

  if (!isValidInput(ratio)) {
    result.valid = false;
    result.fallbackUsed = true;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    if (m_verbose) Serial.println("[KHSolver] WARNING: Invalid ratio");
    return result;
  }

  if (!checkHysteresis(true)) {
    result.valid = false;
    result.fallbackUsed = true;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    if (m_verbose) Serial.println("[KHSolver] Hysteresis: still stabilizing");
    return result;
  }

  if (ratio < 0.0f) {
    if (m_verbose) Serial.println("[KHSolver] WARNING: Negative ratio, clamping to 0");
    ratio = 0.0f;
  }

  float input = ratio;
  if (m_smoothingMode == KHSmoothingMode::SMOOTH_INPUT) {
    input = applyInputSmoothing(ratio);
  }

  float kh = safeCompute(input);

  if (m_smoothingMode == KHSmoothingMode::SMOOTH_OUTPUT) {
    kh = applyOutputSmoothing(kh);
  }

  kh = applyClamp(kh);

  if (!isValidInput(kh)) {
    result.valid = false;
    result.fallbackUsed = true;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    return result;
  }

  result.valid = true;
  result.fallbackUsed = false;
  result.value = kh;
  result.confidence = computeConfidence(0.0f, ratio);

  m_lastValidKH = kh;
  m_lastValidTime = millis();
  m_wasInvalid = false;

  return result;
}

KHResult KHSolver::computeFromPair(float deltaPH_tank, float deltaPH_ref)
{
  KHResult result;

  if (!isValidInput(deltaPH_tank) || !isValidInput(deltaPH_ref)) {
    result.valid = false;
    result.fallbackUsed = true;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    if (m_verbose) Serial.println("[KHSolver] Invalid input in pair");
    return result;
  }

  if (deltaPH_ref < EPSILON) {
    result.valid = false;
    result.fallbackUsed = false;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    if (m_verbose) {
      Serial.printf("[KHSolver] ERROR: deltaPH_ref too small (%.4f < %.4f)\n",
                    deltaPH_ref, EPSILON);
    }

    if (!m_wasInvalid) {
      m_wasInvalid = true;
      m_invalidStartTime = millis();
    }

    return result;
  }

  if (!checkHysteresis(true)) {
    result.valid = false;
    result.fallbackUsed = true;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    if (m_verbose) Serial.println("[KHSolver] Hysteresis: still stabilizing");
    return result;
  }

  float ratio = deltaPH_tank / deltaPH_ref;

  if (!isValidInput(ratio) || ratio < 0.0f) {
    result.valid = false;
    result.fallbackUsed = true;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    if (m_verbose) Serial.println("[KHSolver] Invalid ratio computed");
    return result;
  }

  float input = ratio;
  if (m_smoothingMode == KHSmoothingMode::SMOOTH_INPUT) {
    input = applyInputSmoothing(ratio);
  }

  float kh = safeCompute(input);

  if (m_smoothingMode == KHSmoothingMode::SMOOTH_OUTPUT) {
    kh = applyOutputSmoothing(kh);
  }

  kh = applyClamp(kh);

  if (!isValidInput(kh)) {
    result.valid = false;
    result.fallbackUsed = true;
    result.value = m_lastValidKH;
    result.confidence = 0.0f;
    return result;
  }

  result.valid = true;
  result.fallbackUsed = false;
  result.value = kh;
  result.confidence = computeConfidence(deltaPH_ref, ratio);

  m_lastValidKH = kh;
  m_lastValidTime = millis();
  m_wasInvalid = false;

  return result;
}

bool KHSolver::checkHysteresis(bool currentValid)
{
  unsigned long now = millis();

  if (currentValid) {
    if (m_wasInvalid) {
      if (now - m_invalidStartTime < HYSTERESIS_STABILITY_MS) {
        return false;
      }
    }
    return true;
  } else {
    if (!m_wasInvalid) {
      m_wasInvalid = true;
      m_invalidStartTime = now;
    }
    return false;
  }
}

float KHSolver::computeConfidence(float deltaPH_ref, float ratio) const
{
  float confidence = 0.5f;

  if (deltaPH_ref > MIN_DELTA_PH_FOR_CONFIDENCE) {
    float deltaConf = min(1.0f, deltaPH_ref / 0.5f);
    confidence = 0.3f + (deltaConf * 0.7f);
  } else if (ratio > 0.0f) {
    float ratioConf = min(1.0f, ratio / 1.0f);
    confidence = 0.3f + (ratioConf * 0.7f);
  }

  return confidence;
}

void KHSolver::saveToEEPROM(bool force)
{
  if (!m_hasValidData) {
    if (m_verbose) Serial.println("[KHSolver] Cannot save: invalid calibration data");
    return;
  }

  unsigned long now = millis();

  if (!force && !hasConfigChanged()) {
    m_eepromDirty = false;
    return;
  }

  if (!force && !m_eepromDirty) {
    return;
  }

  if (!force && (now - m_lastEEPROMWriteTime < EEPROM_WRITE_DEBOUNCE_MS)) {
    if (m_verbose) {
      Serial.println("[KHSolver] Skipping save: debounce active");
    }
    return;
  }

  KHCalibrationConfig cfg;
  cfg.magic = MAGIC;
  cfg.version = VERSION;
  cfg.calibType = static_cast<uint8_t>(m_type);
  cfg.inputType = static_cast<uint8_t>(m_inputType);
  cfg.a = m_a;
  cfg.b = m_b;
  cfg.c = m_c;

  uint8_t* data = reinterpret_cast<uint8_t*>(&cfg);
  cfg.checksum = 0;
  for (uint8_t i = 0; i < sizeof(KHCalibrationConfig) - 1; i++) {
    cfg.checksum ^= data[i];
  }

  EEPROM.put(EEPROM_ADDR, cfg);
  EEPROM.commit();

  m_lastEEPROMWriteTime = now;
  m_eepromDirty = false;
  memcpy(&m_lastSavedConfig, &cfg, sizeof(KHCalibrationConfig));

  if (m_verbose) Serial.println("[KHSolver] Saved to EEPROM");
}

bool KHSolver::loadFromEEPROM()
{
  KHCalibrationConfig cfg;
  EEPROM.get(EEPROM_ADDR, cfg);

  if (!validateConfig(cfg)) {
    if (m_verbose) Serial.println("[KHSolver] EEPROM validation failed");
    return false;
  }

  uint8_t oldVersion = cfg.version;
  migrateConfig(cfg);

  m_type = static_cast<KHCalibrationType>(cfg.calibType);
  m_inputType = static_cast<KHInputType>(cfg.inputType);
  m_a = cfg.a;
  m_b = cfg.b;
  m_c = cfg.c;

  m_hasValidData = validateCoefficients();

  memcpy(&m_lastSavedConfig, &cfg, sizeof(KHCalibrationConfig));

  if (m_verbose) {
    if (oldVersion < VERSION) {
      Serial.printf("[KHSolver] Loaded from EEPROM (migrated v%d->v%d)\n", oldVersion, VERSION);
    } else {
      Serial.println("[KHSolver] Loaded from EEPROM");
    }
  }

  return m_hasValidData;
}

void KHSolver::resetToFactory()
{
  m_type = KHCalibrationType::RATIO_LINEAR;
  m_inputType = KHInputType::RATIO;
  m_a = 100.0f;
  m_b = 0.0f;
  m_c = 0.0f;
  m_hasValidData = true;
  m_eepromDirty = true;
  m_lastChangeTime = millis();

  if (m_verbose) Serial.println("[KHSolver] Factory reset applied");
}

void KHSolver::setSmoothingWindow(uint8_t size)
{
  if (size == 0 || size > SMOOTHING_WINDOW) {
    if (m_verbose) Serial.println("[KHSolver] WARNING: Invalid smoothing window, using default");
    size = SMOOTHING_WINDOW;
  }

  m_smoothingWindow = size;
  m_inputSmoothingCount = 0;
  m_outputSmoothingCount = 0;

  for (uint8_t i = 0; i < SMOOTHING_WINDOW; i++) {
    m_inputSmoothingBuffer[i] = 0.0f;
    m_outputSmoothingBuffer[i] = 0.0f;
  }
}

void KHSolver::setSmoothingMode(KHSmoothingMode mode)
{
  m_smoothingMode = mode;
  m_eepromDirty = true;
  m_lastChangeTime = millis();

  if (m_verbose) {
    const char* modeStr = (mode == KHSmoothingMode::SMOOTH_INPUT) ? "INPUT" :
                          (mode == KHSmoothingMode::SMOOTH_OUTPUT) ? "OUTPUT" : "NONE";
    Serial.printf("[KHSolver] Smoothing mode: %s\n", modeStr);
  }
}

void KHSolver::setOutputClamp(float minVal, float maxVal)
{
  if (minVal >= maxVal) {
    if (m_verbose) Serial.println("[KHSolver] WARNING: Invalid clamp range");
    return;
  }

  m_clampMin = minVal;
  m_clampMax = maxVal;
}

void KHSolver::enableOutputClamp(bool enable)
{
  m_outputClampEnabled = enable;
}

void KHSolver::setAutoSaveDelay(unsigned long delayMs)
{
  (void)delayMs;
}

void KHSolver::scheduleAutoSave()
{
}

void KHSolver::printCalibrationTable(uint8_t points)
{
  Serial.println("");
  Serial.println("========== KH Calibration Table ==========");

  const char* inputStr = (m_inputType == KHInputType::RATIO) ? "RATIO" : "DELTA_PH";
  Serial.printf("Input: %s | Type: %s\n", inputStr,
                (m_type == KHCalibrationType::LINEAR || m_type == KHCalibrationType::RATIO_LINEAR)
                  ? "LINEAR" : "QUADRATIC");

  Serial.println("+---------+------------+----------+");
  Serial.println("| Input   | KH (dKH)   | Confident|");
  Serial.println("+---------+------------+----------+");

  float maxInput = (m_inputType == KHInputType::RATIO) ? 2.0f : 1.0f;

  for (uint8_t i = 0; i <= points; i++) {
    float input = (float)i / points * maxInput;
    KHResult r = (m_inputType == KHInputType::RATIO) ?
                 computeFromRatio(input) : computeFromDeltaPH(input);
    Serial.printf("| %7.3f | %10.2f | %8.2f |\n", input, r.value, r.confidence);
  }

  Serial.println("+---------+------------+----------+");
  Serial.println("===========================================");
  Serial.println("");
}

void KHSolver::printCalibrationInfo()
{
  Serial.println("");
  Serial.println("========== KH Calibration Info ==========");

  const char* typeStr = "UNKNOWN";
  switch (m_type) {
    case KHCalibrationType::LINEAR: typeStr = "LINEAR (ΔpH)"; break;
    case KHCalibrationType::QUADRATIC: typeStr = "QUADRATIC (ΔpH)"; break;
    case KHCalibrationType::RATIO_LINEAR: typeStr = "LINEAR (RATIO)"; break;
    case KHCalibrationType::RATIO_QUADRATIC: typeStr = "QUADRATIC (RATIO)"; break;
    default: typeStr = "INVALID"; break;
  }

  Serial.printf("Type: %s\n", typeStr);
  Serial.printf("Input: %s\n", m_inputType == KHInputType::RATIO ? "RATIO" : "DELTA_PH");
  Serial.printf("Valid: %s\n", m_hasValidData ? "YES" : "NO");

  const char* smoothStr = "NONE";
  switch (m_smoothingMode) {
    case KHSmoothingMode::SMOOTH_INPUT: smoothStr = "INPUT"; break;
    case KHSmoothingMode::SMOOTH_OUTPUT: smoothStr = "OUTPUT"; break;
    default: smoothStr = "NONE"; break;
  }
  Serial.printf("Smoothing: %s (%d samples)\n", smoothStr, m_smoothingWindow);

  bool isQuad = (m_type == KHCalibrationType::QUADRATIC || m_type == KHCalibrationType::RATIO_QUADRATIC);

  if (isQuad) {
    Serial.printf("Formula: KH = %.4f * x² + %.4f * x + %.4f\n", m_a, m_b, m_c);
  } else {
    Serial.printf("Formula: KH = %.4f * x + %.4f\n", m_a, m_b);
  }

  Serial.printf("Clamp: %s (%.1f - %.1f)\n",
                m_outputClampEnabled ? "ON" : "OFF", m_clampMin, m_clampMax);

  Serial.println("========================================");
  Serial.println("");
}

bool KHSolver::processSerialCommand(const String& cmd)
{
  if (!cmd.startsWith("kh")) return false;

  String rest = cmd.substring(2);
  rest.trim();

  if (rest.startsWith("calib")) {
    rest = rest.substring(5);
    rest.trim();
  }

  if (rest.startsWith("set")) {
    rest = rest.substring(3);
    rest.trim();
  }

  if (rest.length() == 0) {
    printCalibrationInfo();
    return true;
  }

  if (rest.startsWith("info")) {
    printCalibrationInfo();
    return true;
  }

  if (rest.startsWith("table")) {
    if (rest.length() > 5) {
      uint8_t pts = rest.substring(6).toInt();
      if (pts > 0 && pts <= 50) {
        printCalibrationTable(pts);
        return true;
      }
    }
    printCalibrationTable();
    return true;
  }

  if (rest.startsWith("save")) {
    saveToEEPROM(true);
    Serial.println("[KHSolver] Saved (force)");
    return true;
  }

  if (rest.startsWith("load")) {
    if (loadFromEEPROM()) {
      Serial.println("[KHSolver] Loaded from EEPROM");
      printCalibrationInfo();
    } else {
      Serial.println("[KHSolver] Load failed");
    }
    return true;
  }

  if (rest.startsWith("factory") || rest.startsWith("reset")) {
    resetToFactory();
    saveToEEPROM(true);
    printCalibrationInfo();
    return true;
  }

  if (rest.startsWith("verbose")) {
    m_verbose = !m_verbose;
    Serial.printf("[KHSolver] Verbose: %s\n", m_verbose ? "ON" : "OFF");
    return true;
  }

  if (rest.startsWith("ratio")) {
    setInputType(KHInputType::RATIO);
    saveToEEPROM();
    Serial.println("[KHSolver] Input: RATIO");
    return true;
  }

  if (rest.startsWith("dph") || rest.startsWith("delta")) {
    setInputType(KHInputType::DELTA_PH);
    saveToEEPROM();
    Serial.println("[KHSolver] Input: DELTA_PH");
    return true;
  }

  if (rest.startsWith("smooth")) {
    rest = rest.substring(6);
    rest.trim();

    if (rest.startsWith("input") || rest.startsWith("in")) {
      setSmoothingMode(KHSmoothingMode::SMOOTH_INPUT);
      saveToEEPROM();
      Serial.println("[KHSolver] Smoothing: INPUT");
      return true;
    }

    if (rest.startsWith("output") || rest.startsWith("out")) {
      setSmoothingMode(KHSmoothingMode::SMOOTH_OUTPUT);
      saveToEEPROM();
      Serial.println("[KHSolver] Smoothing: OUTPUT");
      return true;
    }

    if (rest.startsWith("off") || rest.startsWith("none")) {
      setSmoothingMode(KHSmoothingMode::NONE);
      saveToEEPROM();
      Serial.println("[KHSolver] Smoothing: OFF");
      return true;
    }

    uint8_t size = rest.toInt();
    if (size > 0 && size <= SMOOTHING_WINDOW) {
      setSmoothingWindow(size);
      Serial.printf("[KHSolver] Smoothing window: %d\n", size);
      return true;
    }

    Serial.printf("[KHSolver] Smoothing: %s, window: %d\n",
                 (m_smoothingMode == KHSmoothingMode::SMOOTH_INPUT) ? "INPUT" :
                 (m_smoothingMode == KHSmoothingMode::SMOOTH_OUTPUT) ? "OUTPUT" : "OFF",
                 m_smoothingWindow);
    return true;
  }

  if (rest.startsWith("clamp")) {
    rest = rest.substring(5);
    rest.trim();

    if (rest.length() > 0) {
      int colonPos = rest.indexOf(':');
      if (colonPos > 0) {
        float minVal = rest.substring(0, colonPos).toFloat();
        float maxVal = rest.substring(colonPos + 1).toFloat();
        if (minVal < maxVal) {
          setOutputClamp(minVal, maxVal);
          Serial.printf("[KHSolver] Clamp set: %.1f - %.1f\n", minVal, maxVal);
          return true;
        }
      }
    }

    m_outputClampEnabled = !m_outputClampEnabled;
    Serial.printf("[KHSolver] Clamp: %s\n", m_outputClampEnabled ? "ON" : "OFF");
    return true;
  }

  if (rest.startsWith("linear") || rest.startsWith("quad")) {
    bool isQuad = rest.startsWith("quad");
    rest = rest.substring(isQuad ? 4 : 5);
    rest.trim();

    float a = 0, b = 0, c = 0;
    bool aSet = false, bSet = false, cSet = false;

    char buffer[64];
    rest.toCharArray(buffer, sizeof(buffer));

    char* token = strtok(buffer, " ,");
    while (token != NULL) {
      char* eqPos = strchr(token, '=');
      if (eqPos != NULL) {
        *eqPos = '\0';
        char key = token[0];
        char* valStr = eqPos + 1;
        float val = atof(valStr);

        switch (key) {
          case 'a': a = val; aSet = true; break;
          case 'b': b = val; bSet = true; break;
          case 'c': c = val; cSet = true; break;
        }
      }
      token = strtok(NULL, " ,");
    }

    if (isQuad) {
      if (aSet || bSet || cSet) {
        setQuadratic(a, b, c);
        saveToEEPROM();
        printCalibrationInfo();
        return true;
      }
    } else {
      if (aSet || bSet) {
        setLinear(a, b);
        saveToEEPROM();
        printCalibrationInfo();
        return true;
      }
    }
  }

  Serial.println("[KHSolver] Commands:");
  Serial.println("  kh set linear a=100 b=0");
  Serial.println("  kh set quad a=-5 b=50 c=0");
  Serial.println("  kh set ratio / kh set dph");
  Serial.println("  kh set smooth input / kh set smooth output / kh set smooth off");
  Serial.println("  kh set smooth 3");
  Serial.println("  kh set clamp 0:20 / kh set clamp");
  Serial.println("  kh save / kh load / kh factory");
  Serial.println("  kh info / kh table");
  Serial.println("  kh verbose");

  return true;
}

float KHSolver::applyInputSmoothing(float input)
{
  if (m_smoothingWindow == 0) return input;

  m_inputSmoothingBuffer[m_inputSmoothingIndex] = input;
  m_inputSmoothingIndex = (m_inputSmoothingIndex + 1) % m_smoothingWindow;

  if (m_inputSmoothingCount < m_smoothingWindow) {
    m_inputSmoothingCount++;
  }

  float sum = 0.0f;
  for (uint8_t i = 0; i < m_inputSmoothingCount; i++) {
    sum += m_inputSmoothingBuffer[i];
  }

  return sum / m_inputSmoothingCount;
}

float KHSolver::applyOutputSmoothing(float kh)
{
  if (m_smoothingWindow == 0) return kh;

  m_outputSmoothingBuffer[m_outputSmoothingIndex] = kh;
  m_outputSmoothingIndex = (m_outputSmoothingIndex + 1) % m_smoothingWindow;

  if (m_outputSmoothingCount < m_smoothingWindow) {
    m_outputSmoothingCount++;
  }

  float sum = 0.0f;
  for (uint8_t i = 0; i < m_outputSmoothingCount; i++) {
    sum += m_outputSmoothingBuffer[i];
  }

  return sum / m_outputSmoothingCount;
}

float KHSolver::applyClamp(float kh)
{
  if (!m_outputClampEnabled) return kh;

  if (kh < m_clampMin) return m_clampMin;
  if (kh > m_clampMax) return m_clampMax;

  return kh;
}

bool KHSolver::validateCoefficients()
{
  if (isnan(m_a) || isnan(m_b) || isnan(m_c)) return false;
  if (isinf(m_a) || isinf(m_b) || isinf(m_c)) return false;

  if (m_a < COEFF_MIN || m_a > COEFF_MAX) return false;
  if (m_b < COEFF_MIN || m_b > COEFF_MAX) return false;
  if (m_c < COEFF_MIN || m_c > COEFF_MAX) return false;

  return true;
}

bool KHSolver::isValidInput(float x) const
{
  return !isnan(x) && !isinf(x);
}

float KHSolver::safeCompute(float x)
{
  bool isQuad = (m_type == KHCalibrationType::QUADRATIC ||
                 m_type == KHCalibrationType::RATIO_QUADRATIC);

  if (isQuad) {
    return m_a * x * x + m_b * x + m_c;
  } else {
    return m_a * x + m_b;
  }
}

bool KHSolver::hasConfigChanged() const
{
  KHCalibrationConfig current;
  current.magic = MAGIC;
  current.version = VERSION;
  current.calibType = static_cast<uint8_t>(m_type);
  current.inputType = static_cast<uint8_t>(m_inputType);
  current.a = m_a;
  current.b = m_b;
  current.c = m_c;
  current.checksum = 0;

  return memcmp(&current, &m_lastSavedConfig, sizeof(KHCalibrationConfig)) != 0;
}

uint8_t KHSolver::computeChecksum(const uint8_t* data, uint8_t length)
{
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

bool KHSolver::validateConfig(const KHCalibrationConfig& cfg)
{
  if (cfg.magic != MAGIC) return false;
  if (cfg.version == 0 || cfg.version > VERSION) return false;

  uint8_t checksum = cfg.checksum;
  KHCalibrationConfig temp = cfg;
  temp.checksum = 0;

  uint8_t computed = 0;
  const uint8_t* data = reinterpret_cast<const uint8_t*>(&temp);
  for (uint8_t i = 0; i < sizeof(KHCalibrationConfig) - 1; i++) {
    computed ^= data[i];
  }

  return (computed == checksum);
}

bool KHSolver::migrateConfig(KHCalibrationConfig& cfg)
{
  bool migrated = false;

  if (cfg.version < VERSION) {
    if (cfg.version == 1) {
      cfg.calibType = static_cast<uint8_t>(KHCalibrationType::RATIO_LINEAR);
    }

    cfg.version = VERSION;

    uint8_t* data = reinterpret_cast<uint8_t*>(&cfg);
    cfg.checksum = 0;
    for (uint8_t i = 0; i < sizeof(KHCalibrationConfig) - 1; i++) {
      cfg.checksum ^= data[i];
    }

    migrated = true;
  }

  return migrated;
}