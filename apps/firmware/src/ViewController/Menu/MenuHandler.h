/**
 * @file MenuHandler.h
 * @brief Menu screen controller - navigable menu with 4 items.
 *
 * Menu items:
 *   0: "Dosing Cal"   -> CALIBRATE_BEGIN state (calibration flow)
 *   1: "Settings Info" -> SETTINGS state (show speed, steps/mL)
 *   2: "Auto Dosing"  -> Toggle auto dosing (WIP, currently goes to NORMAL)
 *   3: "Set Daily Vol" -> DOSING_SETUP state (volume configuration)
 *
 * Navigation:
 *   - Menu button: Enter menu (from Home) or select current item
 *   - Up/Down (hold): Navigate menu items (wraps around)
 */

#ifndef MENU_HANDLER_H
#define MENU_HANDLER_H

void runMenuSelection();
void MenuHandler();
bool isInMenu();

#endif
