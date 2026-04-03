# Project Memory Skill

## About This Skill
This skill provides context about the SmartPump project for AI assistants.

## Project Structure

### Server (NestJS)
```
apps/server/src/
├── main.ts                    # Entry point
├── app.module.ts              # Root module
├── pump/                      # Pump API (firmware integration)
│   ├── controllers/           # HTTP endpoints
│   │   ├── pump-settings.controller.ts
│   │   ├── pump-commands.controller.ts
│   │   ├── dose-events.controller.ts
│   │   ├── calibrate.controller.ts
│   │   └── test-dose.controller.ts
│   ├── services/              # Business logic
│   │   ├── pump-settings.service.ts
│   │   ├── pump-commands.service.ts
│   │   ├── command-handlers.service.ts
│   │   └── dose-events.service.ts
│   ├── dto/                   # Data transfer objects
│   │   ├── command-payloads.dto.ts
│   │   └── create-*.dto.ts
│   └── schemas/               # MongoDB schemas
│       ├── pump-setting.schema.ts
│       ├── pump-command.schema.ts
│       └── dose-event.schema.ts
├── rag/                       # RAG search
├── crawler/                   # Web scraping
├── mangas/                    # Manga tracker
├── auth/                      # Authentication
├── health/                    # Health checks
└── config/                   # Configuration
```

### Firmware (ESP32)
```
apps/firmware/
├── platformio.ini             # PlatformIO config
├── src/main.cpp               # Entry point
├── lib/                       # Hardware managers (singletons)
│   ├── PumpController/        # Stepper motor control
│   ├── DisplayManager/        # OLED display
│   ├── WiFiManager/          # WiFi + HTTP
│   ├── NetworkTaskManager/    # Core 0 network task
│   ├── ApiClient/             # Type-safe HTTP client
│   ├── RemoteCommandManager/  # Command parsing
│   ├── AutoDosingManager/    # Dosing scheduler
│   ├── ConfigManager/         # Pump ID management
│   └── EEPROMManager/         # Wear-leveling EEPROM
├── src/                       # Application logic
│   ├── CalibrateDosingController/
│   ├── TestDosingController/
│   ├── ManualDosingController/
│   ├── ViewController/        # UI screens
│   │   ├── Home/
│   │   ├── Menu/
│   │   └── Manual/
│   ├── ButtonController/
│   └── DisplayController/
└── include/                   # Config headers
    ├── Config.h               # Pin assignments
    ├── ButtonConfig.h
    └── WifiConfig.h
```

## Coding Conventions

### NestJS (TypeScript)
- **Naming**: camelCase for variables, PascalCase for classes
- **Structure**: controller → service → schema (flow)
- **DTOs**: Use class-validator for validation
- **Modules**: Feature-based with clear imports

### ESP32 (C++)
- **Naming**: camelCase for functions/variables, PascalCase for classes
- **Singletons**: All managers use `getInstance()`
- **Pattern**: `m_` prefix for member variables
- **JSON**: Use JsonDocument (ArduinoJson), NOT string parsing

## Important Patterns

### Server - Command Flow
1. Client calls `/api/pump-commands/calibrate/start`
2. Command saved to MongoDB with status `pending`
3. ESP32 polls `/api/pump-commands/{pumpId}`
4. ESP32 executes command, calls `/complete`
5. Server updates command status to `completed`

### Firmware - Dual Core
- **Core 0**: NetworkTaskManager (WiFi, HTTP, NTP)
- **Core 1**: Main loop (pump, display, buttons)
- Communication via FreeRTOS queues

### API Client (Firmware)
```cpp
// Use ApiClient for HTTP calls (type-safe)
JsonDocument result = ApiClient::getInstance().getPumpSettings(pumpId);
if (result["success"] == true) {
    float stepsPerML = result["data"]["stepsPerML"].as<float>();
}
```

## Testing

### Server
- Uses Jest
- Run: `npm run server:test`

### Firmware
- Uses PlatformIO native tests
- Run: `pio test -e native`

### Debugging
- **Server**: Use OpenCode MCP dev-browser tools
- **Firmware**: Use `pio device monitor` or `log.py` script

## Key Constraints
- NO puppeteer - use OpenCode browser tools
- Use JsonDocument for JSON parsing (ESP32)
- All managers are singletons
- API endpoints return `{ success, data, error }` format
