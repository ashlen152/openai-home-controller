/**
 * @file NetworkTaskManager.cpp
 * @brief Implementation of multi-core network task manager
 * 
 * This file implements the Core 0 network task that handles all WiFi, HTTP,
 * and NTP operations independently from the pump control loop on Core 1.
 */

#include "NetworkTaskManager.h"
#include "WiFiManager.h"
#include <Arduino.h>
#include <ApiClient.h>
#include <ConfigManager.h>
#include <PumpController.h>
#include <AutoDosingManager.h>

// Singleton instance
NetworkTaskManager& NetworkTaskManager::getInstance() {
    static NetworkTaskManager instance;
    return instance;
}

// Constructor
NetworkTaskManager::NetworkTaskManager()
    : m_commandQueue(nullptr)
    , m_responseQueue(nullptr)
    , m_taskHandle(nullptr)
    , m_wifiMutex(nullptr)
    , m_initialized(false)
    , m_running(false)
    , m_lastHealthCheck(0)
    , m_lastTimeSync(0)
    , m_lastWiFiCheck(0)
    , m_lastMidnightCheck(0)
    , m_lastResetDay(0)
    , m_retryQueueHead(0)
    , m_retryQueueTail(0)
    , m_retryQueueCount(0)
    , m_lastRetryAttempt(0)
{
    // Initialize retry queue
    memset(m_retryQueue, 0, sizeof(m_retryQueue));
}

// Destructor
NetworkTaskManager::~NetworkTaskManager() {
    stop();
    
    if (m_commandQueue) {
        vQueueDelete(m_commandQueue);
    }
    if (m_responseQueue) {
        vQueueDelete(m_responseQueue);
    }
    if (m_wifiMutex) {
        vSemaphoreDelete(m_wifiMutex);
    }
}

// Initialize queues and mutexes
bool NetworkTaskManager::initialize(uint8_t commandQueueSize, uint8_t responseQueueSize) {
    if (m_initialized) {
        Serial.println("[NetworkTask] Already initialized");
        return true;
    }

    // Create command queue (Core 1 -> Core 0)
    m_commandQueue = xQueueCreate(commandQueueSize, sizeof(NetworkCommandMessage));
    if (!m_commandQueue) {
        Serial.println("[NetworkTask] Failed to create command queue");
        return false;
    }

    // Create response queue (Core 0 -> Core 1)
    m_responseQueue = xQueueCreate(responseQueueSize, sizeof(NetworkResponseMessage));
    if (!m_responseQueue) {
        Serial.println("[NetworkTask] Failed to create response queue");
        vQueueDelete(m_commandQueue);
        m_commandQueue = nullptr;
        return false;
    }

    // Create WiFi mutex for thread-safe access
    m_wifiMutex = xSemaphoreCreateMutex();
    if (!m_wifiMutex) {
        Serial.println("[NetworkTask] Failed to create WiFi mutex");
        vQueueDelete(m_commandQueue);
        vQueueDelete(m_responseQueue);
        m_commandQueue = nullptr;
        m_responseQueue = nullptr;
        return false;
    }

    m_initialized = true;
    Serial.println("[NetworkTask] Initialized successfully");
    return true;
}

// Start network task on Core 0
bool NetworkTaskManager::start(uint32_t stackSize, UBaseType_t priority) {
    if (!m_initialized) {
        Serial.println("[NetworkTask] Not initialized. Call initialize() first.");
        return false;
    }

    if (m_running) {
        Serial.println("[NetworkTask] Already running");
        return true;
    }

    // Create task pinned to Core 0
    BaseType_t result = xTaskCreatePinnedToCore(
        networkTask,        // Task function
        "NetworkTask",      // Task name
        stackSize,          // Stack size
        this,               // Parameter (pass this pointer)
        priority,           // Priority
        &m_taskHandle,      // Task handle
        0                   // Core 0
    );

    if (result != pdPASS) {
        Serial.println("[NetworkTask] Failed to create task");
        return false;
    }

    m_running = true;
    Serial.println("[NetworkTask] Started on Core 0");
    return true;
}

// Stop network task
void NetworkTaskManager::stop() {
    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
        m_running = false;
        Serial.println("[NetworkTask] Stopped");
    }
}

// Send command to network task
bool NetworkTaskManager::sendCommand(const NetworkCommandMessage& cmd, uint32_t timeoutMs) {
    if (!m_initialized || !m_commandQueue) {
        return false;
    }

    TickType_t ticks = (timeoutMs == 0) ? 0 : pdMS_TO_TICKS(timeoutMs);
    return xQueueSend(m_commandQueue, &cmd, ticks) == pdTRUE;
}

// Get response from network task
bool NetworkTaskManager::getResponse(NetworkResponseMessage& response, uint32_t timeoutMs) {
    if (!m_initialized || !m_responseQueue) {
        return false;
    }

    TickType_t ticks = (timeoutMs == 0) ? 0 : pdMS_TO_TICKS(timeoutMs);
    return xQueueReceive(m_responseQueue, &response, ticks) == pdTRUE;
}

// Get pending commands count
uint8_t NetworkTaskManager::getPendingCommands() const {
    if (!m_commandQueue) return 0;
    return uxQueueMessagesWaiting(m_commandQueue);
}

// Get pending responses count
uint8_t NetworkTaskManager::getPendingResponses() const {
    if (!m_responseQueue) return 0;
    return uxQueueMessagesWaiting(m_responseQueue);
}

// Network task loop (runs on Core 0)
void NetworkTaskManager::networkTask(void* parameter) {
    NetworkTaskManager* manager = static_cast<NetworkTaskManager*>(parameter);
    NetworkCommandMessage cmd;
    
    Serial.println("[NetworkTask] Task loop started on Core 0");

    while (true) {
        // Check for incoming commands (non-blocking, 100ms timeout)
        if (xQueueReceive(manager->m_commandQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Process command
            manager->processCommand(cmd);
        }

        // Run background tasks (WiFi keepalive, auto-sync, etc.)
        manager->runBackgroundTasks();

        // Small delay to prevent watchdog issues
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Process a single network command
void NetworkTaskManager::processCommand(const NetworkCommandMessage& cmd) {
    Serial.printf("[NetworkTask] Processing command: %d\n", static_cast<uint8_t>(cmd.command));

    switch (cmd.command) {
        case NetworkCommand::CONNECT_WIFI:
            handleConnectWiFi();
            break;

        case NetworkCommand::DISCONNECT_WIFI:
            handleDisconnectWiFi();
            break;

        case NetworkCommand::SYNC_TIME:
            handleSyncTime();
            break;

        case NetworkCommand::HTTP_GET_SETTINGS:
            handleHttpGetSettings();
            break;

        case NetworkCommand::HTTP_POST_SETTINGS:
            handleHttpPostSettings(cmd.data);
            break;

        case NetworkCommand::HTTP_POST_DOSE_LOG:
            handleHttpPostDoseLog(cmd.data);
            break;

        case NetworkCommand::HTTP_GET_COMMANDS:
            handleHttpGetCommands();
            break;

        case NetworkCommand::HTTP_POST_COMMAND_COMPLETE:
            handleHttpPostCommandComplete(cmd.data);
            break;

        case NetworkCommand::HEALTH_CHECK:
            handleHealthCheck();
            break;

        case NetworkCommand::GET_STATUS:
            handleGetStatus();
            break;

        default:
            Serial.printf("[NetworkTask] Unknown command: %d\n", static_cast<uint8_t>(cmd.command));
            break;
    }
}

// Send response back to Core 1
void NetworkTaskManager::sendResponse(const NetworkResponseMessage& response) {
    if (m_responseQueue) {
        // Non-blocking send, drop response if queue is full
        if (xQueueSend(m_responseQueue, &response, 0) != pdTRUE) {
            Serial.println("[NetworkTask] Response queue full, dropping response");
        }
    }
}

// Handle WiFi connection command
void NetworkTaskManager::handleConnectWiFi() {
    NetworkResponseMessage response;
    response.command = NetworkCommand::CONNECT_WIFI;
    response.value = 0;
    memset(response.data, 0, sizeof(response.data));

    // Lock WiFi mutex
    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        WiFiManager& wifi = WiFiManager::getInstance();
        
        if (wifi.connect()) {
            response.status = NetworkStatus::WIFI_CONNECTED;
            response.value = wifi.getSignalStrength();
            snprintf(response.data, sizeof(response.data), "Connected to WiFi");
            Serial.println("[NetworkTask] WiFi connected successfully");
        } else {
            response.status = NetworkStatus::FAILED;
            snprintf(response.data, sizeof(response.data), "WiFi connection failed");
            Serial.println("[NetworkTask] WiFi connection failed");
        }

        xSemaphoreGive(m_wifiMutex);
    } else {
        response.status = NetworkStatus::TIMEOUT;
        snprintf(response.data, sizeof(response.data), "WiFi mutex timeout");
        Serial.println("[NetworkTask] WiFi mutex timeout");
    }

    sendResponse(response);
}

// Handle WiFi disconnection command
void NetworkTaskManager::handleDisconnectWiFi() {
    NetworkResponseMessage response;
    response.command = NetworkCommand::DISCONNECT_WIFI;
    response.value = 0;
    memset(response.data, 0, sizeof(response.data));

    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        WiFiManager& wifi = WiFiManager::getInstance();
        wifi.disconnect();
        response.status = NetworkStatus::WIFI_DISCONNECTED;
        snprintf(response.data, sizeof(response.data), "WiFi disconnected");
        Serial.println("[NetworkTask] WiFi disconnected");
        xSemaphoreGive(m_wifiMutex);
    } else {
        response.status = NetworkStatus::TIMEOUT;
        snprintf(response.data, sizeof(response.data), "WiFi mutex timeout");
    }

    sendResponse(response);
}

// Handle NTP time sync command
void NetworkTaskManager::handleSyncTime()
{
    NetworkResponseMessage response;
    response.command = NetworkCommand::SYNC_TIME;
    response.value = 0;
    memset(response.data, 0, sizeof(response.data));

    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        WiFiManager& wifi = WiFiManager::getInstance();
        
        if (wifi.isConnected()) {
            wifi.syncTime();
            response.status = NetworkStatus::TIME_SYNCED;
            response.value = wifi.getCurrentTimeEpoch();
            snprintf(response.data, sizeof(response.data), "Time synced");
            Serial.println("[NetworkTask] Time synced successfully");
            m_lastTimeSync = millis();
        } else {
            response.status = NetworkStatus::FAILED;
            snprintf(response.data, sizeof(response.data), "WiFi not connected");
            Serial.println("[NetworkTask] Cannot sync time - WiFi not connected");
        }

        xSemaphoreGive(m_wifiMutex);
    } else {
        response.status = NetworkStatus::TIMEOUT;
        snprintf(response.data, sizeof(response.data), "WiFi mutex timeout");
    }

    sendResponse(response);
}

// Handle HTTP GET settings command
void NetworkTaskManager::handleHttpGetSettings() {
    NetworkResponseMessage response;
    response.command = NetworkCommand::HTTP_GET_SETTINGS;
    response.value = 0;
    memset(response.data, 0, sizeof(response.data));

    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        const char* pumpId = ConfigManager::getInstance().getPumpId();
        JsonDocument result = ApiClient::getInstance().getPumpSettings(pumpId);
        
        if (result["success"].as<bool>()) {
            response.status = NetworkStatus::HTTP_OK;
            response.value = result["httpCode"].as<int>();
            String dataStr;
            serializeJson(result["data"], dataStr);
            strncpy(response.data, dataStr.c_str(), sizeof(response.data) - 1);
            Serial.println("[NetworkTask] HTTP GET settings successful");
        } else {
            response.status = NetworkStatus::HTTP_ERROR;
            String errorMsg = result["error"].as<String>();
            if (errorMsg.length() == 0) errorMsg = "HTTP GET failed";
            snprintf(response.data, sizeof(response.data), "%s", errorMsg.c_str());
            Serial.println("[NetworkTask] HTTP GET settings failed");
        }

        xSemaphoreGive(m_wifiMutex);
    } else {
        response.status = NetworkStatus::TIMEOUT;
        snprintf(response.data, sizeof(response.data), "WiFi mutex timeout");
    }

    sendResponse(response);
}

// Handle HTTP POST settings command
void NetworkTaskManager::handleHttpPostSettings(const char* data) {
    NetworkResponseMessage response;
    response.command = NetworkCommand::HTTP_POST_SETTINGS;
    response.value = 0;
    memset(response.data, 0, sizeof(response.data));

    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        const char* pumpId = ConfigManager::getInstance().getPumpId();
        
        bool success = ApiClient::getInstance().updatePumpSettingsRaw(data);
        
        if (success) {
            response.status = NetworkStatus::HTTP_OK;
            response.value = 200;
            snprintf(response.data, sizeof(response.data), "Settings updated");
            Serial.println("[NetworkTask] HTTP POST settings successful");
        } else {
            response.status = NetworkStatus::HTTP_ERROR;
            snprintf(response.data, sizeof(response.data), "Settings update failed");
            Serial.println("[NetworkTask] HTTP POST settings failed");
        }

        xSemaphoreGive(m_wifiMutex);
    } else {
        response.status = NetworkStatus::TIMEOUT;
        snprintf(response.data, sizeof(response.data), "WiFi mutex timeout");
    }

    sendResponse(response);
}

// Handle HTTP POST dose log command (Phase 2)
void NetworkTaskManager::handleHttpPostDoseLog(const char* data) {
    NetworkResponseMessage response;
    response.command = NetworkCommand::HTTP_POST_DOSE_LOG;
    response.value = 0;
    memset(response.data, 0, sizeof(response.data));

    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        JsonDocument payload;
        DeserializationError error = deserializeJson(payload, data);
        if (error) {
            response.status = NetworkStatus::HTTP_ERROR;
            snprintf(response.data, sizeof(response.data), "Invalid JSON payload");
            xSemaphoreGive(m_wifiMutex);
            sendResponse(response);
            return;
        }
        
        bool success = ApiClient::getInstance().postDoseEvent(payload);
        
        if (success) {
            response.status = NetworkStatus::HTTP_OK;
            response.value = 200;
            snprintf(response.data, sizeof(response.data), "Dose logged");
            Serial.println("[NetworkTask] Dose log posted successfully");
        } else {
            response.status = NetworkStatus::HTTP_ERROR;
            snprintf(response.data, sizeof(response.data), "Dose log POST failed, queued for retry");
            Serial.println("[NetworkTask] Dose log POST failed, adding to retry queue");
            addToRetryQueue(data);
        }

        xSemaphoreGive(m_wifiMutex);
    } else {
        response.status = NetworkStatus::TIMEOUT;
        snprintf(response.data, sizeof(response.data), "WiFi mutex timeout");
    }

    sendResponse(response);
}

// Handle server health check command
void NetworkTaskManager::handleHealthCheck() {
    NetworkResponseMessage response;
    response.command = NetworkCommand::HEALTH_CHECK;
    response.value = 0;
    memset(response.data, 0, sizeof(response.data));

    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        WiFiManager& wifi = WiFiManager::getInstance();
        
        if (wifi.isConnected()) {
            if (wifi.checkServerHealth()) {
                response.status = NetworkStatus::SUCCESS;
                snprintf(response.data, sizeof(response.data), "Server healthy");
                Serial.println("[NetworkTask] Server health check OK");
                m_lastHealthCheck = millis();
            } else {
                response.status = NetworkStatus::FAILED;
                snprintf(response.data, sizeof(response.data), "Server unreachable");
                Serial.println("[NetworkTask] Server health check failed");
            }
        } else {
            response.status = NetworkStatus::FAILED;
            snprintf(response.data, sizeof(response.data), "WiFi not connected");
            Serial.println("[NetworkTask] Cannot check health - WiFi not connected");
        }

        xSemaphoreGive(m_wifiMutex);
    } else {
        response.status = NetworkStatus::TIMEOUT;
        snprintf(response.data, sizeof(response.data), "WiFi mutex timeout");
    }

    sendResponse(response);
}

// Handle status query command
void NetworkTaskManager::handleGetStatus() {
    NetworkResponseMessage response;
    response.command = NetworkCommand::GET_STATUS;
    response.value = 0;
    memset(response.data, 0, sizeof(response.data));

    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        WiFiManager& wifi = WiFiManager::getInstance();
        
        if (wifi.isConnected()) {
            response.status = NetworkStatus::WIFI_CONNECTED;
            response.value = wifi.getSignalStrength();
            snprintf(response.data, sizeof(response.data), 
                    "WiFi: Connected, RSSI: %d dBm", 
                    wifi.getSignalStrength());
        } else {
            response.status = NetworkStatus::WIFI_DISCONNECTED;
            snprintf(response.data, sizeof(response.data), "WiFi: Disconnected");
        }

        xSemaphoreGive(m_wifiMutex);
    } else {
        response.status = NetworkStatus::TIMEOUT;
        snprintf(response.data, sizeof(response.data), "WiFi mutex timeout");
    }

    sendResponse(response);
}

// Run background tasks (WiFi keepalive, auto-sync)
void NetworkTaskManager::runBackgroundTasks() {
    unsigned long now = millis();

    // WiFi keepalive check every 5 seconds
    if (now - m_lastWiFiCheck >= WIFI_CHECK_INTERVAL) {
        m_lastWiFiCheck = now;

        if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            WiFiManager& wifi = WiFiManager::getInstance();
            
            // If WiFi is supposed to be connected but isn't, try to reconnect
            if (!wifi.isConnected()) {
                Serial.println("[NetworkTask] WiFi disconnected, attempting reconnect...");
                wifi.connect();
            }

            xSemaphoreGive(m_wifiMutex);
        }
    }

    // Auto time sync every hour
    if (now - m_lastTimeSync >= TIME_SYNC_INTERVAL) {
        Serial.println("[NetworkTask] Auto time sync triggered");
        handleSyncTime();
    }

    // Auto health check every 3 minutes
    if (now - m_lastHealthCheck >= HEALTH_CHECK_INTERVAL) {
        Serial.println("[NetworkTask] Auto health check triggered");
        handleHealthCheck();
    }
    
    // Auto status sync every 3 minutes (settings + health heartbeat)
    static unsigned long lastStatusSync = 0;
    if (now - lastStatusSync >= HEALTH_CHECK_INTERVAL) {
        lastStatusSync = now;
        AutoDosingManager::getInstance().syncSettings();
    }
    
    // Midnight auto-dosing reset check (every minute)
    if (now - m_lastMidnightCheck >= MIDNIGHT_CHECK_INTERVAL) {
        m_lastMidnightCheck = now;
        
        time_t currentTime = time(nullptr);
        if (currentTime > 0) {  // Valid time
            struct tm *timeinfo = localtime(&currentTime);
            uint8_t currentDay = timeinfo->tm_mday;
            
            // Check if day changed (midnight passed)
            if (m_lastResetDay != 0 && currentDay != m_lastResetDay) {
                Serial.printf("[NetworkTask] ⏰ Midnight detected (day %d -> %d) - Auto-dosing reset\n", 
                             m_lastResetDay, currentDay);
                
                // Send reset response to Core 1
                NetworkResponseMessage response;
                response.command = NetworkCommand::AUTO_DOSING_RESET;
                response.status = NetworkStatus::SUCCESS;
                response.value = currentDay;
                snprintf(response.data, sizeof(response.data), "Midnight reset day %d", currentDay);
                sendResponse(response);
            }
        }
    }
    
    // Process retry queue every 5 minutes (Phase 3 Sprint 4)
    if (now - m_lastRetryAttempt >= RETRY_INTERVAL) {
        processRetryQueue();
    }
}

// Phase 3 Sprint 4: Add failed dose log to retry queue
void NetworkTaskManager::addToRetryQueue(const char* jsonPayload) {
    // Queue is full - drop oldest entry (FIFO)
    if (m_retryQueueCount >= RETRY_QUEUE_SIZE) {
        Serial.println("[NetworkTask] Retry queue full, dropping oldest entry");
        m_retryQueueTail = (m_retryQueueTail + 1) % RETRY_QUEUE_SIZE;
        m_retryQueueCount--;
    }
    
    // Parse JSON to extract fields (simple parsing)
    // Expected format: {"timestamp":1234,"volume":1.5,"success":true,"pumpId":"SmartPump_01"}
    DoseLogEntry& entry = m_retryQueue[m_retryQueueHead];
    
    // Extract timestamp
    const char* ts = strstr(jsonPayload, "\"timestamp\":");
    entry.timestamp = ts ? atol(ts + 12) : 0;
    
    // Extract volume
    const char* vol = strstr(jsonPayload, "\"volume\":");
    entry.volume = vol ? atof(vol + 9) : 0.0f;
    
    // Extract success
    const char* suc = strstr(jsonPayload, "\"success\":");
    entry.success = (suc && strstr(suc, "true"));
    
    // Extract pumpId
    const char* id = strstr(jsonPayload, "\"pumpId\":\"");
    if (id) {
        id += 10;  // Skip "pumpId":"
        const char* idEnd = strchr(id, '"');
        if (idEnd) {
            size_t len = min((size_t)(idEnd - id), sizeof(entry.pumpId) - 1);
            strncpy(entry.pumpId, id, len);
            entry.pumpId[len] = '\0';
        }
    }
    
    entry.retryCount = 0;
    
    m_retryQueueHead = (m_retryQueueHead + 1) % RETRY_QUEUE_SIZE;
    m_retryQueueCount++;
    
    Serial.printf("[NetworkTask] Added to retry queue (count: %d)\n", m_retryQueueCount);
}

// Phase 3 Sprint 4: Process retry queue
void NetworkTaskManager::processRetryQueue() {
    m_lastRetryAttempt = millis();
    
    if (m_retryQueueCount == 0) {
        return;  // Nothing to retry
    }
    
    Serial.printf("[NetworkTask] Processing retry queue (%d items)...\n", m_retryQueueCount);
    
    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (!ApiClient::getInstance().getPumpSettings("health").is<JsonObject>()) {
            Serial.println("[NetworkTask] Retry queue: API not available, skipping");
            xSemaphoreGive(m_wifiMutex);
            return;
        }
        
        int retried = 0;
        while (m_retryQueueCount > 0 && retried < 3) {
            DoseLogEntry& entry = m_retryQueue[m_retryQueueTail];
            
            JsonDocument payload;
            payload["timestamp"] = entry.timestamp;
            payload["volume"] = entry.volume;
            payload["success"] = entry.success;
            payload["pumpId"] = entry.pumpId;
            
            bool success = ApiClient::getInstance().postDoseEvent(payload);
            
            if (success) {
                Serial.printf("[NetworkTask] Retry successful for dose at %lu\n", entry.timestamp);
                m_retryQueueTail = (m_retryQueueTail + 1) % RETRY_QUEUE_SIZE;
                m_retryQueueCount--;
            } else {
                entry.retryCount++;
                
                if (entry.retryCount >= 3) {
                    Serial.printf("[NetworkTask] Retry limit reached for dose at %lu, dropping\n", entry.timestamp);
                    m_retryQueueTail = (m_retryQueueTail + 1) % RETRY_QUEUE_SIZE;
                    m_retryQueueCount--;
                } else {
                    Serial.printf("[NetworkTask] Retry failed for dose at %lu (attempt %d/3)\n",
                                  entry.timestamp, entry.retryCount);
                    break;
                }
            }
            
            retried++;
        }
        
        xSemaphoreGive(m_wifiMutex);
    }
}

void NetworkTaskManager::handleHttpGetCommands() {
    NetworkResponseMessage response;
    response.command = NetworkCommand::HTTP_GET_COMMANDS;
    response.value = 0;
    memset(response.data, 0, sizeof(response.data));

    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        const char* pumpId = ConfigManager::getInstance().getPumpId();
        JsonDocument result = ApiClient::getInstance().getCommands(pumpId);
        
        if (result["success"].as<bool>()) {
            response.status = NetworkStatus::HTTP_OK;
            response.value = result["httpCode"].as<int>();
            String dataStr;
            serializeJson(result["data"], dataStr);
            strncpy(response.data, dataStr.c_str(), sizeof(response.data) - 1);
            Serial.println("[NetworkTask] HTTP GET commands successful");
        } else {
            response.status = NetworkStatus::HTTP_ERROR;
            String errorMsg = result["error"].as<String>();
            if (errorMsg.length() == 0) errorMsg = "HTTP GET commands failed";
            snprintf(response.data, sizeof(response.data), "%s", errorMsg.c_str());
            Serial.println("[NetworkTask] HTTP GET commands failed");
        }

        xSemaphoreGive(m_wifiMutex);
    } else {
        response.status = NetworkStatus::TIMEOUT;
        snprintf(response.data, sizeof(response.data), "WiFi mutex timeout");
    }

    sendResponse(response);
}

void NetworkTaskManager::handleHttpPostCommandComplete(const char* data) {
    NetworkResponseMessage response;
    response.command = NetworkCommand::HTTP_POST_COMMAND_COMPLETE;
    response.value = 0;
    memset(response.data, 0, sizeof(response.data));

    if (xSemaphoreTake(m_wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        const char* pumpId = ConfigManager::getInstance().getPumpId();
        
        JsonDocument payload;
        DeserializationError error = deserializeJson(payload, data);
        if (error) {
            response.status = NetworkStatus::HTTP_ERROR;
            snprintf(response.data, sizeof(response.data), "Invalid JSON payload");
            xSemaphoreGive(m_wifiMutex);
            sendResponse(response);
            return;
        }
        
        bool success = ApiClient::getInstance().completeCommand(pumpId, payload);
        
        if (success) {
            response.status = NetworkStatus::HTTP_OK;
            response.value = 200;
            snprintf(response.data, sizeof(response.data), "Command completed");
            Serial.println("[NetworkTask] HTTP POST command complete successful");
        } else {
            response.status = NetworkStatus::HTTP_ERROR;
            snprintf(response.data, sizeof(response.data), "HTTP POST command complete failed");
            Serial.println("[NetworkTask] HTTP POST command complete failed");
        }

        xSemaphoreGive(m_wifiMutex);
    } else {
        response.status = NetworkStatus::TIMEOUT;
        snprintf(response.data, sizeof(response.data), "WiFi mutex timeout");
    }

    sendResponse(response);
}
