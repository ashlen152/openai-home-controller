#ifndef REMOTE_COMMAND_MANAGER_H
#define REMOTE_COMMAND_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

class RemoteCommandManager {
public:
    static RemoteCommandManager& getInstance();
    
    void parseCommands(const String& responseStr);
    void parseCommandsFromJson(JsonDocument& doc);
    void update();
    bool isCommandActive();
    void getCommandIds(String& calibCmdId, String& testDoseCmdId);
    
private:
    RemoteCommandManager();
    
    void handleSaveCalibrationCommand(JsonVariant cmd);
    void handleCalibrateCommand(JsonVariant cmd);
    void handleTestDoseCommand(JsonVariant cmd);
    void handleSaveSettingsCommand(JsonVariant cmd);
    void reportCommandComplete(const String& commandId, const String& status, const String& error = "");
    
    bool m_calibrationActive;
    bool m_testDoseActive;
    bool m_saveCalibrationActive;
    bool m_saveSettingsActive;
    String m_pendingCalibrationCmdId;
    String m_pendingTestDoseCmdId;
    String m_pendingSaveCalibrationCmdId;
    String m_pendingSaveSettingsCmdId;
    long m_pendingTestDoseSteps;
    int m_pendingTestDoseSpeed;
};

#endif
