# Smart Pump DIY

This project is a DIY smart pump system built using an ESP32 microcontroller. It includes features such as WiFi connectivity, pump control, and a display interface.

## Features
- WiFi connectivity for remote control and monitoring
- Pump speed and calibration management
- OLED display for real-time status updates
- Integration with REST APIs for data synchronization
- Auto-dosing with day/night schedule
- Server-side settings and dose event logging

---

## Architecture

### Hardware
- **MCU**: ESP32 DevKit v1 (dual-core, WiFi + BT)
- **Stepper Driver**: TMC2209 (UART mode via Serial2)
- **Stepper Motor**: Peristaltic pump motor
- **Display**: SSD1306 128x64 OLED (I2C)
- **Buttons**: 4x physical buttons (Enable, Speed Up, Speed Down, Menu)

### Multi-Core Architecture
- **Core 0**: Network operations (WiFi, HTTP, NTP)
- **Core 1**: Pump control, display, buttons

---

## REST API Integration

### Server Endpoints

The ESP32 firmware communicates with a backend server for settings sync and dose event logging.

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/health` | GET | Health check |
| `/api/pump-settings` | GET | List all pumps |
| `/api/pump-settings/:pumpId` | GET | Get pump settings |
| `/api/pump-settings` | POST | Create/update pump settings |
| `/api/pump-settings/:pumpId` | DELETE | Delete pump |
| `/api/dose-events` | POST | Log dose event |
| `/api/dose-events/:pumpId` | GET | Get dose history |
| `/api/dose-events/:pumpId/today` | GET | Get today's doses |

### Server Configuration

The server address is configured in `platformio.ini`:
```ini
build_flags =
    -DWIFI_SSID=\"SofaHome\"
    -DWIFI_PASSWORD=\"Sofa@123\"
    -DSERVER_ADDRESS=\"192.168.68.109\"
    -DSERVER_PORT=3000
```

### Request/Response DTOs

#### POST /api/pump-settings
```json
// Request
{
  "pumpId": "SmartPump_01",
  "enabled": true,
  "dailyVolume": 30.0,
  "dayStartHour": 8,
  "dayEndHour": 20,
  "dayPercent": 70,
  "stepsPerML": 12800.0,
  "activeProfile": 1,
  "pausedUntil": 0
}

// Response
{ "success": true, "message": "Settings saved", "pumpId": "SmartPump_01" }
```

#### POST /api/dose-events
```json
// Request
{
  "pumpId": "SmartPump_01",
  "eventId": "1709876543000",
  "timestamp": 1709876543,
  "volume": 0.625,
  "status": "started",  // "started", "completed", "failed"
  "success": null,      // null for started, true/false for completed/failed
  "metadata": {
    "totalToday": 12.5,
    "remaining": 17.5,
    "isAuto": true
  }
}

// Response
{ "success": true, "eventId": "1709876543000", "message": "Dose event logged" }
```

---

## Server Setup (NestJS)

The server is located in the `openai-workflow` project under `src/pump/`.

### Project Structure
```
src/pump/
├── controllers/
│   ├── pump-settings.controller.ts
│   └── dose-events.controller.ts
├── services/
│   ├── pump-settings.service.ts
│   └── dose-events.service.ts
├── schemas/
│   ├── pump-setting.schema.ts
│   └── dose-event.schema.ts
└── dto/
    ├── create-pump-settings.dto.ts
    ├── create-dose-event.dto.ts
    └── metadata.dto.ts
```

### Start Server
```bash
cd /path/to/openai-workflow
pnpm install
pnpm start:dev
```

### Running with mDNS (Optional)
```bash
pnpm start:dev -- --mdns
```
This advertises the server as `smartpump.local:3000` on macOS.

---

## Firmware Build & Upload

### 1. Build
```bash
pio run
```

### 2. Upload to ESP32
```bash
pio run --target upload --upload-port /dev/cu.usbserial-1130
```

### 3. Monitor Serial
```bash
pio device monitor --port /dev/cu.usbserial-1130 --baud 115200
```

---

## Usage

1. Power on the ESP32 board.
2. The device will connect to WiFi and sync time.
3. Use the 4 buttons to navigate the menu:
   - **Enable**: Confirm/Select
   - **Menu**: Cancel/Back
   - **Speed Up/Down**: Navigate/Adjust values
4. The OLED display shows current status, time, and dosing info.
5. Auto-dosing runs based on schedule (day/night split).

---

## Menu Options

| # | Menu Item | Description |
|---|-----------|-------------|
| 0 | Dosing Cal | Calibration flow |
| 1 | Settings Info | Display current settings |
| 2 | Auto Dosing | Toggle auto-dosing on/off |
| 3 | Set Daily Vol | Configure daily volume (mL) |
| 4 | Day Period | Set day start/end hours |
| 5 | Day/Night % | Configure day/night split |
| 6 | Set Pump ID | Edit pump identifier |
| 7 | Reset Config | Factory reset |
| 8 | Pause Dosing | Pause for 1h/6h/12h/24h/∞ |
| 9 | Resume Dosing | Resume from pause |
| 10 | Dose History | View today's doses |
| 11 | Speed Profile | Select Slow/Medium/Fast |
| 12 | Edit Profiles | Customize profile speeds |

---

## Troubleshooting

### WiFi Connection Issues
- Check WiFi credentials in `platformio.ini`
- Ensure ESP32 is within range of the WiFi network

### Server Not Reachable
- Verify server is running: `curl http://<SERVER_IP>:3000/api/health`
- Check firewall settings allow local network connections

### Build Errors
- Ensure all dependencies are installed:
  ```bash
  pio lib install
  ```

### ModuleNotFoundError: No module named 'dotenv'
```bash
source ~/.platformio/penv/bin/activate
pip install python-dotenv
deactivate
```

---

## Configuration

### WiFi Credentials
Edit `platformio.ini`:
```ini
build_flags =
    -DWIFI_SSID=\"YourSSID\"
    -DWIFI_PASSWORD=\"YourPassword\"
```

### Server Address
```ini
build_flags =
    -DSERVER_ADDRESS=\"192.168.1.100\"
    -DSERVER_PORT=3000
```

---

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.
