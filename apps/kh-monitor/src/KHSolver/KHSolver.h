#ifndef KH_SOLVER_H
#define KH_SOLVER_H

#include <Arduino.h>
#include <EEPROM.h>

enum class KHCalibrationType : uint8_t
{
  LINEAR = 0,
  QUADRATIC = 1,
  RATIO_LINEAR = 2,
  RATIO_QUADRATIC = 3,
  INVALID = 0xFF
};

enum class KHInputType : uint8_t
{
  RATIO = 0,
  DELTA_PH = 1,
  INVALID = 0xFF
};

enum class KHSmoothingMode : uint8_t
{
  NONE = 0,
  SMOOTH_INPUT = 1,
  SMOOTH_OUTPUT = 2
};

struct KHResult
{
  float value;
  bool valid;
  bool fallbackUsed;
  float confidence;

  KHResult() : value(0.0f), valid(false), fallbackUsed(false), confidence(0.0f) {}
};

struct KHCalibrationConfig
{
  uint32_t magic;
  uint8_t version;
  uint8_t calibType;
  uint8_t inputType;
  float a;
  float b;
  float c;
  uint8_t checksum;
};

class KHSolver
{
public:
  static constexpr uint16_t EEPROM_ADDR = 64;
  static constexpr uint16_t EEPROM_SIZE = 64;
  static constexpr uint32_t MAGIC = 0x4B48534F;
  static constexpr uint8_t VERSION = 1;

  static constexpr float EPSILON = 0.001f;
  static constexpr float KH_MIN = 0.0f;
  static constexpr float KH_MAX = 20.0f;
  static constexpr float COEFF_MIN = -1000.0f;
  static constexpr float COEFF_MAX = 1000.0f;
  static constexpr float MIN_DELTA_PH_FOR_CONFIDENCE = 0.05f;

  static constexpr unsigned long EEPROM_WRITE_DEBOUNCE_MS = 2000UL;
  static constexpr unsigned long AUTO_SAVE_DELAY_MS = 5000UL;
  static constexpr unsigned long HYSTERESIS_STABILITY_MS = 1000UL;
  static constexpr uint8_t SMOOTHING_WINDOW = 5;

  KHSolver();

  static KHSolver& getInstance();

  KHSolver(const KHSolver&) = delete;
  KHSolver& operator=(const KHSolver&) = delete;

  void begin();

  void setLinear(float a, float b);
  void setQuadratic(float a, float b, float c);
  void setCalibrationType(KHCalibrationType type);
  void setInputType(KHInputType type);

  KHResult computeFromDeltaPH(float deltaPH);
  KHResult computeFromRatio(float ratio);
  KHResult computeFromPair(float deltaPH_tank, float deltaPH_ref);

  void saveToEEPROM(bool force = false);
  bool loadFromEEPROM();
  void resetToFactory();

  float getA() const { return m_a; }
  float getB() const { return m_b; }
  float getC() const { return m_c; }
  KHCalibrationType getCalibrationType() const { return m_type; }
  KHInputType getInputType() const { return m_inputType; }
  float getLastValidKH() const { return m_lastValidKH; }

  void setVerbose(bool enable) { m_verbose = enable; }
  bool isVerbose() const { return m_verbose; }

  void setSmoothingWindow(uint8_t size);
  void setSmoothingMode(KHSmoothingMode mode);
  void setOutputClamp(float minVal, float maxVal);
  void enableOutputClamp(bool enable);
  void setAutoSaveDelay(unsigned long delayMs);

  void printCalibrationTable(uint8_t points = 10);
  void printCalibrationInfo();

  bool processSerialCommand(const String& cmd);

  bool hasValidData() const { return m_hasValidData; }

private:
  KHCalibrationType m_type;
  KHInputType m_inputType;
  float m_a;
  float m_b;
  float m_c;

  bool m_hasValidData;
  float m_lastValidKH;

  bool m_verbose;
  bool m_outputClampEnabled;
  float m_clampMin;
  float m_clampMax;

  KHSmoothingMode m_smoothingMode;
  uint8_t m_smoothingWindow;
  float m_inputSmoothingBuffer[SMOOTHING_WINDOW];
  uint8_t m_inputSmoothingIndex;
  uint8_t m_inputSmoothingCount;
  float m_outputSmoothingBuffer[SMOOTHING_WINDOW];
  uint8_t m_outputSmoothingIndex;
  uint8_t m_outputSmoothingCount;

  unsigned long m_lastEEPROMWriteTime;
  unsigned long m_lastChangeTime;
  bool m_eepromDirty;
  KHCalibrationConfig m_lastSavedConfig;

  unsigned long m_lastValidTime;
  bool m_wasInvalid;
  unsigned long m_invalidStartTime;

  float computeKHFromSmoothedInput(float x);
  float computeConfidence(float deltaPH_ref, float ratio) const;
  bool checkHysteresis(bool currentValid);

  float applyInputSmoothing(float input);
  float applyOutputSmoothing(float kh);
  float applyClamp(float kh);
  bool validateCoefficients();
  bool isValidInput(float x) const;
  float safeCompute(float x);

  bool hasConfigChanged() const;
  void scheduleAutoSave();

  uint8_t computeChecksum(const uint8_t* data, uint8_t length);
  bool validateConfig(const KHCalibrationConfig& cfg);
  bool migrateConfig(KHCalibrationConfig& cfg);
};

#endif