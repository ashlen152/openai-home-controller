# Wave2 Profiling Plan for ESP32 firmware (apps/firmware)

- Objective
  Add non-intrusive, build-time profiling hooks to capture memory, stack, and timing characteristics with minimal runtime impact. All instrumentation is gated behind a PROFILING macro so it can be enabled/disabled at build time.

- Metrics to capture
  - Heap: free heap amount (ESP.getFreeHeap())
  - Stack: high-water mark for important tasks (uxTaskGetStackHighWaterMark())
  - Timing: loop duration and per-section durations (micros())
  - Optional CPU time spent in the main loop (derived from micros over sections)

- How to capture them (hooks)
  - Heap: read ESP.getFreeHeap() at key points (loop start, loop end, and critical sections)
  - Stack: read uxTaskGetStackHighWaterMark(NULL) for the current task and, if available, for main sub-tasks
  - Timing: capture micros() at start/end of loop and per-section blocks; compute deltas
  - Keep overhead small by recording only delta values and periodically emitting results

- Where to add instrumentation
  - Main loop: at the top and bottom of each iteration
  - Dose events: on start and completion of each dosing operation
  - Display updates: before and after render/refresh of the OLED/dashboard

- Build flags to enable/disable
  - Wrap instrumentation blocks with #ifdef PROFILING ... #endif
  - Enable by adding PROFILING to the build defines (e.g., platformio.ini or CMake defines)
  - Optional: use PROFILING_LEVEL to control detail (1 = essentials, 2 = per-section)

- Simple output format for results
  - Serial console output as a single line per measurement window
  - Example line (compact):
    [PROFILE] loop_us=1234 heap_free=56789 stack_main_mw=1024,sec=0.5
  - CSV-friendly alternative for log aggregation:
    1234,56789,1024,0.50

- Validation approach
  - Build with PROFILING defined; verify no functional changes when disabled
  - Run a short scenario and confirm serial profiling output appears at expected cadence
  - Ensure outputs are concise enough to not perturb real-time behavior when enabled
