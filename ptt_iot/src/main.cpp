#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h> 
#include <ArduinoJson.h>

// ==========================================
// 1. DEKLARASI PIN SENSOR & AKTUATOR
// ==========================================
#define PIN_SUHU       26   // Sensor DS18B20
#define PIN_PH         34  // Sensor pH (Analog)
#define PIN_TURBIDITY  35  // Sensor Turbidity (Analog)
#define PIN_TRIG       5   // Ultrasonik Trigger
#define PIN_ECHO       18  // Ultrasonik Echo
#define PIN_BUZZER     2   // Alarm/Indikator

// ==========================================
// 2. KONFIGURASI JARINGAN & API
// ==========================================
const char* ssid = "JAM GADANG";
const char* password = "bukittinggi";

// Endpoint API untuk menerima data dari IoT (Arahkan ke route baru)
const char* serverUrl = "https://aqua-vion.vercel.app/api/mqtt/receive";
const char* host = "104.214.185.159";
const uint16_t port = 1884;

// Identitas perangkat untuk database
const char* userId = "8JAGfvb3EXPKijhoAllz2ObC0Bf2";
const char* device_id = "AQN-IOT-001";

// ==========================================
// 3. OBJEK & VARIABEL GLOBAL
// ==========================================
OneWire oneWire(PIN_SUHU);
DallasTemperature sensorSuhu(&oneWire);
DeviceAddress alamatSensorSuhu;

// Variabel untuk menyimpan data sensor terkini
float g_suhuC = 0;
float g_nilaiPH = 0;
float g_jarakAir = 0;
float g_turbidityNTU = 0;


// Prototipe Fungsi
void setup_wifi();
void bacaSensorDanEvaluasi();
void sendDataToVercel();

// ==========================================
// 4. SETUP: INISIALISASI PERANGKAT
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // Inisialisasi Sensor Suhu DS18B20
  sensorSuhu.begin();
  if (sensorSuhu.getAddress(alamatSensorSuhu, 0)) {
    sensorSuhu.setResolution(alamatSensorSuhu, 12); // Set presisi 12-bit
    Serial.println("Sensor Suhu Terdeteksi.");
  } else {
    Serial.println("PERINGATAN: Sensor Suhu Tidak Ditemukan!");
  }

  // Konfigurasi Pin I/O
  pinMode(PIN_PH, INPUT);
  pinMode(PIN_TURBIDITY, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Hubungkan ke WiFi
  setup_wifi();
  
  Serial.println("--- Sistem AquaVion Ready ---");
}

// ==========================================
// 5. LOOP: EKSEKUSI UTAMA (NON-BLOCKING)
// ==========================================
void loop() {
  // Timer untuk pembacaan sensor (setiap 5 detik)
  static unsigned long lastReading = 0;
  if (millis() - lastReading > 5000) {
    lastReading = millis();
    bacaSensorDanEvaluasi();
  }

  // Timer untuk pengiriman data ke server (setiap 15 detik)
  static unsigned long lastPublish = 0;
  if (millis() - lastPublish > 15000) {
    lastPublish = millis();
    sendDataToVercel();
  }
}

// ==========================================
// 6. FUNGSI PENDUKUNG
// ==========================================

/**
 * Menghubungkan ESP32 ke jaringan WiFi yang telah ditentukan.
 */
void setup_wifi() {
  delay(10);
  Serial.print("\nMenghubungkan ke WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

/**
 * Mengirimkan data sensor dalam format JSON ke endpoint Vercel.
 */
void sendDataToVercel() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Membuat dokumen JSON menggunakan library ArduinoJson
    JsonDocument doc;
    doc["userId"] = userId;
    doc["device_id"] = device_id;
    doc["ssid"] = WiFi.SSID(); // Mengirimkan nama WiFi yang sedang terhubung
    doc["temperature"] = g_suhuC;
    doc["ph"] = g_nilaiPH;
    doc["turbidity"] = g_turbidityNTU;
    doc["waterLevel"] = g_jarakAir;

    String jsonString;
    serializeJson(doc, jsonString);

    Serial.println("Mengirim data ke Vercel...");
    int httpResponseCode = http.POST(jsonString);

    // Mengevaluasi respon dari server
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("HTTP Code: ");
      Serial.println(httpResponseCode);
      Serial.println("Response: " + response);
    } else {
      Serial.print("Gagal mengirim POST. Error: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }
    http.end(); // Tutup koneksi
  } else {
    Serial.println("WiFi Terputus, tidak dapat mengirim data.");
  }
}

/**
 * Membaca nilai dari seluruh sensor dan melakukan kalkulasi dasar.
 */
void bacaSensorDanEvaluasi() {
  // 1. Membaca Suhu (DS18B20)
  sensorSuhu.requestTemperatures(); 
  float suhuC = sensorSuhu.getTempCByIndex(0); 
  
  // 2. Membaca pH (Averaging ADC untuk kestabilan)
  long sumPH = 0;
  for(int i=0; i<10; i++) { 
    sumPH += analogRead(PIN_PH); 
    delay(10); 
  }
  int avgPH = sumPH / 10;
  // Konversi ADC pH ke Tegangan
  float voltagePH = avgPH * (3.3 / 4095.0);
  
  // Kalkulasi estimasi pH berdasarkan kalibrasi di larutan buffer pH 10.01 @25°C:
  // [KALIBRASI 17/05/2026] Buffer pH 10.01 → rata-rata tegangan terukur = 1.991 V
  // V_neutral dihitung: 1.991 + (3.01 × 0.18) = 2.533 V
  // Slope 0.18 V/pH dipertahankan (kalibrasi 1-titik).
  float nilaiPH = 7.0 + ((2.533 - voltagePH) / 0.18);

  // 3. Membaca Kekeruhan / Turbidity (Median filter 20 sampel untuk stabilitas)
  // Ambil 20 sampel, sort, lalu average 10 nilai tengah (buang 5 terluar di atas & bawah)
  int turbSamples[20];
  for (int i = 0; i < 20; i++) {
    turbSamples[i] = analogRead(PIN_TURBIDITY);
    delay(5);
  }
  // Bubble sort ascending
  for (int i = 0; i < 19; i++) {
    for (int j = 0; j < 19 - i; j++) {
      if (turbSamples[j] > turbSamples[j + 1]) {
        int tmp = turbSamples[j];
        turbSamples[j] = turbSamples[j + 1];
        turbSamples[j + 1] = tmp;
      }
    }
  }
  // Rata-rata 10 nilai tengah (index 5-14), buang 5 terkecil & 5 terbesar
  long sumTurbidity = 0;
  for (int i = 5; i < 15; i++) sumTurbidity += turbSamples[i];
  int rawTurbidity = sumTurbidity / 10;
  // Konversi ke Tegangan (ESP32: 3.3V referensi, resolusi 12-bit)
  float teganganTurbidity = rawTurbidity * (3.3 / 4095.0);

  // [KALIBRASI 17/05/2026] V_CLEAR = tegangan sensor saat air jernih di posisi terpasang.
  // PERLU RE-KALIBRASI: baca tegangan di Serial Monitor saat air kran jernih stabil,
  // lalu ganti nilai V_CLEAR di bawah dengan tegangan tersebut.
  // Slope 1000 NTU/V adalah estimasi — perlu larutan standar untuk kalibrasi akurat.
  const float V_CLEAR = 2.950; // TODO: ganti dengan tegangan air jernih di posisi terpasang
  float ntu = 0.0;
  if (teganganTurbidity < V_CLEAR) {
    ntu = (V_CLEAR - teganganTurbidity) * 1000.0;
  }
  if (ntu < 0) ntu = 0.0;

  // 4. Membaca Level Air (Ultrasonik JSN-SR04T)
  // JSN-SR04T memerlukan trigger HIGH minimal 20µs (berbeda dengan HC-SR04 yg 10µs)
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(5);   // Stabilisasi sebelum trigger
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(20);  // Trigger pulse 20µs untuk JSN-SR04T
  digitalWrite(PIN_TRIG, LOW);

  long durasi = pulseIn(PIN_ECHO, HIGH, 40000); // Timeout 40ms (~680cm max range)

  // Kecepatan suara dikompensasi berdasarkan suhu aktual (lebih akurat dari konstanta 0.034)
  // Rumus: v (cm/µs) = (331.3 + 0.606 * T°C) / 10000
  float kecepatanSuara = (331.3 + 0.606 * suhuC) / 10000.0;
  float jarakAir = (durasi == 0) ? 0 : durasi * kecepatanSuara / 2.0;

  // Debug: tampilkan durasi raw untuk diagnosa & kalibrasi
  Serial.printf("ULTRASONIK: durasi raw = %ld µs | kec.suara = %.4f cm/µs\n", durasi, kecepatanSuara);

  // Memperbarui variabel global agar bisa diakses oleh fungsi pengirim data
  g_suhuC = suhuC;
  g_nilaiPH = nilaiPH;
  g_turbidityNTU = ntu; 
  g_jarakAir = jarakAir;

  // 5. Evaluasi Kualitas Air (Logika Buzzer)
  // Suhu normal: 25 - 30 C
  bool isSuhuAbnormal = (suhuC != -127.0) && (suhuC < 25.0 || suhuC > 30.0);
  // pH normal: 6.5 - 8.5
  bool isPHAbnormal = (nilaiPH < 6.5 || nilaiPH > 8.5);
  // Kekeruhan air normal: 0 - 50 NTU (SNI 01-6484.5-2002)
  bool isTurbidityAbnormal = (ntu > 50.0);

  if (isSuhuAbnormal || isPHAbnormal || isTurbidityAbnormal) {
    digitalWrite(PIN_BUZZER, HIGH); // Nyalakan buzzer jika ada yang abnormal
    Serial.println("[ALARM] Peringatan! Parameter kualitas air di luar batas normal.");
  } else {
    digitalWrite(PIN_BUZZER, LOW);  // Matikan buzzer jika semua normal
  }

  // Output Serial yang lebih bersih
  Serial.println("\n--- MONITORING DATA ---");
  Serial.printf("SUHU      : %.2f C %s\n", suhuC, (suhuC == -127.0) ? "[TIDAK TERDETEKSI]" : (isSuhuAbnormal ? "[ABNORMAL]" : "[OK]"));
  Serial.printf("PH        : %.2f %s (RAW ADC: %d | VOLT: %.3f V)\n", nilaiPH, isPHAbnormal ? "[ABNORMAL]" : "[OK]", avgPH, voltagePH);
  Serial.printf("TURBIDITY : %.2f NTU %s (RAW ADC: %d | VOLT: %.3f V)\n", ntu, isTurbidityAbnormal ? "[ABNORMAL]" : "[OK]", rawTurbidity, teganganTurbidity);
  Serial.printf("JARAK AIR : %.2f cm %s\n", jarakAir, (jarakAir == 0) ? "[TIDAK TERDETEKSI]" : "");
  Serial.printf("BUZZER    : %s\n", (isSuhuAbnormal || isPHAbnormal || isTurbidityAbnormal) ? "ON (ALARM)" : "OFF");
  Serial.println("-----------------------");
}