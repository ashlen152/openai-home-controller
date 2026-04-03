/**
 * @file DisplayUpdater.h
 * @brief Main loop display update orchestrator.
 *
 * Called every loop() iteration to populate DisplayContext with current
 * system state (pump status, WiFi time, auto-dosing state) and trigger
 * DisplayManager::updateDisplayState() to render the appropriate screen.
 */

#ifndef DISPLAY_UPDATER_H
#define DISPLAY_UPDATER_H

void updateDisplayStatus();

#endif
