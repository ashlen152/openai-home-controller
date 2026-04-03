/**
 * @file WiFiSync.cpp
 * @brief WiFi connection management and data synchronization.
 *
 * DEPRECATED: This file is kept for backwards compatibility only.
 * Network operations now run on Core 0 via NetworkTaskManager.
 * 
 * The handleWiFi() and syncData() functions are now DEPRECATED and should
 * not be called from main.cpp. All WiFi/HTTP/NTP operations are handled
 * automatically by the NetworkTaskManager running on Core 0.
 *
 * @note Uses hardcoded credentials from WifiConfig.h (ssid, password).
 *       These should come from build flags instead. See AGENTS.md Known Issues #2.
 */

#include "WiFiSync.h"
#include <DisplayManager.h>
#include <ArduinoJson.h>
#include <NetworkTaskManager.h>
// #include "PumpController.h"
#include <WiFiManager.h>
#include <WifiConfig.h>
#include <Config.h>

static DisplayManager &display = DisplayManager::getInstance();
static WiFiManager &wifi = WiFiManager::getInstance();
static NetworkTaskManager &networkTask = NetworkTaskManager::getInstance();

/**
 * @brief DEPRECATED: Use NetworkTaskManager instead
 * 
 * This function sends pump telemetry via queue to Core 0 for processing.
 * Direct WiFi access from Core 1 is deprecated.
 */
void syncData()
{
  // Build JSON payload
  JsonDocument doc;
  int rssi = wifi.getSignalStrength();
  doc["rssi"] = rssi;
  
  // Update display signal strength
  display.setSignalStrength(rssi);
  
  String jsonData;
  serializeJson(doc, jsonData);
  
  // Send HTTP POST command to Core 0 network task
  NetworkCommandMessage cmd;
  cmd.command = NetworkCommand::HTTP_POST_SETTINGS;
  cmd.param1 = 0;
  cmd.param2 = 0;
  strncpy(cmd.data, jsonData.c_str(), sizeof(cmd.data) - 1);
  cmd.data[sizeof(cmd.data) - 1] = '\0';
  
  if (networkTask.sendCommand(cmd, 0)) {
    Serial.println("[WiFiSync] Data sync command queued to Core 0");
  } else {
    Serial.println("[WiFiSync] Failed to queue data sync command");
  }
}

/**
 * @brief DEPRECATED: Use NetworkTaskManager instead
 * 
 * This function is no longer needed. WiFi connection is now handled
 * automatically by the NetworkTaskManager background tasks on Core 0.
 * 
 * The network task auto-reconnects every 5 seconds if disconnected.
 * Time sync happens every hour automatically.
 * 
 * @param currentTime Current millis() timestamp
 * @param lastWiFiRetryTime Last retry timestamp (unused, kept for API compat)
 */
void handleWiFi(unsigned long currentTime, unsigned long &lastWiFiRetryTime)
{
  // This function is deprecated and does nothing.
  // All WiFi operations are now handled by Core 0 NetworkTaskManager.
  // The network task performs automatic reconnection every 5 seconds
  // and time sync every hour via background tasks.
  
  // Note: Display updates for WiFi status are now handled via responses
  // from NetworkTaskManager in main.cpp loop().
  
  (void)currentTime;          // Suppress unused parameter warning
  (void)lastWiFiRetryTime;    // Suppress unused parameter warning
}