/**
 * @file PumpController.h
 * @brief Stepper motor controller for peristaltic/dosing pump using TMC2209 + AccelStepper.
 *
 * Provides three operating modes:
 *   - PERISTALTIC: Continuous speed mode for constant flow
 *   - DOSING: Position control mode for precise volumetric dispensing
 *   - HOLDING: Idle state between movements
 *
 * Uses Singleton pattern - access via PumpController::getInstance().
 * Hardware: TMC2209 stepper driver (UART), AccelStepper for motion control.
 */

#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>

// Backwards-compatibility aliasing: map legacy private member names to new m_ prefix
#define driver m_driver
#define stepper m_stepper
#define enPin m_enPin
#define isEnable m_isEnable
#define mode m_mode
#define lastMoveTime m_lastMoveTime
#define lastDebugTime m_lastDebugTime
#define holdDelay m_holdDelay
#define lastPosition m_lastPosition
#define peristalticStepsPerML m_peristalticStepsPerML
#define dosingStepsPerML m_dosingStepsPerML
#define currentSpeed m_currentSpeed
#define speedStep m_speedStep
#define maxSpeedStep m_maxSpeedStep
#define speedProfiles m_speedProfiles
#define activeProfile m_activeProfile

/**
 * @enum PumpMode
 * @brief Operating modes for the pump controller.
 */
enum class PumpMode
{
  PERISTALTIC, ///< Continuous speed mode - constant flow rate
  DOSING,      ///< Position control mode - move exact steps for target mL
  HOLDING      ///< Idle/waiting state between movements
};

/**
 * @class PumpController
 * @brief Singleton controller for TMC2209-driven peristaltic pump.
 *
 * Manages stepper motor initialization, movement commands, calibration values,
 * and mode switching. Default calibration: 709.22 steps/mL.
 *
 * Usage:
 *   PumpController& pump = PumpController::getInstance();
 *   pump.init(&Serial2, stepPin, dirPin, enPin, rSense, addr);
 *   pump.begin();
 *   pump.moveML(5.0);  // Dispense 5mL
 */
class PumpController
{
public:
  /// @brief Get singleton instance
  static PumpController &getInstance();

  /**
   * @brief Initialize hardware pins and TMC2209 driver.
   * @param serialPort UART stream for TMC2209 communication (typically &Serial2)
   * @param stepPin GPIO pin for stepper STEP signal
   * @param dirPin GPIO pin for stepper DIR signal
   * @param enablePin GPIO pin for stepper ENABLE (LOW = enabled for TMC2209)
   * @param rSense Current sense resistor value in ohms (typically 0.11)
   * @param addr TMC2209 UART address (typically 0b00)
   */
  void init(Stream *serialPort, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin, float rSense, uint8_t addr);

  /**
   * @brief Configure driver settings and initial speed/acceleration.
   * Sets: toff=5, rms_current=500mA, stealthChop enabled,
   * maxSpeed=4000, acceleration=2000, speedStep=2000.
   */
  void begin();

  // --- Mode Control ---

  /// @brief Set the pump operating mode
  void setMode(PumpMode newMode) { m_mode = newMode; }
  /// @brief Get the current pump operating mode
  PumpMode getMode() const { return m_mode; }
  /// @brief Set delay between hold movements (ms)
  void setHoldDelay(unsigned long delay) { m_holdDelay = delay; }

  // --- Movement Control ---

  /// @brief Execute continuous speed movement (PERISTALTIC mode only)
  void runPeristaltic();

  /**
   * @brief Execute position-based movement (DOSING mode only).
   * Must be called repeatedly in loop(). Automatically stops
   * and switches to HOLDING when target position is reached.
   */
  void runDosing();

  /// @brief Stop all movement, disable pump, switch to HOLDING mode
  void stop();

  /// @brief Move to absolute step position
  void moveToPosition(long position);

  /// @brief Move relative number of steps from current position
  void moveRelative(long steps);

  /**
   * @brief Dispense a specific volume in milliliters.
   * Resets position to 0, calculates steps from stepsPerML calibration,
   * enables pump, and starts DOSING mode movement.
   * @param ml Volume to dispense in milliliters
   */
  void moveML(float ml);

  // --- Position & State ---

  /// @brief Get current stepper position in steps
  long getCurrentPosition();
  /// @brief Get remaining steps to target position
  long getDistanceToGo();
  /// @brief Set current position (used for resetting/calibration)
  void setCurrentPosition(int32_t position);
  /// @brief Check if pump is currently enabled (motor energized)
  bool getIsEnable() const { return m_isEnable; }

  // --- Calibration & Steps ---

  /// @brief Get steps/mL for the current mode (DOSING or PERISTALTIC)
  float getStepsPerML() const
  {
    return m_mode == PumpMode::DOSING ? m_dosingStepsPerML : m_peristalticStepsPerML;
  }

  /// @brief Set steps/mL for the current mode
  void setStepsPerML(float steps)
  {
    if (m_mode == PumpMode::DOSING)
    {
      m_dosingStepsPerML = steps;
    }
    else
    {
      m_peristalticStepsPerML = steps;
    }
  }

  float getDosingStepsPerML() const { return m_dosingStepsPerML; }
  float getPeristalticStepsPerML() const { return m_peristalticStepsPerML; }
  void setDosingStepsPerML(float steps) { m_dosingStepsPerML = steps; }
  void setPeristalticStepsPerML(float steps) { m_peristalticStepsPerML = steps; }

  // --- Speed & Acceleration ---

  /// @brief Check if stepper motor is currently in motion
  bool isRunning();
  /// @brief Set stepper acceleration in steps/sec^2
  void setAcceleration(float accel);
  /// @brief Set TMC2209 microstepping (e.g., 16, 32, 64)
  void setMicrosteps(uint16_t ms);
  /// @brief Set target speed, clamped to [0, maxSpeed]
  void setSpeed(float speed);
  float getSpeed() const { return m_currentSpeed; }
  void setSpeedStep(int step) { m_speedStep = step; }
  int getSpeedStep() const { return m_speedStep; }
  void setMaxSpeed(float speed) { m_stepper.setMaxSpeed(speed); }

  // --- Speed Profiles (Phase 3 Sprint 7) ---

  /// @brief Set active speed profile (0=Slow, 1=Medium, 2=Fast)
  void setSpeedProfile(uint8_t profile);
  /// @brief Get current active speed profile index (0-2)
  uint8_t getActiveProfile() const { return m_activeProfile; }
  /// @brief Get speed value for a specific profile (0-2)
  float getProfileSpeed(uint8_t profile) const;
  /// @brief Set custom speed for a specific profile (0-2)
  void setProfileSpeed(uint8_t profile, float speed);
  /// @brief Load speed profiles from EEPROM
  void loadSpeedProfiles();
  void saveSpeedProfiles();
  void saveStepsPerML(float steps);
  void saveSpeedProfile(uint8_t profile);

  void enablePump();
  void disablePump();

private:
  PumpController();
  PumpController(const PumpController &) = delete;
  PumpController &operator=(const PumpController &) = delete;

  // Hardware
  TMC2209Stepper m_driver;   ///< TMC2209 UART driver instance
  AccelStepper m_stepper;     ///< AccelStepper motion controller
  uint8_t m_enPin;            ///< Enable pin (LOW = motor on)

  // State
  bool m_isEnable = false;
  PumpMode m_mode = PumpMode::HOLDING;
  unsigned long m_lastMoveTime = 0;
  unsigned long m_lastDebugTime = 0;
  unsigned long m_holdDelay = 0;
  long m_lastPosition = 0;

  float m_peristalticStepsPerML = 7642.0f;
  float m_dosingStepsPerML = 7642.0f;

  // Speed
  float m_currentSpeed = 0;
  int m_speedStep = 2000;     ///< Speed adjustment increment
  int m_maxSpeedStep = 4000;  ///< Maximum allowed speed step

  // Speed Profiles (Phase 3 Sprint 7)
  static constexpr uint8_t SPEED_PROFILE_COUNT = 3;
  float m_speedProfiles[SPEED_PROFILE_COUNT] = {10000.0f, 20000.0f, 40000.0f}; ///< Slow, Medium, Fast
  uint8_t m_activeProfile = 1;  ///< Default to Medium (index 1)

  void updateCurrentPosition();
};

#endif
