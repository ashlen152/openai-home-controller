/**
 * @file WifiConfig.h
 * @brief WiFi credentials and backend API configuration.
 *
 * WiFi credentials are passed via build flags from platformio.ini:
 *   - WIFI_SSID: WiFi network name (from env variable or default)
 *   - WIFI_PASSWORD: WiFi password (from env variable or default)
 *
 * These are set by load_env.py pre-build script which reads from .env file
 * or uses defaults if .env is not present.
 *
 * API Endpoints:
 *   - /api/pump-settings: POST pump telemetry
 *   - /api/pump-settings/getById: GET current pump settings
 *   - /api/health: Backend health check
 *
 * Intervals:
 *   - WIFI_RETRY_INTERVAL: 5 seconds between connection attempts
 *   - SYNC_INTERVAL: 3 minutes between data sync attempts
 */

#ifndef WIFICONFIG_H
#define WIFICONFIG_H

// WiFi Credentials (from build flags - see platformio.ini and load_env.py)
#ifndef WIFI_SSID
#define WIFI_SSID "DefaultSSID"
#warning "WIFI_SSID not defined, using default. Create .env file with WIFI_SSID=your_network"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "DefaultPassword"
#warning "WIFI_PASSWORD not defined, using default. Create .env file with WIFI_PASSWORD=your_password"
#endif

constexpr const char *ssid = WIFI_SSID;
constexpr const char *password = WIFI_PASSWORD;

// Server Configuration (from build flags - see platformio.ini and load_env.py)
#ifndef SERVER_ADDRESS
#define SERVER_ADDRESS "192.168.68.108"
#warning "SERVER_ADDRESS not defined, using default. Add SERVER_ADDRESS to .env file"
#endif

#ifndef SERVER_PORT
#define SERVER_PORT 3000
#warning "SERVER_PORT not defined, using default. Add SERVER_PORT to .env file"
#endif

constexpr const char *serverAddress = SERVER_ADDRESS;
constexpr const int serverPort = SERVER_PORT;

// Device & API Configuration
constexpr const char *ID_PERISTALTIC_STEPPER = "pump-1";          ///< Device identifier (legacy)
constexpr const char *PUMP_SETTINGS_API = "/api/pump-settings";   ///< POST settings endpoint
constexpr const char *PUMP_BY_ID_API = "/api/pump-settings";      ///< GET settings endpoint
constexpr const char *DOSE_LOG_API = "/api/dose-events";          ///< Dose events endpoint

// Timing Constants
constexpr const int WIFI_RETRY_INTERVAL = 5000;  ///< WiFi reconnect interval (ms)
constexpr const int SYNC_INTERVAL = 180000;      ///< Data sync interval (ms) - 3 minutes

#endif
