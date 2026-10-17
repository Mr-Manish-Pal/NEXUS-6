/*
 * ============================================================
 *  CLIENT 6 - ROOM 6 (HTTP VERSION)
 *  Sensors: Flame Sensor + Servo Motor
 * ============================================================
 *
 *  Flame DO      -> GPIO4
 *  Servo Signal  -> GPIO5
 *  LED           -> GPIO2 (inbuilt)
 *
 *  Host IP: 10.43.47.96
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// ============================================================
// CONFIGURATION
// ============================================================
const char* WIFI_SSID     = "Manish";
const char* WIFI_PASSWORD = "8439923487";

const char* HOST_URL = "http://10.43.47.96/api/data";

#define CLIENT_ID 6

// ============================================================
// PIN DEFINITIONS
// ============================================================
#define FLAME_DO_PIN  4
#define SERVO_PIN     5
#define LED_PIN       2

// ============================================================
// SERVO OBJECT + STATE
// ============================================================
Servo myServo;

int currentAngle = 0;
int roundCount = 0;

const int SWEEP_DELAY = 1000;

// ============================================================
// SEND JSON TO HOST
// ============================================================
void sendData(float flame, int angle, int rounds) {
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
    json += "\"flame_value\":" + String(flame, 1) + ",";
    json += "\"mq135_value\":0,";
    json += "\"ir_value\":0,";
    json += "\"dht_temp\":0,";
    json += "\"dht_hum\":0,";
    json += "\"servo_angle\":" + String(angle) + ",";
    json += "\"servo_rounds\":" + String(rounds);
    json += "}";

    int code = http.POST(json);
    if (code > 0) {
        Serial.printf("✓ HTTP %d | Angle: %d | Rounds: %d | %s\n",
                      code, angle, rounds, http.getString().c_str());
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
    Serial.println("  CLIENT 6 - ROOM 6");
    Serial.println("  Flame + Servo (Full Sweep)");
    Serial.println("========================================");

    pinMode(FLAME_DO_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN, 500, 2400);
    myServo.write(0);
    delay(500);
    currentAngle = 0;
    Serial.println("✓ Servo initialized at 0°");

    connectWiFi();
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠ WiFi disconnected! Reconnecting...");
        connectWiFi();
    }

    float flameValue = (float)digitalRead(FLAME_DO_PIN);

    // Sweep 0 -> 180
    Serial.println("→ Sweep: 0 → 180");
    for (int angle = 0; angle <= 180; angle += 5) {
        myServo.write(angle);
        currentAngle = angle;
        delay(15);
    }
    myServo.write(180);
    currentAngle = 180;
    delay(200);

    roundCount++;
    Serial.printf("✓ Round %d complete (180°)\n", roundCount);
    sendData(flameValue, currentAngle, roundCount);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    // Sweep 180 -> 0
    Serial.println("← Sweep: 180 → 0");
    for (int angle = 180; angle >= 0; angle -= 5) {
        myServo.write(angle);
        currentAngle = angle;
        delay(15);
    }
    myServo.write(0);
    currentAngle = 0;
    delay(200);

    roundCount++;
    Serial.printf("✓ Round %d complete (0°)\n", roundCount);
    sendData(flameValue, currentAngle, roundCount);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    delay(SWEEP_DELAY);
}