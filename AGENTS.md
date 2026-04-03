# SmartPump Monorepo — AGENTS.md

## Project Overview

A monorepo containing two tightly coupled projects:

| App          | Path             | Tech                               | Purpose                                                            |
| ------------ | ---------------- | ---------------------------------- | ------------------------------------------------------------------ |
| **Server**   | `apps/server/`   | NestJS + MongoDB + Node.js         | Backend API for pump control, manga tracking, RAG search           |
| **Firmware** | `apps/firmware/` | PlatformIO + ESP32 + C++ (Arduino) | Peristaltic pump controller firmware with OLED UI, WiFi, dual-core |

The firmware calls the server's REST API for settings sync and dose event logging.

---

## Monorepo Structure

```
openai-workflow/
├── package.json              # Root workspace config (npm workspaces)
├── .gitignore                # Shared ignores
├── AGENTS.md                 # This file
├── apps/
│   ├── server/               # NestJS server (formerly standalone openai-workflow)
│   │   ├── package.json      # Server deps + scripts
│   │   ├── src/              # NestJS modules (pump, mangas, rag, crawler, etc.)
│   │   ├── test/             # Jest tests
│   │   ├── scripts/          # CLI scripts (tiktok crawler, etc.)
│   │   ├── public/           # Static assets (dashboard UI)
│   │   ├── Dockerfile*       # Container configs
│   │   └── docker-compose*.yml
│   └── firmware/             # ESP32 firmware (formerly SmartPump)
│       ├── platformio.ini    # PlatformIO build config
│       ├── src/              # Main entry + controllers
│       ├── lib/              # Hardware managers (PumpController, WiFiManager, etc.)
│       ├── include/          # Config headers (pins, WiFi, constants)
│       ├── test/             # Native unit tests
│       └── load_env.py       # Pre-build env loader
└── .sisyphus/                # Agent planning (auto-generated)
```

---

## How the Two Apps Communicate

### API Contract (Firmware → Server)

The ESP32 firmware makes HTTP calls to the NestJS server. Key endpoints:

| Endpoint                      | Method | Direction   | Purpose                             |
| ----------------------------- | ------ | ----------- | ----------------------------------- |
| `/api/health`                 | GET    | FW → Server | Health check (every 3 min)          |
| `/api/pump-settings/{pumpId}` | GET    | FW → Server | Fetch pump settings on startup      |
| `/api/pump-settings`          | POST   | FW → Server | Update settings after config change |
| `/api/dose-events`            | POST   | FW → Server | Log dose events (start + complete)  |

### Server → Firmware (Future)

The `pump-commands` module on the server supports push commands to firmware via queued commands that the firmware polls.

### Shared Configuration

- **Server address**: Configured in `apps/firmware/platformio.ini` (`SERVER_ADDRESS`, `SERVER_PORT`)
- **Pump ID**: Set via firmware menu, stored in EEPROM, used as API path param
- **WiFi credentials**: Build flags in `platformio.ini`, loaded via `load_env.py`

---

## Working with This Monorepo

### Installing Dependencies

```bash
# From repo root — installs all workspace packages
npm install
```

### Running the Server

```bash
# From repo root
npm run server:dev          # Watch mode
npm run server:mdns         # With mDNS (access at http://openai.local:3000)
npm run server:build        # Production build
npm run server:test         # Run Jest tests
npm run server:test:e2e     # E2E tests

# Or from apps/server/
cd apps/server && npm run start:dev
```

### Building the Firmware

```bash
# From apps/firmware/
cd apps/firmware
pio run                     # Build
pio run --target upload     # Upload to ESP32
pio device monitor          # Serial monitor
pio test -e native          # Run unit tests
```

### Docker (Server)

```bash
npm run docker:dev          # Start MongoDB + Redis for dev
npm run docker:test         # Start test infrastructure
```

---

## Cross-App Development Workflow

When making changes that affect **both** apps:

1. **API changes**: Update server endpoints first, then update firmware WiFiManager calls
2. **DTO/schema changes**: Update server DTOs, then update firmware JSON payloads in WiFiManager.cpp
3. **Testing**: Test server API with curl/Postman, then flash firmware for end-to-end test
4. **Server address**: The firmware has the server IP hardcoded in `platformio.ini` — update when server IP changes

### Key Files for Cross-App Changes

| Change Type         | Server File                         | Firmware File                                                 |
| ------------------- | ----------------------------------- | ------------------------------------------------------------- |
| Add API endpoint    | `apps/server/src/pump/controllers/` | `apps/firmware/lib/WiFiManager/WiFiManager.cpp`               |
| Change request body | `apps/server/src/pump/dto/`         | `apps/firmware/lib/WiFiManager/WiFiManager.cpp`               |
| Change response     | `apps/server/src/pump/services/`    | `apps/firmware/lib/NetworkTaskManager/NetworkTaskManager.cpp` |
| Health check        | `apps/server/src/health/`           | `apps/firmware/lib/WiFiManager/WiFiManager.cpp`               |

---

## Important Notes

- **DO NOT use puppeteer** in the server — it uses OpenCode browser tools instead
- **Firmware .git/**: The firmware has its own git history. Consider whether to keep it or squash into monorepo git
- **node_modules**: Managed by npm workspaces at root level. `apps/server/node_modules` will be symlinked
- **MongoDB data**: Stored in `apps/server/data/mongo/` — excluded from git
- **PlatformIO**: Firmware builds independently — no npm dependency

---

## Quick Reference

### Server Modules

- `pump/` — Pump settings, dose events, pump commands (API for firmware)
- `mangas/` — Manga reader & tracker
- `crawler/` — Web scraping (AsuraScans, etc.)
- `rag/` — RAG-powered search with OpenAI/Ollama
- `auth/` — Authentication
- `scheduler/` — Scheduled chapter crawling
- `workflows/` — Workflow automation
- `logs/` — Logging endpoint
- `health/` — Health check

### Firmware Libraries

- `PumpController/` — Stepper motor control (TMC2209 + AccelStepper)
- `WiFiManager/` — WiFi, HTTP client, NTP, API calls
- `NetworkTaskManager/` — Core 0 network task (dual-core)
- `DisplayManager/` — SSD1306 OLED display
- `AutoDosingManager/` — Schedule-based auto-dosing
- `ConfigManager/` — Pump ID management
- `EEPROMManager/` — Wear-leveling EEPROM storage
