/**
 * @file WiFiSync.h
 * @brief WiFi connection handler and periodic data synchronization.
 *
 * handleWiFi(): Called in main loop (skipped during DOSING mode).
 *   - Attempts reconnect if disconnected (every WIFI_RETRY_INTERVAL = 5s)
 *   - On connect: syncs NTP time, updates signal strength on display
 *
 * syncData(): Sends pump telemetry (RSSI) to backend server via HTTP POST.
 *   Currently partially commented out - only sends signal strength.
 */

#ifndef WIFISYNC_H
#define WIFISYNC_H

void syncData();

void handleWiFi(unsigned long currentTime, unsigned long &lastWiFiRetryTime);

#endif