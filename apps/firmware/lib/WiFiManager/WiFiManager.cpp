
#include "WiFiManager.h"
#include <DisplayManager.h>
#include <ConfigManager.h>
#include "../../include/WifiConfig.h"

WiFiManager::WiFiManager()
  : _serverAddress(serverAddress), _port(serverPort)
{
  Serial.printf("[WiFiManager] Server: %s:%d\n", _serverAddress.c_str(), _port);
}

WiFiManager &WiFiManager::getInstance()
{
  static WiFiManager instance;
  return instance;
}

WiFiManager::~WiFiManager()
{
  disconnect();
}

bool WiFiManager::connect(char const *ssid, char const *password)
{
  Serial.printf("[WiFi] Connecting to SSID: '%s'\n", ssid);
  Serial.printf("[WiFi] Password length: %d\n", strlen(password));
  
  WiFi.begin(ssid, password);

  DisplayManager::getInstance().showText("Connecting to WiFi...");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS)
  {
    delay(500);
    Serial.printf("[WiFi] Attempt %d/%d, Status: %d\n", attempts+1, MAX_ATTEMPTS, WiFi.status());
    
    String dots = ".";
    for (int i = 0; i < (attempts % 4) + 1; i++)
    {
      dots += ".";
    }

    std::vector<String> lines = {"Connecting", dots};
    DisplayManager::getInstance().showText(lines);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    if (httpClient == nullptr)
    {
      httpClient = new HttpClient(wifiClient, _serverAddress.c_str(), _port);
      if (httpClient == nullptr)
      {
        Serial.println("Failed to create HttpClient");
        return false;
      }
      httpClient->setTimeout(HTTP_TIMEOUT);
    }

    int rssi = getSignalStrength();
    String signalIndicator = "Signal: " + String(rssi) + " dBm";
    String signalStatus = (rssi < MIN_RSSI) ? "Weak Signal" : "Good Signal";
    std::vector<String> lines = {"", "Connected!", "IP: " + String(WiFi.localIP()), signalIndicator, signalStatus};
    DisplayManager::getInstance().showText(lines);
    delay(2000);
    return true;
  }
  else
  {
    DisplayManager::getInstance().showText("Failed to connect to WiFi");
    delay(1000);
    return false;
  }
}

bool WiFiManager::isConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::disconnect()
{
  WiFi.disconnect();
  if (httpClient != nullptr)
  {
    delete httpClient;
    httpClient = nullptr;
  }
  DisplayManager::getInstance().showText("Disconnected from WiFi");
  delay(1000);
}

int WiFiManager::getSignalStrength()
{
  if (isConnected())
  {
    int rssi = WiFi.RSSI();
    Serial.print("Signal Strength: ");
    Serial.println(rssi);
    return rssi;
  }
  else
  {
    Serial.println("Not connected to WiFi");
    return -1;
  }
}

// GET request
bool WiFiManager::get(const char *path, String &response)
{
  if (!isConnected())
  {
    Serial.println("Cannot perform GET: Not connected to WiFi");
    return false;
  }

  Serial.print("[WiFiManager] GET ");
  Serial.println(path);

  unsigned long startTime = millis();

  httpClient->beginRequest();
  httpClient->get(path);
  httpClient->endRequest();

  while (!httpClient->available() && (millis() - startTime) < HTTP_TIMEOUT)
  {
    delay(10);
  }

  if ((millis() - startTime) >= HTTP_TIMEOUT)
  {
    Serial.println("[WiFiManager] GET request timed out");
    return false;
  }

  int httpCode = httpClient->responseStatusCode();
  response = httpClient->responseBody();

  Serial.print("[WiFiManager] HTTP Code: ");
  Serial.println(httpCode);
  Serial.print("[WiFiManager] Response: ");
  Serial.println(response);

  if (httpCode > 0 && httpCode < 400)
  {
    return true;
  }
  else
  {
    Serial.print("[WiFiManager] GET failed with code: ");
    Serial.println(httpCode);
    return false;
  }
}

// POST request
bool WiFiManager::post(const char *path, const char *contentType, const char *body, String &response)
{
  if (!isConnected())
  {
    Serial.println("Cannot perform POST: Not connected to WiFi");
    return false;
  }

  Serial.print("POST ");
  Serial.println(path);

  unsigned long startTime = millis(); // Start the timeout timer

  httpClient->beginRequest();
  httpClient->post(path, contentType, body);
  httpClient->endRequest();

  // Wait for the response with a timeout
  while (!httpClient->available() && (millis() - startTime) < HTTP_TIMEOUT)
  {
    delay(10); // Small delay to avoid busy-waiting
  }

  if ((millis() - startTime) >= HTTP_TIMEOUT)
  {
    Serial.println("POST request timed out");
    return false;
  }

  int httpCode = httpClient->responseStatusCode();
  response = httpClient->responseBody();

  if (httpCode > 0 && httpCode < 400) // Success codes (2xx and 3xx)
  {
    Serial.print("HTTP Code: ");
    Serial.println(httpCode);
    Serial.print("Response: ");
    Serial.println(response);
    return true;
  }
  else
  {
    Serial.print("POST failed with code: ");
    Serial.println(httpCode);
    return false;
  }
}

// PUT request
bool WiFiManager::put(const char *path, const char *contentType, const char *body, String &response)
{
  if (!isConnected())
  {
    Serial.println("Cannot perform PUT: Not connected to WiFi");
    return false;
  }

  Serial.print("PUT ");
  Serial.println(path);

  unsigned long startTime = millis(); // Start the timeout timer

  httpClient->beginRequest();
  httpClient->put(path, contentType, body);
  httpClient->endRequest();

  // Wait for the response with a timeout
  while (!httpClient->available() && (millis() - startTime) < HTTP_TIMEOUT)
  {
    delay(10); // Small delay to avoid busy-waiting
  }

  if ((millis() - startTime) >= HTTP_TIMEOUT)
  {
    Serial.println("PUT request timed out");
    return false;
  }

  int httpCode = httpClient->responseStatusCode();
  response = httpClient->responseBody();

  if (httpCode > 0 && httpCode < 400) // Success codes (2xx and 3xx)
  {
    Serial.print("HTTP Code: ");
    Serial.println(httpCode);
    Serial.print("Response: ");
    Serial.println(response);
    return true;
  }
  else
  {
    Serial.print("PUT failed with code: ");
    Serial.println(httpCode);
    return false;
  }
}

// DELETE request
bool WiFiManager::del(const char *path, String &response)
{
  if (!isConnected())
  {
    Serial.println("Cannot perform DELETE: Not connected to WiFi");
    return false;
  }

  Serial.print("DELETE ");
  Serial.println(path);

  unsigned long startTime = millis(); // Start the timeout timer

  httpClient->beginRequest();
  httpClient->del(path);
  httpClient->endRequest();

  // Wait for the response with a timeout
  while (!httpClient->available() && (millis() - startTime) < HTTP_TIMEOUT)
  {
    delay(10); // Small delay to avoid busy-waiting
  }

  if ((millis() - startTime) >= HTTP_TIMEOUT)
  {
    Serial.println("DELETE request timed out");
    return false;
  }

  int httpCode = httpClient->responseStatusCode();
  response = httpClient->responseBody();

  if (httpCode > 0 && httpCode < 400) // Success codes (2xx and 3xx)
  {
    Serial.print("HTTP Code: ");
    Serial.println(httpCode);
    Serial.print("Response: ");
    Serial.println(response);
    return true;
  }
  else
  {
    Serial.print("DELETE failed with code: ");
    Serial.println(httpCode);
    return false;
  }
}

// Legacy health check method
bool WiFiManager::checkApiHealth()
{
  Serial.printf("[WiFiManager] checkApiHealth to %s:%d\n", _serverAddress.c_str(), _port);
  String response;
  return get("/api/health", response);
}

char WiFiManager::timeStr[9] = "00:00:00";

void WiFiManager::configureTime(const char *ntpServer, const char *timezone)
{
  // Configure multiple NTP servers with Asia region priority
  const char *servers[] = {
      "asia.pool.ntp.org",   // Asia NTP pool
      "sg.pool.ntp.org",     // Singapore NTP pool
      "3.asia.pool.ntp.org", // Asia pool server 3
      "2.asia.pool.ntp.org"  // Asia pool server 2
  };

  // Set timezone to ICT/Vietnam (UTC+7)
  configTzTime("ICT-7", servers[0], servers[1], servers[2]);

  Serial.println("Waiting for NTP time sync...");
  int retry = 0;
  const int maxRetries = 3;

  while (retry < maxRetries)
  {
    time_t now = time(nullptr);
    if (now > 24 * 3600)
    { // Valid time received
      timeInitialized = true;
      lastTimeSync = millis();
      lastSyncedTime = now;
      lastTimeUpdate = millis();

      struct tm timeinfo;
      if (getLocalTime(&timeinfo))
      {
        Serial.printf("Time synchronized: %02d:%02d:%02d\n",
                      timeinfo.tm_hour,
                      timeinfo.tm_min,
                      timeinfo.tm_sec);
        return;
      }
    }

    // If first server fails, try next one
    if (retry % 5 == 4)
    { // After every 5 attempts
      int serverIndex = (retry / 5) % 4;
      if (serverIndex < 3)
      { // We have 4 servers
        Serial.printf("Trying alternate NTP server: %s\n", servers[serverIndex + 1]);
        configTzTime("ICT-7", servers[serverIndex + 1]);
      }
    }

    Serial.print(".");
    delay(1000);
    retry++;
  }

  Serial.println("\nTime sync failed!");
}

const char *WiFiManager::getCurrentTime()
{
  if (!isConnected() || !timeInitialized)
  {
    return "00:00:00";
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to get local time");

    // Try to re-sync if we can't get the time
    unsigned long currentMillis = millis();
    if (currentMillis - lastTimeSync >= TIME_SYNC_INTERVAL)
    {
      Serial.println("Attempting time re-sync...");
      configTzTime("ICT-7", "pool.ntp.org", "time.nist.gov");
      delay(100); // Brief delay to allow sync

      if (getLocalTime(&timeinfo))
      {
        lastTimeSync = currentMillis;
        lastSyncedTime = time(nullptr);
        lastTimeUpdate = currentMillis;
        Serial.println("Time re-sync successful");
      }
      else
      {
        Serial.println("Time re-sync failed");
      }
    }

    return "00:00:00";
  }

  // Update time string if a second has passed
  unsigned long currentMillis = millis();
  if (currentMillis - lastTimeUpdate >= TIME_UPDATE_INTERVAL)
  {
    sprintf(timeStr, "%02d:%02d:%02d",
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
    lastTimeUpdate = currentMillis;

    // Periodically sync with NTP (every hour)
    if (currentMillis - lastTimeSync >= TIME_SYNC_INTERVAL)
    {
      lastTimeSync = currentMillis;
      lastSyncedTime = time(nullptr);
    }
  }

  return timeStr;
}

// High-level methods for NetworkTaskManager

bool WiFiManager::connect()
{
  // Use credentials from WifiConfig.h
  return connect(ssid, password);
}

void WiFiManager::syncTime()
{
  configureTime("asia.pool.ntp.org", "ICT-7");
}

time_t WiFiManager::getCurrentTimeEpoch()
{
  return time(nullptr);
}

bool WiFiManager::postDoseLog(const String &payload)
{
  String response;
  // Phase 4: Updated endpoint for dose events (start/complete/failed)
  const char* DOSE_LOG_API = "/api/dose-events";
  bool success = post(DOSE_LOG_API, "application/json", payload.c_str(), response);
  
  if (success) {
    Serial.println("[WiFiManager] Dose event posted successfully");
    Serial.print("[WiFiManager] Payload: ");
    Serial.println(payload);
  } else {
    Serial.println("[WiFiManager] Failed to post dose event (mockup - server may not exist)");
  }
  
  return success;
}

bool WiFiManager::checkServerHealth()
{
  return checkApiHealth();
}

bool WiFiManager::getPumpSettings(String &response)
{
  Serial.println("[WiFiManager] getPumpSettings() - fetching from server");
  
  char path[64];
  const char* pumpId = ConfigManager::getInstance().getPumpId();
  snprintf(path, sizeof(path), "/api/pump-settings/%s", pumpId);
  
  bool success = get(path, response);
  
  if (success) {
    Serial.println("[WiFiManager] Settings fetched successfully");
  } else {
    Serial.println("[WiFiManager] Failed to get settings from server");
    response = "";
  }
  
  return success;
}

// Phase 4: Mockup POST pump settings
bool WiFiManager::updatePumpSettings(const String &payload)
{
  Serial.println("[WiFiManager] updatePumpSettings() - MOCKUP");
  Serial.print("[WiFiManager] Payload: ");
  Serial.println(payload);
  Serial.println("[WiFiManager] Mockup: Settings would be saved to server");
  
  return true;
}

bool WiFiManager::getCommands(String &response)
{
  Serial.println("[WiFiManager] getCommands()");
  
  char path[64];
  const char* pumpId = ConfigManager::getInstance().getPumpId();
  snprintf(path, sizeof(path), "/api/pump-commands/%s", pumpId);
  
  bool success = get(path, response);
  
  if (success) {
    Serial.println("[WiFiManager] Commands fetched successfully");
    Serial.print("[WiFiManager] Response: ");
    Serial.println(response);
  } else {
    Serial.println("[WiFiManager] Failed to get commands");
    response = "[]";
  }
  
  return success;
}

bool WiFiManager::completeCommand(const String &payload)
{
  Serial.println("[WiFiManager] completeCommand()");
  
  char path[64];
  const char* pumpId = ConfigManager::getInstance().getPumpId();
  snprintf(path, sizeof(path), "/api/pump-commands/%s/complete", pumpId);
  
  String response;
  bool success = post(path, "application/json", payload.c_str(), response);
  
  if (success) {
    Serial.println("[WiFiManager] Command completion posted successfully");
    Serial.print("[WiFiManager] Response: ");
    Serial.println(response);
  } else {
    Serial.println("[WiFiManager] Failed to post command completion");
  }
  
  return success;
}
