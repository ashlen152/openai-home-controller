#include "KHStateMachine/KHStateMachine.h"

#include <string.h>

KHStateMachine::KHStateMachine()
  : m_currentState(KHState::IDLE)
  , m_previousState(KHState::IDLE)
  , m_stateEntryTime(0)
  , m_totalStartTime(0)
  , m_totalRuntime(0)
  , m_isRunning(false)
  , m_verbose(false)
  , m_errorMessage("")
  , m_refPump(nullptr)
  , m_tankPump(nullptr)
  , m_aerationPump(nullptr)
  , m_phProbe(nullptr)
  , m_khSolver(nullptr)
  , m_pumpOrAerationRunning(false)
  , m_currentPump(KHPump::NONE)
  , m_pumpDirectionForward(true)
  , m_stabilityThreshold(0.02f)
  , m_stabilityDurationMs(3000UL)
  , m_stabilityTimeoutMs(30000UL)
  , m_measurementStartTime(0)
  , m_stabilityChecked(false)
  , m_refInitialPH(0.0f)
  , m_refFinalPH(0.0f)
  , m_tankInitialPH(0.0f)
  , m_tankFinalPH(0.0f)
  , m_calculatedKH(0.0f)
  , m_khValid(false)
  , m_calibrationMode(false)
  , m_targetCalibKH(0.0f)
  , m_calibPointCount(0)
  , m_lastMeasuredX(0.0f)
{
}

KHStateMachine& KHStateMachine::getInstance()
{
  static KHStateMachine instance;
  return instance;
}

void KHStateMachine::begin()
{
  begin(nullptr, nullptr, nullptr);
}

void KHStateMachine::begin(RefPumpController* refPump, TankPumpController* tankPump, AerationPump* aerationPump)
{
  begin(refPump, tankPump, aerationPump, nullptr, nullptr);
}

void KHStateMachine::begin(RefPumpController* refPump, TankPumpController* tankPump, AerationPump* aerationPump,
                          PHProbe* phProbe, KHSolver* khSolver)
{
  m_refPump = refPump;
  m_tankPump = tankPump;
  m_aerationPump = aerationPump;
  m_phProbe = phProbe;
  m_khSolver = khSolver;

  m_currentState = KHState::IDLE;
  m_previousState = KHState::IDLE;
  m_stateEntryTime = millis();
  m_totalRuntime = 0;
  m_pumpOrAerationRunning = false;
  m_currentPump = KHPump::NONE;

  m_stabilityThreshold = 0.02f;
  m_stabilityDurationMs = 3000UL;
  m_stabilityTimeoutMs = 30000UL;
  m_measurementStartTime = 0;
  m_stabilityChecked = false;

  m_refInitialPH = 0.0f;
  m_refFinalPH = 0.0f;
  m_tankInitialPH = 0.0f;
  m_tankFinalPH = 0.0f;
  m_calculatedKH = 0.0f;
  m_khValid = false;

  if (m_phProbe) {
    m_phProbe->setStabilityThreshold(m_stabilityThreshold);
    m_phProbe->setStabilityDuration(m_stabilityDurationMs);
    m_phProbe->setDebugEnabled(m_verbose);
  }

  if (m_verbose) {
    Serial.println("[KHState] Initialized with pump, pH, and KH solver");
  }
}

void KHStateMachine::update()
{
  if (!m_isRunning) {
    return;
  }

  m_totalRuntime = millis() - m_totalStartTime;

  if (m_refPump) m_refPump->update();
  if (m_tankPump) m_tankPump->update();
  if (m_aerationPump) m_aerationPump->update();
  if (m_phProbe) m_phProbe->update();

  bool canTransition = false;

  switch (m_currentState) {
    case KHState::IDLE:
      handle_IDLE();
      canTransition = canExit_IDLE();
      if (canTransition) transitionTo(KHState::PRE_FLUSH);
      break;

    case KHState::PRE_FLUSH:
      handle_PRE_FLUSH();
      canTransition = canExit_PRE_FLUSH();
      if (canTransition) transitionTo(KHState::FILL_REFERENCE);
      break;

    case KHState::FILL_REFERENCE:
      handle_FILL_REFERENCE();
      canTransition = canExit_FILL_REFERENCE();
      if (canTransition) transitionTo(KHState::FLUSH_LINE);
      break;

    case KHState::FLUSH_LINE:
      handle_FLUSH_LINE();
      canTransition = canExit_FLUSH_LINE();
      if (canTransition) transitionTo(KHState::STABILIZE_REFERENCE);
      break;

    case KHState::STABILIZE_REFERENCE:
      handle_STABILIZE_REFERENCE();
      canTransition = canExit_STABILIZE_REFERENCE();
      if (canTransition) transitionTo(KHState::MEASURE_REFERENCE_INITIAL);
      break;

    case KHState::MEASURE_REFERENCE_INITIAL:
      handle_MEASURE_REFERENCE_INITIAL();
      canTransition = canExit_MEASURE_REFERENCE_INITIAL();
      if (canTransition) transitionTo(KHState::AERATE_REFERENCE);
      break;

    case KHState::AERATE_REFERENCE:
      handle_AERATE_REFERENCE();
      canTransition = canExit_AERATE_REFERENCE();
      if (canTransition) transitionTo(KHState::WAIT_AFTER_AERATION_REF);
      break;

    case KHState::WAIT_AFTER_AERATION_REF:
      handle_WAIT_AFTER_AERATION_REF();
      canTransition = canExit_WAIT_AFTER_AERATION_REF();
      if (canTransition) transitionTo(KHState::MEASURE_REFERENCE_FINAL);
      break;

    case KHState::MEASURE_REFERENCE_FINAL:
      handle_MEASURE_REFERENCE_FINAL();
      canTransition = canExit_MEASURE_REFERENCE_FINAL();
      if (canTransition) {
#if USE_MOCK_PH
        if (m_phProbe && m_phProbe->isMockEnabled()) {
          m_phProbe->setMockIsTank(true);
          m_phProbe->setMockPostAeration(false);
        }
#endif
        transitionTo(KHState::PARTIAL_DRAIN);
      }
      break;

    case KHState::PARTIAL_DRAIN:
      handle_PARTIAL_DRAIN();
      canTransition = canExit_PARTIAL_DRAIN();
      if (canTransition) transitionTo(KHState::FLUSH_CHAMBER);
      break;

    case KHState::FLUSH_CHAMBER:
      handle_FLUSH_CHAMBER();
      canTransition = canExit_FLUSH_CHAMBER();
      if (canTransition) transitionTo(KHState::FILL_TANK);
      break;

    case KHState::FILL_TANK:
      handle_FILL_TANK();
      canTransition = canExit_FILL_TANK();
      if (canTransition) transitionTo(KHState::FLUSH_LINE_TANK);
      break;

    case KHState::FLUSH_LINE_TANK:
      handle_FLUSH_LINE_TANK();
      canTransition = canExit_FLUSH_LINE_TANK();
      if (canTransition) transitionTo(KHState::STABILIZE_TANK);
      break;

    case KHState::STABILIZE_TANK:
      handle_STABILIZE_TANK();
      canTransition = canExit_STABILIZE_TANK();
      if (canTransition) transitionTo(KHState::MEASURE_TANK_INITIAL);
      break;

    case KHState::MEASURE_TANK_INITIAL:
      handle_MEASURE_TANK_INITIAL();
      canTransition = canExit_MEASURE_TANK_INITIAL();
      if (canTransition) transitionTo(KHState::AERATE_TANK);
      break;

    case KHState::AERATE_TANK:
      handle_AERATE_TANK();
      canTransition = canExit_AERATE_TANK();
      if (canTransition) transitionTo(KHState::WAIT_AFTER_AERATION_TANK);
      break;

    case KHState::WAIT_AFTER_AERATION_TANK:
      handle_WAIT_AFTER_AERATION_TANK();
      canTransition = canExit_WAIT_AFTER_AERATION_TANK();
      if (canTransition) transitionTo(KHState::MEASURE_TANK_FINAL);
      break;

    case KHState::MEASURE_TANK_FINAL:
      handle_MEASURE_TANK_FINAL();
      canTransition = canExit_MEASURE_TANK_FINAL();
      if (canTransition) transitionTo(KHState::CALCULATE_KH);
      break;

    case KHState::CALCULATE_KH:
      handle_CALCULATE_KH();
      canTransition = canExit_CALCULATE_KH();
      if (canTransition) {
        if (m_calibrationMode) {
          transitionTo(KHState::CALIB_STORE);
        } else {
          transitionTo(KHState::DOSE);
        }
      }
      break;

    case KHState::DOSE:
      handle_DOSE();
      canTransition = canExit_DOSE();
      if (canTransition) transitionTo(KHState::FINALIZE_CHAMBER);
      break;

    case KHState::FINALIZE_CHAMBER:
      handle_FINALIZE_CHAMBER();
      canTransition = canExit_FINALIZE_CHAMBER();
      if (canTransition) transitionTo(KHState::IDLE);
      break;

    case KHState::ERROR:
      handle_ERROR();
      break;

    case KHState::CALIB_IDLE:
      handle_CALIB_IDLE();
      canTransition = canExit_CALIB_IDLE();
      if (canTransition) transitionTo(KHState::CALIB_MEASURE);
      break;

    case KHState::CALIB_MEASURE:
      handle_CALIB_MEASURE();
      canTransition = canExit_CALIB_MEASURE();
      if (canTransition) transitionTo(KHState::FILL_REFERENCE);
      break;

    case KHState::CALIB_STORE:
      handle_CALIB_STORE();
      canTransition = canExit_CALIB_STORE();
      if (canTransition) transitionTo(KHState::CALIB_DONE);
      break;

    case KHState::CALIB_DONE:
      handle_CALIB_DONE();
      canTransition = canExit_CALIB_DONE();
      if (canTransition) transitionTo(KHState::IDLE);
      break;
  }
}

void KHStateMachine::start()
{
  if (m_verbose) {
    Serial.println("[KHState] Starting measurement cycle");
  }

  m_isRunning = true;
  m_totalStartTime = millis();
  m_totalRuntime = 0;
  m_errorMessage = "";

  transitionTo(KHState::PRE_FLUSH);
}

void KHStateMachine::stop()
{
  if (m_verbose) {
    Serial.println("[KHState] Stopping");
  }

  m_isRunning = false;
  transitionTo(KHState::IDLE);
}

void KHStateMachine::reset()
{
  stop();
  m_errorMessage = "";
  m_totalRuntime = 0;
  m_currentState = KHState::IDLE;
  m_stateEntryTime = millis();
}

void KHStateMachine::setConfig(const KHStateConfig& config)
{
  m_config = config;
}

void KHStateMachine::setVerbose(bool enable)
{
  m_verbose = enable;
}

bool KHStateMachine::isComplete() const
{
  return (m_currentState == KHState::IDLE && m_isRunning == false && m_totalRuntime > 0);
}

const char* KHStateMachine::getStateName() const
{
  switch (m_currentState) {
    case KHState::IDLE: return "IDLE";
    case KHState::PRE_FLUSH: return "PRE_FLUSH";
    case KHState::FILL_REFERENCE: return "FILL_REFERENCE";
    case KHState::FLUSH_LINE: return "FLUSH_LINE";
    case KHState::STABILIZE_REFERENCE: return "STABILIZE_REFERENCE";
    case KHState::MEASURE_REFERENCE_INITIAL: return "MEASURE_REFERENCE_INITIAL";
    case KHState::AERATE_REFERENCE: return "AERATE_REFERENCE";
    case KHState::WAIT_AFTER_AERATION_REF: return "WAIT_AFTER_AERATION_REF";
    case KHState::MEASURE_REFERENCE_FINAL: return "MEASURE_REFERENCE_FINAL";
    case KHState::PARTIAL_DRAIN: return "PARTIAL_DRAIN";
    case KHState::FLUSH_CHAMBER: return "FLUSH_CHAMBER";
    case KHState::FILL_TANK: return "FILL_TANK";
    case KHState::FLUSH_LINE_TANK: return "FLUSH_LINE_TANK";
    case KHState::STABILIZE_TANK: return "STABILIZE_TANK";
    case KHState::MEASURE_TANK_INITIAL: return "MEASURE_TANK_INITIAL";
    case KHState::AERATE_TANK: return "AERATE_TANK";
    case KHState::WAIT_AFTER_AERATION_TANK: return "WAIT_AFTER_AERATION_TANK";
    case KHState::MEASURE_TANK_FINAL: return "MEASURE_TANK_FINAL";
    case KHState::CALCULATE_KH: return "CALCULATE_KH";
    case KHState::DOSE: return "DOSE";
    case KHState::FINALIZE_CHAMBER: return "FINALIZE_CHAMBER";
    case KHState::ERROR: return "ERROR";
    case KHState::CALIB_IDLE: return "CALIB_IDLE";
    case KHState::CALIB_MEASURE: return "CALIB_MEASURE";
    case KHState::CALIB_STORE: return "CALIB_STORE";
    case KHState::CALIB_DONE: return "CALIB_DONE";
    default: return "UNKNOWN";
  }
}

KHStateInfo KHStateMachine::getStateInfo() const
{
  KHStateInfo info;
  info.state = m_currentState;
  info.entryTime = m_stateEntryTime;
  info.totalRuntime = m_totalRuntime;
  info.hasError = (m_currentState == KHState::ERROR);
  info.errorMessage = m_errorMessage;
  return info;
}

void KHStateMachine::transitionTo(KHState newState)
{
  if (m_currentState == newState) {
    return;
  }

  m_previousState = m_currentState;
  m_currentState = newState;
  m_stateEntryTime = millis();

  if (m_verbose) {
    Serial.printf("[KHState] %s -> %s\n", getStateName(), nullptr);
  }

  if (newState == KHState::MEASURE_REFERENCE_INITIAL ||
      newState == KHState::MEASURE_REFERENCE_FINAL ||
      newState == KHState::MEASURE_TANK_INITIAL ||
      newState == KHState::MEASURE_TANK_FINAL) {
    m_stabilityChecked = false;
    m_measurementStartTime = millis();
  }
}

void KHStateMachine::handle_IDLE()
{
}

bool KHStateMachine::canExit_IDLE()
{
  return false;
}

void KHStateMachine::handle_PRE_FLUSH()
{
  if (!m_pumpOrAerationRunning) {
    if (m_verbose) Serial.println("[KHState] PRE_FLUSH: flush 2.5x dead volume");
    startPumpVolume(KHPump::REFERENCE, true, m_fluidConfig.getFlushVolumeMl());
  }
}

bool KHStateMachine::canExit_PRE_FLUSH()
{
  return checkPumpOrAerationComplete();
}

void KHStateMachine::handle_FLUSH_LINE()
{
}

bool KHStateMachine::canExit_FLUSH_LINE()
{
  return (millis() - m_stateEntryTime >= 2000UL);
}

void KHStateMachine::handle_FLUSH_LINE_TANK()
{
  if (!m_pumpOrAerationRunning) {
    if (m_verbose) Serial.println("[KHState] FLUSH_LINE_TANK: flush dead volume");
    startPumpVolume(KHPump::TANK, true, m_fluidConfig.getDeadVolumeMl());
  }
}

bool KHStateMachine::canExit_FLUSH_LINE_TANK()
{
  return checkPumpOrAerationComplete();
}

void KHStateMachine::handle_FILL_REFERENCE()
{
  if (!m_pumpOrAerationRunning) {
    if (m_verbose) Serial.println("[KHState] FILL_REFERENCE: push through chamber + dead volume");
    float vol = m_fluidConfig.chamberVolumeMl + m_fluidConfig.getDeadVolumeMl();
    startPumpVolume(KHPump::REFERENCE, true, vol);
  }
}

bool KHStateMachine::canExit_FILL_REFERENCE()
{
  return checkPumpOrAerationComplete();
}

void KHStateMachine::handle_STABILIZE_REFERENCE()
{
}

bool KHStateMachine::canExit_STABILIZE_REFERENCE()
{
  return (millis() - m_stateEntryTime >= m_config.stabilizeTimeMs);
}

void KHStateMachine::handle_MEASURE_REFERENCE_INITIAL()
{
  if (!m_phProbe) return;

  m_phProbe->update();

  if (!m_stabilityChecked) {
    m_measurementStartTime = millis();
    m_stabilityChecked = true;
    if (m_verbose) Serial.println("[KHState] Checking pH stability for ref initial...");
  }

  if (checkStability()) {
    if (m_verbose) Serial.println("[KHState] pH stable - storing ref initial");
    readAndStorePH(true, true);
  }
}

bool KHStateMachine::canExit_MEASURE_REFERENCE_INITIAL()
{
  if (!m_phProbe) return true;
  return checkStability() || checkStabilityTimeout();
}

void KHStateMachine::handle_AERATE_REFERENCE()
{
  if (!m_pumpOrAerationRunning) {
    if (m_verbose) Serial.println("[KHState] Starting aeration for reference");
    startAeration(m_config.aerationTimeMs);
    
#if USE_MOCK_PH
    if (m_phProbe && m_phProbe->isMockEnabled()) {
      m_phProbe->setMockPostAeration(true);
      m_phProbe->setMockAerationStart(millis());
    }
#endif
  }
}

bool KHStateMachine::canExit_AERATE_REFERENCE()
{
  return checkPumpOrAerationComplete();
}

void KHStateMachine::handle_WAIT_AFTER_AERATION_REF()
{
}

bool KHStateMachine::canExit_WAIT_AFTER_AERATION_REF()
{
  return (millis() - m_stateEntryTime >= m_config.waitAfterAerationMs);
}

void KHStateMachine::handle_MEASURE_REFERENCE_FINAL()
{
  if (!m_phProbe) return;

  m_phProbe->update();

  if (!m_stabilityChecked) {
    m_measurementStartTime = millis();
    m_stabilityChecked = true;
    if (m_verbose) Serial.println("[KHState] Checking pH stability for ref final...");
  }

  if (checkStability()) {
    if (m_verbose) Serial.println("[KHState] pH stable - storing ref final");
    readAndStorePH(true, false);
  }
}

bool KHStateMachine::canExit_MEASURE_REFERENCE_FINAL()
{
  if (!m_phProbe) return true;
  return checkStability() || checkStabilityTimeout();
}

void KHStateMachine::handle_PARTIAL_DRAIN()
{
  if (!m_pumpOrAerationRunning) {
    if (m_verbose) Serial.println("[KHState] Starting partial drain (displacement)");
    float drainVol = m_fluidConfig.getDeadVolumeMl() * 0.5f;
    startPumpVolume(KHPump::REFERENCE, false, drainVol);
  }
}

bool KHStateMachine::canExit_PARTIAL_DRAIN()
{
  return checkPumpOrAerationComplete();
}

void KHStateMachine::handle_FLUSH_CHAMBER()
{
  if (!m_pumpOrAerationRunning) {
    if (m_verbose) Serial.println("[KHState] Starting chamber flush (displacement)");
    float flushVol = m_fluidConfig.getFlushVolumeMl();
    startPumpVolume(KHPump::REFERENCE, true, flushVol);
  }
}

bool KHStateMachine::canExit_FLUSH_CHAMBER()
{
  return checkPumpOrAerationComplete();
}

void KHStateMachine::handle_FILL_TANK()
{
  if (!m_pumpOrAerationRunning) {
    if (m_verbose) Serial.println("[KHState] Starting tank pump for FILL");
    startPump(KHPump::TANK, true, m_config.fillTimeMs);
  }
}

bool KHStateMachine::canExit_FILL_TANK()
{
  return checkPumpOrAerationComplete();
}

void KHStateMachine::handle_STABILIZE_TANK()
{
}

bool KHStateMachine::canExit_STABILIZE_TANK()
{
  return (millis() - m_stateEntryTime >= m_config.stabilizeTimeMs);
}

void KHStateMachine::handle_MEASURE_TANK_INITIAL()
{
  if (!m_phProbe) return;

  m_phProbe->update();

  if (!m_stabilityChecked) {
    m_measurementStartTime = millis();
    m_stabilityChecked = true;
    if (m_verbose) Serial.println("[KHState] Checking pH stability for tank initial...");
  }

  if (checkStability()) {
    if (m_verbose) Serial.println("[KHState] pH stable - storing tank initial");
    readAndStorePH(false, true);
  }
}

bool KHStateMachine::canExit_MEASURE_TANK_INITIAL()
{
  if (!m_phProbe) return true;
  return checkStability() || checkStabilityTimeout();
}

void KHStateMachine::handle_AERATE_TANK()
{
  if (!m_pumpOrAerationRunning) {
    if (m_verbose) Serial.println("[KHState] Starting aeration for tank");
    startAeration(m_config.aerationTimeMs);
    
#if USE_MOCK_PH
    if (m_phProbe && m_phProbe->isMockEnabled()) {
      m_phProbe->setMockPostAeration(true);
      m_phProbe->setMockAerationStart(millis());
    }
#endif
  }
}

bool KHStateMachine::canExit_AERATE_TANK()
{
  return checkPumpOrAerationComplete();
}

void KHStateMachine::handle_WAIT_AFTER_AERATION_TANK()
{
}

bool KHStateMachine::canExit_WAIT_AFTER_AERATION_TANK()
{
  return (millis() - m_stateEntryTime >= m_config.waitAfterAerationMs);
}

void KHStateMachine::handle_MEASURE_TANK_FINAL()
{
  if (!m_phProbe) return;

  m_phProbe->update();

  if (!m_stabilityChecked) {
    m_measurementStartTime = millis();
    m_stabilityChecked = true;
    if (m_verbose) Serial.println("[KHState] Checking pH stability for tank final...");
  }

  if (checkStability()) {
    if (m_verbose) Serial.println("[KHState] pH stable - storing tank final");
    readAndStorePH(false, false);
  }
}

bool KHStateMachine::canExit_MEASURE_TANK_FINAL()
{
  if (!m_phProbe) return true;
  return checkStability() || checkStabilityTimeout();
}

void KHStateMachine::handle_CALCULATE_KH()
{
  if (m_verbose) Serial.println("[KHState] Calculating KH...");
  performKHCalculation();
  logMeasurementResults();

  if (!m_khValid) {
    if (m_verbose) Serial.println("[KHState] KH calculation failed - going to ERROR");
    m_errorMessage = "KH calculation invalid";
    transitionTo(KHState::ERROR);
  }
}

bool KHStateMachine::canExit_CALCULATE_KH()
{
  return m_khValid;
}

void KHStateMachine::handle_DOSE()
{
}

bool KHStateMachine::canExit_DOSE()
{
  return (millis() - m_stateEntryTime >= m_config.doseTimeMs);
}

void KHStateMachine::handle_FINALIZE_CHAMBER()
{
  if (!m_pumpOrAerationRunning) {
    if (m_verbose) Serial.println("[KHState] FINALIZE_CHAMBER: ensure probe stays wet");
    float flushVol = m_fluidConfig.getFlushVolumeMl();
    startPumpVolume(KHPump::TANK, false, flushVol);
  }
}

bool KHStateMachine::canExit_FINALIZE_CHAMBER()
{
  return checkPumpOrAerationComplete();
}

void KHStateMachine::handle_ERROR()
{
}

bool KHStateMachine::canExit_ERROR()
{
  return false;
}

void KHStateMachine::startPump(KHPump pump, bool forward, unsigned long durationMs)
{
  m_currentPump = pump;
  m_pumpDirectionForward = forward;
  m_pumpOrAerationRunning = true;

  switch (pump) {
    case KHPump::REFERENCE:
      if (m_refPump) {
        m_refPump->runTimed(forward ? PumpDirection::FORWARD : PumpDirection::REVERSE, durationMs);
      }
      break;
    case KHPump::DRAIN:
      if (m_refPump) {
        m_refPump->runTimed(forward ? PumpDirection::FORWARD : PumpDirection::REVERSE, durationMs);
      }
      break;
    case KHPump::FLUSH:
      if (m_refPump) {
        m_refPump->runTimed(forward ? PumpDirection::FORWARD : PumpDirection::REVERSE, durationMs);
      }
      break;
    case KHPump::TANK:
      if (m_tankPump) {
        m_tankPump->runTimed(forward ? PumpDirection::FORWARD : PumpDirection::REVERSE, durationMs);
      }
      break;
    default:
      break;
  }
}

void KHStateMachine::startPumpVolume(KHPump pump, bool forward, float volumeMl)
{
  m_currentPump = pump;
  m_pumpDirectionForward = forward;
  m_pumpOrAerationRunning = true;
  
  float stepsPerMl = m_config.stepsPerMl;
  
  switch (pump) {
    case KHPump::REFERENCE:
      if (m_refPump) {
        m_refPump->runVolume(forward ? PumpDirection::FORWARD : PumpDirection::REVERSE, volumeMl);
      }
      break;
    case KHPump::TANK:
      if (m_tankPump) {
        m_tankPump->runVolume(forward ? PumpDirection::FORWARD : PumpDirection::REVERSE, volumeMl);
      }
      break;
    default:
      break;
  }
}

void KHStateMachine::startAeration(unsigned long durationMs)
{
  m_currentPump = KHPump::NONE;
  m_pumpOrAerationRunning = true;

  if (m_aerationPump) {
    m_aerationPump->runForDuration(durationMs);
  }
}

void KHStateMachine::stopAllPumps()
{
  if (m_refPump) {
    m_refPump->stop();
  }
  if (m_tankPump) {
    m_tankPump->stop();
  }
  if (m_aerationPump) {
    m_aerationPump->turnOff();
  }

  m_pumpOrAerationRunning = false;
  m_currentPump = KHPump::NONE;
}

bool KHStateMachine::isPumpOrAerationRunning()
{
  bool pumpRunning = false;

  if (m_refPump && m_refPump->isTimedRunActive()) {
    pumpRunning = true;
  }
  if (m_tankPump && m_tankPump->isTimedRunActive()) {
    pumpRunning = true;
  }
  if (m_aerationPump && m_aerationPump->isRunning()) {
    pumpRunning = true;
  }

  return pumpRunning;
}

bool KHStateMachine::checkPumpOrAerationComplete()
{
  if (!m_pumpOrAerationRunning) {
    return true;
  }

  if (!isPumpOrAerationRunning()) {
    m_pumpOrAerationRunning = false;
    return true;
  }

  return false;
}

bool KHStateMachine::checkStability()
{
  if (!m_phProbe) return false;
  return m_phProbe->isStable(m_stabilityThreshold, m_stabilityDurationMs);
}

bool KHStateMachine::checkStabilityTimeout()
{
  return (millis() - m_measurementStartTime >= m_stabilityTimeoutMs);
}

void KHStateMachine::readAndStorePH(bool isReference, bool isInitial)
{
  if (!m_phProbe) return;

  float ph = m_phProbe->readFilteredPH();

  if (isReference) {
    if (isInitial) {
      m_refInitialPH = ph;
    } else {
      m_refFinalPH = ph;
    }
  } else {
    if (isInitial) {
      m_tankInitialPH = ph;
    } else {
      m_tankFinalPH = ph;
    }
  }
}

void KHStateMachine::performKHCalculation()
{
  if (!m_khSolver) {
    m_khValid = false;
    return;
  }

  float deltaRef = m_refFinalPH - m_refInitialPH;
  float deltaTank = m_tankFinalPH - m_tankInitialPH;

  KHResult result = m_khSolver->computeFromPair(deltaTank, deltaRef);

  m_calculatedKH = result.value;
  m_khValid = result.valid;

  if (m_verbose) {
    Serial.printf("[KHState] Delta ref: %.3f, Delta tank: %.3f\n", deltaRef, deltaTank);
    Serial.printf("[KHState] KH result: valid=%s, value=%.2f, confidence=%.2f\n",
                 result.valid ? "YES" : "NO", result.value, result.confidence);
  }
}

void KHStateMachine::logMeasurementResults()
{
  Serial.println("");
  Serial.println("========== KH Measurement Results ==========");
  Serial.printf("Ref pH: initial=%.3f, final=%.3f, delta=%.3f\n",
                m_refInitialPH, m_refFinalPH, m_refFinalPH - m_refInitialPH);
  Serial.printf("Tank pH: initial=%.3f, final=%.3f, delta=%.3f\n",
                m_tankInitialPH, m_tankFinalPH, m_tankFinalPH - m_tankInitialPH);

  float deltaRef = m_refFinalPH - m_refInitialPH;
  float deltaTank = m_tankFinalPH - m_tankInitialPH;
  float ratio = (deltaRef != 0.0f) ? (deltaTank / deltaRef) : 0.0f;

  Serial.printf("Ratio (tank/ref): %.4f\n", ratio);
Serial.printf("Calculated KH: %.2f dKH (%s)\n",
                 m_calculatedKH, m_khValid ? "VALID" : "INVALID");
  Serial.println("============================================");
  Serial.println("");
}

void KHStateMachine::handle_CALIB_IDLE()
{
}

bool KHStateMachine::canExit_CALIB_IDLE()
{
  return m_targetCalibKH > 0.0f;
}

void KHStateMachine::handle_CALIB_MEASURE()
{
}

bool KHStateMachine::canExit_CALIB_MEASURE()
{
  return true;
}

void KHStateMachine::handle_CALIB_STORE()
{
  if (m_calibPointCount >= MAX_CALIB_POINTS) {
    if (m_verbose) Serial.println("[KHState] Calibration point buffer full");
    return;
  }

  float deltaRef = m_refFinalPH - m_refInitialPH;
  float deltaTank = m_tankFinalPH - m_tankInitialPH;

  float x = (deltaRef != 0.0f) ? (deltaTank / deltaRef) : deltaTank;

  m_calibPoints[m_calibPointCount].x = x;
  m_calibPoints[m_calibPointCount].kh = m_targetCalibKH;
  m_calibPoints[m_calibPointCount].valid = true;
  m_lastMeasuredX = x;
  m_calibPointCount++;

  if (m_verbose) {
    Serial.printf("[KHState] Stored calibration point #%d: x=%.4f, KH=%.1f\n",
                 m_calibPointCount, x, m_targetCalibKH);
  }
}

bool KHStateMachine::canExit_CALIB_STORE()
{
  return true;
}

void KHStateMachine::handle_CALIB_DONE()
{
  if (m_verbose) {
    Serial.println("[KHState] Calibration measurement complete");
    Serial.printf("[KHState] Use 'kh calib add <kh>' to add more or 'kh calib finish' to apply\n");
  }
}

bool KHStateMachine::canExit_CALIB_DONE()
{
  return true;
}

bool KHStateMachine::calibStart(float knownKH)
{
  if (knownKH <= 0.0f || knownKH > 20.0f) {
    Serial.printf("[KHState] Invalid KH value: %.1f (must be 0-20)\n", knownKH);
    return false;
  }

  m_targetCalibKH = knownKH;
  m_calibrationMode = true;
  m_isRunning = true;
  m_totalStartTime = millis();
  m_totalRuntime = 0;
  m_currentState = KHState::CALIB_IDLE;

  Serial.printf("[KHState] Calibration started with known KH=%.1f dKH\n", knownKH);
  Serial.println("[KHState] Running measurement cycle...");

  return true;
}

bool KHStateMachine::calibAdd(float knownKH)
{
  if (!m_isRunning || m_currentState != KHState::CALIB_DONE) {
    Serial.println("[KHState] No active calibration. Use 'kh calib start <kh>' first");
    return false;
  }

  if (knownKH <= 0.0f || knownKH > 20.0f) {
    Serial.printf("[KHState] Invalid KH value: %.1f\n", knownKH);
    return false;
  }

  m_targetCalibKH = knownKH;
  m_currentState = KHState::CALIB_MEASURE;

  Serial.printf("[KHState] Adding calibration point with known KH=%.1f dKH\n", knownKH);

  return true;
}

void KHStateMachine::calibFinish()
{
  if (m_calibPointCount < 2) {
    Serial.printf("[KHState] Need at least 2 points, have %d\n", m_calibPointCount);
    return;
  }

  if (m_khSolver == nullptr) {
    Serial.println("[KHState] KHSolver not initialized");
    return;
  }

  float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
  uint8_t validCount = 0;

  for (uint8_t i = 0; i < m_calibPointCount; i++) {
    if (m_calibPoints[i].valid) {
      sumX += m_calibPoints[i].x;
      sumY += m_calibPoints[i].kh;
      sumXY += m_calibPoints[i].x * m_calibPoints[i].kh;
      sumX2 += m_calibPoints[i].x * m_calibPoints[i].x;
      validCount++;
    }
  }

  if (validCount < 2) {
    Serial.println("[KHState] Not enough valid calibration points");
    return;
  }

  if (validCount == 2) {
    float slope = (validCount * sumXY - sumX * sumY) / (validCount * sumX2 - sumX * sumX);
    float intercept = (sumY - slope * sumX) / validCount;

    m_khSolver->setLinear(slope, intercept);
    Serial.printf("[KHState] Linear fit: KH = %.4f * x + %.4f\n", slope, intercept);
  } else {
    float n = (float)validCount;
    float sumX3 = 0, sumX4 = 0, sumY2 = 0, sumXY2 = 0;
    for (uint8_t i = 0; i < m_calibPointCount; i++) {
      if (m_calibPoints[i].valid) {
        float x = m_calibPoints[i].x;
        float y = m_calibPoints[i].kh;
        sumX3 += x * x * x;
        sumX4 += x * x * x * x;
        sumY2 += y * x * x;
        sumXY2 += y * x;
      }
    }

    float denom = n * sumX4 - 2 * sumX * sumX3 + sumX * sumX * sumX2;
    if (abs(denom) < 0.0001f) {
      Serial.println("[KHState] Quadratic fit failed, falling back to linear");
      calibFinish();
      return;
    }

    float c = (n * sumY2 - sumX * sumXY2) / denom;
    float b = (sumXY2 - c * sumX3 - sumX * sumX * sumY / n) / (sumX2 - sumX * sumX / n);
    float a = (sumY / n) - b * sumX / n - c * sumX2 / n;

    m_khSolver->setQuadratic(a, b, c);
    Serial.printf("[KHState] Quadratic fit: KH = %.4f + %.4f*x + %.4f*x^2\n", a, b, c);
  }

  m_khSolver->saveToEEPROM();

  Serial.println("[KHState] Calibration applied and saved to EEPROM");

  m_calibrationMode = false;
  m_targetCalibKH = 0.0f;
  m_calibPointCount = 0;
  m_currentState = KHState::IDLE;
  m_isRunning = false;
}

void KHStateMachine::calibClear()
{
  m_calibPointCount = 0;
  m_calibrationMode = false;
  m_targetCalibKH = 0.0f;

  if (m_khSolver) {
    m_khSolver->resetToFactory();
  }

  Serial.println("[KHState] Calibration cleared");
}

void KHStateMachine::calibList()
{
  Serial.println("========== Calibration Points ==========");
  Serial.printf("Stored points: %d/%d\n", m_calibPointCount, MAX_CALIB_POINTS);

  for (uint8_t i = 0; i < m_calibPointCount; i++) {
    Serial.printf("  #%d: x=%.4f, KH=%.1f dKH (%s)\n",
                 i + 1,
                 m_calibPoints[i].x,
                 m_calibPoints[i].kh,
                 m_calibPoints[i].valid ? "valid" : "invalid");
  }

  Serial.println("========================================");

  if (m_khSolver) {
    m_khSolver->printCalibrationInfo();
  }
}

bool KHStateMachine::processCommand(const String& cmd)
{
  if (cmd.startsWith("kh calib start ")) {
    float kh = cmd.substring(15).toFloat();
    return calibStart(kh);
  }
  else if (cmd.startsWith("kh calib add ")) {
    float kh = cmd.substring(13).toFloat();
    return calibAdd(kh);
  }
  else if (cmd == "kh calib finish") {
    calibFinish();
    return true;
  }
  else if (cmd == "kh calib clear") {
    calibClear();
    return true;
  }
  else if (cmd == "kh calib list") {
    calibList();
    return true;
  }

  return false;
}