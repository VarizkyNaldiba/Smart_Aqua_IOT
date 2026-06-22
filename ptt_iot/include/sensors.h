#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// ==========================================
// 1. STRUKTUR DATA SENSOR
// ==========================================
struct SensorData {
  float temperature;            // Suhu (°C)
  float ph;                    // Nilai pH
  float turbidity;             // Kekeruhan (NTU)
  float water_level;           // Tinggi air (cm)
  
  // Data Mentah Diagnosa (Raw ADC & Volt)
  int raw_ph_adc;
  float raw_ph_volt;
  int raw_turbidity_adc;
  float raw_turbidity_volt;
  float raw_ultrasonic_cm; // Raw ultrasonic distance in cm (before calibration)
  
  bool is_alarm_active;        // Status alarm (Buzzer)
};

// Variabel Data Sensor Global
extern SensorData currentSensorData;

// ==========================================
// 2. FUNGSI UTAMA SENSOR
// ==========================================
void initSensors();
void readAllSensors();
void evaluateWaterQuality();

#endif // SENSORS_H
