# Firmware Config Environment Migration

## TL;DR
> **Summary**: Convert hardcoded USB serial port and server address to environment-driven config with DNS-first + IP fallback.
> **Deliverables**: 
> - UPLOAD_PORT env var for USB serial in Python scripts
> - DNS-first then 192.168.68.160 fallback in ApiClient.cpp
> - Keep load_env.py for .env loading
> **Effort**: Medium
> **Parallel**: YES - 2 waves
> **Critical Path**: Define env interface → Implement scripts → Implement DNS fallback → Test

## Context
### Original Request
- Change hardcoded `usbserial-1130` to use env for easier change later
- Server API: use DNS (openai.local) when fail to connect use IP fallback 192.168.68.160 - all from config/env
- Keep load_env.py for .env loading (don't change)

### Interview Summary
- USB serial port: Use `UPLOAD_PORT` env var with default `/dev/cu.usbserial-1130`
- Server address: DNS first (`openai.local`), then fallback to `192.168.68.160`
- .env loading: Keep existing `load_env.py` approach

### Metis Review (gaps addressed)
- **Edge cases**: Empty UPLOAD_PORT handling, DNS timeout/retry policy
- **Guardrails**: Keep changes isolated to config handling, preserve load_env.py behavior
- **Scope creep risk**: Avoid broader DNS caching strategies, keep port discovery simple

## Work Objectives
### Core Objective
Migrate hardcoded configuration values to environment-driven approach for easier maintenance and deployment.

### Deliverables
1. Python scripts (`log_all.py`, `log_40_messages.py`) read USB port from `UPLOAD_PORT` env var
2. `ApiClient.cpp` implements DNS-first server resolution with IP fallback
3. `.env` file updated to include new environment variables
4. Documentation updated with new configuration options

### Definition of Done (verifiable conditions)
- [ ] `log_all.py` uses `UPLOAD_PORT` env var with default fallback
- [ ] `log_40_messages.py` uses `UPLOAD_PORT` env var with default fallback
- [ ] `ApiClient.cpp` attempts DNS resolution before falling back to IP
- [ ] `.env` contains `SERVER_DNS_NAME=openai.local` and `SERVER_IP=192.168.68.160`
- [ ] Unit tests pass for both USB port and DNS fallback logic

### Must Have
- Environment variable support with sensible defaults
- DNS-first resolution with IP fallback logic
- Backward compatibility when env vars are not set

### Must NOT Have
- No changes to server-side API code
- No breaking changes to existing functionality
- No removal of load_env.py approach

## Verification Strategy
- **Test decision**: tests-after (existing test infrastructure in `test/native`)
- **QA policy**: Every task has agent-executed scenarios
- **Evidence**: `.sisyphus/evidence/` files for each task

## Execution Strategy
### Parallel Execution Waves

Wave 1 (Foundation - can run in parallel):
- Update load_env.py to support new env vars (UPLOAD_PORT, SERVER_DNS_NAME)
- Modify log_all.py and log_40_messages.py to use env var
- Add DNS fallback logic to ApiClient.cpp

Wave 2 (Verification):
- Update .env file with new configuration
- Add/update unit tests
- Update documentation

### Dependency Matrix
| Task | Blocks | Blocked By |
|------|--------|------------|
| 1. Update load_env.py | 2, 3 | - |
| 2. Modify log scripts | 5 | 1 |
| 3. DNS fallback in ApiClient | 5 | - |
| 4. Update .env | - | - |
| 5. Add tests | - | 2, 3 |
| 6. Update docs | - | 5 |

### Agent Dispatch Summary
- Wave 1: 3 tasks (config loader, script updates, API client)
- Wave 2: 3 tasks (.env update, tests, docs)

## TODOs

- [x] 1. Update load_env.py to support UPLOAD_PORT and SERVER_DNS_NAME

  **What to do**: Extend load_env.py to read additional environment variables: UPLOAD_PORT (for USB serial), SERVER_DNS_NAME (for DNS server name). Add to CPPDEFINES output.

  **Must NOT do**: Remove existing SERVER_ADDRESS/SERVER_PORT handling - keep for fallback.

  **Recommended Agent Profile**:
  - Category: `quick` - straightforward modification to existing script
  - Skills: [`context-inject`, `coding-conventions`]
  - Omitted: `project-memory` - not needed for simple config script

  **Parallelization**: Can Parallel: YES | Wave 1 | Blocks: 2 | Blocked By: -

  **References**:
  - Pattern: `apps/firmware/load_env.py:existing_cppdefines` - follow existing structure
  - Test: Use existing `.env` to verify new vars are read

  **Acceptance Criteria**:
  - [ ] load_env.py outputs UPLOAD_PORT as CPP define when set
  - [ ] load_env.py outputs SERVER_DNS_NAME as CPP define when set

  **QA Scenarios**:
  ```
  Scenario: load_env.py reads new env vars
    Tool: Bash
    Steps: |
      cd apps/firmware
      export UPLOAD_PORT=/dev/cu.usbserial-1130
      export SERVER_DNS_NAME=openai.local
      python load_env.py
    Expected: Output includes -DUPLOAD_PORT=\"/dev/cu.usbserial-1130\" and -DSERVER_DNS_NAME=\"openai.local\"
    Evidence: .sisyphus/evidence/task-1-load-env-output.txt
  ```

  **Commit**: YES | Message: `feat(config): add UPLOAD_PORT and SERVER_DNS_NAME to load_env.py` | Files: [apps/firmware/load_env.py]

- [x] 2. Modify log_all.py to use UPLOAD_PORT env var

  **What to do**: Replace hardcoded `/dev/cu.usbserial-1130` with `os.environ.get('UPLOAD_PORT', '/dev/cu.usbserial-1130')`

  **Must NOT do**: Remove the default fallback - maintain backward compatibility.

  **Recommended Agent Profile**:
  - Category: `quick` - simple env var usage
  - Skills: [`context-inject`, `coding-conventions`]

  **Parallelization**: Can Parallel: YES | Wave 1 | Blocks: 5 | Blocked By: 1

  **References**:
  - Pattern: `apps/firmware/log_all.py:PORT` - current hardcoded value at line 6
  - API: Python `os.environ.get()` for env var reading

  **Acceptance Criteria**:
  - [ ] log_all.py reads UPLOAD_PORT from environment
  - [ ] Falls back to /dev/cu.usbserial-1130 when env var not set

  **QA Scenarios**:
  ```
  Scenario: log_all.py uses UPLOAD_PORT env var
    Tool: Bash
    Steps: |
      cd apps/firmware
      unset UPLOAD_PORT
      python -c "import os; print(os.environ.get('UPLOAD_PORT', '/dev/cu.usbserial-1130'))"
    Expected: /dev/cu.usbserial-1130

  Scenario: log_all.py uses custom port from env
    Tool: Bash
    Steps: |
      cd apps/firmware
      export UPLOAD_PORT=/dev/cu.custom
      python -c "import os; print(os.environ.get('UPLOAD_PORT', '/dev/cu.usbserial-1130'))"
    Expected: /dev/cu.custom
    Evidence: .sisyphus/evidence/task-2-log-all-env.txt
  ```

  **Commit**: YES | Message: `feat(script): read UPLOAD_PORT from env in log_all.py` | Files: [apps/firmware/log_all.py]

- [x] 3. Modify log_40_messages.py to use UPLOAD_PORT env var

  **What to do**: Replace hardcoded `/dev/cu.usbserial-1130` with `os.environ.get('UPLOAD_PORT', '/dev/cu.usbserial-1130')`

  **Must NOT do**: Remove the default fallback.

  **Recommended Agent Profile**:
  - Category: `quick` - simple env var usage
  - Skills: [`context-inject`, `coding-conventions`]

  **Parallelization**: Can Parallel: YES | Wave 1 | Blocks: 5 | Blocked By: 1

  **References**:
  - Pattern: `apps/firmware/log_40_messages.py:serial.Port` - current hardcoded at line 9

  **Acceptance Criteria**:
  - [ ] log_40_messages.py reads UPLOAD_PORT from environment
  - [ ] Falls back to /dev/cu.usbserial-1130 when env var not set

  **QA Scenarios**:
  ```
  Scenario: log_40_messages.py uses env var with fallback
    Tool: Bash
    Steps: |
      cd apps/firmware
      python -c "import os; print(os.environ.get('UPLOAD_PORT', '/dev/cu.usbserial-1130'))"
    Expected: /dev/cu.usbserial-1130 (default)
    Evidence: .sisyphus/evidence/task-3-log-messages-env.txt
  ```

  **Commit**: YES | Message: `feat(script): read UPLOAD_PORT from env in log_40_messages.py` | Files: [apps/firmware/log_40_messages.py]

- [x] 4. Implement DNS-first + IP fallback in ApiClient.cpp

  **What to do**: Modify ApiClient to attempt DNS resolution using SERVER_DNS_NAME (from env/load_env.py), and if DNS fails, fallback to SERVER_IP (192.168.68.160). Add timeout for DNS attempt.

  **Must NOT do**: Remove existing SERVER_ADDRESS usage completely - use as fallback. Don't change HTTP client initialization beyond address resolution.

  **Recommended Agent Profile**:
  - Category: `deep` - requires understanding of network resolution logic
  - Skills: [`context-inject`, `coding-conventions`]
  - Omitted: `project-memory` - existing patterns well documented

  **Parallelization**: Can Parallel: YES | Wave 1 | Blocks: 5 | Blocked By: -

  **References**:
  - Pattern: `apps/firmware/lib/ApiClient/ApiClient.cpp:fullUrl` - current URL construction at line with `String fullUrl = String("http://") + _serverAddress`
  - Config: `apps/firmware/include/WifiConfig.h` - SERVER_ADDRESS and SERVER_PORT defines
  - Env: `apps/firmware/.env` - server configuration

  **Acceptance Criteria**:
  - [ ] ApiClient attempts DNS resolution using SERVER_DNS_NAME
  - [ ] Falls back to SERVER_IP (192.168.68.160) if DNS fails
  - [ ] Logs which address was chosen for debugging

  **QA Scenarios**:
  ```
  Scenario: DNS resolution succeeds
    Tool: interactive_bash (or code inspection)
    Steps: |
      - Review ApiClient.cpp code to verify DNS resolution logic added
      - Verify SERVER_DNS_NAME macro is used
    Expected: Code includes DNS resolution attempt before fallback

  Scenario: DNS fallback to IP
    Tool: interactive_bash
    Steps: |
      - Review fallback logic in ApiClient.cpp
      - Verify SERVER_IP (192.168.68.160) is used when DNS fails
    Expected: Fallback logic present
    Evidence: .sisyphus/evidence/task-4-dns-fallback.txt
  ```

  **Commit**: YES | Message: `feat(network): add DNS-first with IP fallback in ApiClient` | Files: [apps/firmware/lib/ApiClient/ApiClient.cpp]

- [x] 5. Update .env file with new configuration

  **What to do**: Add SERVER_DNS_NAME=openai.local and SERVER_IP=192.168.68.160 to .env file.

  **Must NOT do**: Remove existing SERVER_ADDRESS - needed for backward compatibility.

  **Recommended Agent Profile**:
  - Category: `quick` - simple config file edit
  - Skills: []

  **Parallelization**: Can Parallel: YES | Wave 2 | Blocks: - | Blocked By: -

  **References**:
  - Pattern: `apps/firmware/.env` - existing env vars

  **Acceptance Criteria**:
  - [ ] .env contains SERVER_DNS_NAME=openai.local
  - [ ] .env contains SERVER_IP=192.168.68.160
  - [ ] Existing SERVER_ADDRESS preserved for backward compatibility

  **QA Scenarios**:
  ```
  Scenario: .env contains new DNS config
    Tool: Bash
    Steps: grep -E "SERVER_DNS_NAME|SERVER_IP" apps/firmware/.env
    Expected: Both variables present with correct values
    Evidence: .sisyphus/evidence/task-5-env-update.txt
  ```

  **Commit**: YES | Message: `chore(config): add SERVER_DNS_NAME and SERVER_IP to .env` | Files: [apps/firmware/.env]

- [ ] 6. Add unit tests for env-driven config

  **What to do**: Add unit tests for:
  - UPLOAD_PORT env var reading in Python scripts (can test in Python)
  - DNS fallback logic in ApiClient (can test with mock or inspection)

  **Must NOT do**: Create complex test infrastructure - use existing native test framework.

  **Recommended Agent Profile**:
  - Category: `quick` - straightforward test additions
  - Skills: [`git-workflow`]

  **Parallelization**: Can Parallel: YES | Wave 2 | Blocks: - | Blocked By: 2, 3, 4

  **References**:
  - Pattern: `apps/firmware/test/test_config/test_config_manager.cpp` - existing test structure

  **Acceptance Criteria**:
  - [ ] Tests for env var reading in Python scripts pass
  - [ ] Tests for DNS fallback logic exist (or code inspection verification)

  **QA Scenarios**:
  ```
  Scenario: Run existing tests to verify no regression
    Tool: Bash
    Steps: cd apps/firmware && pio test -e native
    Expected: All existing tests pass
    Evidence: .sisyphus/evidence/task-6-tests.txt
  ```

  **Commit**: YES | Message: `test(config): add tests for env-driven config` | Files: [apps/firmware/test/]

- [x] 7. Update README.md documentation

  **What to do**: Update README.md to document:
  - UPLOAD_PORT environment variable for USB serial port
  - SERVER_DNS_NAME and SERVER_IP for server address configuration
  - How to override defaults

  **Must NOT do**: Remove existing documentation about server address.

  **Recommended Agent Profile**:
  - Category: `writing` - documentation update
  - Skills: []

  **Parallelization**: Can Parallel: YES | Wave 2 | Blocks: - | Blocked By: 5

  **References**:
  - Pattern: `apps/firmware/README.md` - existing documentation structure

  **Acceptance Criteria**:
  - [ ] README documents UPLOAD_PORT env var
  - [ ] README documents SERVER_DNS_NAME and SERVER_IP
  - [ ] Examples show how to override defaults

  **QA Scenarios**:
  ```
  Scenario: Documentation reflects new config options
    Tool: Bash
    Steps: grep -E "UPLOAD_PORT|SERVER_DNS_NAME" apps/firmware/README.md
    Expected: Both documented
    Evidence: .sisyphus/evidence/task-7-docs.txt
  ```

  **Commit**: YES | Message: `docs: document env-driven config options` | Files: [apps/firmware/README.md]

## Final Verification Wave (MANDATORY — after ALL implementation tasks)
> 4 review agents run in PARALLEL. ALL must APPROVE. Present consolidated results to user and get explicit "okay" before completing.

- [ ] F1. Plan Compliance Audit — oracle
- [ ] F2. Code Quality Review — unspecified-high
- [ ] F3. Real Manual QA — unspecified-high (verify scripts work with env vars)
- [ ] F4. Scope Fidelity Check — deep

## Commit Strategy
Atomic commits per task:
- Task 1: `feat(config): add UPLOAD_PORT and SERVER_DNS_NAME to load_env.py`
- Task 2: `feat(script): read UPLOAD_PORT from env in log_all.py`
- Task 3: `feat(script): read UPLOAD_PORT from env in log_40_messages.py`
- Task 4: `feat(network): add DNS-first with IP fallback in ApiClient`
- Task 5: `chore(config): add SERVER_DNS_NAME and SERVER_IP to .env`
- Task 6: `test(config): add tests for env-driven config`
- Task 7: `docs: document env-driven config options`

## Success Criteria
- All Python scripts use UPLOAD_PORT env var with default fallback
- ApiClient.cpp implements DNS-first resolution with IP fallback
- .env contains new configuration variables
- Documentation updated
- Tests pass (or existing tests not broken)