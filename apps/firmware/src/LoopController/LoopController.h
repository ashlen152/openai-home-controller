#pragma once

#include <Arduino.h>

void handleButtons();
void handleDisplay();
void handlePump();
void handleNetworkResponses();
void handleAutoDosing(unsigned long currentTime);