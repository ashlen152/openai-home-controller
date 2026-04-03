# Project Context

## Overview
SmartPump Monorepo - ESP32-based peristaltic pump controller with NestJS backend

## Tech Stack
- **Server**: NestJS + MongoDB + TypeScript
- **Firmware**: ESP32 + PlatformIO + C++ (Arduino)
- **Testing**: Jest (server), PlatformIO native tests (firmware)
- **Browser Automation**: OpenCode MCP tools (NOT puppeteer)

## Key Files
- `AGENTS.md` - Full project documentation
- `apps/server/` - NestJS backend
- `apps/firmware/` - ESP32 firmware

## Important Notes
- NEVER use puppeteer - use OpenCode browser tools instead
- ESP32 uses dual-core: Core 0 (network), Core 1 (pump/display)
- Server API: http://openai.local:3000/api/*
- mDNS hostname: openai.local

## Commands
```bash
# Server
npm run server:dev      # Start server with watch
npm run server:build   # Build server
npm run server:test     # Run tests

# Firmware
pio run                # Build firmware
pio run --target upload # Upload to ESP32
pio device monitor     # Serial monitor
pio test -e native    # Run unit tests
```

## Recent Changes
- Refactored to use ApiClient (type-safe JsonDocument)
- Command handlers separated: CalibrateController, TestDoseController
- RemoteCommandManager uses JsonDocument parsing (no string indexOf)
- Added SAVE_SETTINGS command for EEPROM sync on web edit
- Created UpdatePumpSettingsDto (partial update, don't reuse CreatePumpSettingsDto)
- Added GlobalExceptionFilter + LoggingInterceptor for full request/failure logging
- Fixed dayPercent reporting bug (was double-multiplying by 100)
- dayPercent: server stores 0-100, ESP32 stores uint8_t 0-100, divides by 100 for calculation
- Refactored to monorepo structure (apps/server + apps/firmware)

## NestJS Controller Convention

**IMPORTANT**: Controllers should NOT include `/api` prefix in `@Controller()` decorator.

The global prefix `/api` is set in `main.ts` via `app.setGlobalPrefix('api')`. Adding `/api` to controller path results in double `/api/api/...` paths.

**Correct**:
```typescript
@Controller('serial')  // Results in /api/serial ✅
```

**Incorrect**:
```typescript
@Controller('api/serial')  // Results in /api/api/serial ❌
```
