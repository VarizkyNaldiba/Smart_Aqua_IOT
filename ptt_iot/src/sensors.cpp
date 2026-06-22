#include "sensors.h"
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Definisikan variabel global
SensorData currentSensorData;

// Objek Sensor Suhu Internal
OneWire oneWire(PIN_SUHU);
DallasTemperature sensorSuhu(&oneWire);
DeviceAddress alamatSensorSuhu;

void initSensors() {
  sensorSuhu.begin();
  if (sensorSuhu.getAddress(alamatSensorSuhu, 0)) {
    sensorSuhu.setResolution(alamatSensorSuhu, 12); // Set presisi 12-bit
    Serial.println("[Sensors] Sensor Suhu DS18B20 Terdeteksi.");
  } else {
    Serial.println("[Sensors] PERINGATAN: Sensor Suhu DS18B20 Tidak Ditemukan!");
  }

  // Konfigurasi Pin
  pinMode(PIN_PH, INPUT);
  pinMode(PIN_TURBIDITY, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Matikan buzzer di awal
  digitalWrite(PIN_BUZZER, LOW);
  currentSensorData.is_alarm_active = false;
}

void readAllSensors() {
  // 1. Baca Suhu
  sensorSuhu.requestTemperatures();
  float suhuC = sensorSuhu.getTempCByIndex(0);
  if (suhuC == -127.0) {
    // DS18B20 error fallback
    suhuC = 27.0; // Fallback ke suhu rata-rata
  }

  // 2. Baca pH (Averaging ADC 10 sampel)
  long sumPH = 0;
  for (int i = 0; i < 10; i++) {
    sumPH += analogRead(PIN_PH);
    delay(10);
  }
  int avgPH = sumPH / 10;
  float voltagePH = avgPH * (3.3 / 4095.0);
  
  // Hitung nilai pH dengan nilai kalibrasi aktif
  float nilaiPH = 7.0 + ((activeConfig.ph_v_neutral - voltagePH) / activeConfig.ph_slope);
  // Simpan nilai terukur ke struct
  currentSensorData.temperature = suhuC;
  currentSensorData.ph = nilaiPH;
  // 3. Baca Turbidity (Median Filter 20 sampel)
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
  // Rata-rata 10 nilai tengah
  long sumTurbidity = 0;
  for (int i = 5; i < 15; i++) sumTurbidity += turbSamples[i];
  int avgTurbidity = sumTurbidity / 10;
  float teganganTurbidity = avgTurbidity * (3.3 / 4095.0);

  // Hitung kekeruhan NTU (Menggunakan baseline 1.2 NTU)
  float ntu = 1.2;
  if (teganganTurbidity < activeConfig.v_clear) {
    ntu = 1.2 + (activeConfig.v_clear - teganganTurbidity) * 1000.0;
  }
  if (ntu < 0) ntu = 1.2;
  currentSensorData.turbidity = ntu;

  // 4. Baca Level Air (Ultrasonik JSN-SR04T)
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(20);
  digitalWrite(PIN_TRIG, LOW);

  float durasi = pulseIn(PIN_ECHO, HIGH, 40000); // timeout 40ms (~680cm)
  
  // Kompensasi kecepatan suara berdasarkan suhu
  float kecepatanSuara = (331.3 + 0.606 * suhuC) / 10000.0;
  float jarakAir = (durasi == 0) ? 0 : durasi * kecepatanSuara / 2.0;

  // Raw distance in cm (jarakAir) is stored in currentSensorData.raw_ultrasonic_cm
  // Apply calibration: calibrated = raw * scale + offset
  float calibratedDistance = jarakAir * activeConfig.ultrasonic_scale + activeConfig.ultrasonic_offset;
  // Hitung Tinggi Air Kolam yang Sebenarnya (Kalibrasi Tinggi Kolam)
  float tinggiAir = 0.0;
  if (durasi != 0) {
    if (activeConfig.pond_height > 0.0) {
      // Kolam total tinggi - jarak terkalibrasi dari sensor ke permukaan air
      tinggiAir = activeConfig.pond_height - calibratedDistance;
      if (tinggiAir < 0.0) tinggiAir = 0.0; // Batasi agar tidak negatif
    } else {
      // Jika tinggi kolam tidak diset, gunakan nilai terkalibrasi langsung
      tinggiAir = calibratedDistance;
    }
  }
  currentSensorData.water_level = tinggiAir;
  
  currentSensorData.raw_ph_adc = avgPH;
  currentSensorData.raw_ph_volt = voltagePH;
  currentSensorData.raw_turbidity_adc = avgTurbidity;
  currentSensorData.raw_turbidity_volt = teganganTurbidity;
  currentSensorData.raw_ultrasonic_cm = jarakAir; // store raw distance in cm

  // Evaluasi alarm
  evaluateWaterQuality();
}

void evaluateWaterQuality() {
  // Aturan Alarm Lokal
  // Suhu abnormal: < 25.0 atau > 30.0
  bool isSuhuAbnormal = (currentSensorData.temperature < 25.0 || currentSensorData.temperature > 30.0);
  // pH abnormal: < 6.5 atau > 8.5
  bool isPHAbnormal = (currentSensorData.ph < 6.5 || currentSensorData.ph > 8.5);
  // Turbidity abnormal: > 50 NTU
  bool isTurbidityAbnormal = (currentSensorData.turbidity > 50.0);

  if (isSuhuAbnormal || isPHAbnormal || isTurbidityAbnormal) {
    digitalWrite(PIN_BUZZER, HIGH);
    currentSensorData.is_alarm_active = true;
  } else {
    digitalWrite(PIN_BUZZER, LOW);
    currentSensorData.is_alarm_active = false;
  }
}
