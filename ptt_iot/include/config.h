#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. DEFINISI PIN SENSOR & AKTUATOR
// ==========================================
#define PIN_SUHU       26   // Sensor DS18B20
#define PIN_PH         34   // Sensor pH (Analog)
#define PIN_TURBIDITY  35   // Sensor Turbidity (Analog)
#define PIN_TRIG       5    // Ultrasonik Trigger
#define PIN_ECHO       18   // Ultrasonik Echo
#define PIN_BUZZER     2    // Alarm/Indikator

// ==========================================
// 2. STRUKTUR KONFIGURASI PERANGKAT
// ==========================================
struct DeviceConfig {
  char wifi_ssid[33];
  char wifi_password[65];
  char server_url[128];
  char user_id[64];
  char device_id[32];
  float v_clear;         // Kalibrasi Turbidity: Tegangan air jernih
  float ph_v_neutral;    // Kalibrasi pH: Tegangan saat pH 7.0 (V_neutral)
  float ph_slope;        // Kalibrasi pH: V per unit pH (Default: 0.18)
};

// Variabel Konfigurasi Global
extern DeviceConfig activeConfig;

// ==========================================
// 3. FUNGSI LOAD & SAVE (NVS / Preferences)
// ==========================================
void loadDeviceConfig();
void saveDeviceConfig(const DeviceConfig &newConfig);

#endif // CONFIG_H
