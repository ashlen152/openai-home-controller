# AGENTS.md

## Build, Lint, and Test Commands

- **Build**: `platformio run`
- **Upload**: `platformio run --target upload`
- **Monitor Serial**: `platformio device monitor`
- **Clean**: `platformio run --target clean`
- **Testing**: No explicit test framework is configured. Add tests under `test/` and configure PlatformIO's testing capabilities.

## Code Style Guidelines

- **Imports**:
  - Use `#include` for standard and library headers.
  - Group standard headers first, followed by library headers.

- **Formatting**:
  - Follow Arduino-style indentation (2 spaces per level).
  - Use braces `{}` for all control structures, even single-line blocks.

- **Types**:
  - Prefer `int`, `float`, and `bool` for simplicity.
  - Use `size_t` for sizes and indices.

- **Naming Conventions**:
  - Use camelCase for functions and variables.
  - Use PascalCase for classes.

- **Error Handling**:
  - Use `Serial.print` for debugging.
  - Implement error codes or flags for critical failures.

## Notes for Agents

- Ensure all changes are tested on the target hardware (ESP32-S3).
- Document any new dependencies or libraries added to `platformio.ini`.
- Maintain consistency with the existing code style and structure.