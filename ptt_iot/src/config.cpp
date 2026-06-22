#include "config.h"
#include <Preferences.h>
#include <SPIFFS.h>

// Definisi variabel global
DeviceConfig activeConfig;

Preferences preferences;

void loadDeviceConfig() {
  // Initialize filesystem
  if(!SPIFFS.begin(true)) {
    Serial.println("[NVS] Gagal menginisialisasi SPIFFS");
  }
  preferences.begin("aquavion", false);

  // Ambil nilai string dengan default value
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("pass", "");
  String url = preferences.getString("url", "https://aqua-vion.vercel.app/api/mqtt/receive");
  String uid = preferences.getString("uid", "8JAGfvb3EXPKijhoAllz2ObC0Bf2");
  String did = preferences.getString("did", "AQN-IOT-001");

  // Ambil nilai float dengan default value (sesuai kalibrasi asal di main.cpp)
  float v_clear = preferences.getFloat("v_clear", 3.099f);
  float ph_v_neut = preferences.getFloat("ph_v_neut", 2.587f);
  float ph_slope = preferences.getFloat("ph_slope", 0.0896f);
  float pond_height = preferences.getFloat("pond_height", 67.0f); // Default 67 cm
  float ultrasonic_offset = preferences.getFloat("ultrasonic_offset", 0.0f);
  float ultrasonic_scale = preferences.getFloat("ultrasonic_scale", 1.0f);

  // Jika nilai di NVS masih nilai default lama (yang salah), timpa dengan nilai kalibrasi baru
  if (v_clear == 3.07f || v_clear == 2.95f || v_clear == 2.950f) {
    v_clear = 3.099f;
    preferences.putFloat("v_clear", 3.099f);
  }
  if (ph_v_neut == 3.26f || ph_v_neut == 2.533f || ph_v_neut == 2.53f) {
    ph_v_neut = 2.587f;
    preferences.putFloat("ph_v_neut", 2.587f);
  }
  if (ph_slope == 0.18f || ph_slope == 0.073f) {
    ph_slope = 0.0896f;
    preferences.putFloat("ph_slope", 0.0896f);
  }

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
  activeConfig.ultrasonic_offset = ultrasonic_offset;
  activeConfig.ultrasonic_scale = ultrasonic_scale;

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
  preferences.putFloat("ultrasonic_offset", newConfig.ultrasonic_offset);
  preferences.putFloat("ultrasonic_scale", newConfig.ultrasonic_scale);

  preferences.end();

  // Update memory
  activeConfig = newConfig;
  Serial.println("[NVS] Konfigurasi baru berhasil disimpan!");
}

// ==========================================
// 4. LOGGING SYSTEM FOR WEB SERIAL MONITOR
// ==========================================
String systemLogs[LOG_MAX_LINES];
int logHead = 0;
int logCount = 0;

void addLog(const String &line) {
  // Print to Serial as usual
  Serial.println(line);

  // Store in circular buffer
  systemLogs[logHead] = line;
  logHead = (logHead + 1) % LOG_MAX_LINES;
  if (logCount < LOG_MAX_LINES) {
    logCount++;
  }
}

void addLogf(const char *format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  // Strip trailing newline if any
  String line = String(buf);
  if (line.endsWith("\n")) {
    line.remove(line.length() - 1);
  }
  addLog(line);
}

// ------------------------------------------
// Calibration logging for ultrasonic sensor
// ------------------------------------------
void logUltrasonicCalibration(float offset, float scale) {
  // Create CSV line: timestamp,offset,scale
  unsigned long ts = millis(); // milliseconds since boot
  char csvLine[64];
  snprintf(csvLine, sizeof(csvLine), "%lu,%.3f,%.3f", ts, offset, scale);

  // Append to file on SPIFFS
  File f = SPIFFS.open("/calibration_log.csv", FILE_APPEND);
  if (f) {
    f.println(csvLine);
    f.close();
  } else {
    Serial.println("[Log] Gagal membuka file log kalibrasi");
  }
}
