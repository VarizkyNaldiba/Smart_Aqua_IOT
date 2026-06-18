#include "config.h"
#include <Preferences.h>

// Definisi variabel global
DeviceConfig activeConfig;

Preferences preferences;

void loadDeviceConfig() {
  preferences.begin("aquavion", false);

  // Ambil nilai string dengan default value
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("pass", "");
  String url = preferences.getString("url", "https://aqua-vion.vercel.app/api/mqtt/receive");
  String uid = preferences.getString("uid", "8JAGfvb3EXPKijhoAllz2ObC0Bf2");
  String did = preferences.getString("did", "AQN-IOT-001");

  // Ambil nilai float dengan default value (sesuai kalibrasi asal di main.cpp)
  float v_clear = preferences.getFloat("v_clear", 2.950f);
  float ph_v_neut = preferences.getFloat("ph_v_neut", 2.533f);
  float ph_slope = preferences.getFloat("ph_slope", 0.18f);
  float pond_height = preferences.getFloat("pond_height", 50.0f); // Default 50 cm

  preferences.end();

  // Salin ke struct global activeConfig
  strncpy(activeConfig.wifi_ssid, ssid.c_str(), sizeof(activeConfig.wifi_ssid));
  strncpy(activeConfig.wifi_password, pass.c_str(), sizeof(activeConfig.wifi_password));
  strncpy(activeConfig.server_url, url.c_str(), sizeof(activeConfig.server_url));
  strncpy(activeConfig.user_id, uid.c_str(), sizeof(activeConfig.user_id));
  strncpy(activeConfig.device_id, did.c_str(), sizeof(activeConfig.device_id));
  
  activeConfig.v_clear = v_clear;
  activeConfig.ph_v_neutral = ph_v_neut;
  activeConfig.ph_slope = ph_slope;
  activeConfig.pond_height = pond_height;

  Serial.println("--- Konfigurasi Dimuat dari NVS ---");
  Serial.printf("SSID: %s\n", activeConfig.wifi_ssid);
  Serial.printf("API URL: %s\n", activeConfig.server_url);
  Serial.printf("Device ID: %s\n", activeConfig.device_id);
  Serial.printf("V_CLEAR: %.3f V\n", activeConfig.v_clear);
  Serial.printf("pH V_Neutral: %.3f V | Slope: %.3f\n", activeConfig.ph_v_neutral, activeConfig.ph_slope);
  Serial.printf("Pond Height: %.2f cm\n", activeConfig.pond_height);
  Serial.println("----------------------------------");
}

void saveDeviceConfig(const DeviceConfig &newConfig) {
  preferences.begin("aquavion", false);

  preferences.putString("ssid", newConfig.wifi_ssid);
  preferences.putString("pass", newConfig.wifi_password);
  preferences.putString("url", newConfig.server_url);
  preferences.putString("uid", newConfig.user_id);
  preferences.putString("did", newConfig.device_id);

  preferences.putFloat("v_clear", newConfig.v_clear);
  preferences.putFloat("ph_v_neut", newConfig.ph_v_neutral);
  preferences.putFloat("ph_slope", newConfig.ph_slope);
  preferences.putFloat("pond_height", newConfig.pond_height);

  preferences.end();

  // Update memory
  activeConfig = newConfig;
  Serial.println("[NVS] Konfigurasi baru berhasil disimpan!");
}
