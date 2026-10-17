/*
 * ============================================================
 *  CLIENT 4 - ROOM 4 (HTTP VERSION)
 *  Sensors: MQ135 (Air Quality) + DHT11 (Temp & Humidity)
 * ============================================================
 *
 *  MQ135 AO      -> GPIO34
 *  DHT11 DATA    -> GPIO18
 *  LED           -> GPIO2 (inbuilt)
 *
 *  Host IP: 10.43.47.96
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// ============================================================
// CONFIGURATION
// ============================================================
const char* WIFI_SSID     = "Manish";
const char* WIFI_PASSWORD = "8439923487";

const char* HOST_URL = "http://10.43.47.96/api/data";

#define CLIENT_ID 4

// ============================================================
// PIN DEFINITIONS
// ============================================================
#define MQ135_AO_PIN  34
#define DHT_PIN       18
#define DHT_TYPE      DHT11
#define LED_PIN       2

// ============================================================
// SENSOR OBJECT
// ============================================================
DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
// SEND JSON TO HOST
// ============================================================
void sendData(float mq135, float temp, float hum) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠ WiFi not connected!");
        return;
    }

    HTTPClient http;
    http.begin(HOST_URL);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"client_id\":" + String(CLIENT_ID) + ",";
    json += "\"mq2_value\":0,";
    json += "\"ultrasonic_cm\":0,";
    json += "\"flame_value\":0,";
    json += "\"mq135_value\":" + String(mq135, 1) + ",";
    json += "\"ir_value\":0,";
    json += "\"dht_temp\":" + String(temp, 1) + ",";
    json += "\"dht_hum\":" + String(hum, 1) + ",";
    json += "\"servo_angle\":0";
    json += "}";

    int code = http.POST(json);
    if (code > 0) {
        Serial.printf("✓ HTTP %d | %s\n", code, http.getString().c_str());
    } else {
        Serial.printf("✗ HTTP error: %s\n", http.errorToString(code).c_str());
    }
    http.end();
}

// ============================================================
// WIFI CONNECT FUNCTION
// ============================================================
void connectWiFi() {
    Serial.println();
    Serial.println("─────────────────────────────");
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("╔══════════════════════════════╗");
        Serial.println("║   ✓ WIFI CONNECTED!          ║");
        Serial.println("╚══════════════════════════════╝");
        Serial.print("Client IP:   ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal:      ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.print("Host URL:    ");
        Serial.println(HOST_URL);
        Serial.println("─────────────────────────────");
        Serial.println();

        // 3 quick LED blinks
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH); delay(150);
            digitalWrite(LED_PIN, LOW);  delay(150);
        }
    } else {
        Serial.println("╔══════════════════════════════╗");
        Serial.println("║   ✗ WIFI FAILED!             ║");
        Serial.println("╚══════════════════════════════╝");
        Serial.println("Check SSID/password and hotspot.");
        Serial.println("─────────────────────────────");
    }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  CLIENT 4 - ROOM 4");
    Serial.println("  MQ135 + DHT11");
    Serial.println("========================================");

    pinMode(MQ135_AO_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialize DHT
    dht.begin();
    Serial.println("✓ DHT11 initialized");

    connectWiFi();
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    // WiFi reconnect agar disconnect ho jaye
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠ WiFi disconnected! Reconnecting...");
        connectWiFi();
    }

    // Read MQ135
    float mq135Value = (float)analogRead(MQ135_AO_PIN);

    // Read DHT11
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    // Check DHT read failed
    if (isnan(temp) || isnan(hum)) {
        Serial.println("✗ DHT11 read FAILED!");
        temp = -999.0;
        hum  = -999.0;
    }

    // LED blink
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    Serial.printf("MQ135: %.0f | Temp: %.1f°C | Hum: %.1f%%\n",
                  mq135Value, temp, hum);

    sendData(mq135Value, temp, hum);

    delay(2000);  // DHT11 needs at least 2 seconds between reads
}