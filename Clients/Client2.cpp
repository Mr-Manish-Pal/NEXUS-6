/*
 * ============================================================
 *  CLIENT 2 - ROOM 2 (HTTP VERSION)
 *  Sensors: MQ-2 (Analog Gas) + Flame Sensor (Digital)
 *  + Inbuilt Blue LED blink every 1 second
 * ============================================================
 *
 *  MQ-2 AO       -> GPIO34   (Analog: 0-4095)
 *  Flame DO      -> GPIO5    (Digital: 0 or 1)
 *  Blue LED      -> GPIO2    (inbuilt)
 *
 *  Host IP: 10.102.145.100 (STATIC)
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>

// ============================================================
// CONFIGURATION
// ============================================================
const char* WIFI_SSID     = "Manish";
const char* WIFI_PASSWORD = "Y11111111";

const char* HOST_URL = "http://10.102.145.100/api/data";

#define CLIENT_ID 2

// ============================================================
// PIN DEFINITIONS
// ============================================================
#define MQ2_AO_PIN    34
#define FLAME_DO_PIN  5
#define LED_PIN       2      // Inbuilt blue LED on ESP32

// ============================================================
// SEND JSON TO HOST
// ============================================================
void sendData(float mq2, float flame) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return;
    }

    HTTPClient http;
    http.begin(HOST_URL);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"client_id\":" + String(CLIENT_ID) + ",";
    json += "\"mq2_value\":" + String(mq2, 1) + ",";
    json += "\"ultrasonic_cm\":0,";
    json += "\"flame_value\":" + String(flame, 1) + ",";
    json += "\"mq135_value\":0,";
    json += "\"ir_value\":0,";
    json += "\"dht_temp\":0,";
    json += "\"dht_hum\":0,";
    json += "\"servo_angle\":0";
    json += "}";

    int code = http.POST(json);
    if (code > 0) {
        Serial.printf("HTTP %d | %s\n", code, http.getString().c_str());
    } else {
        Serial.printf("HTTP error: %s\n", http.errorToString(code).c_str());
    }
    http.end();
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println(" CLIENT 2 - ROOM 2 (HTTP)");
    Serial.println(" MQ-2 + Flame + LED Blink");
    Serial.println("================================");

    pinMode(MQ2_AO_PIN, INPUT);
    pinMode(FLAME_DO_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // Initial LED test - 3 quick blinks
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("Client IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Sending to: ");
        Serial.println(HOST_URL);
    } else {
        Serial.println("\nWiFi FAILED! Check SSID/password.");
    }
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    // ---- Read sensors ----
    float mq2Value = (float)analogRead(MQ2_AO_PIN);
    float flameValue = (float)digitalRead(FLAME_DO_PIN);

    // ---- LED Blink ----
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);

    // ---- Send data ----
    Serial.printf("MQ2: %.0f | Flame: %.0f\n", mq2Value, flameValue);
    sendData(mq2Value, flameValue);
}