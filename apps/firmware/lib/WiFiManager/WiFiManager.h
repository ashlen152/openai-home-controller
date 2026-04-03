/**
 * @file WiFiManager.h
 * @brief WiFi connectivity, HTTP client, and NTP time management for ESP32.
 *
 * Handles WiFi connection/reconnection, HTTP REST methods (GET/POST/PUT/DELETE),
 * NTP time synchronization (Asia/Vietnam UTC+7), and signal strength monitoring.
 *
 * Singleton pattern - access via WiFiManager::getInstance().
 *
 * Server: Configured via WifiConfig.h (from .env or defaults)
 * NTP: Asia pool servers, timezone ICT-7 (Vietnam UTC+7)
 * HTTP timeout: 1 second
 * NTP re-sync: every hour
 *
 * @note All WiFi operations run on Core 0 via NetworkTaskManager (non-blocking)
 */

#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include "../../include/WifiConfig.h"  // For serverAddress and serverPort

class WiFiManager
{
private:
  const char *_ssid;
  const char *_password;
  const int MAX_ATTEMPTS = 20;              ///< Max WiFi connection retry attempts
  String _serverAddress;                    ///< Backend server IP (from WifiConfig.h)
  int _port;                                ///< Backend server port (from WifiConfig.h)
  const int HTTP_TIMEOUT = 5000;            ///< HTTP request timeout (ms)
  const int MIN_RSSI = -80;                 ///< Minimum acceptable RSSI (dBm)
  WiFiClient wifiClient;                    ///< Underlying WiFi client
  HttpClient *httpClient = nullptr;         ///< HTTP client, initialized on first connect

  unsigned long lastTimeSync = 0;           ///< Last NTP sync timestamp (millis)
  unsigned long lastTimeUpdate = 0;         ///< Last time string update (millis)
  time_t lastSyncedTime = 0;               ///< Epoch time from last NTP sync
  const int TIME_SYNC_INTERVAL = 3600000;   ///< NTP re-sync interval: 1 hour (ms)
  const int TIME_UPDATE_INTERVAL = 1000;    ///< Display time update interval: 1 sec (ms)
  bool timeInitialized = false;             ///< True after first successful NTP sync

public:
  static WiFiManager &getInstance();

  ~WiFiManager(); // Destructor to clean up
  bool connect(char const *ssid, char const *password);
  bool isConnected();
  void disconnect();
  int getSignalStrength();

  // HTTP Methods
  bool get(const char *path, String &response);
  bool post(const char *path, const char *contentType, const char *body, String &response);
  bool put(const char *path, const char *contentType, const char *body, String &response);
  bool del(const char *path, String &response);

  bool checkApiHealth();
  void configureTime(const char *ntpServer = "pool.ntp.org", const char *timezone = "UTC");
  const char *getCurrentTime();
  
  bool connect();
  void syncTime();
  time_t getCurrentTimeEpoch();

  // DEPRECATED: Use ApiClient instead for HTTP APIs
  // These methods are kept for backward compatibility only
  bool checkServerHealth();
  bool getPumpSettings(String &response);
  bool updatePumpSettings(const String &payload);
  bool postDoseLog(const String &payload);
  bool getCommands(String &response);
  bool completeCommand(const String &payload);

private:
  WiFiManager(); // private constructor
  WiFiManager(const WiFiManager &) = delete;
  WiFiManager &operator=(const WiFiManager &) = delete;
  static char timeStr[9]; // HH:MM:SS + null terminator
};

#endif