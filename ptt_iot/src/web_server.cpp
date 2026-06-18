#include "web_server.h"
#include "config.h"
#include "sensors.h"
#include "wifi_connection.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>

WebServer server(80);
extern DNSServer dnsServer; // dideklarasikan di wifi_connection.cpp

// Flag untuk memicu reboot di main loop
bool shouldReboot = false;

// HTML Portal Konfigurasi & Dashboard (Disimpan di FLASH PROGMEM)
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AquaVion - Panel Monitoring & Konfigurasi IoT</title>
  <style>
    :root {
      --bg-gradient: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);
      --card-bg: rgba(30, 41, 59, 0.45);
      --border-color: rgba(255, 255, 255, 0.08);
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --primary: #0ea5e9;
      --secondary: #10b981;
      --warning: #f59e0b;
      --danger: #ef4444;
    }
    
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    
    body {
      background: var(--bg-gradient);
      color: var(--text-main);
      font-family: system-ui, -apple-system, sans-serif;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 20px;
    }

    header {
      width: 100%;
      max-width: 1100px;
      margin-bottom: 24px;
      text-align: center;
    }

    header h1 {
      font-size: 28px;
      font-weight: 700;
      letter-spacing: -0.5px;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 10px;
    }

    header p {
      color: var(--text-muted);
      font-size: 14px;
      margin-top: 4px;
    }

    .main-container {
      width: 100%;
      max-width: 1100px;
      display: grid;
      grid-template-columns: 1.2fr 1fr;
      gap: 24px;
    }

    @media (max-width: 900px) {
      .main-container {
        grid-template-columns: 1fr;
      }
    }

    .panel {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--border-color);
      border-radius: 16px;
      padding: 24px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.25);
    }

    .panel-title {
      font-size: 18px;
      font-weight: 600;
      margin-bottom: 20px;
      padding-bottom: 10px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.05);
      display: flex;
      align-items: center;
      gap: 8px;
    }

    /* Telemetry Cards Grid */
    .telemetry-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 16px;
      margin-bottom: 24px;
    }

    @media (max-width: 480px) {
      .telemetry-grid {
        grid-template-columns: 1fr;
      }
    }

    .t-card {
      background: rgba(15, 23, 42, 0.4);
      border: 1px solid rgba(255, 255, 255, 0.04);
      border-radius: 12px;
      padding: 16px;
      display: flex;
      flex-direction: column;
      position: relative;
      transition: all 0.3s ease;
    }

    .t-card:hover {
      border-color: rgba(14, 165, 233, 0.3);
      transform: translateY(-2px);
    }

    .t-label {
      font-size: 12px;
      color: var(--text-muted);
      font-weight: 500;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }

    .t-value {
      font-size: 26px;
      font-weight: 700;
      margin: 8px 0;
      color: var(--text-main);
    }

    .t-status {
      font-size: 11px;
      font-weight: 600;
      padding: 2px 6px;
      border-radius: 4px;
      align-self: flex-start;
    }

    .status-ok {
      background: rgba(16, 185, 129, 0.15);
      color: var(--secondary);
    }

    .status-warning {
      background: rgba(239, 68, 68, 0.15);
      color: var(--danger);
    }

    .status-none {
      background: rgba(148, 163, 184, 0.15);
      color: var(--text-muted);
    }

    /* System Diagnostics info */
    .system-info {
      background: rgba(15, 23, 42, 0.3);
      border-radius: 12px;
      padding: 16px;
      border: 1px solid rgba(255, 255, 255, 0.03);
    }

    .info-row {
      display: flex;
      justify-content: space-between;
      padding: 6px 0;
      font-size: 13px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.03);
    }

    .info-row:last-child {
      border-bottom: none;
    }

    .info-label {
      color: var(--text-muted);
    }

    .info-value {
      font-weight: 600;
    }

    /* Form Styles */
    .form-group {
      margin-bottom: 16px;
    }

    .form-group label {
      display: block;
      font-size: 13px;
      color: var(--text-muted);
      margin-bottom: 6px;
      font-weight: 500;
    }

    .form-row {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }

    input {
      width: 100%;
      background: rgba(15, 23, 42, 0.6);
      border: 1px solid rgba(255, 255, 255, 0.12);
      color: var(--text-main);
      padding: 10px 14px;
      border-radius: 8px;
      font-size: 14px;
      transition: all 0.2s ease;
    }

    input:focus {
      outline: none;
      border-color: var(--primary);
      box-shadow: 0 0 0 3px rgba(14, 165, 233, 0.2);
    }

    .btn-container {
      margin-top: 24px;
      display: flex;
      gap: 12px;
    }

    .btn {
      flex: 1;
      padding: 12px;
      font-size: 14px;
      font-weight: 600;
      border-radius: 8px;
      border: none;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      transition: all 0.2s ease;
    }

    .btn-primary {
      background: linear-gradient(135deg, #0ea5e9 0%, #0284c7 100%);
      color: white;
    }

    .btn-primary:hover {
      box-shadow: 0 4px 12px rgba(14, 165, 233, 0.3);
      opacity: 0.95;
    }

    .btn-danger {
      background: rgba(239, 68, 68, 0.15);
      color: var(--danger);
      border: 1px solid rgba(239, 68, 68, 0.3);
    }

    .btn-danger:hover {
      background: var(--danger);
      color: white;
    }

    /* Toast Notification */
    #toast {
      visibility: hidden;
      min-width: 250px;
      background-color: #1e293b;
      color: #fff;
      text-align: center;
      border-radius: 8px;
      padding: 16px;
      position: fixed;
      z-index: 100;
      bottom: 30px;
      font-size: 14px;
      border: 1px solid var(--primary);
      box-shadow: 0 10px 25px rgba(0,0,0,0.3);
      transition: visibility 0s, opacity 0.5s linear;
      opacity: 0;
    }

    #toast.show {
      visibility: visible;
      opacity: 1;
    }

    .badge-status {
      display: inline-block;
      width: 8px;
      height: 8px;
      border-radius: 50%;
      margin-right: 6px;
    }

    .badge-online { background-color: var(--secondary); }
    .badge-offline { background-color: var(--danger); }
  </style>
</head>
<body>

  <header>
    <h1>🌊 AquaVion IoT</h1>
    <p>Sistem Cerdas Pengendalian Kualitas Kolam Air Lele</p>
  </header>

  <div class="main-container">
    
    <!-- PANEL MONITOR DATA SENSOR -->
    <div class="panel">
      <div class="panel-title">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><line x1="9" y1="3" x2="9" y2="21"></line><line x1="15" y1="3" x2="15" y2="21"></line><line x1="3" y1="9" x2="21" y2="9"></line><line x1="3" y1="15" x2="21" y2="15"></line></svg>
        Dashboard Pemantauan Sensor
      </div>
      
      <div class="telemetry-grid">
        <!-- Temp Card -->
        <div class="t-card" id="card-temp">
          <span class="t-label">Suhu Air</span>
          <span class="t-value" id="val-temp">--.- °C</span>
          <span class="t-status status-none" id="status-temp">Membaca...</span>
        </div>
        
        <!-- pH Card -->
        <div class="t-card" id="card-ph">
          <span class="t-label">Keasaman (pH)</span>
          <span class="t-value" id="val-ph">--.--</span>
          <span class="t-status status-none" id="status-ph">Membaca...</span>
        </div>
        
        <!-- Turbidity Card -->
        <div class="t-card" id="card-turb">
          <span class="t-label">Kekeruhan</span>
          <span class="t-value" id="val-turb">--- NTU</span>
          <span class="t-status status-none" id="status-turb">Membaca...</span>
        </div>
        
        <!-- Water Level Card -->
        <div class="t-card" id="card-level">
          <span class="t-label">Jarak Air</span>
          <span class="t-value" id="val-level">--.- cm</span>
          <span class="t-status status-none" id="status-level">Membaca...</span>
        </div>
      </div>

      <div class="panel-title" style="margin-top: 24px;">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line></svg>
        Diagnostik Sistem
      </div>
      
      <div class="system-info">
        <div class="info-row">
          <span class="info-label">Mode WiFi</span>
          <span class="info-value" id="sys-wifimode">--</span>
        </div>
        <div class="info-row">
          <span class="info-label">IP Address</span>
          <span class="info-value" id="sys-ip">0.0.0.0</span>
        </div>
        <div class="info-row">
          <span class="info-label">Kekuatan Sinyal (RSSI)</span>
          <span class="info-value" id="sys-rssi">-- dBm</span>
        </div>
        <div class="info-row">
          <span class="info-label">Status Alarm</span>
          <span class="info-value" id="sys-alarm">OFF</span>
        </div>
        <div class="info-row">
          <span class="info-label">Uptime Perangkat</span>
          <span class="info-value" id="sys-uptime">0 dtk</span>
        </div>
        <div class="info-row">
          <span class="info-label">Memori Bebas</span>
          <span class="info-value" id="sys-heap">-- KB</span>
        </div>
      </div>
    </div>

    <!-- PANEL FORM KONFIGURASI -->
    <div class="panel">
      <div class="panel-title">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>
        Pengaturan Perangkat
      </div>
      
      <form action="/save" method="POST" id="config-form">
        <h3 style="font-size: 13px; color: var(--primary); margin-bottom: 12px; text-transform: uppercase; font-weight: 600;">1. Jaringan & API</h3>
        
        <div class="form-group">
          <label for="ssid">WiFi SSID</label>
          <input type="text" id="ssid" name="ssid" maxlength="32" placeholder="Nama Jaringan WiFi" required>
        </div>
        
        <div class="form-group">
          <label for="pass">WiFi Password</label>
          <input type="password" id="pass" name="pass" maxlength="64" placeholder="Sandi WiFi">
        </div>
        
        <div class="form-group">
          <label for="url">Endpoint API Server</label>
          <input type="text" id="url" name="url" maxlength="127" placeholder="https://api.domain.com/endpoint" required>
        </div>

        <div class="form-row">
          <div class="form-group">
            <label for="uid">Firebase User ID</label>
            <input type="text" id="uid" name="uid" placeholder="Firebase UID" required>
          </div>
          
          <div class="form-group">
            <label for="did">Device ID</label>
            <input type="text" id="did" name="did" placeholder="AQN-IOT-001" required>
          </div>
        </div>

        <h3 style="font-size: 13px; color: var(--primary); margin-top: 20px; margin-bottom: 12px; text-transform: uppercase; font-weight: 600;">2. Kalibrasi Sensor</h3>
        
        <div class="form-group">
          <label for="v_clear">Tegangan Kekeruhan Air Jernih (V_CLEAR)</label>
          <input type="number" id="v_clear" name="v_clear" step="0.001" placeholder="2.950" required>
        </div>
        
        <div class="form-row">
          <div class="form-group">
            <label for="ph_slope">Slope Sensor pH (V/pH)</label>
            <input type="number" id="ph_slope" name="ph_slope" step="0.001" placeholder="0.180" required>
          </div>
          
          <div class="form-group">
            <label for="pond_height">Tinggi Kolam Total (cm)</label>
            <input type="number" id="pond_height" name="pond_height" step="0.1" placeholder="50.0" required>
          </div>
        </div>

        <div class="btn-container">
          <button type="submit" class="btn btn-primary">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path><polyline points="17 21 17 13 7 13 7 21"></polyline><polyline points="7 3 7 8 15 8"></polyline></svg>
            Simpan & Terapkan
          </button>
          <button type="button" class="btn btn-danger" onclick="triggerReboot()">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>
            Reboot
          </button>
        </div>
      </form>
    </div>
  </div>

  <div id="toast">Notifikasi</div>

  <script>
    // Memuat konfigurasi aktif saat load
    function loadConfig() {
      fetch('/api/config')
        .then(res => res.json())
        .then(data => {
          document.getElementById('ssid').value = data.ssid || '';
          document.getElementById('pass').value = data.pass || '';
          document.getElementById('url').value = data.url || '';
          document.getElementById('uid').value = data.uid || '';
          document.getElementById('did').value = data.did || '';
          document.getElementById('v_clear').value = parseFloat(data.v_clear).toFixed(3) || '2.950';
          document.getElementById('ph_v_neut').value = parseFloat(data.ph_v_neut).toFixed(3) || '2.533';
          document.getElementById('ph_slope').value = parseFloat(data.ph_slope).toFixed(3) || '0.180';
          document.getElementById('pond_height').value = parseFloat(data.pond_height).toFixed(1) || '50.0';
        })
        .catch(err => showToast('Gagal memuat konfigurasi perangkat', true));
    }

    // Polling data sensor dan diagnosa
    function pollData() {
      fetch('/api/data')
        .then(res => res.json())
        .then(d => {
          // Update Sensor Readings
          updateCard('temp', d.temperature, d.temperature.toFixed(1) + ' °C', d.temperature < 25 || d.temperature > 30);
          updateCard('ph', d.ph, d.ph.toFixed(2), d.ph < 6.5 || d.ph > 8.5);
          updateCard('turb', d.turbidity, d.turbidity.toFixed(1) + ' NTU', d.turbidity > 50);
          updateCard('level', d.water_level, d.water_level.toFixed(1) + ' cm', d.water_level == 0);

          // Update Diagnostics
          document.getElementById('sys-wifimode').innerText = d.wifi_mode;
          document.getElementById('sys-ip').innerHTML = `<span class="badge-status badge-online"></span> ${d.ip_address}`;
          document.getElementById('sys-rssi').innerText = d.rssi + ' dBm';
          document.getElementById('sys-alarm').innerText = d.is_alarm_active ? 'AKTIF (ALARM ON)' : 'MATI (NORMAL)';
          document.getElementById('sys-alarm').style.color = d.is_alarm_active ? 'var(--danger)' : 'var(--secondary)';
          document.getElementById('sys-uptime').innerText = formatUptime(d.uptime);
          document.getElementById('sys-heap').innerText = (d.free_heap / 1024).toFixed(1) + ' KB';
        })
        .catch(err => {
          console.error(err);
          // Set status membaca jika kehilangan koneksi dengan ESP32
          document.getElementById('sys-ip').innerHTML = `<span class="badge-status badge-offline"></span> Terputus`;
        });
    }

    function updateCard(id, rawVal, displayVal, isAbnormal) {
      const valEl = document.getElementById('val-' + id);
      const statusEl = document.getElementById('status-' + id);
      const cardEl = document.getElementById('card-' + id);

      valEl.innerText = displayVal;
      
      if (rawVal == -127.0 || rawVal == 0 && id === 'level') {
        statusEl.innerText = "Error / Disconnect";
        statusEl.className = "t-status status-warning";
        cardEl.style.borderColor = "var(--danger)";
      } else if (isAbnormal) {
        statusEl.innerText = "Abnormal / Bahaya";
        statusEl.className = "t-status status-warning";
        cardEl.style.borderColor = "var(--danger)";
      } else {
        statusEl.innerText = "Aman / Normal";
        statusEl.className = "t-status status-ok";
        cardEl.style.borderColor = "rgba(16, 185, 129, 0.3)";
      }
    }

    function formatUptime(sec) {
      let h = Math.floor(sec / 3600);
      let m = Math.floor((sec % 3600) / 60);
      let s = sec % 60;
      return `${h}j ${m}m ${s}s`;
    }

    function showToast(message, isError = false) {
      const toast = document.getElementById('toast');
      toast.innerText = message;
      toast.style.borderColor = isError ? 'var(--danger)' : 'var(--secondary)';
      toast.className = "show";
      setTimeout(() => { toast.className = toast.className.replace("show", ""); }, 4000);
    }

    function triggerReboot() {
      if(confirm('Reboot perangkat sekarang?')) {
        fetch('/reboot')
          .then(() => {
            showToast('Memicu reboot. Koneksi akan terputus sebentar...');
          })
          .catch(() => showToast('Reboot dipicu!', false));
      }
    }

    // Inisialisasi
    window.onload = function() {
      loadConfig();
      pollData();
      setInterval(pollData, 2000);
    };
  </script>
</body>
</html>
)rawliteral";

// Callback untuk Route Index
void handleIndex() {
  server.send(200, "text/html", INDEX_HTML);
}

// Callback untuk Route API Data (Sensor & Diagnostics)
void handleApiData() {
  JsonDocument doc;
  
  // Data sensor
  doc["temperature"] = currentSensorData.temperature;
  doc["ph"] = currentSensorData.ph;
  doc["turbidity"] = currentSensorData.turbidity;
  doc["water_level"] = currentSensorData.water_level;
  doc["is_alarm_active"] = currentSensorData.is_alarm_active;

  // Diagnostik sistem
  doc["wifi_mode"] = isAPModeActive ? "Access Point (Fallback)" : "Station (Connected)";
  doc["ip_address"] = localIPAddress;
  doc["rssi"] = isAPModeActive ? 0 : WiFi.RSSI();
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// Callback untuk Route API Config
void handleApiConfig() {
  JsonDocument doc;
  doc["ssid"] = activeConfig.wifi_ssid;
  doc["pass"] = activeConfig.wifi_password; // disajikan ke form agar user tahu
  doc["url"] = activeConfig.server_url;
  doc["uid"] = activeConfig.user_id;
  doc["did"] = activeConfig.device_id;
  doc["v_clear"] = activeConfig.v_clear;
  doc["ph_v_neut"] = activeConfig.ph_v_neutral;
  doc["ph_slope"] = activeConfig.ph_slope;
  doc["pond_height"] = activeConfig.pond_height;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// Callback untuk Route Save (Simpan Konfigurasi)
void handleSave() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  DeviceConfig newCfg;
  
  // Baca data string dari form arguments
  strncpy(newCfg.wifi_ssid, server.arg("ssid").c_str(), sizeof(newCfg.wifi_ssid));
  strncpy(newCfg.wifi_password, server.arg("pass").c_str(), sizeof(newCfg.wifi_password));
  strncpy(newCfg.server_url, server.arg("url").c_str(), sizeof(newCfg.server_url));
  strncpy(newCfg.user_id, server.arg("uid").c_str(), sizeof(newCfg.user_id));
  strncpy(newCfg.device_id, server.arg("did").c_str(), sizeof(newCfg.device_id));
  
  // Baca data float
  newCfg.v_clear = server.arg("v_clear").toFloat();
  newCfg.ph_v_neutral = server.arg("ph_v_neut").toFloat();
  newCfg.ph_slope = server.arg("ph_slope").toFloat();
  newCfg.pond_height = server.arg("pond_height").toFloat();
  
  // Simpan ke Preferences
  saveDeviceConfig(newCfg);
  
  // Kirim halaman respons sukses ke browser
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="id">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Konfigurasi Disimpan</title>
    <style>
      body { background: #0f172a; color: #f8fafc; font-family: system-ui, sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0; padding: 20px; box-sizing: border-box;}
      .container { text-align: center; background: rgba(30,41,59,0.5); padding: 40px; border-radius: 16px; border: 1px solid rgba(255,255,255,0.08); max-width: 400px; box-shadow: 0 10px 30px rgba(0,0,0,0.3); }
      .spinner { border: 4px solid rgba(255,255,255,0.1); width: 40px; height: 40px; border-radius: 50%; border-left-color: #0ea5e9; animation: spin 1s linear infinite; margin: 24px auto; }
      @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
      h2 { margin-bottom: 12px; font-weight: 700; color: #0ea5e9; }
      p { font-size: 14px; color: #94a3b8; line-height: 1.5; }
    </style>
    <script>
      // Redirect ke root sesudah 8 detik
      setTimeout(function() { window.location.href = "http://192.168.4.1/"; }, 8000);
    </script>
  </head>
  <body>
    <div class="container">
      <h2>Sukses!</h2>
      <p>Konfigurasi disimpan. Perangkat sedang reboot untuk menyambung ke WiFi baru...</p>
      <div class="spinner"></div>
      <p style="font-size: 12px; color: #64748b; margin-top: 16px;">Jika Anda mengganti SSID WiFi, pastikan laptop Anda terhubung ke jaringan baru tersebut.</p>
    </div>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", html);
  
  // Set flag untuk memicu reboot di main loop
  shouldReboot = true;
}

// Callback untuk Memicu Reboot Manual
void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  shouldReboot = true;
}

void startWebServer() {
  server.on("/", HTTP_GET, handleIndex);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/api/config", HTTP_GET, handleApiConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reboot", HTTP_GET, handleReboot);
  
  // Redirect kueri Captive Portal di mode AP
  server.onNotFound([]() {
    if (isAPModeActive) {
      // Alihkan semua request yang tidak dikenal ke portal config (http://192.168.4.1/)
      server.sendHeader("Location", "http://192.168.4.1/", true);
      server.send(302, "text/plain", "Redirecting to Captive Portal");
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });

  server.begin();
  Serial.println("[Web Server] HTTP server berhasil dijalankan!");
}

void handleWebServerClients() {
  if (isAPModeActive) {
    dnsServer.processNextRequest();
  }
  server.handleClient();
}
