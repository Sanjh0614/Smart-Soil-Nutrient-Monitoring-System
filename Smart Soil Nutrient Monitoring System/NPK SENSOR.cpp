#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ==========================================
// Network & Endpoint Configuration
// ==========================================
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* SERVER_URL    = "http://YOUR_SERVER_IP:8000/api/telemetry";

const char* DEVICE_ID     = "esp32-soil-node-01";
const unsigned long TELEMETRY_INTERVAL_MS = 10000; // Sample every 10 seconds

// ==========================================
// Pin Definitions
// ==========================================
#define RE_DE_PIN   4    // Controls MAX485 Driver/Receiver enable
#define RX2_PIN     16   // ESP32 RX2 connected to MAX485 RO
#define TX2_PIN     17   // ESP32 TX2 connected to MAX485 DI
#define MOISTURE_PIN 34  // Analog capacitive moisture pin (ADC1)

// Soil Moisture Calibration (12-bit ADC: 0 - 4095)
const int AIR_VALUE   = 3200; // Sensor value in dry air
const int WATER_VALUE = 1500; // Sensor value in water

// ==========================================
// Modbus RTU Query Frames for NPK Sensors
// Standard inquiry hex frames (Slave ID: 0x01, Function: 0x03)
// ==========================================
const byte NITROGEN_QUERY[]   = {0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte PHOSPHORUS_QUERY[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte POTASSIUM_QUERY[]  = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

// Hardware Serial instance for RS485 communication
HardwareSerial modbusSerial(2);

// ==========================================
// Helper Functions
// ==========================================

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();
  // Wait up to 10 seconds for connection
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected! IP Address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[WiFi] Connection timeout. Retrying next cycle.");
  }
}

uint16_t queryModbusRegister(const byte* queryFrame, size_t frameSize) {
  byte response[7] = {0};

  // Clear stale data from RX buffer
  while (modbusSerial.available()) {
    modbusSerial.read();
  }

  // 1. Set MAX485 to Transmit Mode
  digitalWrite(RE_DE_PIN, HIGH);
  delay(10);

  // 2. Send the Modbus request frame
  modbusSerial.write(queryFrame, frameSize);
  modbusSerial.flush(); // Wait until transmission is complete

  // 3. Set MAX485 back to Receive Mode
  digitalWrite(RE_DE_PIN, LOW);
  delay(10);

  // 4. Wait for and read the 7-byte response
  unsigned long timeout = millis();
  int bytesRead = 0;

  while ((millis() - timeout < 1000) && (bytesRead < 7)) {
    if (modbusSerial.available()) {
      response[bytesRead++] = modbusSerial.read();
    }
  }

  // Verify response structure: [SlaveID (0x01), FunctionCode (0x03), ByteCount (0x02), HighByte, LowByte, CRC_L, CRC_H]
  if (bytesRead == 7 && response[0] == 0x01 && response[1] == 0x03) {
    uint16_t value = (response[3] << 8) | response[4];
    return value;
  }

  Serial.println("[Modbus] Sensor read timeout or checksum error");
  return 0; // Return 0 or fallback value on error
}

float readSoilMoisture() {
  int rawAdc = analogRead(MOISTURE_PIN);
  // Map raw ADC to percentage 0.0% to 100.0%
  float moisturePct = map(rawAdc, AIR_VALUE, WATER_VALUE, 0, 1000) / 10.0;
  return constrain(moisturePct, 0.0, 100.0);
}

void sendTelemetryPayload(uint16_t n, uint16_t p, uint16_t k, float moisture) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) return;
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  // Construct JSON payload
  char payload[256];
  snprintf(payload, sizeof(payload),
    "{\"device_id\":\"%s\",\"nitrogen_mg_kg\":%u,\"phosphorus_mg_kg\":%u,\"potassium_mg_kg\":%u,\"soil_moisture_pct\":%.1f}",
    DEVICE_ID, n, p, k, moisture
  );

  Serial.print("[HTTP] POST payload: ");
  Serial.println(payload);

  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    Serial.printf("[HTTP] Response status: %d\n", httpCode);
  } else {
    Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

// ==========================================
// Setup & Main Loop
// ==========================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize Flow Control Pin
  pinMode(RE_DE_PIN, OUTPUT);
  digitalWrite(RE_DE_PIN, LOW); // Start in listening/receive mode

  // Initialize HardwareSerial for Modbus RTU at 9600 baud, 8 data bits, no parity, 1 stop bit
  modbusSerial.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);

  // Configure ADC resolution
  analogReadResolution(12);

  Serial.println("\n--- Soil Nutrient Monitoring Node Starting ---");
  connectWiFi();
}

void loop() {
  // Query N, P, K consecutively with brief settling delays
  uint16_t nitrogen = queryModbusRegister(NITROGEN_QUERY, sizeof(NITROGEN_QUERY));
  delay(250);

  uint16_t phosphorus = queryModbusRegister(PHOSPHORUS_QUERY, sizeof(PHOSPHORUS_QUERY));
  delay(250);

  uint16_t potassium = queryModbusRegister(POTASSIUM_QUERY, sizeof(POTASSIUM_QUERY));
  delay(250);

  // Read analog moisture
  float moisture = readSoilMoisture();

  // Print results locally
  Serial.printf("\n[Readings] N: %u mg/kg | P: %u mg/kg | K: %u mg/kg | Moisture: %.1f%%\n",
                nitrogen, phosphorus, potassium, moisture);

  // Dispatch data to server
  sendTelemetryPayload(nitrogen, phosphorus, potassium, moisture);

  // Wait for the next sampling cycle
  delay(TELEMETRY_INTERVAL_MS);
}