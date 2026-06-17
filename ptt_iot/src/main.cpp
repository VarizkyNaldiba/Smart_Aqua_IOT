#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h> 
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#include "config.h"
#include "sensors.h"
#include "wifi_connection.h"
#include "web_server.h"

// Flag eksternal untuk mendelegasikan reboot dari web portal
extern bool shouldReboot;

// Prototipe Fungsi
void sendDataToVercel();

// ==========================================
// 1. SETUP: INISIALISASI PERANGKAT
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n==================================");
  Serial.println("   Memulai Sistem AquaVion...     ");
  Serial.println("==================================");

  // A. Muat konfigurasi dari NVS Preferences
  loadDeviceConfig();

  // B. Inisialisasi pin & driver sensor fisik
  initSensors();

  // C. Inisialisasi koneksi WiFi (Fallback AP jika gagal)
  setupWiFi();

  // D. Jalankan local Web Server
  startWebServer();

  Serial.println("\n--- Sistem AquaVion Ready ---");
}

// ==========================================
// 2. LOOP: EKSEKUSI UTAMA (NON-BLOCKING)
// ==========================================
void loop() {
  // Tangani request klien web portal & captive portal
  handleWebServerClients();

  // Reboot aman apabila diinstruksikan oleh Web Portal
  if (shouldReboot) {
    Serial.println("[System] Mereboot perangkat dalam 1 detik...");
    delay(1000);
    ESP.restart();
  }

  // Timer pembacaan sensor (setiap 5 detik)
  static unsigned long lastReading = 0;
  if (millis() - lastReading > 5000) {
    lastReading = millis();
    readAllSensors();

    // Log pembacaan sensor ke Serial Monitor
    Serial.println("\n--- MONITORING DATA ---");
    Serial.printf("SUHU      : %.2f C %s\n", 
                  currentSensorData.temperature, 
                  (currentSensorData.temperature < 25.0 || currentSensorData.temperature > 30.0) ? "[ABNORMAL]" : "[OK]");
    Serial.printf("PH        : %.2f %s (RAW ADC: %d | VOLT: %.3f V)\n", 
                  currentSensorData.ph, 
                  (currentSensorData.ph < 6.5 || currentSensorData.ph > 8.5) ? "[ABNORMAL]" : "[OK]", 
                  currentSensorData.raw_ph_adc, 
                  currentSensorData.raw_ph_volt);
    Serial.printf("TURBIDITY : %.2f NTU %s (RAW ADC: %d | VOLT: %.3f V)\n", 
                  currentSensorData.turbidity, 
                  (currentSensorData.turbidity > 50.0) ? "[ABNORMAL]" : "[OK]", 
                  currentSensorData.raw_turbidity_adc, 
                  currentSensorData.raw_turbidity_volt);
    Serial.printf("JARAK AIR : %.2f cm\n", currentSensorData.water_level);
    Serial.printf("BUZZER    : %s\n", currentSensorData.is_alarm_active ? "ON (ALARM)" : "OFF");
    Serial.println("-----------------------");
  }

  // Timer pengiriman data ke server Vercel (setiap 15 detik)
  static unsigned long lastPublish = 0;
  if (millis() - lastPublish > 15000) {
    lastPublish = millis();
    sendDataToVercel();
  }
}

// ==========================================
// 3. PENGIRIMAN DATA TELEMETRI
// ==========================================
void sendDataToVercel() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // Mengizinkan HTTPS tanpa verifikasi sertifikat
    
    HTTPClient http;
    if (http.begin(client, activeConfig.server_url)) {
      http.addHeader("Content-Type", "application/json");

      // Membuat payload JSON
      JsonDocument doc;
      doc["userId"] = activeConfig.user_id;
      doc["device_id"] = activeConfig.device_id;
      doc["ssid"] = WiFi.SSID(); 
      doc["temperature"] = currentSensorData.temperature;
      doc["ph"] = currentSensorData.ph;
      doc["turbidity"] = currentSensorData.turbidity;
      doc["waterLevel"] = currentSensorData.water_level;

      String jsonString;
      serializeJson(doc, jsonString);

      Serial.print("[Cloud] Mengirim data ke: ");
      Serial.println(activeConfig.server_url);
      
      int httpResponseCode = http.POST(jsonString);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("[Cloud] HTTP Code: ");
        Serial.println(httpResponseCode);
        Serial.println("[Cloud] Response: " + response);
      } else {
        Serial.print("[Cloud] Gagal mengirim POST. Error: ");
        Serial.println(http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    } else {
      Serial.println("[Cloud] Gagal memulai koneksi HTTP.");
    }
  } else {
    Serial.println("[Cloud] WiFi tidak terhubung, pengiriman dilewati.");
  }
}