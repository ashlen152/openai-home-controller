#include "ApiClient.h"

ApiClient::ApiClient()
    : _serverAddress(serverAddress), _port(serverPort)
{
}

ApiClient& ApiClient::getInstance() {
    static ApiClient instance;
    return instance;
}

void ApiClient::setServer(const char* address, int port) {
    _serverAddress = address;
    _port = port;
}

bool ApiClient::init() {
    if (_httpClient != nullptr) {
        delete _httpClient;
    }
    
    _httpClient = new HttpClient(_wifiClient, _serverAddress, _port);
    _httpClient->setTimeout(HTTP_TIMEOUT);
    Serial.printf("[ApiClient] Initialized: %s:%d\n", _serverAddress, _port);
    return true;
}

bool ApiClient::ensureConnected() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ApiClient] WiFi not connected");
        return false;
    }
    if (_httpClient == nullptr) {
        init();
    }
    return _httpClient != nullptr;
}

JsonDocument ApiClient::parseResponse(const String& response) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Serial.printf("[ApiClient] JSON parse error: %s\n", error.c_str());
    }
    return doc;
}

JsonDocument ApiClient::get(const char* path) {
    JsonDocument result;

    if (!ensureConnected()) {
        result["success"] = false;
        result["error"] = "WiFi not connected";
        return result;
    }

    Serial.printf("[ApiClient] GET %s\n", path);

    _httpClient->beginRequest();
    _httpClient->get(path);
    _httpClient->endRequest();

    // responseStatusCode() blocks until response is received or timeout
    int httpCode = _httpClient->responseStatusCode();
    String response = _httpClient->responseBody();

    Serial.printf("[ApiClient] GET HTTP %d\n", httpCode);

    if (httpCode > 0 && httpCode < 400) {
        result["success"] = true;
        result["httpCode"] = httpCode;
        JsonDocument data = parseResponse(response);
        result["data"] = data;
    } else {
        result["success"] = false;
        result["httpCode"] = httpCode;
        result["error"] = response;
    }

    return result;
}

JsonDocument ApiClient::post(const char* path, const JsonDocument& payload) {
    JsonDocument result;

    if (!ensureConnected()) {
        result["success"] = false;
        result["error"] = "WiFi not connected";
        return result;
    }

    Serial.printf("[ApiClient] POST %s\n", path);

    String body;
    serializeJson(payload, body);
    Serial.printf("[ApiClient] Payload: %s\n", body.c_str());

    _httpClient->beginRequest();
    _httpClient->post(path, "application/json", body.c_str());
    _httpClient->endRequest();

    // responseStatusCode() blocks until response is received or timeout
    int httpCode = _httpClient->responseStatusCode();
    String response = _httpClient->responseBody();

    Serial.printf("[ApiClient] POST HTTP %d\n", httpCode);

    if (httpCode > 0 && httpCode < 400) {
        result["success"] = true;
        result["httpCode"] = httpCode;
        JsonDocument data = parseResponse(response);
        result["data"] = data;
    } else {
        result["success"] = false;
        result["httpCode"] = httpCode;
        result["error"] = response;
    }

    return result;
}

JsonDocument ApiClient::put(const char* path, const JsonDocument& payload) {
    JsonDocument result;

    if (!ensureConnected()) {
        result["success"] = false;
        result["error"] = "WiFi not connected";
        return result;
    }

    Serial.printf("[ApiClient] PUT %s\n", path);

    String body;
    serializeJson(payload, body);

    _httpClient->beginRequest();
    _httpClient->put(path, "application/json", body.c_str());
    _httpClient->endRequest();

    // responseStatusCode() blocks until response is received or timeout
    int httpCode = _httpClient->responseStatusCode();
    String response = _httpClient->responseBody();

    if (httpCode > 0 && httpCode < 400) {
        result["success"] = true;
        result["httpCode"] = httpCode;
        JsonDocument data = parseResponse(response);
        result["data"] = data;
    } else {
        result["success"] = false;
        result["httpCode"] = httpCode;
        result["error"] = response;
    }

    return result;
}

bool ApiClient::del(const char* path) {
    if (!ensureConnected()) {
        return false;
    }

    Serial.printf("[ApiClient] DELETE %s\n", path);

    _httpClient->beginRequest();
    _httpClient->del(path);
    _httpClient->endRequest();

    // responseStatusCode() blocks until response is received or timeout
    int httpCode = _httpClient->responseStatusCode();

    return httpCode > 0 && httpCode < 400;
}

JsonDocument ApiClient::getPumpSettings(const char* pumpId) {
    char path[64];
    snprintf(path, sizeof(path), "/api/pump-settings/%s", pumpId);
    return get(path);
}

bool ApiClient::updatePumpSettings(const JsonDocument& settings) {
    const char* pumpId = settings["pumpId"] | "SmartPump_01";
    char path[128];
    snprintf(path, sizeof(path), "/api/pump-settings/report/%s", pumpId);
    
    String fullUrl = String("http://") + _serverAddress + ":" + String(_port) + path;
    String body;
    serializeJson(settings, body);
    Serial.printf("[ApiClient] POST SETTINGS URL: %s\n", fullUrl.c_str());
    Serial.printf("[ApiClient] POST SETTINGS Payload: %s\n", body.c_str());
    
    JsonDocument result = post(path, settings);
    
    if (!result["success"].as<bool>()) {
        Serial.printf("[ApiClient] POST SETTINGS FAILED - HTTP %d\n", result["httpCode"].as<int>());
        Serial.printf("[ApiClient] POST SETTINGS Error: %s\n", result["error"].as<const char*>());
    }
    
    return result["success"].as<bool>();
}

bool ApiClient::updatePumpSettingsRaw(const char* jsonPayload) {
    if (!ensureConnected()) {
        Serial.println("[ApiClient] Raw settings: WiFi not connected");
        return false;
    }

    // Extract pumpId from raw JSON for the URL path
    char pumpIdBuf[16] = "SmartPump_01";
    const char* start = strstr(jsonPayload, "\"pumpId\":\"");
    if (start) {
        start += 10;
        const char* end = strchr(start, '"');
        if (end) {
            size_t len = end - start;
            if (len < sizeof(pumpIdBuf)) {
                strncpy(pumpIdBuf, start, len);
                pumpIdBuf[len] = '\0';
            }
        }
    }

    char path[128];
    snprintf(path, sizeof(path), "/api/pump-settings/report/%s", pumpIdBuf);

    String fullUrl = String("http://") + _serverAddress + ":" + String(_port) + path;
    Serial.printf("[ApiClient] POST SETTINGS URL: %s\n", fullUrl.c_str());
    Serial.printf("[ApiClient] POST SETTINGS Payload: %s\n", jsonPayload);

    _httpClient->beginRequest();
    _httpClient->post(path, "application/json", jsonPayload);
    _httpClient->endRequest();

    // responseStatusCode() blocks until response is received or timeout
    int httpCode = _httpClient->responseStatusCode();
    String response = _httpClient->responseBody();

    Serial.printf("[ApiClient] POST SETTINGS HTTP %d\n", httpCode);

    if (httpCode > 0 && httpCode < 400) {
        return true;
    }

    Serial.printf("[ApiClient] POST SETTINGS FAILED - HTTP %d\n", httpCode);
    Serial.printf("[ApiClient] POST SETTINGS Error: %s\n", response.c_str());
    return false;
}

JsonDocument ApiClient::getCommands(const char* pumpId) {
    char path[64];
    snprintf(path, sizeof(path), "/api/pump-commands/%s", pumpId);
    return get(path);
}

bool ApiClient::completeCommand(const char* pumpId, const JsonDocument& payload) {
    char path[64];
    snprintf(path, sizeof(path), "/api/pump-commands/%s/complete", pumpId);
    JsonDocument result = post(path, payload);
    return result["success"].as<bool>();
}

bool ApiClient::postDoseEvent(const JsonDocument& payload) {
    JsonDocument result = post("/api/dose-events", payload);
    return result["success"].as<bool>();
}

JsonDocument ApiClient::getCalibrateStart(const char* pumpId) {
    char path[64];
    snprintf(path, sizeof(path), "/api/pump-commands/calibrate/start");
    
    JsonDocument payload;
    payload["pumpId"] = pumpId;
    payload["volume"] = 5.0;
    
    return post(path, payload);
}

JsonDocument ApiClient::postCalibrateSave(const char* pumpId, const JsonDocument& payload) {
    char path[64];
    snprintf(path, sizeof(path), "/api/pump-commands/calibrate/save");
    return post(path, payload);
}

JsonDocument ApiClient::postTestDose(const char* pumpId, const JsonDocument& payload) {
    char path[64];
    snprintf(path, sizeof(path), "/api/pump-commands/test-dose");
    return post(path, payload);
}
