#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <Arduino.h>

// Status Jaringan
extern String localIPAddress;
extern bool isAPModeActive;

// Fungsi Jaringan
void setupWiFi();
void startAPFallback();
bool isWiFiConnected();

#endif // WIFI_CONNECTION_H
