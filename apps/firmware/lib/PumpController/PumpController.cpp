/**
 * @file PumpController.cpp
 * @brief Implementation of TMC2209 + AccelStepper pump controller.
 *
 * Key behaviors:
 *   - moveML() resets position to 0, calculates steps, enables pump, starts DOSING mode
 *   - runDosing() must be called in loop() - it runs stepper.run() and auto-stops on completion
 *   - stop() disables the motor driver and switches to HOLDING mode
 *   - TMC2209 configured with: toff=5, 500mA RMS, stealthChop PWM autoscale
 */

#include "PumpController.h"
#include <EEPROM.h>
#include "../../include/Config.h"

// Private constructor for Singleton pattern
PumpController::PumpController()
  : driver(&Serial, 0.11f, 0x00)  // Temporary init, re-initialized in init()
  , stepper(AccelStepper::DRIVER, 0, 0)  // Temporary init, re-initialized in init()
{
  // Real initialization happens in init() method
}

void PumpController::init(Stream *serialPort, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin, float rSense, uint8_t addr)
{
  // Reinitialize driver using placement new to avoid copy-assignment (TMC2209Stepper has no operator=)
  new (&driver) TMC2209Stepper(serialPort, rSense, addr);
  stepper = AccelStepper(AccelStepper::DRIVER, stepPin, dirPin);
  enPin = enablePin;
  this->dirPin = dirPin;
}

PumpController &PumpController::getInstance()
{
  static PumpController instance;
  return instance;
}

void PumpController::begin()
{
  pinMode(enPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(enPin, HIGH); // Disabled by default (HIGH = off for TMC2209)
  digitalWrite(dirPin, LOW); // Keep onboard D2 LED off while pump is idle

  driver.begin();
  driver.toff(5);             // Enable driver
  driver.rms_current(500);    // Increase motor current (mA)
  driver.pwm_autoscale(false); // Disable stealthChop, use spreadCycle
  driver.microsteps(16);      // Set to 16 microsteps (matches calibration)

  stepper.setMaxSpeed(4000);
  stepper.setAcceleration(2000);
  speedStep = 2000;
  currentSpeed = 0;
  
  // Load speed profiles from EEPROM (Phase 3 Sprint 7)
  loadSpeedProfiles();
  
  // Load server-synced settings from EEPROM (Phase 4 Sprint 3)
  float serverStepsPerML = 0;
  EEPROM.get(::Config::EEPROM_SERVER_STEPS_PERML_ADDR, serverStepsPerML);
  constexpr float MIN_VALID_STEPS_PER_ML = 100.0f;
  constexpr float MAX_VALID_STEPS_PER_ML = 100000.0f;
  if (serverStepsPerML >= MIN_VALID_STEPS_PER_ML && serverStepsPerML <= MAX_VALID_STEPS_PER_ML) {
    dosingStepsPerML = serverStepsPerML;
    Serial.printf("Loaded server stepsPerML from EEPROM: %.2f\n", serverStepsPerML);
  } else {
    // Fallback: load from original calibration address
    float calibratedSteps = 0;
    EEPROM.get(::Config::EEPROM_DOSING_STEPS_ADDR, calibratedSteps);
    if (calibratedSteps >= MIN_VALID_STEPS_PER_ML && calibratedSteps <= MAX_VALID_STEPS_PER_ML) {
      dosingStepsPerML = calibratedSteps;
      Serial.printf("Loaded calibration stepsPerML from EEPROM: %.2f\n", calibratedSteps);
    } else {
      Serial.printf("WARN: No valid stepsPerML in EEPROM (server=%.2f, cal=%.2f), using default: %.2f\n",
                    serverStepsPerML, calibratedSteps, dosingStepsPerML);
    }
  }
  
  uint8_t serverProfile = 0;
  EEPROM.get(::Config::EEPROM_SERVER_PROFILE_ADDR, serverProfile);
  if (serverProfile <= 2) {
    activeProfile = serverProfile;
    setSpeedProfile(activeProfile);
    Serial.printf("Loaded server speed profile from EEPROM: %d\n", serverProfile);
  }
}

void PumpController::runPeristaltic()
{
  if (isEnable && currentSpeed > 0)
  {
    stepper.setSpeed(currentSpeed);
    stepper.runSpeed();
  }
}

long PumpController::getDistanceToGo()
{
  return stepper.distanceToGo();
}

/**
 * runDosing() should only be called when mode is set to PumpMode::DOSING.
 * Caller must set mode = PumpMode::DOSING before invoking this function.
 */
void PumpController::runDosing()
{
  if (mode != PumpMode::DOSING || !isEnable)
    return;

  long remaining = abs(getDistanceToGo());

  if (remaining <= 0)
  {
    Serial.println("[runDosing] DONE - remaining <= 0, calling stop()");
    stop();
    Serial.println("Dosing complete - target reached");
    return;
  }

  stepper.run();
}

void PumpController::stop()
{
  currentSpeed = 0;
  mode = PumpMode::HOLDING;
  stepper.stop();
  disablePump();
  digitalWrite(dirPin, LOW); // Ensure D2 LED turns off after pump stops
}

void PumpController::moveToPosition(long position)
{
  stepper.moveTo(position);
}

void PumpController::moveRelative(long steps)
{
  stepper.move(steps);
}

void PumpController::updateCurrentPosition()
{
  long currentPosition = getCurrentPosition();

  if (currentPosition != lastPosition)
  {
    lastMoveTime = millis();
    lastPosition = currentPosition;
  }
}

void PumpController::enablePump()
{
  digitalWrite(enPin, LOW);
  isEnable = true;
}

void PumpController::disablePump()
{
  digitalWrite(enPin, HIGH);
  isEnable = false;
}

void PumpController::moveML(float ml)
{
  mode = PumpMode::DOSING;
  // Reset position to 0 before starting new movement
  long currentPos = stepper.currentPosition();
  stepper.setCurrentPosition(0);

  // Calculate required steps using calibrated value
  long steps = lroundf(ml * getStepsPerML());
  Serial.printf("moveML: Moving %.2f mL = %ld steps (stepsPerML: %.2f)\n",
                ml, steps, getStepsPerML());

  enablePump();
  // Move relative to new zero position
  moveRelative(steps);
  Serial.printf("moveML: Target position: %ld, Started from: %ld\n",
                steps, currentPos);
}

long PumpController::getCurrentPosition()
{
  return stepper.currentPosition();
}

void PumpController::setCurrentPosition(int32_t position)
{
  stepper.setCurrentPosition(position);
}

void PumpController::setSpeed(float speed)
{
  currentSpeed = constrain(speed, 0.0f, stepper.maxSpeed() * 1.0f);
}

bool PumpController::isRunning()
{
  return stepper.isRunning();
}

void PumpController::setAcceleration(float accel)
{
  stepper.setAcceleration(accel);
}

void PumpController::setMicrosteps(uint16_t ms)
{
  driver.microsteps(ms);
}

// ========================
// Speed Profiles (Phase 3 Sprint 7)
// ========================

void PumpController::setSpeedProfile(uint8_t profile)
{
  if (profile >= SPEED_PROFILE_COUNT)
  {
    Serial.printf("ERROR: Invalid profile %d (must be 0-%d)\n", profile, SPEED_PROFILE_COUNT - 1);
    return;
  }
  
  activeProfile = profile;
  setSpeed(speedProfiles[profile]);
  
  const char* profileNames[] = {"Slow", "Medium", "Fast"};
  Serial.printf("Speed profile set to %s (%.0f steps/sec)\n", profileNames[profile], speedProfiles[profile]);
}

float PumpController::getProfileSpeed(uint8_t profile) const
{
  if (profile >= SPEED_PROFILE_COUNT)
  {
    Serial.printf("ERROR: Invalid profile %d (must be 0-%d)\n", profile, SPEED_PROFILE_COUNT - 1);
    return 0.0f;
  }
  
  return speedProfiles[profile];
}

void PumpController::setProfileSpeed(uint8_t profile, float speed)
{
  if (profile >= SPEED_PROFILE_COUNT)
  {
    Serial.printf("ERROR: Invalid profile %d (must be 0-%d)\n", profile, SPEED_PROFILE_COUNT - 1);
    return;
  }
  
  // Clamp speed to reasonable range (1000 - 50000 steps/sec)
  speed = constrain(speed, 1000.0f, 50000.0f);
  speedProfiles[profile] = speed;
  
  const char* profileNames[] = {"Slow", "Medium", "Fast"};
  Serial.printf("Profile %s speed set to %.0f steps/sec\n", profileNames[profile], speed);
  
  saveSpeedProfiles();
}

void PumpController::loadSpeedProfiles()
{
  // Read from EEPROM: 3 floats (12 bytes) + 1 uint8_t (1 byte) = 13 bytes total
  // EEPROM address defined in Config.h as EEPROM_SPEED_PROFILES_ADDR (231)
  
  uint16_t addr = ::Config::EEPROM_SPEED_PROFILES_ADDR;
  
  // Read 3 profile speeds
  for (uint8_t i = 0; i < SPEED_PROFILE_COUNT; i++)
  {
    EEPROM.get(addr, speedProfiles[i]);
    addr += sizeof(float);
  }
  
  // Read active profile index
  EEPROM.get(addr, activeProfile);
  
  // Validate loaded values
  bool valid = true;
  for (uint8_t i = 0; i < SPEED_PROFILE_COUNT; i++)
  {
    if (speedProfiles[i] < 1000.0f || speedProfiles[i] > 50000.0f)
    {
      valid = false;
      break;
    }
  }
  
  if (!valid || activeProfile >= SPEED_PROFILE_COUNT)
  {
    // Reset to defaults if invalid
    Serial.println("Invalid speed profiles in EEPROM, resetting to defaults");
    speedProfiles[0] = 10000.0f;  // Slow
    speedProfiles[1] = 20000.0f;  // Medium
    speedProfiles[2] = 40000.0f;  // Fast
    activeProfile = 1;            // Default to Medium
    saveSpeedProfiles();
  }
  else
  {
    Serial.println("Speed profiles loaded from EEPROM:");
    const char* profileNames[] = {"Slow", "Medium", "Fast"};
    for (uint8_t i = 0; i < SPEED_PROFILE_COUNT; i++)
    {
      Serial.printf("  %s: %.0f steps/sec%s\n", profileNames[i], speedProfiles[i], 
                    (i == activeProfile) ? " (active)" : "");
    }
  }
}

void PumpController::saveSpeedProfiles()
{
  // Write to EEPROM: 3 floats (12 bytes) + 1 uint8_t (1 byte) = 13 bytes total
  
  uint16_t addr = ::Config::EEPROM_SPEED_PROFILES_ADDR;
  
  // Write 3 profile speeds
  for (uint8_t i = 0; i < SPEED_PROFILE_COUNT; i++)
  {
    EEPROM.put(addr, speedProfiles[i]);
    addr += sizeof(float);
  }
  
  // Write active profile index
  EEPROM.put(addr, activeProfile);
  
  EEPROM.commit();
  Serial.println("Speed profiles saved to EEPROM");
}

void PumpController::saveStepsPerML(float steps) {
  EEPROM.put(::Config::EEPROM_SERVER_STEPS_PERML_ADDR, steps);
  EEPROM.commit();
  Serial.printf("Server stepsPerML saved to EEPROM: %.2f\n", steps);
}

void PumpController::saveSpeedProfile(uint8_t profile) {
  if (profile <= 2) {
    activeProfile = profile;
    EEPROM.put(::Config::EEPROM_SERVER_PROFILE_ADDR, activeProfile);
    EEPROM.commit();
    Serial.printf("Server speed profile saved to EEPROM: %d\n", profile);
  }
}
