
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

// FitTrack ESP32-S3 Firmware
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* SERVER_URL = "http://YOUR_COMPUTER_IP:3000/api/workout";

#define MPU_ADDR 0x68

int reps = 0;
int sets = 0;
int16_t accelX, accelY, accelZ;

unsigned long startTime;
unsigned long lastSend = 0;

bool sensorReady = false;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println("FitTrack Starting...");

  // Initialize MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  sensorReady = (Wire.endTransmission() == 0);

  Serial.println(sensorReady ?
    "MPU6050 Connected" : "MPU6050 Not Found");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting WiFi...");
  unsigned long timeout = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - timeout < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println(WiFi.status() == WL_CONNECTED ?
    "WiFi Connected" : "WiFi Not Connected");

  startTime = millis();
}

void loop() {
  if (sensorReady) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);

    if (Wire.available() >= 6) {
      accelX = Wire.read() << 8 | Wire.read();
      accelY = Wire.read() << 8 | Wire.read();
      accelZ = Wire.read() << 8 | Wire.read();

      Serial.print("X: ");
      Serial.print(accelX);
      Serial.print(" Y: ");
      Serial.print(accelY);
      Serial.print(" Z: ");
      Serial.println(accelZ);
    }
  }

  if (millis() - lastSend >= 5000) {
    sendWorkoutData();
    lastSend = millis();
  }

  delay(100);
}

void sendWorkoutData() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  unsigned long duration = (millis() - startTime) / 1000;

  String payload = "{\"machine\":\"Chest Press\",";
  payload += "\"reps\":" + String(reps) + ",";
  payload += "\"sets\":" + String(sets) + ",";
  payload += "\"duration\":" + String(duration) + ",";
  payload += "\"mode\":\"HARDWARE\"}";

  int result = http.POST(payload);

  Serial.print("API Response: ");
  Serial.println(result);

  http.end();
}
