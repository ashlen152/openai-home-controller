#include "KHCalibrationSystem/KHCalibrationSystem.h"

KHCalibrationSystem::KHCalibrationSystem()
  : m_phProbe(nullptr)
  , m_airPump(nullptr)
  , m_currentState(KHState::IDLE)
  , m_previousState(KHState::IDLE)
  , m_isRunning(false)
  , m_stateStartTime(0)
  , m_totalStartTime(0)
  , m_stabilizedPH(0.0f)
  , m_stabilityAchieved(false)
{
  memset(&m_result, 0, sizeof(m_result));
}

KHCalibrationSystem& KHCalibrationSystem::getInstance()
{
  static KHCalibrationSystem instance;
  return instance;
}

void KHCalibrationSystem::begin(PHProbe* phProbe, AerationPump* airPump)
{
  m_phProbe = phProbe;
  m_airPump = airPump;
  
  if (m_phProbe) {
    m_phProbe->setStabilityThreshold(m_config.stabilityThreshold);
    m_phProbe->setStabilityDuration(m_config.stabilityDurationMs);
  }
  
  if (m_config.enableLogging) {
    Serial.println("[KH] Calibration System initialized");
    Serial.printf("[KH] Config: fill=%lums, stabilize=%lums, aerate=%lums\n",
                  m_config.fillTimeMs, m_config.stabilizeTimeMs, m_config.aerationTimeMs);
  }
}

void KHCalibrationSystem::update()
{
  if (!m_isRunning || !m_phProbe) {
    return;
  }
  
  m_phProbe->update();
  
  bool canTransition = false;
  
  switch (m_currentState) {
    case KHState::IDLE:
      break;
      
    case KHState::FILL_REFERENCE:
      canTransition = exit_FILL_REFERENCE();
      if (canTransition) transitionTo(KHState::STABILIZE_REFERENCE);
      break;
      
    case KHState::STABILIZE_REFERENCE:
      canTransition = exit_STABILIZE_REFERENCE();
      if (canTransition) transitionTo(KHState::MEASURE_REFERENCE_INITIAL);
      break;
      
    case KHState::MEASURE_REFERENCE_INITIAL:
      canTransition = exit_MEASURE_REFERENCE_INITIAL();
      if (canTransition) transitionTo(KHState::AERATE_REFERENCE);
      break;
      
    case KHState::AERATE_REFERENCE:
      canTransition = exit_AERATE_REFERENCE();
      if (canTransition) transitionTo(KHState::MEASURE_REFERENCE_FINAL);
      break;
      
    case KHState::MEASURE_REFERENCE_FINAL:
      canTransition = exit_MEASURE_REFERENCE_FINAL();
      if (canTransition) transitionTo(KHState::DRAIN);
      break;
      
    case KHState::DRAIN:
      canTransition = exit_DRAIN();
      if (canTransition) transitionTo(KHState::FLUSH);
      break;
      
    case KHState::FLUSH:
      canTransition = exit_FLUSH();
      if (canTransition) transitionTo(KHState::FILL_TANK);
      break;
      
    case KHState::FILL_TANK:
      canTransition = exit_FILL_TANK();
      if (canTransition) transitionTo(KHState::STABILIZE_TANK);
      break;
      
    case KHState::STABILIZE_TANK:
      canTransition = exit_STABILIZE_TANK();
      if (canTransition) transitionTo(KHState::MEASURE_TANK_INITIAL);
      break;
      
    case KHState::MEASURE_TANK_INITIAL:
      canTransition = exit_MEASURE_TANK_INITIAL();
      if (canTransition) transitionTo(KHState::AERATE_TANK);
      break;
      
    case KHState::AERATE_TANK:
      canTransition = exit_AERATE_TANK();
      if (canTransition) transitionTo(KHState::MEASURE_TANK_FINAL);
      break;
      
    case KHState::MEASURE_TANK_FINAL:
      canTransition = exit_MEASURE_TANK_FINAL();
      if (canTransition) transitionTo(KHState::CALCULATE_KH);
      break;
      
    case KHState::CALCULATE_KH:
      transitionTo(KHState::DONE);
      break;
      
    case KHState::DONE:
    case KHState::ERROR:
      break;
  }
}

void KHCalibrationSystem::start()
{
  if (m_config.enableLogging) {
    Serial.println("[KH] === Starting KH Calibration ===");
  }
  
  m_isRunning = true;
  m_totalStartTime = millis();
  m_stateStartTime = millis();
  
  memset(&m_result, 0, sizeof(m_result));
  m_result.finalState = KHState::IDLE;
  
  transitionTo(KHState::FILL_REFERENCE);
}

void KHCalibrationSystem::stop()
{
  if (m_config.enableLogging) {
    Serial.println("[KH] Stopping calibration");
  }
  
  m_isRunning = false;
  RefPumpController::getInstance().stop();
  TankPumpController::getInstance().stop();
  
  if (m_airPump) {
    m_airPump->turnOff();
  }
  
  transitionTo(KHState::IDLE);
}

void KHCalibrationSystem::reset()
{
  stop();
  memset(&m_result, 0, sizeof(m_result));
  transitionTo(KHState::IDLE);
}

void KHCalibrationSystem::transitionTo(KHState newState)
{
  m_previousState = m_currentState;
  m_currentState = newState;
  m_stateStartTime = millis();
  m_stabilityAchieved = false;
  
  if (m_config.enableLogging) {
    logStateTransition(getStateName(), nullptr);
  }
  
  switch (newState) {
    case KHState::IDLE: enter_IDLE(); break;
    case KHState::FILL_REFERENCE: enter_FILL_REFERENCE(); break;
    case KHState::STABILIZE_REFERENCE: enter_STABILIZE_REFERENCE(); break;
    case KHState::MEASURE_REFERENCE_INITIAL: enter_MEASURE_REFERENCE_INITIAL(); break;
    case KHState::AERATE_REFERENCE: enter_AERATE_REFERENCE(); break;
    case KHState::MEASURE_REFERENCE_FINAL: enter_MEASURE_REFERENCE_FINAL(); break;
    case KHState::DRAIN: enter_DRAIN(); break;
    case KHState::FLUSH: enter_FLUSH(); break;
    case KHState::FILL_TANK: enter_FILL_TANK(); break;
    case KHState::STABILIZE_TANK: enter_STABILIZE_TANK(); break;
    case KHState::MEASURE_TANK_INITIAL: enter_MEASURE_TANK_INITIAL(); break;
    case KHState::AERATE_TANK: enter_AERATE_TANK(); break;
    case KHState::MEASURE_TANK_FINAL: enter_MEASURE_TANK_FINAL(); break;
    case KHState::CALCULATE_KH: enter_CALCULATE_KH(); break;
    case KHState::DONE: enter_DONE(); break;
    case KHState::ERROR: enter_ERROR(); break;
  }
}

bool KHCalibrationSystem::checkStability()
{
  if (!m_phProbe) return false;
  return m_phProbe->isStable(m_config.stabilityThreshold, m_config.stabilityDurationMs);
}

void KHCalibrationSystem::calculateKH()
{
  m_result.refDeltaPH = m_result.refFinalPH - m_result.refInitialPH;
  m_result.tankDeltaPH = m_result.tankFinalPH - m_result.tankInitialPH;
  
  if (m_result.refDeltaPH != 0.0f) {
    m_result.ratio = m_result.tankDeltaPH / m_result.refDeltaPH;
  } else {
    m_result.ratio = 0.0f;
  }
  
  m_result.khValue = m_config.khSlopeA * m_result.ratio + m_config.khOffsetB;
  m_result.success = true;
  
  if (m_config.enableLogging) {
    Serial.println("[KH] === KH Calculation ===");
    Serial.printf("[KH] Ref: initial=%.3f, final=%.3f, delta=%.3f\n",
                 m_result.refInitialPH, m_result.refFinalPH, m_result.refDeltaPH);
    Serial.printf("[KH] Tank: initial=%.3f, final=%.3f, delta=%.3f\n",
                 m_result.tankInitialPH, m_result.tankFinalPH, m_result.tankDeltaPH);
    Serial.printf("[KH] Ratio: %.4f, KH: %.2f dKH\n", m_result.ratio, m_result.khValue);
  }
}

void KHCalibrationSystem::logStateTransition(const char* from, const char* to)
{
  if (m_config.enableLogging && to) {
    Serial.printf("[KH] %s -> %s\n", from, to);
  }
}

void KHCalibrationSystem::logMeasurement(const char* label, float ph)
{
  if (m_config.enableLogging) {
    Serial.printf("[KH] %s: %.3f pH\n", label, ph);
  }
}

float KHCalibrationSystem::getCurrentPH() const
{
  return m_phProbe ? m_phProbe->readRawPH() : 0.0f;
}

float KHCalibrationSystem::getFilteredPH() const
{
  return m_phProbe ? m_phProbe->readFilteredPH() : 0.0f;
}

const char* KHCalibrationSystem::getStateName() const
{
  switch (m_currentState) {
    case KHState::IDLE: return "IDLE";
    case KHState::FILL_REFERENCE: return "FILL_REFERENCE";
    case KHState::STABILIZE_REFERENCE: return "STABILIZE_REFERENCE";
    case KHState::MEASURE_REFERENCE_INITIAL: return "MEASURE_REFERENCE_INITIAL";
    case KHState::AERATE_REFERENCE: return "AERATE_REFERENCE";
    case KHState::MEASURE_REFERENCE_FINAL: return "MEASURE_REFERENCE_FINAL";
    case KHState::DRAIN: return "DRAIN";
    case KHState::FLUSH: return "FLUSH";
    case KHState::FILL_TANK: return "FILL_TANK";
    case KHState::STABILIZE_TANK: return "STABILIZE_TANK";
    case KHState::MEASURE_TANK_INITIAL: return "MEASURE_TANK_INITIAL";
    case KHState::AERATE_TANK: return "AERATE_TANK";
    case KHState::MEASURE_TANK_FINAL: return "MEASURE_TANK_FINAL";
    case KHState::CALCULATE_KH: return "CALCULATE_KH";
    case KHState::DONE: return "DONE";
    case KHState::ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

void KHCalibrationSystem::enter_IDLE()
{
  RefPumpController::getInstance().stop();
  TankPumpController::getInstance().stop();
  if (m_airPump) m_airPump->turnOff();
}

void KHCalibrationSystem::enter_FILL_REFERENCE()
{
  if (m_config.enableLogging) {
    Serial.printf("[KH] Filling reference chamber (%lums)\n", m_config.fillTimeMs);
  }
  RefPumpController::getInstance().forward();
}

bool KHCalibrationSystem::exit_FILL_REFERENCE()
{
  return (millis() - m_stateStartTime >= m_config.fillTimeMs);
}

void KHCalibrationSystem::enter_STABILIZE_REFERENCE()
{
  RefPumpController::getInstance().stop();
  if (m_config.enableLogging) {
    Serial.printf("[KH] Stabilizing reference (%lums)\n", m_config.stabilizeTimeMs);
  }
}

bool KHCalibrationSystem::exit_STABILIZE_REFERENCE()
{
  return (millis() - m_stateStartTime >= m_config.stabilizeTimeMs);
}

void KHCalibrationSystem::enter_MEASURE_REFERENCE_INITIAL()
{
  if (m_config.enableLogging) {
    Serial.println("[KH] Measuring reference initial pH");
  }
}

bool KHCalibrationSystem::exit_MEASURE_REFERENCE_INITIAL()
{
  if (m_phProbe && m_phProbe->isReady()) {
    m_result.refInitialPH = m_phProbe->readFilteredPH();
    logMeasurement("Ref Initial", m_result.refInitialPH);
    return true;
  }
  return false;
}

void KHCalibrationSystem::enter_AERATE_REFERENCE()
{
  if (m_airPump) {
    m_airPump->turnOn();
  }
  if (m_config.enableLogging) {
    Serial.printf("[KH] Aerating reference (%lums)\n", m_config.aerationTimeMs);
  }
}

bool KHCalibrationSystem::exit_AERATE_REFERENCE()
{
  return (millis() - m_stateStartTime >= m_config.aerationTimeMs);
}

void KHCalibrationSystem::enter_MEASURE_REFERENCE_FINAL()
{
  if (m_airPump) {
    m_airPump->turnOff();
  }
  if (m_config.enableLogging) {
    Serial.println("[KH] Measuring reference final pH");
  }
}

bool KHCalibrationSystem::exit_MEASURE_REFERENCE_FINAL()
{
  if (m_phProbe && m_phProbe->isReady()) {
    delay(500);
    m_result.refFinalPH = m_phProbe->readFilteredPH();
    logMeasurement("Ref Final", m_result.refFinalPH);
    return true;
  }
  return false;
}

void KHCalibrationSystem::enter_DRAIN()
{
  RefPumpController::getInstance().reverse();
  if (m_config.enableLogging) {
    Serial.printf("[KH] Draining (%lums)\n", m_config.drainTimeMs);
  }
}

bool KHCalibrationSystem::exit_DRAIN()
{
  return (millis() - m_stateStartTime >= m_config.drainTimeMs);
}

void KHCalibrationSystem::enter_FLUSH()
{
  RefPumpController::getInstance().stop();
  RefPumpController::getInstance().forward();
  if (m_config.enableLogging) {
    Serial.printf("[KH] Flushing (%lums)\n", m_config.flushTimeMs);
  }
}

bool KHCalibrationSystem::exit_FLUSH()
{
  return (millis() - m_stateStartTime >= m_config.flushTimeMs);
}

void KHCalibrationSystem::enter_FILL_TANK()
{
  RefPumpController::getInstance().stop();
  TankPumpController::getInstance().forward();
  if (m_config.enableLogging) {
    Serial.printf("[KH] Filling tank (%lums)\n", m_config.fillTimeMs);
  }
}

bool KHCalibrationSystem::exit_FILL_TANK()
{
  return (millis() - m_stateStartTime >= m_config.fillTimeMs);
}

void KHCalibrationSystem::enter_STABILIZE_TANK()
{
  TankPumpController::getInstance().stop();
  if (m_config.enableLogging) {
    Serial.printf("[KH] Stabilizing tank (%lums)\n", m_config.stabilizeTimeMs);
  }
}

bool KHCalibrationSystem::exit_STABILIZE_TANK()
{
  return (millis() - m_stateStartTime >= m_config.stabilizeTimeMs);
}

void KHCalibrationSystem::enter_MEASURE_TANK_INITIAL()
{
  if (m_config.enableLogging) {
    Serial.println("[KH] Measuring tank initial pH");
  }
}

bool KHCalibrationSystem::exit_MEASURE_TANK_INITIAL()
{
  if (m_phProbe && m_phProbe->isReady()) {
    m_result.tankInitialPH = m_phProbe->readFilteredPH();
    logMeasurement("Tank Initial", m_result.tankInitialPH);
    return true;
  }
  return false;
}

void KHCalibrationSystem::enter_AERATE_TANK()
{
  if (m_airPump) {
    m_airPump->turnOn();
  }
  if (m_config.enableLogging) {
    Serial.printf("[KH] Aerating tank (%lums)\n", m_config.aerationTimeMs);
  }
}

bool KHCalibrationSystem::exit_AERATE_TANK()
{
  return (millis() - m_stateStartTime >= m_config.aerationTimeMs);
}

void KHCalibrationSystem::enter_MEASURE_TANK_FINAL()
{
  if (m_airPump) {
    m_airPump->turnOff();
  }
  if (m_config.enableLogging) {
    Serial.println("[KH] Measuring tank final pH");
  }
}

bool KHCalibrationSystem::exit_MEASURE_TANK_FINAL()
{
  if (m_phProbe && m_phProbe->isReady()) {
    delay(500);
    m_result.tankFinalPH = m_phProbe->readFilteredPH();
    logMeasurement("Tank Final", m_result.tankFinalPH);
    return true;
  }
  return false;
}

void KHCalibrationSystem::enter_CALCULATE_KH()
{
  calculateKH();
}

void KHCalibrationSystem::enter_DONE()
{
  m_result.totalDurationMs = millis() - m_totalStartTime;
  m_result.finalState = KHState::DONE;
  m_isRunning = false;
  
  RefPumpController::getInstance().stop();
  TankPumpController::getInstance().stop();
  if (m_airPump) m_airPump->turnOff();
  
  if (m_config.enableLogging) {
    Serial.println("[KH] === Calibration Complete ===");
    Serial.printf("[KH] Final KH: %.2f dKH\n", m_result.khValue);
    Serial.printf("[KH] Total duration: %lu ms\n", m_result.totalDurationMs);
  }
}

void KHCalibrationSystem::enter_ERROR()
{
  m_result.success = false;
  m_result.finalState = KHState::ERROR;
  m_isRunning = false;
  
  RefPumpController::getInstance().stop();
  TankPumpController::getInstance().stop();
  if (m_airPump) m_airPump->turnOff();
  
  if (m_config.enableLogging) {
    Serial.println("[KH] !!! Calibration Error !!!");
  }
}