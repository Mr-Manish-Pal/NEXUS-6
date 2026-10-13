/*
 * ============================================================
 *  CLIENT 5 - ROOM 5 (HTTP VERSION)
 *  Sensors: Flame Sensor + Servo Motor
 *  Servo: Full 180° sweep, phir reverse
 *  Round counter: kitne baar 180° ghum chuka
 * ============================================================
 *
 *  Flame DO      -> GPIO5
 *  Servo Signal  -> GPIO4
 *  LED           -> GPIO2 (inbuilt)
 *
 *  Host IP: 10.102.145.100 (STATIC)
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// ============================================================
// CONFIGURATION
// ============================================================
const char* WIFI_SSID     = "Manish";
const char* WIFI_PASSWORD = "Y11111111";

const char* HOST_URL = "http://10.102.145.100/api/data";

#define CLIENT_ID 5

// ============================================================
// PIN DEFINITIONS
// ============================================================
#define FLAME_DO_PIN  5
#define SERVO_PIN     4
#define LED_PIN       2

// ============================================================
// SERVO OBJECT + STATE
// ============================================================
Servo myServo;

int currentAngle = 0;
int roundCount = 0;

const int SWEEP_DELAY = 1000;   // 1 second wait between sweeps

// ============================================================
// SEND JSON TO HOST
// ============================================================
void sendData(float flame, int angle, int rounds) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
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
        Serial.printf("HTTP %d | Angle: %d | Rounds: %d\n",
                      code, angle, rounds);
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
    Serial.println(" CLIENT 5 - ROOM 5 (HTTP)");
    Serial.println(" Flame + Servo (Full Sweep)");
    Serial.println("================================");

    pinMode(FLAME_DO_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // Servo init
    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN, 500, 2400);
    myServo.write(0);
    delay(500);
    currentAngle = 0;
    Serial.println("Servo initialized at 0 degrees");

    // 3 quick LED blinks
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH); delay(100);
        digitalWrite(LED_PIN, LOW);  delay(100);
    }

    // WiFi connect
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
    float flameValue = (float)digitalRead(FLAME_DO_PIN);

    // -------- Full sweep: 0 -> 180 --------
    Serial.println("Sweep START: 0 -> 180");
    for (int angle = 0; angle <= 180; angle += 5) {
        myServo.write(angle);
        currentAngle = angle;
        delay(15);
    }
    myServo.write(180);
    currentAngle = 180;
    delay(200);

    roundCount++;
    Serial.printf("Round %d complete (180 reached)\n", roundCount);
    sendData(flameValue, currentAngle, roundCount);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    // -------- Full sweep: 180 -> 0 --------
    Serial.println("Sweep START: 180 -> 0");
    for (int angle = 180; angle >= 0; angle -= 5) {
        myServo.write(angle);
        currentAngle = angle;
        delay(15);
    }
    myServo.write(0);
    currentAngle = 0;
    delay(200);

    roundCount++;
    Serial.printf("Round %d complete (0 reached)\n", roundCount);
    sendData(flameValue, currentAngle, roundCount);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    delay(SWEEP_DELAY);
}