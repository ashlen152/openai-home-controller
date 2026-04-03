# Quick Reference Commands

## Development Commands

### Server (NestJS)
```bash
# Start development server with watch
npm run server:dev

# Start with mDNS (access at http://openai.local:3000)
npm run server:mdns

# Build production
npm run server:build

# Run tests
npm run server:test
```

### Firmware (ESP32)
```bash
# Build firmware
cd apps/firmware && pio run

# Upload to ESP32
cd apps/firmware && pio run --target upload

# Serial monitor
cd apps/firmware && pio device monitor

# Run unit tests
cd apps/firmware && pio test -e native
```

### Both
```bash
# Install dependencies
npm install

# Docker for MongoDB
npm run docker:dev
```

## API Endpoints

### Pump Settings
- `GET /api/pump-settings` - List all pumps
- `GET /api/pump-settings/:pumpId` - Get pump settings
- `POST /api/pump-settings` - Create/update settings

### Commands
- `POST /api/pump-commands/calibrate/start` - Start calibration
- `POST /api/pump-commands/calibrate/save` - Save calibration
- `POST /api/pump-commands/test-dose` - Test dose
- `POST /api/pump-commands/:pumpId/complete` - Complete command

### Dose Events
- `POST /api/dose-events` - Log dose event
- `GET /api/dose-events/:pumpId` - Get dose history

### Health
- `GET /api/health` - Server health check

## Key Files

### Server
- `apps/server/src/main.ts` - Entry point
- `apps/server/src/pump/` - Pump API module

### Firmware
- `apps/firmware/src/main.cpp` - Entry point
- `apps/firmware/lib/ApiClient/` - HTTP client
- `apps/firmware/lib/RemoteCommandManager/` - Command parser
