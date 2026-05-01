#ifndef PUMP_TASK_MANAGER_H
#define PUMP_TASK_MANAGER_H

#include <Arduino.h>
#include "PWMPumpController/PWMPumpController.h"

#ifdef REF_IN1
static constexpr uint8_t REF_IN1_PIN = REF_IN1;
#else
static constexpr uint8_t REF_IN1_PIN = 4;
#endif

#ifdef REF_IN2
static constexpr uint8_t REF_IN2_PIN = REF_IN2;
#else
static constexpr uint8_t REF_IN2_PIN = 5;
#endif

enum class PumpCommand
{
  NONE,
  FORWARD,
  REVERSE,
  STOP,
  FORWARD_TIMED,
  REVERSE_TIMED
};

struct PumpTask
{
  PumpCommand command;
  uint32_t duration;
};

static constexpr uint8_t MAX_PUMP_TASKS = 8;

class PumpTaskManager
{
public:
  static PumpTaskManager &getInstance();

  void begin();
  void update();

  void forward(uint8_t pumpId);
  void reverse(uint8_t pumpId);
  void stop(uint8_t pumpId);
  void forwardTimed(uint8_t pumpId, uint32_t durationMs);
  void reverseTimed(uint8_t pumpId, uint32_t durationMs);

  bool isIdle();
  bool isRunning() const;

private:
  PumpTaskManager();
  PumpTaskManager(const PumpTaskManager &) = delete;
  PumpTaskManager &operator=(const PumpTaskManager &) = delete;

  void processTask();
  PWMPumpController *getPump(uint8_t pumpId);

  PumpTask m_queue[MAX_PUMP_TASKS];
  uint8_t m_head;
  uint8_t m_tail;
  bool m_full;

  PWMPumpController m_refPump;
  PWMPumpController m_tankPump;
};

#endif