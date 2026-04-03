#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include "../ConfigManager/ConfigManager.h"
#include "../../include/WifiConfig.h"

class ApiClient {
public:
    static ApiClient& getInstance();
    
    bool init();
    void setServer(const char* address, int port);
    
    JsonDocument get(const char* path);
    JsonDocument post(const char* path, const JsonDocument& payload);
    JsonDocument put(const char* path, const JsonDocument& payload);
    bool del(const char* path);
    
    JsonDocument getPumpSettings(const char* pumpId);
    bool updatePumpSettings(const JsonDocument& settings);
    bool updatePumpSettingsRaw(const char* jsonPayload);
    JsonDocument getCommands(const char* pumpId);
    bool completeCommand(const char* pumpId, const JsonDocument& payload);
    bool postDoseEvent(const JsonDocument& payload);
    JsonDocument getCalibrateStart(const char* pumpId);
    JsonDocument postCalibrateSave(const char* pumpId, const JsonDocument& payload);
    JsonDocument postTestDose(const char* pumpId, const JsonDocument& payload);

private:
    ApiClient();
    
    bool ensureConnected();
    JsonDocument parseResponse(const String& response);
    
    String _serverAddress;
    int _port;
    WiFiClient _wifiClient;
    HttpClient* _httpClient = nullptr;
    const int HTTP_TIMEOUT = 5000;
};

#endif
