## Wave 1 Plan: EEPROM Wear Leveling & Architecture Review (Task 2 & 3)

## TL;DR
- Focus: 1) Verify EEPROM wear leveling usage and write patterns; 2) Maintainability & Architecture review (coupling, mutex usage, state machines).
- Parallel execution allowed: Tasks 2 & 3 run concurrently.
- Deliverables: concrete findings with file:line references (to be filled after analysis) and a plan for fixes with atomic commits.

## Context
- Firmware: apps/firmware (ESP32, PlatformIO/Arduino)
- Key components to review: lib/EEPROMManager, lib/AutoDosingManager, lib/NetworkTaskManager, lib/WiFiManager, and core integration in src/main.cpp.
- Goal: tighten memory patterns, verify wear-leveling, improve maintainability and architecture clarity.

## Work Objectives

### Task 2 — EEPROM Wear Leveling Status Check
- Objective: Determine if EEPROM wear leveling is currently in use and how writes are performed.
- Approach:
  - Scan for EEPROM write hotspots (EEPROM.put/EEPROM.write/EEPROM.commit) across AutoDosingManager.cpp, PumpController.cpp, DisplayManager.cpp, ConfigManager.cpp, and EEPROMManager.cpp.
  - Identify whether an EEPROM wear-leveling library (EEPROMManager) is integrated and how its API is used.
  - Inventory all write addresses used for important configuration/state data and assess write frequency.
  - Propose an actionable plan to minimize writes or enable wear leveling if not present.
- Deliverables:
  - List of all write patterns with file:line references and quantitative write counts if feasible.
  - Current wear leveling status (Active/Not Active) and rationale.
  - Concrete recommendations with minimal, atomic changes (or a plan for a small refactor).
- Acceptance Criteria:
  - Clear evidence of wear-leveling usage or a concrete plan to enable it.
  - Documentation of hotspots and write minimization strategies.
- QA / Tests:
  - Script to tally write calls (or a manual review) and a recommended unit test for wear-leveling integration.

### Task 3 — Maintainability & Architecture Review
- Objective: Assess coupling between modules, mutex usage, and state machine patterns; surface maintainability improvements.
- Approach:
  - Review inter-module dependencies: PumpController vs DisplayManager vs NetworkTaskManager.
  - Inspect mutex usage around WiFi/Network access in NetworkTaskManager and how Core0/Core1 boundaries are preserved.
  - Audit existing state machines (Display, AutoDosing, Network) for completeness and edge-case handling.
  - Identify any anti-patterns (global state, tight coupling, hard-coded values) and propose refactors.
- Deliverables:
  - 3-5 concrete architecture concerns with file:line references.
  - 2 architectural improvement proposals (include sample code sketch or pseudo-Diff).
- Acceptance Criteria:
  - Documented coupling risk points and concrete improvement plan.
  - At least one concrete refactor sketch that can be implemented atomically.
- QA / Tests:
  - Draft test plan for critical paths that exercise inter-module calls without altering behavior.

## Execution Strategy
- Waves: Wave 1 focuses on Task 2 & Task 3 in parallel.
- Wave 2 would address Task 1 findings and any follow-ups.

## Parallel Execution Plan
- Wave 1: Execute Task 2 and Task 3 concurrently.
- Resource Allocation: 1-2 hours of analysis per task; results will be compiled into the Wave 1 report.

## Output & Artifacts
- Output: Wave 1 Report with file:line references and concrete fixes/diffs.
- Artifacts Location: .sisyphus/plans/wave1-tasks-2-3.md (this file)

## Next Actions
- After Wave 1 completes, consolidate findings and propose a combined plan for Wave 2 (covering Task 1 results and deeper cross-cutting improvements).
