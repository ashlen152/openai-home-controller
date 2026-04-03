/**
 * @file NetworkTaskManager.h
 * @brief Multi-core network task manager for ESP32 Core 0
 * 
 * Manages all network operations (WiFi, HTTP, NTP) on ESP32 Core 0 to prevent
 * blocking time-critical pump control operations running on Core 1.
 * 
 * Architecture:
 * - Core 0: Network operations (WiFi connect, HTTP requests, NTP sync, health checks)
 * - Core 1: Pump control, display updates, button handling (main loop)
 * - Communication: FreeRTOS queues for command/response passing between cores
 * 
 * Thread Safety:
 * - WiFiManager access is mutex-protected
 * - Queue operations are thread-safe (built-in FreeRTOS)
 * - No shared state except through queues
 * 
 * Usage:
 *   NetworkTaskManager::initialize();
 *   NetworkTaskManager::start();
 *   NetworkTaskManager::sendCommand(cmd);
 *   NetworkTaskManager::getResponse(response);
 */

#ifndef NETWORKTASKMANAGER_H
#define NETWORKTASKMANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <ArduinoJson.h>
#include "../ApiClient/ApiClient.h"
#include "../ConfigManager/ConfigManager.h"

/**
 * @brief Network command types for inter-core communication
 */
enum class NetworkCommand : uint8_t {
    NONE = 0,
    CONNECT_WIFI,
    DISCONNECT_WIFI,
    SYNC_TIME,
    HTTP_GET_SETTINGS,
    HTTP_POST_SETTINGS,
    HTTP_POST_DOSE_LOG,
    HEALTH_CHECK,
    GET_STATUS,
    AUTO_DOSING_RESET,
    HTTP_GET_COMMANDS,
    HTTP_POST_COMMAND_COMPLETE
};

/**
 * @brief Network response status codes
 */
enum class NetworkStatus : uint8_t {
    IDLE = 0,
    SUCCESS,
    FAILED,
    IN_PROGRESS,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    TIME_SYNCED,
    HTTP_OK,
    HTTP_ERROR,
    TIMEOUT
};

/**
 * @brief Command message structure for Core 1 -> Core 0
 */
struct NetworkCommandMessage {
    NetworkCommand command;
    uint32_t param1;        // Optional parameter (e.g., timeout, retry count)
    uint32_t param2;        // Optional parameter
    char data[512];         // Optional data payload (e.g., JSON, URL) - Increased for status report JSON
};

/**
 * @brief Response message structure for Core 0 -> Core 1
 */
struct NetworkResponseMessage {
    NetworkCommand command;     // Which command this responds to
    NetworkStatus status;       // Result status
    uint32_t value;            // Optional return value (e.g., HTTP code, signal strength)
    char data[512];            // Optional response data
};

/**
 * @brief Network Task Manager - Singleton class managing Core 0 network operations
 */
class NetworkTaskManager {
public:
    /**
     * @brief Get singleton instance
     */
    static NetworkTaskManager& getInstance();

    /**
     * @brief Initialize queues and mutexes (call from setup() before start())
     * @param commandQueueSize Size of command queue (default 10)
     * @param responseQueueSize Size of response queue (default 10)
     * @return true if initialization successful
     */
    bool initialize(uint8_t commandQueueSize = 10, uint8_t responseQueueSize = 10);

    /**
     * @brief Start network task on Core 0
     * @param stackSize Task stack size in bytes (default 8192)
     * @param priority Task priority (default 1)
     * @return true if task created successfully
     */
    bool start(uint32_t stackSize = 8192, UBaseType_t priority = 1);

    /**
     * @brief Stop network task (cleanup)
     */
    void stop();

    /**
     * @brief Send command to network task (non-blocking)
     * @param cmd Command message
     * @param timeoutMs Timeout in milliseconds (0 = no wait, portMAX_DELAY = wait forever)
     * @return true if command queued successfully
     */
    bool sendCommand(const NetworkCommandMessage& cmd, uint32_t timeoutMs = 0);

    /**
     * @brief Get response from network task (non-blocking by default)
     * @param response Output response message
     * @param timeoutMs Timeout in milliseconds (0 = no wait, portMAX_DELAY = wait forever)
     * @return true if response received
     */
    bool getResponse(NetworkResponseMessage& response, uint32_t timeoutMs = 0);

    /**
     * @brief Check if network task is running
     */
    bool isRunning() const { return m_taskHandle != nullptr; }

    /**
     * @brief Get number of pending commands in queue
     */
    uint8_t getPendingCommands() const;

    /**
     * @brief Get number of pending responses in queue
     */
    uint8_t getPendingResponses() const;

private:
    NetworkTaskManager();
    ~NetworkTaskManager();
    NetworkTaskManager(const NetworkTaskManager&) = delete;
    NetworkTaskManager& operator=(const NetworkTaskManager&) = delete;

    /**
     * @brief Network task loop (runs on Core 0)
     */
    static void networkTask(void* parameter);

    /**
     * @brief Process a single network command
     */
    void processCommand(const NetworkCommandMessage& cmd);

    /**
     * @brief Send response back to Core 1
     */
    void sendResponse(const NetworkResponseMessage& response);

    /**
     * @brief Handle WiFi connection command
     */
    void handleConnectWiFi();

    /**
     * @brief Handle WiFi disconnection command
     */
    void handleDisconnectWiFi();

    /**
     * @brief Handle NTP time sync command
     */
    void handleSyncTime();

    /**
     * @brief Handle HTTP GET settings command
     */
    void handleHttpGetSettings();

    /**
     * @brief Handle HTTP POST settings command
     */
    void handleHttpPostSettings(const char* data);

    void handleHttpPostDoseLog(const char* data);

    void handleHttpGetCommands();

    void handleHttpPostCommandComplete(const char* data);

    /**
     * @brief Handle server health check command
     */
    void handleHealthCheck();

    /**
     * @brief Handle status query command
     */
    void handleGetStatus();

    /**
     * @brief Periodic background tasks (WiFi keepalive, auto-sync)
     */
    void runBackgroundTasks();
    
    /**
     * @brief Process retry queue for failed dose POST requests (Phase 3 Sprint 4)
     */
    void processRetryQueue();
    
    /**
     * @brief Add failed dose log to retry queue
     */
    void addToRetryQueue(const char* jsonPayload);

private:
    // Retry queue for failed dose POST requests (Phase 3 Sprint 4)
    struct DoseLogEntry {
        uint32_t timestamp;
        float volume;
        bool success;
        char pumpId[16];
        uint32_t retryCount;  // Number of retry attempts
    };
    
    static constexpr int RETRY_QUEUE_SIZE = 5;
    DoseLogEntry m_retryQueue[RETRY_QUEUE_SIZE];
    uint8_t m_retryQueueHead;    // Write index
    uint8_t m_retryQueueTail;    // Read index
    uint8_t m_retryQueueCount;   // Number of items in queue
    unsigned long m_lastRetryAttempt;
    static constexpr uint32_t RETRY_INTERVAL = 300000;  // 5 minutes
    
    QueueHandle_t m_commandQueue;           // Core 1 -> Core 0 command queue
    QueueHandle_t m_responseQueue;          // Core 0 -> Core 1 response queue
    TaskHandle_t m_taskHandle;              // FreeRTOS task handle
    SemaphoreHandle_t m_wifiMutex;          // Mutex for WiFiManager access
    
    bool m_initialized;
    bool m_running;
    
    // Background task timing
    unsigned long m_lastHealthCheck;
    unsigned long m_lastTimeSync;
    unsigned long m_lastWiFiCheck;
    unsigned long m_lastMidnightCheck;      // Midnight reset check
    uint8_t m_lastResetDay;                 // Track which day we last reset (1-31)
    
    static constexpr uint32_t HEALTH_CHECK_INTERVAL = 180000;  // 3 minutes
    static constexpr uint32_t TIME_SYNC_INTERVAL = 3600000;    // 1 hour
    static constexpr uint32_t WIFI_CHECK_INTERVAL = 5000;      // 5 seconds
    static constexpr uint32_t MIDNIGHT_CHECK_INTERVAL = 60000; // 1 minute
};

#endif // NETWORKTASKMANAGER_H
