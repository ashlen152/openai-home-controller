#ifndef KH_STATE_MACHINE_H
#define KH_STATE_MACHINE_H

#include <Arduino.h>
#include "RefPumpController/RefPumpController.h"
#include "TankPumpController/TankPumpController.h"
#include "AerationPump/AerationPump.h"
#include "PHProbe/PHProbe.h"
#include "KHSolver/KHSolver.h"

enum class KHPump
{
  NONE,
  REFERENCE,
  DRAIN,
  FLUSH,
  TANK
};

enum class KHState
{
  IDLE,
  PRE_FLUSH,
  FILL_REFERENCE,
  FLOW_SETTLE,
  STABILIZE_REFERENCE,
  MEASURE_REFERENCE_INITIAL,
  AERATE_REFERENCE,
  WAIT_AFTER_AERATION_REF,
  MEASURE_REFERENCE_FINAL,
  PARTIAL_DRAIN,
  FLUSH_CHAMBER,
  FILL_TANK,
  FLOW_SETTLE_TANK,
  STABILIZE_TANK,
  MEASURE_TANK_INITIAL,
  AERATE_TANK,
  WAIT_AFTER_AERATION_TANK,
  MEASURE_TANK_FINAL,
  CALCULATE_KH,
  DOSE,
  FINALIZE_CHAMBER,
  ERROR,
  // Calibration mode states
  CALIB_IDLE,
  CALIB_MEASURE,
  CALIB_STORE,
  CALIB_DONE
};

struct FluidSystemConfig
{
  float tubeVolumeMl = 2.0f;
  float pumpHeadVolumeMl = 1.5f;
  float chamberVolumeMl = 3.0f;
  float referenceVolumeMl = 20.0f;
  float tankVolumeMl = 20.0f;
  float doseVolumeMl = 5.0f;
  float flushMultiplier = 2.5f;

  float getDeadVolumeMl() const {
    return tubeVolumeMl + pumpHeadVolumeMl + chamberVolumeMl;
  }
  
  float getFlushVolumeMl() const {
    return getDeadVolumeMl() * flushMultiplier;
  }
};

struct KHStateConfig
{
  unsigned long fillTimeMs = 5000UL;
  unsigned long stabilizeTimeMs = 3000UL;
  unsigned long aerationTimeMs = 900000UL;
  unsigned long waitAfterAerationMs = 5000UL;
  unsigned long doseTimeMs = 3000UL;
  unsigned long cleanTubeTimeMs = 5000UL;
  
  float referenceVolumeMl = 20.0f;
  float tankVolumeMl = 20.0f;
  float doseVolumeMl = 5.0f;
  float stepsPerMl = 100.0f;
};

struct KHStateInfo
{
  KHState state;
  unsigned long entryTime;
  unsigned long totalRuntime;
  bool hasError;
  String errorMessage;
};

struct CalibrationPoint
{
  float x;
  float kh;
  bool valid;
};

static constexpr uint8_t MAX_CALIB_POINTS = 8;

class KHStateMachine
{
public:
  static KHStateMachine& getInstance();

  void begin();
  void begin(RefPumpController* refPump, TankPumpController* tankPump, AerationPump* aerationPump);
  void begin(RefPumpController* refPump, TankPumpController* tankPump, AerationPump* aerationPump,
            PHProbe* phProbe, KHSolver* khSolver);
  void update();

  void start();
  void stop();
  void reset();
  void setConfig(const KHStateConfig& config);
  void setVerbose(bool enable);

  // Calibration interface
  bool calibStart(float knownKH);
  bool calibAdd(float knownKH);
  void calibFinish();
  void calibClear();
  void calibList();
  uint8_t getCalibPointCount() const { return m_calibPointCount; }
  bool isCalibrationMode() const { return m_calibrationMode; }

  // Command processor
  bool processCommand(const String& cmd);

  KHState getState() const { return m_currentState; }
  bool isRunning() const { return m_isRunning; }
  bool isComplete() const;
  bool hasError() const { return m_currentState == KHState::ERROR; }

  const char* getStateName() const;
  KHStateInfo getStateInfo() const;

private:
  KHStateMachine();
  KHStateMachine(const KHStateMachine&) = delete;
  KHStateMachine& operator=(const KHStateMachine&) = delete;

  void transitionTo(KHState newState);

  void handle_IDLE();
  bool canExit_IDLE();

  void handle_PRE_FLUSH();
  bool canExit_PRE_FLUSH();

  void handle_FLOW_SETTLE();
  bool canExit_FLOW_SETTLE();

  void handle_FLOW_SETTLE_TANK();
  bool canExit_FLOW_SETTLE_TANK();

  void handle_FILL_REFERENCE();
  bool canExit_FILL_REFERENCE();

  void handle_STABILIZE_REFERENCE();
  bool canExit_STABILIZE_REFERENCE();

  void handle_MEASURE_REFERENCE_INITIAL();
  bool canExit_MEASURE_REFERENCE_INITIAL();

  void handle_AERATE_REFERENCE();
  bool canExit_AERATE_REFERENCE();

  void handle_WAIT_AFTER_AERATION_REF();
  bool canExit_WAIT_AFTER_AERATION_REF();

  void handle_MEASURE_REFERENCE_FINAL();
  bool canExit_MEASURE_REFERENCE_FINAL();

  void handle_PARTIAL_DRAIN();
  bool canExit_PARTIAL_DRAIN();

  void handle_FLUSH_CHAMBER();
  bool canExit_FLUSH_CHAMBER();

  void handle_FILL_TANK();
  bool canExit_FILL_TANK();

  void handle_STABILIZE_TANK();
  bool canExit_STABILIZE_TANK();

  void handle_MEASURE_TANK_INITIAL();
  bool canExit_MEASURE_TANK_INITIAL();

  void handle_AERATE_TANK();
  bool canExit_AERATE_TANK();

  void handle_WAIT_AFTER_AERATION_TANK();
  bool canExit_WAIT_AFTER_AERATION_TANK();

  void handle_MEASURE_TANK_FINAL();
  bool canExit_MEASURE_TANK_FINAL();

  void handle_CALCULATE_KH();
  bool canExit_CALCULATE_KH();

  void handle_DOSE();
  bool canExit_DOSE();

  void handle_FINALIZE_CHAMBER();
  bool canExit_FINALIZE_CHAMBER();

  void handle_ERROR();
  bool canExit_ERROR();

  void handle_CALIB_IDLE();
  bool canExit_CALIB_IDLE();
  void handle_CALIB_MEASURE();
  bool canExit_CALIB_MEASURE();
  void handle_CALIB_STORE();
  bool canExit_CALIB_STORE();
  void handle_CALIB_DONE();
  bool canExit_CALIB_DONE();

  KHState m_currentState;
  KHState m_previousState;
  unsigned long m_stateEntryTime;
  unsigned long m_totalStartTime;
  unsigned long m_totalRuntime;
  bool m_isRunning;
  bool m_verbose;

  KHStateConfig m_config;
  FluidSystemConfig m_fluidConfig;

  String m_errorMessage;

  RefPumpController* m_refPump;
  TankPumpController* m_tankPump;
  AerationPump* m_aerationPump;
  PHProbe* m_phProbe;
  KHSolver* m_khSolver;

  bool m_pumpOrAerationRunning;
  KHPump m_currentPump;
  bool m_pumpDirectionForward;

  float m_stabilityThreshold;
  unsigned long m_stabilityDurationMs;
  unsigned long m_stabilityTimeoutMs;

  unsigned long m_measurementStartTime;
  bool m_stabilityChecked;

  float m_refInitialPH;
  float m_refFinalPH;
  float m_tankInitialPH;
  float m_tankFinalPH;
  float m_calculatedKH;
  bool m_khValid;

  // Calibration mode variables
  bool m_calibrationMode;
  float m_targetCalibKH;
  CalibrationPoint m_calibPoints[MAX_CALIB_POINTS];
  uint8_t m_calibPointCount;
  float m_lastMeasuredX;

  // Calibration command interface
  bool processCalibCommand(const String& cmd);

  void startPump(KHPump pump, bool forward, unsigned long durationMs);
  void startPumpVolume(KHPump pump, bool forward, float volumeMl);
  void startAeration(unsigned long durationMs);
  void stopAllPumps();
  bool isPumpOrAerationRunning();
  bool checkPumpOrAerationComplete();

  bool checkStability();
  bool checkStabilityTimeout();
  void readAndStorePH(bool isReference, bool isInitial);
  void performKHCalculation();
  void logMeasurementResults();
};

#endif