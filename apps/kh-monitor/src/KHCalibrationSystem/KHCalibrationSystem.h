#ifndef KH_CALIBRATION_SYSTEM_H
#define KH_CALIBRATION_SYSTEM_H

#include <Arduino.h>
#include "RefPumpController/RefPumpController.h"
#include "TankPumpController/TankPumpController.h"
#include "PHProbe/PHProbe.h"
#include "AerationPump/AerationPump.h"

enum class KHState
{
  IDLE,
  FILL_REFERENCE,
  STABILIZE_REFERENCE,
  MEASURE_REFERENCE_INITIAL,
  AERATE_REFERENCE,
  MEASURE_REFERENCE_FINAL,
  DRAIN,
  FLUSH,
  FILL_TANK,
  STABILIZE_TANK,
  MEASURE_TANK_INITIAL,
  AERATE_TANK,
  MEASURE_TANK_FINAL,
  CALCULATE_KH,
  DONE,
  ERROR
};

struct KHConfig
{
  unsigned long fillTimeMs = 5000UL;
  unsigned long stabilizeTimeMs = 3000UL;
  unsigned long aerationTimeMs = 60000UL;
  unsigned long drainTimeMs = 8000UL;
  unsigned long flushTimeMs = 10000UL;
  
  float stabilityThreshold = 0.02f;
  unsigned long stabilityDurationMs = 3000UL;
  
  float khSlopeA = 100.0f;
  float khOffsetB = 0.0f;
  
  bool enableLogging = true;
};

struct KHResult
{
  float refInitialPH;
  float refFinalPH;
  float refDeltaPH;
  float tankInitialPH;
  float tankFinalPH;
  float tankDeltaPH;
  float ratio;
  float khValue;
  bool success;
  unsigned long totalDurationMs;
  KHState finalState;
};

class KHCalibrationSystem
{
public:
  static KHCalibrationSystem& getInstance();
  
  void begin(PHProbe* phProbe, AerationPump* airPump);
  void update();
  
  void start();
  void stop();
  void reset();
  
  KHState getState() const { return m_currentState; }
  bool isRunning() const { return m_isRunning; }
  bool isComplete() const { return m_currentState == KHState::DONE || m_currentState == KHState::ERROR; }
  
  KHResult getResult() const { return m_result; }
  
  void setConfig(const KHConfig& config) { m_config = config; }
  KHConfig getConfig() const { return m_config; }
  
  void setFillTime(unsigned long ms) { m_config.fillTimeMs = ms; }
  void setStabilizeTime(unsigned long ms) { m_config.stabilizeTimeMs = ms; }
  void setAerationTime(unsigned long ms) { m_config.aerationTimeMs = ms; }
  void setDrainTime(unsigned long ms) { m_config.drainTimeMs = ms; }
  void setFlushTime(unsigned long ms) { m_config.flushTimeMs = ms; }
  
  void setKHCoefficients(float slopeA, float offsetB) {
    m_config.khSlopeA = slopeA;
    m_config.khOffsetB = offsetB;
  }
  
  float getCurrentPH() const;
  float getFilteredPH() const;
  const char* getStateName() const;
  
private:
  KHCalibrationSystem();
  KHCalibrationSystem(const KHCalibrationSystem&) = delete;
  KHCalibrationSystem& operator=(const KHCalibrationSystem&) = delete;
  
  void transitionTo(KHState newState);
  bool checkStability();
  void calculateKH();
  void logStateTransition(const char* from, const char* to);
  void logMeasurement(const char* label, float ph);
  
  void enter_IDLE();
  void enter_FILL_REFERENCE();
  bool exit_FILL_REFERENCE();
  void enter_STABILIZE_REFERENCE();
  bool exit_STABILIZE_REFERENCE();
  void enter_MEASURE_REFERENCE_INITIAL();
  bool exit_MEASURE_REFERENCE_INITIAL();
  void enter_AERATE_REFERENCE();
  bool exit_AERATE_REFERENCE();
  void enter_MEASURE_REFERENCE_FINAL();
  bool exit_MEASURE_REFERENCE_FINAL();
  void enter_DRAIN();
  bool exit_DRAIN();
  void enter_FLUSH();
  bool exit_FLUSH();
  void enter_FILL_TANK();
  bool exit_FILL_TANK();
  void enter_STABILIZE_TANK();
  bool exit_STABILIZE_TANK();
  void enter_MEASURE_TANK_INITIAL();
  bool exit_MEASURE_TANK_INITIAL();
  void enter_AERATE_TANK();
  bool exit_AERATE_TANK();
  void enter_MEASURE_TANK_FINAL();
  bool exit_MEASURE_TANK_FINAL();
  void enter_CALCULATE_KH();
  void enter_DONE();
  void enter_ERROR();
  
  PHProbe* m_phProbe;
  AerationPump* m_airPump;
  
  KHState m_currentState;
  KHState m_previousState;
  bool m_isRunning;
  unsigned long m_stateStartTime;
  unsigned long m_totalStartTime;
  
  KHConfig m_config;
  KHResult m_result;
  
  float m_stabilizedPH;
  bool m_stabilityAchieved;
};

#endif