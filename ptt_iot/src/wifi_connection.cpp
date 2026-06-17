#include "wifi_connection.h"
#include "config.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>

// Status Jaringan Global
String localIPAddress = "0.0.0.0";
bool isAPModeActive = false;

// DNS Server untuk Captive Portal
DNSServer dnsServer;
const byte DNS_PORT = 53;

void setupWiFi() {
  Serial.print("\n[WiFi] Mencoba terhubung ke: ");
  Serial.println(activeConfig.wifi_ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(activeConfig.wifi_ssid, activeConfig.wifi_password);
  
  int timeout_seconds = 15;
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < (timeout_seconds * 2)) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    isAPModeActive = false;
    localIPAddress = WiFi.localIP().toString();
    Serial.println("\n[WiFi] Terhubung!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(localIPAddress);
    
    // Inisialisasi MDNS (http://aquavion.local)
    if (MDNS.begin("aquavion")) {
      Serial.println("[mDNS] Responder berhasil dijalankan di http://aquavion.local");
    }
  } else {
    Serial.println("\n[WiFi] Koneksi gagal! Masuk ke mode Fallback AP.");
    startAPFallback();
  }
}

void startAPFallback() {
  isAPModeActive = true;
  localIPAddress = "192.168.4.1";
  
  // Set ESP32 sebagai Access Point
  const char* ap_ssid = "AquaVion-Config-AP";
  const char* ap_pass = "12345678";
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_pass);
  
  Serial.println("[AP] Access Point diaktifkan!");
  Serial.printf("[AP] SSID: %s\n", ap_ssid);
  Serial.printf("[AP] Sandi: %s\n", ap_pass);
  Serial.printf("[AP] IP Config: http://%s/\n", localIPAddress.c_str());
  
  // Setup DNS Server untuk mengarahkan semua kueri DNS ke IP ESP32 (Captive Portal)
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("[DNS] Captive Portal siap dialihkan.");
  
  // Inisialisasi MDNS juga untuk AP (jika mDNS client terhubung)
  if (MDNS.begin("aquavion")) {
    Serial.println("[mDNS] Responder siap di http://aquavion.local");
  }
}

bool isWiFiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}
