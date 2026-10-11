/*
 * ============================================================
 * HOST ESP32 - 6 ROOM SENSOR DASHBOARD (HTTP VERSION)
 * ============================================================
 * Static IP Version - IP kabhi nahi badlega
 *
 * Client POST endpoint: http://10.163.44.100/api/data
 * Dashboard: http://10.163.44.100
 *
 * Required libraries:
 *   - ArduinoJson (v7 compatible)
 * ============================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// -------- Wi-Fi configuration --------
const char* WIFI_SSID     = "Manish";
const char* WIFI_PASSWORD = "Y11111111";

// ============================================================
// STATIC IP CONFIGURATION
// ============================================================
// Yeh IP tumhare network ke hisaab se set karo
IPAddress local_IP(10, 102, 145, 100);
IPAddress gateway(10, 102, 145, 203);
IPAddress subnet(255, 255, 255, 0);     // Subnet mask
IPAddress primaryDNS(8, 8, 8, 8);        // Google DNS
IPAddress secondaryDNS(8, 8, 4, 4);      // Google DNS 2

// -------- HTTP server --------
WebServer server(80);

// -------- Data structure --------
struct RoomData {
  uint8_t client_id = 0;
  float mq2_value = 0.0f;
  float ultrasonic_cm = -1.0f;
  float flame_value = 0.0f;
  float mq135_value = 0.0f;
  float ir_value = 0.0f;
  float dht_temp = 0.0f;
  float dht_hum = 0.0f;
  int servo_angle = 0;
};

RoomData sensorData[7];
unsigned long lastReceiveTime[7] = {0};

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Sensor Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', system-ui, sans-serif;
            background: linear-gradient(135deg, #0f0c29, #302b63, #24243e);
            min-height: 100vh;
            padding: 20px;
            color: #fff;
        }
        h1 {
            text-align: center;
            margin-bottom: 10px;
            font-size: 2rem;
            background: linear-gradient(90deg, #00d2ff, #3a7bd5);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .subtitle {
            text-align: center;
            color: #888;
            margin-bottom: 30px;
            font-size: 0.9rem;
        }
        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
            gap: 20px;
            max-width: 1400px;
            margin: 0 auto;
        }
        .room-card {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 16px;
            padding: 20px;
            backdrop-filter: blur(10px);
            transition: transform 0.2s, border-color 0.2s;
        }
        .room-card:hover {
            transform: translateY(-2px);
            border-color: rgba(0,210,255,0.3);
        }
        .room-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 16px;
            padding-bottom: 12px;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        .room-title {
            font-size: 1.2rem;
            font-weight: 600;
            color: #00d2ff;
        }
        .status-dot {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            background: #ff4444;
            transition: background 0.3s;
        }
        .status-dot.online  { background: #00ff88; box-shadow: 0 0 8px #00ff88; }
        .status-dot.offline { background: #ff4444; box-shadow: 0 0 8px #ff4444; }
        .sensor-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 12px;
        }
        .sensor-item {
            background: rgba(0,0,0,0.2);
            border-radius: 10px;
            padding: 12px;
            text-align: center;
        }
        .sensor-label {
            font-size: 0.7rem;
            text-transform: uppercase;
            letter-spacing: 1px;
            color: #888;
            margin-bottom: 4px;
        }
        .sensor-value {
            font-size: 1.4rem;
            font-weight: 700;
            color: #fff;
        }
        .sensor-unit {
            font-size: 0.75rem;
            color: #666;
            margin-left: 2px;
        }
        .sensor-item.wide { grid-column: span 2; }
        .footer {
            text-align: center;
            margin-top: 30px;
            color: #555;
            font-size: 0.8rem;
        }
        @media (max-width: 400px) {
            .sensor-grid { grid-template-columns: 1fr; }
            .sensor-item.wide { grid-column: span 1; }
        }
    </style>
</head>
<body>
    <h1>ESP32 Sensor Dashboard</h1>
    <p class="subtitle">Real-time monitoring across 6 rooms | Wi-Fi HTTP Network</p>

    <div class="dashboard" id="dashboard"></div>

    <p class="footer">Refresh: 2s | Host IP: <span id="hostIp">--</span></p>

    <script>
        const CLIENT_CONFIG = {
            1: { name: "Room 1" },
            2: { name: "Room 2" },
            3: { name: "Room 3" },
            4: { name: "Room 4" },
            5: { name: "Room 5" },
            6: { name: "Room 6" }
        };

        function renderDashboard(data) {
            const dashboard = document.getElementById('dashboard');
            let html = '';

            for (let id = 1; id <= 6; id++) {
                const cfg = CLIENT_CONFIG[id];
                const roomData = data[id] || {};
                const isOnline = roomData.online === true;

                let sensorHtml = '';

                if (id === 1) {
                    sensorHtml = `
                        <div class="sensor-item">
                            <div class="sensor-label">MQ2 Gas</div>
                            <div class="sensor-value">${roomData.mq2_value ?? '--'}</div>
                        </div>
                        <div class="sensor-item">
                            <div class="sensor-label">Distance</div>
                            <div class="sensor-value">${roomData.ultrasonic_cm ?? '--'}<span class="sensor-unit">cm</span></div>
                        </div>`;
                } else if (id === 2) {
                    sensorHtml = `
                        <div class="sensor-item">
                            <div class="sensor-label">MQ2 Gas</div>
                            <div class="sensor-value">${roomData.mq2_value ?? '--'}</div>
                        </div>
                        <div class="sensor-item">
                            <div class="sensor-label">Flame</div>
                            <div class="sensor-value">${roomData.flame_value ?? '--'}</div>
                        </div>`;
                } else if (id === 3) {
                    sensorHtml = `
                        <div class="sensor-item">
                            <div class="sensor-label">MQ135 Air</div>
                            <div class="sensor-value">${roomData.mq135_value ?? '--'}</div>
                        </div>
                        <div class="sensor-item">
                            <div class="sensor-label">IR Motion</div>
                            <div class="sensor-value">${roomData.ir_value ?? '--'}</div>
                        </div>`;
                } else if (id === 4) {
                    sensorHtml = `
                        <div class="sensor-item">
                            <div class="sensor-label">MQ135 Air</div>
                            <div class="sensor-value">${roomData.mq135_value ?? '--'}</div>
                        </div>
                        <div class="sensor-item">
                            <div class="sensor-label">Temperature</div>
                            <div class="sensor-value">${roomData.dht_temp ?? '--'}<span class="sensor-unit">°C</span></div>
                        </div>
                        <div class="sensor-item wide">
                            <div class="sensor-label">Humidity</div>
                            <div class="sensor-value">${roomData.dht_hum ?? '--'}<span class="sensor-unit">%</span></div>
                        </div>`;
                } else if (id === 5) {
                    sensorHtml = `
                        <div class="sensor-item">
                            <div class="sensor-label">Flame</div>
                            <div class="sensor-value">${roomData.flame_value ?? '--'}</div>
                        </div>
                        <div class="sensor-item">
                            <div class="sensor-label">Servo Position</div>
                            <div class="sensor-value">${roomData.servo_angle ?? '--'}<span class="sensor-unit">°</span></div>
                        </div>`;
                } else if (id === 6) {
                    sensorHtml = `
                        <div class="sensor-item wide">
                            <div class="sensor-label">Servo Position</div>
                            <div class="sensor-value">${roomData.servo_angle ?? '--'}<span class="sensor-unit">°</span></div>
                        </div>`;
                }

                html += `
                    <div class="room-card">
                        <div class="room-header">
                            <span class="room-title">${cfg.name}</span>
                            <span class="status-dot ${isOnline ? 'online' : 'offline'}"></span>
                        </div>
                        <div class="sensor-grid">${sensorHtml}</div>
                    </div>`;
            }

            dashboard.innerHTML = html;
        }

        function fetchData() {
            fetch('/data')
                .then(r => r.json())
                .then(data => {
                    renderDashboard(data);
                    document.getElementById('hostIp').textContent = window.location.host;
                })
                .catch(e => console.error('Fetch error:', e));
        }

        setInterval(fetchData, 2000);
        fetchData();
    </script>
</body>
</html>
)rawliteral";

// ============================================================
// RECEIVE CLIENT JSON: POST /api/data
// ============================================================
void handleClientData() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"Missing JSON body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  if (!doc["client_id"].is<int>()) {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"client_id must be an integer\"}");
    return;
  }

  const int id = doc["client_id"].as<int>();
  if (id < 1 || id > 6) {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"client_id must be 1-6\"}");
    return;
  }

  sensorData[id].client_id = (uint8_t)id;

  if (doc["mq2_value"].is<float>() || doc["mq2_value"].is<int>())
    sensorData[id].mq2_value = doc["mq2_value"].as<float>();

  if (doc["ultrasonic_cm"].is<float>() || doc["ultrasonic_cm"].is<int>())
    sensorData[id].ultrasonic_cm = doc["ultrasonic_cm"].as<float>();

  if (doc["flame_value"].is<float>() || doc["flame_value"].is<int>())
    sensorData[id].flame_value = doc["flame_value"].as<float>();

  if (doc["mq135_value"].is<float>() || doc["mq135_value"].is<int>())
    sensorData[id].mq135_value = doc["mq135_value"].as<float>();

  if (doc["ir_value"].is<float>() || doc["ir_value"].is<int>())
    sensorData[id].ir_value = doc["ir_value"].as<float>();

  if (doc["dht_temp"].is<float>() || doc["dht_temp"].is<int>())
    sensorData[id].dht_temp = doc["dht_temp"].as<float>();

  if (doc["dht_hum"].is<float>() || doc["dht_hum"].is<int>())
    sensorData[id].dht_hum = doc["dht_hum"].as<float>();

  if (doc["servo_angle"].is<int>())
    sensorData[id].servo_angle = doc["servo_angle"].as<int>();

  lastReceiveTime[id] = millis();

  Serial.printf("[HTTP] Client %d | MQ2:%.1f | Ultra:%.1f | Flame:%.1f | "
                "MQ135:%.1f | IR:%.1f | T:%.1f | H:%.1f | Servo:%d\n",
                id,
                sensorData[id].mq2_value,
                sensorData[id].ultrasonic_cm,
                sensorData[id].flame_value,
                sensorData[id].mq135_value,
                sensorData[id].ir_value,
                sensorData[id].dht_temp,
                sensorData[id].dht_hum,
                sensorData[id].servo_angle);

  server.send(200, "application/json", "{\"ok\":true}");
}

// ============================================================
// DASHBOARD DATA: GET /data
// ============================================================
void handleDashboardData() {
  JsonDocument doc;
  const unsigned long now = millis();

  for (int id = 1; id <= 6; id++) {
    const bool online = (lastReceiveTime[id] != 0) &&
                        ((unsigned long)(now - lastReceiveTime[id]) < 10000UL);

    JsonObject client = doc[String(id)].to<JsonObject>();
    client["online"] = online;

    if (lastReceiveTime[id] != 0) {
      client["mq2_value"] = sensorData[id].mq2_value;
      client["ultrasonic_cm"] = sensorData[id].ultrasonic_cm;
      client["flame_value"] = sensorData[id].flame_value;
      client["mq135_value"] = sensorData[id].mq135_value;
      client["ir_value"] = sensorData[id].ir_value;
      client["dht_temp"] = sensorData[id].dht_temp;
      client["dht_hum"] = sensorData[id].dht_hum;
      client["servo_angle"] = sensorData[id].servo_angle;
    }
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("====================================");
  Serial.println(" HOST ESP32 - HTTP SENSOR DASHBOARD");
  Serial.println(" (Static IP Version)");
  Serial.println("====================================");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  // ============================================================
  // STATIC IP CONFIG — WiFi.begin() se PEHLE
  // ============================================================
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("WARNING: Static IP config FAILED!");
    Serial.println("Falling back to dynamic IP.");
  } else {
    Serial.println("Static IP configured.");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED &&
         (unsigned long)(millis() - startAttempt) < 20000UL) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Wi-Fi connected.");
    Serial.print("Host IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Host Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Host Subnet: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Host MAC: ");
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println();
    Serial.println("WARNING: Wi-Fi not connected!");
    Serial.println("Check WIFI_SSID, WIFI_PASSWORD, and static IP settings.");
  }

  // Browser dashboard
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  // Browser polls this endpoint
  server.on("/data", HTTP_GET, handleDashboardData);

  // Sensor clients POST JSON
  server.on("/api/data", HTTP_POST, handleClientData);

  server.onNotFound([]() {
    server.send(404, "application/json",
                "{\"ok\":false,\"error\":\"Not found\"}");
  });

  server.begin();
  Serial.println("HTTP server started on port 80.");
  Serial.print("Open dashboard: http://");
  Serial.println(WiFi.localIP());
  Serial.println("*** FIXED IP: 10.163.44.100 ***");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  static bool wasConnected = false;
  static unsigned long lastReconnectAttempt = 0;

  const bool connected = (WiFi.status() == WL_CONNECTED);

  if (connected && !wasConnected) {
    Serial.println("Wi-Fi connected/reconnected.");
    Serial.print("Host IP: ");
    Serial.println(WiFi.localIP());
  }

  if (!connected && millis() - lastReconnectAttempt >= 5000UL) {
    lastReconnectAttempt = millis();
    Serial.println("Wi-Fi disconnected; reconnecting...");
    WiFi.reconnect();
  }

  wasConnected = connected;
  server.handleClient();
  delay(2);
}