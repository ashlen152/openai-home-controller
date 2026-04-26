#include "DisplayManager.h"
#include <time.h>

DisplayManager::DisplayManager()
    : m_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

DisplayManager &DisplayManager::getInstance()
{
  static DisplayManager instance;
  return instance;
}

// Call this in your main loop to update display every second
void DisplayManager::updateDisplayState()
{
  unsigned long now = millis();
  if (now - m_lastUpdate < 200)
    return;
  m_lastUpdate = now;

  if (!m_dirty && memcmp(&m_ctx, &m_prevCtx, sizeof(DisplayContext)) == 0 && 
      m_currentState != DisplayState::NORMAL)
    return;
  
  m_dirty = false;
  memcpy(&m_prevCtx, &m_ctx, sizeof(DisplayContext));

  m_display.clearDisplay();

  // States that are handled manually and should not auto-revert
  bool isManualState = (m_currentState == DisplayState::CALIBRATE_BEGIN ||
                        m_currentState == DisplayState::CALIBRATE_PROGRESS ||
                        m_currentState == DisplayState::CALIBRATE_COMPLETE ||
                        m_currentState == DisplayState::DOSING_SETUP ||
                        m_currentState == DisplayState::DOSING_PROGRESS ||
                        m_currentState == DisplayState::DOSING_COMPLETE);

  // Timing logic for STATUS state
  if ((m_currentState != DisplayState::NORMAL || m_currentState != DisplayState::MENU) && now - stateChangeTime > 3000)
  {
    m_currentState = DisplayState::NORMAL;
  }
  // Single switch-case for all states
  switch (m_currentState)
  {
  case DisplayState::NORMAL:
    updateStatus(m_ctx.pumpEnabled, m_ctx.value, m_ctx.currentTime, m_ctx.autodosingEnabled, m_ctx.nextSchedule, m_ctx.totalVolume, m_ctx.stepsPerML, m_ctx.activeProfile);
    stateChangeTime = now;
    break;
  case DisplayState::MENU:
    stateChangeTime = now;
    showMenu(m_ctx.menuIndex, m_ctx.m_menuItems, m_ctx.m_menuItemCount);
    break;
  case DisplayState::SETTINGS:
    showSettingsInfo(m_ctx.currentSpeed, m_ctx.stepsPerML, m_ctx.speedStep);
    break;
  case DisplayState::CALIBRATE_BEGIN:
    showCalibrationStart(m_ctx.timeLeft);
    break;
  case DisplayState::CALIBRATE_PROGRESS:
    showCalibrationInput(m_ctx.ml);
    break;
  case DisplayState::CALIBRATE_COMPLETE:
    showCalibrationResult(m_ctx.stepsPerML, m_ctx.speedStep);
    break;
  case DisplayState::DOSING_SETUP:
    showDosingManualSetup(m_ctx.value);
    break;
  case DisplayState::DOSING_BEGIN:
    showDosingManualBegin(m_ctx.duration);
    break;
  case DisplayState::DOSING_PROGRESS:
    showDosingManualProgress(m_ctx.value, m_ctx.remainingVolume, m_ctx.remainingTime);
    break;
  case DisplayState::DOSING_COMPLETE:
    showDosingManualComplete(m_ctx.totalVolume);
    break;
  case DisplayState::DOSE_HISTORY:  // Phase 3 Sprint 6
    showDoseHistory();
    break;
  case DisplayState::STATUS:
    updateStatus(m_ctx.pumpEnabled, m_ctx.value, m_ctx.currentTime, m_ctx.autodosingEnabled, m_ctx.nextSchedule);
    break;
  default:
    updateStatus(m_ctx.pumpEnabled, m_ctx.value, m_ctx.currentTime, m_ctx.autodosingEnabled, m_ctx.nextSchedule);
    break;
  }
  lastState = m_currentState;
  m_display.display();
}

void DisplayManager::begin()
{
  m_display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  m_display.clearDisplay();
  m_display.setTextSize(1);
  m_display.setTextColor(SSD1306_WHITE);
  m_display.setCursor(0, 0);
  m_display.println(F("Hello OLED!"));
  displaySignalStrength();
  m_display.display();
}

void DisplayManager::setSignalStrength(int strength)
{
  rssi = strength;
}

void DisplayManager::updateStatus(bool pumpEnabled, float value, const char *currentTime, bool autodosingEnabled, const char *nextSchedule, float totalVolume, float stepsPerML, int activeProfile)
{
  if (isDisplayInUse(DisplayManager::DisplayState::NORMAL))
    return;
  m_display.clearDisplay();
  m_display.setCursor(0, 0);

  const char* profileName = "???";
  if (activeProfile == 0) profileName = "Slow";
  else if (activeProfile == 1) profileName = "Med";
  else if (activeProfile == 2) profileName = "Fast";

  m_display.print("Steps:");
  m_display.print(stepsPerML, 0);
  display.println("/ml");

  m_display.print("Profile:");
  m_display.println(profileName);

  m_display.print("Auto:");
  m_display.print(autodosingEnabled ? "ON" : "OFF");
  m_display.print(" Vol:");
  m_display.print(totalVolume, 1);
  m_display.println("ml");

  if (autodosingEnabled && nextSchedule) {
    m_display.print("Next:");
    m_display.println(nextSchedule);
  }
  
  display.print("Time:");
  if (currentTime)
    m_display.println(currentTime);
  else
    m_display.println("--:--:--");

  displaySignalStrength();
  m_display.display();
}

void DisplayManager::showHomeStatus(float stepsPerML, int activeProfile)
{
  if (isDisplayInUse(DisplayManager::DisplayState::NORMAL))
    return;
  display.clearDisplay();
  display.setCursor(0, 0);

  const char* profileName = "Unknown";
  if (activeProfile == 0) profileName = "Slow";
  else if (activeProfile == 1) profileName = "Medium";
  else if (activeProfile == 2) profileName = "Fast";

  display.print("Steps/mL:");
  display.println(stepsPerML, 0);
  display.print("Profile:");
  display.println(profileName);

  displaySignalStrength();
  display.display();
}

bool DisplayManager::isDisplayInUse(DisplayManager::DisplayState state)
{
  if (currentState == state && !displaySleeping)
    return false;
  return true;
}

void DisplayManager::showMenu(int menuIndex, const char *menuItems[], int itemCount)
{
  if (isDisplayInUse(DisplayManager::DisplayState::MENU))
    return;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Menu:");
  for (int i = 0; i < itemCount; i++)
  {
    display.print(menuIndex == i ? "> " : "  ");
    display.println(menuItems[i]);
  }
  displaySignalStrength(); // Will be updated by caller if needed
  display.display();
}

void DisplayManager::showSettingsInfo(int currentSpeed, float stepsPerML, int speedStep)
{
  if (isDisplayInUse(DisplayManager::DisplayState::SETTINGS))
    return;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Settings Info:");
  display.print("Speed: ");
  display.println(currentSpeed);
  display.print("Steps/mL: ");
  display.println(stepsPerML, 2);
  display.print("Step Adj: ");
  display.println(speedStep);
  displaySignalStrength();
  display.display();
}

void DisplayManager::showCalibrationStart(int timeLeft)
{
  if (isDisplayInUse(DisplayManager::DisplayState::CALIBRATE_BEGIN))
    return;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibrating...");
  display.print("Time left: ");
  display.print(timeLeft);
  display.println("s");
  displaySignalStrength();
  display.display();
}

void DisplayManager::showCalibrationInput(float ml)
{
  if (isDisplayInUse(DisplayManager::DisplayState::CALIBRATE_PROGRESS))
    return;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Enter mL result:");
  display.setCursor(0, 20);
  display.print("mL: ");
  display.print(ml);
  display.print("   ");
  displaySignalStrength();
  display.display();
}

void DisplayManager::showCalibrationResult(float stepsPerML, int speedStep)
{
  if (isDisplayInUse(DisplayManager::DisplayState::CALIBRATE_COMPLETE))
    return;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibration Done");
  display.print("Steps/mL: ");
  display.println(stepsPerML, 2);
  display.print("Step Adj: ");
  display.println(speedStep);
  displaySignalStrength();
  display.display();
}

void DisplayManager::showText(const char *text)
{
  dirty = true;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(text);
  displaySignalStrength();
  display.display();
}

void DisplayManager::showText(const std::vector<String> &textArray)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  for (size_t i = 0; i < textArray.size(); i++)
  {
    display.println(textArray[i]);
  }
  displaySignalStrength();
  display.display();
}

void DisplayManager::sleepDisplay()
{
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  displaySleeping = true;
}

void DisplayManager::wakeDisplay()
{
  display.ssd1306_command(SSD1306_DISPLAYON);
  displaySleeping = false;
}

void DisplayManager::showDosingManualSetup(float volume)
{
  if (isDisplayInUse(DisplayManager::DisplayState::DOSING_SETUP))
    return;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Manual Dosing Setup");
  display.println();

  display.println("Set Target Volume:");
  display.print(volume, 2);
  display.println(" mL");
  display.println();
  display.println("UP/DOWN to adjust");
  display.println("ENABLE to confirm");
  display.println("MENU to cancel");

  displaySignalStrength();
  display.display();
}

void DisplayManager::showDosingManualBegin(int duration)
{
  if (isDisplayInUse(DisplayManager::DisplayState::DOSING_MANUAL_BEGIN))
    return;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Manual Dosing Setup");
  display.println();

  display.println("Set Time Duration:");
  display.print(duration);
  display.println(" min");
  display.println();
  display.println("UP/DOWN to adjust");
  display.println("ENABLE to start");
  display.println("MENU to cancel");

  displaySignalStrength();
  display.display();
}

void DisplayManager::showDosingManualProgress(float volume, float remainingVolume, const char *remainingTime)
{
  if (isDisplayInUse(DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS))
    return;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Dosing in Progress");
  display.println();

  display.print("Total: ");
  display.print(volume, 2);
  display.println(" mL");

  display.print("Remain: ");
  display.print(remainingVolume, 2);
  display.println(" mL");

  display.println();
  display.print("Time Left: ");
  display.println(remainingTime);

  displaySignalStrength();
  display.display();
}

void DisplayManager::showDosingManualComplete(float totalVolume)
{
  if (isDisplayInUse(DisplayManager::DisplayState::DOSING_MANUAL_COMPLETE))
    return;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Dosing Complete!");
  display.println();

  display.print("Total Dosed: ");
  display.print(totalVolume, 2);
  display.println(" mL");

  displaySignalStrength();
  display.display();
}

// Phase 3 Sprint 6: Show dose history
// Note: This will be called from updateDisplayState when state is DOSE_HISTORY
// The history data should be fetched in the menu handler before setting the state
void DisplayManager::showDoseHistory()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  
  display.println("=== Dose History ===");
  display.println();
  display.println("View from menu");
  display.println("Press any button");
  display.println("to exit");
  
  display.display();
}

void DisplayManager::displaySignalStrength()
{
  display.setCursor(0, SCREEN_HEIGHT - 8);
  if (rssi < -50)
  {
    drawWiFiSignal(4);
  }
  else if (rssi < -60)
  {
    drawWiFiSignal(3);
  }
  else if (rssi < -70)
  {
    drawWiFiSignal(2);
  }
  else if (rssi < -80)
  {
    drawWiFiSignal(1);
  }
  else
  {
    drawWiFiSignal(0);
  }
}

void DisplayManager::drawWiFiSignal(int strength)
{
  // strength: 0 to 4
  for (int i = 0; i < 4; i++)
  {
    int barHeight = 2;
    if (i < strength)
    {
      display.fillRect(0 + i * 4, SCREEN_HEIGHT - barHeight, 3, barHeight, WHITE);
    }
    else
    {
      display.drawRect(0 + i * 4, SCREEN_HEIGHT - barHeight, 3, barHeight, WHITE);
    }
  }
}

void DisplayManager::showValue(const char *label, float value)
{
  if (isDisplayInUse(DisplayManager::DisplayState::NORMAL))
    return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Show label
  display.setCursor(0, 0);
  display.println(label);

  // Show value in larger text
  display.setTextSize(1);
  display.setCursor(0, 16);
  display.print(value, 1);

  displaySignalStrength();
  display.display();
}
