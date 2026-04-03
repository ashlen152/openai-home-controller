#include "RemoteCommandManager.h"
#include <PumpController.h>
#include <DisplayManager.h>
#include <ConfigManager.h>
#include <ApiClient.h>
#include <AutoDosingManager.h>
#include "../../src/CalibrateDosingController/CalibrateDosingController.h"
#include "../../src/TestDosingController/TestDosingController.h"

RemoteCommandManager::RemoteCommandManager()
    : m_calibrationActive(false)
    , m_testDoseActive(false)
    , m_saveCalibrationActive(false)
    , m_saveSettingsActive(false)
    , m_pendingTestDoseSteps(0)
    , m_pendingTestDoseSpeed(2000)
{
}

RemoteCommandManager& RemoteCommandManager::getInstance() {
    static RemoteCommandManager instance;
    return instance;
}

void RemoteCommandManager::parseCommands(const String& responseStr) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, responseStr);
    if (error) {
        Serial.printf("[RemoteCmd] JSON parse error: %s\n", error.c_str());
        return;
    }
    parseCommandsFromJson(doc);
}

void RemoteCommandManager::parseCommandsFromJson(JsonDocument& doc) {
    JsonArray commands;
    
    if (doc.is<JsonArray>()) {
        commands = doc.as<JsonArray>();
    } else if (doc.is<JsonObject>()) {
        JsonObject obj = doc.as<JsonObject>();
        if (obj.containsKey("pendingCommands")) {
            JsonArray pending = obj["pendingCommands"];
            if (!pending.isNull()) {
                commands = pending;
            } else {
                Serial.println("[RemoteCmd] No pendingCommands in response");
                return;
            }
        } else {
            Serial.println("[RemoteCmd] Response is object but no pendingCommands key");
            return;
        }
    } else {
        Serial.println("[RemoteCmd] Response is not an array or object");
        return;
    }
    
    if (commands.isNull()) {
        Serial.println("[RemoteCmd] Commands array is null");
        return;
    }
    
    Serial.printf("[RemoteCmd] Processing %d commands\n", commands.size());
    
    for (size_t i = 0; i < commands.size(); i++) {
        JsonObject cmd = commands[i].as<JsonObject>();
        if (cmd.isNull() || !cmd.containsKey("command")) continue;
        
        const char* command = cmd["command"].as<const char*>();
        
        if (strcmp(command, "SAVE_SETTINGS") == 0 && !m_saveSettingsActive) {
            handleSaveSettingsCommand(cmd);
        }
        else if (strcmp(command, "SAVE_CALIBRATION") == 0 && !m_saveCalibrationActive) {
            handleSaveCalibrationCommand(cmd);
        }
        else if (strcmp(command, "CALIBRATE") == 0 && !m_calibrationActive) {
            handleCalibrateCommand(cmd);
        }
        else if (strcmp(command, "TEST_DOSE") == 0 && !m_testDoseActive) {
            handleTestDoseCommand(cmd);
        }
    }
}

void RemoteCommandManager::handleSaveSettingsCommand(JsonVariant cmdVar) {
    if (!cmdVar.is<JsonObject>()) return;
    
    JsonObject cmd = cmdVar.as<JsonObject>();
    if (!cmd.containsKey("commandId") || !cmd.containsKey("payload")) return;
    
    m_pendingSaveSettingsCmdId = cmd["commandId"].as<const char*>();
    
    JsonObject payload = cmd["payload"].as<JsonObject>();
    
    Serial.printf("[RemoteCmd] SAVE_SETTINGS: cmdId=%s\n", m_pendingSaveSettingsCmdId.c_str());
    
    PumpController &pump = PumpController::getInstance();
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    
    if (payload.containsKey("enabled")) {
        bool enabled = payload["enabled"].as<bool>();
        if (enabled != autoDosing.isEnabled()) {
            if (enabled) autoDosing.enable();
            else autoDosing.disable();
        }
    }
    
    if (payload.containsKey("dailyVolume")) {
        float dailyVol = payload["dailyVolume"].as<float>();
        if (dailyVol > 0 && dailyVol <= 200) {
            autoDosing.setDailyVolume(dailyVol);
        }
    }
    
    if (payload.containsKey("dayStartHour") && payload.containsKey("dayEndHour")) {
        uint8_t startH = payload["dayStartHour"].as<uint8_t>();
        uint8_t endH = payload["dayEndHour"].as<uint8_t>();
        if (startH <= 23 && endH <= 23 && startH != endH) {
            autoDosing.setDayPeriod(startH, endH);
        }
    }
    
    if (payload.containsKey("dayPercent")) {
        uint8_t dayPercent = payload["dayPercent"].as<uint8_t>();
        if (dayPercent <= 100) {
            autoDosing.setDayNightSplit(dayPercent);
        }
    }
    
    if (payload.containsKey("stepsPerML")) {
        float stepsPerML = payload["stepsPerML"].as<float>();
        if (stepsPerML > 0) {
            pump.setDosingStepsPerML(stepsPerML);
            pump.saveStepsPerML(stepsPerML);
        }
    }
    
    if (payload.containsKey("activeProfile")) {
        uint8_t profile = payload["activeProfile"].as<uint8_t>();
        if (profile <= 2) {
            pump.setSpeedProfile(profile);
            pump.saveSpeedProfile(profile);
        }
    }
    
    if (payload.containsKey("pausedUntil")) {
        uint32_t pausedUntil = payload["pausedUntil"].as<uint32_t>();
        if (pausedUntil > 0) {
            time_t now = time(nullptr);
            if (now > 0 && pausedUntil > (uint32_t)now) {
                autoDosing.pause(pausedUntil - (uint32_t)now);
            }
        } else {
            if (autoDosing.isPaused()) {
                autoDosing.resume();
            }
        }
    }
    
    m_saveSettingsActive = true;
}

void RemoteCommandManager::handleSaveCalibrationCommand(JsonVariant cmdVar) {
    if (!cmdVar.is<JsonObject>()) return;
    
    JsonObject cmd = cmdVar.as<JsonObject>();
    if (!cmd.containsKey("commandId") || !cmd.containsKey("payload")) return;
    
    m_pendingSaveCalibrationCmdId = cmd["commandId"].as<const char*>();
    
    JsonObject payload = cmd["payload"].as<JsonObject>();
    if (payload.containsKey("stepsPerML")) {
        long newStepsPerML = payload["stepsPerML"].as<long>();
        
        Serial.printf("[RemoteCmd] SAVE_CALIBRATION: cmdId=%s, stepsPerML=%ld\n", 
                      m_pendingSaveCalibrationCmdId.c_str(), newStepsPerML);
        
        if (newStepsPerML > 0) {
            PumpController &pump = PumpController::getInstance();
            pump.setDosingStepsPerML((float)newStepsPerML);
            pump.saveStepsPerML((float)newStepsPerML);
            Serial.printf("[RemoteCmd] Saved new stepsPerML to EEPROM: %ld\n", newStepsPerML);
            
            m_saveCalibrationActive = true;
        }
    }
}

void RemoteCommandManager::handleCalibrateCommand(JsonVariant cmdVar) {
    if (!cmdVar.is<JsonObject>()) return;
    
    JsonObject cmd = cmdVar.as<JsonObject>();
    if (!cmd.containsKey("commandId")) return;
    
    m_pendingCalibrationCmdId = cmd["commandId"].as<const char*>();
    
    Serial.printf("[RemoteCmd] Starting remote calibration for command: %s\n", 
                  m_pendingCalibrationCmdId.c_str());
    
    startRemoteCalibration();
    m_calibrationActive = true;
}

void RemoteCommandManager::handleTestDoseCommand(JsonVariant cmdVar) {
    if (!cmdVar.is<JsonObject>()) return;
    
    JsonObject cmd = cmdVar.as<JsonObject>();
    if (!cmd.containsKey("commandId") || !cmd.containsKey("payload")) return;
    
    m_pendingTestDoseCmdId = cmd["commandId"].as<const char*>();
    
    JsonObject payload = cmd["payload"].as<JsonObject>();
    m_pendingTestDoseSteps = payload.containsKey("steps") ? payload["steps"].as<long>() : 0;
    m_pendingTestDoseSpeed = payload.containsKey("speed") ? payload["speed"].as<int>() : 2000;
    
    Serial.printf("[RemoteCmd] TEST_DOSE: cmdId=%s, steps=%ld, speed=%d\n", 
                  m_pendingTestDoseCmdId.c_str(), m_pendingTestDoseSteps, m_pendingTestDoseSpeed);
    
    if (m_pendingTestDoseSteps > 0) {
        startRemoteTestDose(m_pendingTestDoseSteps, m_pendingTestDoseSpeed);
        m_testDoseActive = true;
    }
}

void RemoteCommandManager::update() {
    if (m_calibrationActive) {
        bool complete = updateRemoteCalibration();
        if (complete && m_pendingCalibrationCmdId.length() > 0) {
            long actualSteps = PumpController::getInstance().getCurrentPosition();
            
            JsonDocument payload;
            payload["commandId"] = m_pendingCalibrationCmdId;
            payload["status"] = "completed";
            payload["stepsCompleted"] = actualSteps;
            
            const char* pumpId = ConfigManager::getInstance().getPumpId();
            ApiClient::getInstance().completeCommand(pumpId, payload);
            
            Serial.printf("[RemoteCmd] Reported calibration complete: %ld steps\n", actualSteps);
            
            PumpController::getInstance().stop();
            DisplayManager::getInstance().setState(DisplayManager::DisplayState::NORMAL);
            
            JsonDocument settings = ApiClient::getInstance().getPumpSettings(pumpId);
            if (settings["success"] == true) {
                JsonVariant data = settings["data"];
                if (!data.isNull()) {
                    JsonVariant stepsPerMLVar = data["stepsPerML"];
                    if (!stepsPerMLVar.isNull()) {
                        float stepsPerML = stepsPerMLVar.as<float>();
                        if (stepsPerML > 0) {
                            PumpController::getInstance().setDosingStepsPerML(stepsPerML);
                            PumpController::getInstance().saveStepsPerML(stepsPerML);
                            Serial.printf("[RemoteCmd] Synced stepsPerML: %.2f\n", stepsPerML);
                        }
                    }
                }
            }
            
            m_pendingCalibrationCmdId = "";
            m_calibrationActive = false;
        }
    }
    
    if (m_testDoseActive) {
        bool complete = updateRemoteTestDose();
        if (complete && m_pendingTestDoseCmdId.length() > 0) {
            JsonDocument payload;
            payload["commandId"] = m_pendingTestDoseCmdId;
            payload["status"] = "completed";
            payload["stepsCompleted"] = m_pendingTestDoseSteps;
            
            const char* pumpId = ConfigManager::getInstance().getPumpId();
            ApiClient::getInstance().completeCommand(pumpId, payload);
            
            Serial.printf("[RemoteCmd] Reported test dose complete: %ld steps\n", m_pendingTestDoseSteps);
            
            m_pendingTestDoseCmdId = "";
            m_testDoseActive = false;
        }
    }
    
    if (m_saveCalibrationActive) {
        if (m_pendingSaveCalibrationCmdId.length() > 0) {
            Serial.printf("[RemoteCmd] Reporting SAVE_CALIBRATION complete: %s\n", m_pendingSaveCalibrationCmdId.c_str());
            reportCommandComplete(m_pendingSaveCalibrationCmdId, "completed");
            m_pendingSaveCalibrationCmdId = "";
        }
        m_saveCalibrationActive = false;
    }
    
    if (m_saveSettingsActive) {
        if (m_pendingSaveSettingsCmdId.length() > 0) {
            Serial.printf("[RemoteCmd] Reporting SAVE_SETTINGS complete: %s\n", m_pendingSaveSettingsCmdId.c_str());
            reportCommandComplete(m_pendingSaveSettingsCmdId, "completed");
            m_pendingSaveSettingsCmdId = "";
        }
        m_saveSettingsActive = false;
    }
}

bool RemoteCommandManager::isCommandActive() {
    return m_calibrationActive || m_testDoseActive || m_saveCalibrationActive || m_saveSettingsActive;
}

void RemoteCommandManager::getCommandIds(String& calibCmdId, String& testDoseCmdId) {
    calibCmdId = m_pendingCalibrationCmdId;
    testDoseCmdId = m_pendingTestDoseCmdId;
}

void RemoteCommandManager::reportCommandComplete(const String& commandId, const String& status, const String& error) {
    JsonDocument payload;
    payload["commandId"] = commandId;
    payload["status"] = status;
    if (error.length() > 0) {
        payload["error"] = error;
    }
    
    const char* pumpId = ConfigManager::getInstance().getPumpId();
    bool success = ApiClient::getInstance().completeCommand(pumpId, payload);
    
    if (success) {
        Serial.printf("[RemoteCmd] Reported %s complete\n", commandId.c_str());
    } else {
        Serial.printf("[RemoteCmd] Failed to report %s complete\n", commandId.c_str());
    }
}
