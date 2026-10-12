/*
 * ============================================================
 *  CLIENT 1 - ROOM 1 (HTTP VERSION - FINAL)
 *  Sensors: MQ-2 (Analog) + Ultrasonic (Distance)
 * ============================================================
 *
 *  MQ-2 AO          -> GPIO34
 *  Ultrasonic TRIG  -> GPIO18
 *  Ultrasonic ECHO  -> GPIO19
 *
 *  Host IP: 10.102.145.100
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

#define CLIENT_ID 1

// ============================================================
// PIN DEFINITIONS
// ============================================================
#define MQ2_AO_PIN      34
#define ULTRASONIC_TRIG 18
#define ULTRASONIC_ECHO 19

// ============================================================
// ULTRASONIC READ
// ============================================================
float readUltrasonic() {
    digitalWrite(ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG, LOW);

    unsigned long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
    if (duration == 0) return -1.0;
    return duration / 58.0;
}

// ============================================================
// SEND JSON TO HOST
// ============================================================
void sendData(float mq2, float ultra) {
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
    json += "\"ultrasonic_cm\":" + String(ultra, 1) + ",";
    json += "\"flame_value\":0,";
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
    Serial.println(" CLIENT 1 - ROOM 1 (HTTP)");
    Serial.println(" MQ-2 + Ultrasonic");
    Serial.println("================================");

    pinMode(MQ2_AO_PIN, INPUT);
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    pinMode(ULTRASONIC_ECHO, INPUT);
    digitalWrite(ULTRASONIC_TRIG, LOW);

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
    float mq2Value = (float)analogRead(MQ2_AO_PIN);
    float ultra = readUltrasonic();

    Serial.printf("MQ2: %.0f | Distance: %.1f cm\n", mq2Value, ultra);

    sendData(mq2Value, ultra);

    delay(1000);
}