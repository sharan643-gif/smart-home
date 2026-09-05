/*
  ============================================================
                  ESP32 SMART ROOM SYSTEM
  ============================================================

  DHT11 #1
      VCC  -> 3.3V
      DATA -> GPIO 4
      GND  -> GND

  DHT11 #2
      VCC  -> 3.3V
      DATA -> GPIO 5
      GND  -> GND

  Traffic Light
      RED    -> GPIO 25
      YELLOW -> GPIO 26
      GREEN  -> GPIO 27
      GND    -> GND

  Wi-Fi:
      2.4 GHz
  ============================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>

// ============================================================
// WIFI
// ============================================================

const char* WIFI_SSID = "Sharan'sM17";
const char* WIFI_PASSWORD = "12@12@34";

// Unique ESP32 hostname
const char* HOSTNAME = "smartroom";

// ============================================================
// FALLBACK ACCESS POINT
// ============================================================

const char* AP_NAME = "ESP32-SmartRoom";
const char* AP_PASSWORD = "smartroom123";

// ============================================================
// DHT11
// ============================================================

#define DHT_TYPE DHT11

#define DHT1_PIN 4
#define DHT2_PIN 5

DHT dht1(DHT1_PIN, DHT_TYPE);
DHT dht2(DHT2_PIN, DHT_TYPE);

// ============================================================
// TRAFFIC LIGHT
// ============================================================

#define RED_LED     25
#define YELLOW_LED  26
#define GREEN_LED   27

// ============================================================
// TEMPERATURE LIMITS
// ============================================================

#define GREEN_LIMIT  32.0
#define YELLOW_LIMIT 35.0

// ============================================================
// WEB SERVER
// ============================================================

WebServer server(80);

// ============================================================
// SENSOR DATA
// ============================================================

float temp1 = 0.0;
float hum1  = 0.0;

float temp2 = 0.0;
float hum2  = 0.0;

float averageTemp = 0.0;
float averageHum  = 0.0;

bool sensor1OK = false;
bool sensor2OK = false;

// ============================================================
// WIFI STATUS
// ============================================================

bool wifiOK = false;
bool apMode = false;

unsigned long lastWiFiAttempt = 0;
unsigned long lastSensorRead = 0;

const unsigned long SENSOR_INTERVAL = 2500;
const unsigned long WIFI_INTERVAL = 10000;

// ============================================================
// LED FUNCTIONS
// ============================================================

void ledsOff() {

  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
}


void redLight() {

  ledsOff();
  digitalWrite(RED_LED, HIGH);
}


void yellowLight() {

  ledsOff();
  digitalWrite(YELLOW_LED, HIGH);
}


void greenLight() {

  ledsOff();
  digitalWrite(GREEN_LED, HIGH);
}


// ============================================================
// STARTUP ANIMATION
// ============================================================

void startupAnimation() {

  for (int i = 0; i < 3; i++) {

    redLight();
    delay(180);

    yellowLight();
    delay(180);

    greenLight();
    delay(180);

    ledsOff();
    delay(100);
  }
}


// ============================================================
// TRAFFIC LIGHT LOGIC
// ============================================================

void updateTrafficLight() {

  if (!sensor1OK && !sensor2OK) {

    redLight();
    return;
  }

  if (averageTemp <= GREEN_LIMIT) {

    greenLight();

  }
  else if (averageTemp <= YELLOW_LIMIT) {

    yellowLight();

  }
  else {

    redLight();
  }
}


// ============================================================
// SENSOR READING
// ============================================================

void readSensors() {

  float t1 = dht1.readTemperature();
  float h1 = dht1.readHumidity();

  float t2 = dht2.readTemperature();
  float h2 = dht2.readHumidity();


  // ----------------------------------------------------------
  // SENSOR 1
  // ----------------------------------------------------------

  if (!isnan(t1) && !isnan(h1)) {

    temp1 = t1;
    hum1 = h1;

    sensor1OK = true;

  }
  else {

    sensor1OK = false;
  }


  // ----------------------------------------------------------
  // SENSOR 2
  // ----------------------------------------------------------

  if (!isnan(t2) && !isnan(h2)) {

    temp2 = t2;
    hum2 = h2;

    sensor2OK = true;

  }
  else {

    sensor2OK = false;
  }


  // ----------------------------------------------------------
  // AVERAGE
  // ----------------------------------------------------------

  if (sensor1OK && sensor2OK) {

    averageTemp =
      (temp1 + temp2) / 2.0;

    averageHum =
      (hum1 + hum2) / 2.0;
  }

  else if (sensor1OK) {

    averageTemp = temp1;
    averageHum = hum1;
  }

  else if (sensor2OK) {

    averageTemp = temp2;
    averageHum = hum2;
  }


  updateTrafficLight();
}


// ============================================================
// CONNECT TO WIFI
// ============================================================

void connectWiFi() {

  Serial.println();
  Serial.println("================================");
  Serial.println("CONNECTING TO WIFI");
  Serial.println("================================");

  wifiOK = false;
  apMode = false;

  // Station mode
  WiFi.mode(WIFI_STA);

  // Give ESP32 a permanent hostname
  WiFi.setHostname(HOSTNAME);

  // Automatic reconnection
  WiFi.setAutoReconnect(true);

  // Start connection
  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  unsigned long startTime = millis();

  bool ledState = false;


  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startTime < 25000
  ) {

    ledState = !ledState;

    digitalWrite(
      RED_LED,
      LOW
    );

    digitalWrite(
      GREEN_LED,
      LOW
    );

    digitalWrite(
      YELLOW_LED,
      ledState
    );

    delay(500);

    Serial.print(".");
  }


  Serial.println();


  // ----------------------------------------------------------
  // SUCCESS
  // ----------------------------------------------------------

  if (WiFi.status() == WL_CONNECTED) {

    wifiOK = true;

    Serial.println();
    Serial.println("================================");
    Serial.println("       WIFI CONNECTED");
    Serial.println("================================");

    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    Serial.print("IP ADDRESS: ");
    Serial.println(WiFi.localIP());

    Serial.print("GATEWAY: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");


    // --------------------------------------------------------
    // START MDNS
    // --------------------------------------------------------

    if (MDNS.begin(HOSTNAME)) {

      Serial.println();
      Serial.println("mDNS STARTED");

      Serial.println(
        "Open: http://smartroom.local"
      );

      MDNS.addService(
        "http",
        "tcp",
        80
      );

    }
    else {

      Serial.println(
        "mDNS failed"
      );
    }


    Serial.println();
    Serial.println(
      "Dashboard:"
    );

    Serial.print(
      "http://"
    );

    Serial.println(
      WiFi.localIP()
    );


    greenLight();

    return;
  }


  // ----------------------------------------------------------
  // FAILED
  // ----------------------------------------------------------

  Serial.println();
  Serial.println(
    "WIFI CONNECTION FAILED"
  );

  Serial.print(
    "WiFi Status Code: "
  );

  Serial.println(
    WiFi.status()
  );


  wifiOK = false;

  // Start fallback
  startAccessPoint();
}


// ============================================================
// FALLBACK ACCESS POINT
// ============================================================

void startAccessPoint() {

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "STARTING FALLBACK ACCESS POINT"
  );

  Serial.println(
    "================================"
  );


  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(
    AP_NAME,
    AP_PASSWORD
  );


  apMode = true;


  Serial.print(
    "AP NAME: "
  );

  Serial.println(
    AP_NAME
  );


  Serial.print(
    "AP PASSWORD: "
  );

  Serial.println(
    AP_PASSWORD
  );


  Serial.print(
    "AP IP: "
  );

  Serial.println(
    WiFi.softAPIP()
  );


  yellowLight();
}


// ============================================================
// WIFI RECONNECTION
// ============================================================

void maintainWiFi() {

  if (
    millis() - lastWiFiAttempt <
    WIFI_INTERVAL
  ) {

    return;
  }


  lastWiFiAttempt = millis();


  // Already connected
  if (
    WiFi.status() == WL_CONNECTED
  ) {

    if (!wifiOK) {

      wifiOK = true;

      Serial.println(
        "WiFi connection restored!"
      );

      Serial.print(
        "IP: "
      );

      Serial.println(
        WiFi.localIP()
      );


      // Restart mDNS
      if (
        MDNS.begin(HOSTNAME)
      ) {

        MDNS.addService(
          "http",
          "tcp",
          80
        );
      }
    }

    return;
  }


  // ----------------------------------------------------------
  // CONNECTION LOST
  // ----------------------------------------------------------

  wifiOK = false;


  Serial.println();
  Serial.println(
    "WiFi connection lost."
  );

  Serial.println(
    "Attempting reconnection..."
  );


  WiFi.mode(WIFI_AP_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  unsigned long start =
    millis();


  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - start < 7000
  ) {

    delay(250);
  }


  if (
    WiFi.status() == WL_CONNECTED
  ) {

    wifiOK = true;
    apMode = false;


    Serial.println(
      "WiFi RECONNECTED!"
    );

    Serial.print(
      "IP: "
    );

    Serial.println(
      WiFi.localIP()
    );


    if (
      MDNS.begin(HOSTNAME)
    ) {

      MDNS.addService(
        "http",
        "tcp",
        80
      );
    }


    greenLight();

  }
  else {

    Serial.println(
      "Reconnection failed."
    );

    // Don't destroy AP mode.
    // Keep fallback available.

    if (!apMode) {

      startAccessPoint();
    }
  }
}


// ============================================================
// JSON API
// ============================================================

void handleData() {

  String json = "{";

  json += "\"temp1\":";
  json += String(temp1, 1);

  json += ",";

  json += "\"hum1\":";
  json += String(hum1, 1);

  json += ",";

  json += "\"temp2\":";
  json += String(temp2, 1);

  json += ",";

  json += "\"hum2\":";
  json += String(hum2, 1);

  json += ",";

  json += "\"averageTemp\":";
  json += String(averageTemp, 1);

  json += ",";

  json += "\"averageHum\":";
  json += String(averageHum, 1);

  json += ",";

  json += "\"sensor1\":";
  json += sensor1OK ? "true" : "false";

  json += ",";

  json += "\"sensor2\":";
  json += sensor2OK ? "true" : "false";

  json += ",";

  json += "\"wifi\":";
  json += wifiOK ? "true" : "false";

  json += ",";

  json += "\"ap\":";
  json += apMode ? "true" : "false";

  json += "}";


  server.send(
    200,
    "application/json",
    json
  );
}


// ============================================================
// WEB PAGE
// ============================================================

void handleRoot() {

  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width,initial-scale=1">

<title>ESP32 Smart Room</title>

<style>

* {
  box-sizing: border-box;
}

body {

  margin: 0;

  min-height: 100vh;

  font-family:
    Arial,
    Helvetica,
    sans-serif;

  background:
    linear-gradient(
      135deg,
      #020617,
      #111827
    );

  color: white;

  padding: 20px;
}

.container {

  max-width: 950px;

  margin: auto;
}

.header {

  text-align: center;

  margin-bottom: 25px;
}

.header h1 {

  font-size: 34px;

  margin: 0 0 8px;
}

.header p {

  color: #94a3b8;
}

.connection {

  text-align: center;

  padding: 14px;

  margin-bottom: 20px;

  border-radius: 18px;

  background:
    rgba(255,255,255,0.08);

}

.grid {

  display: grid;

  grid-template-columns:
    repeat(
      auto-fit,
      minmax(240px, 1fr)
    );

  gap: 18px;
}

.card {

  padding: 25px;

  border-radius: 24px;

  background:
    rgba(255,255,255,0.08);

  border:
    1px solid
    rgba(255,255,255,0.1);

  backdrop-filter:
    blur(20px);
}

.card h2 {

  margin-top: 0;
}

.label {

  color: #94a3b8;

  margin-top: 12px;
}

.value {

  font-size: 42px;

  font-weight: bold;

  margin: 8px 0;
}

.average {

  grid-column: 1 / -1;

  text-align: center;
}

.average .value {

  font-size: 58px;
}

.green {

  color: #22c55e;
}

.yellow {

  color: #facc15;
}

.red {

  color: #ef4444;
}

.footer {

  text-align: center;

  color: #64748b;

  margin-top: 25px;

  font-size: 13px;
}

</style>

</head>


<body>

<div class="container">


<div class="header">

<h1>🌡️ Smart Room</h1>

<p>
ESP32 Environmental Monitoring
</p>

</div>


<div class="connection">

<span id="connection">
Connecting to ESP32...
</span>

</div>


<div class="grid">


<div class="card">

<h2>📍 Room Sensor 1</h2>

<div class="label">
Temperature
</div>

<div
class="value"
id="temp1">
-- °C
</div>


<div class="label">
Humidity
</div>

<div
class="value"
id="hum1">
-- %
</div>

</div>



<div class="card">

<h2>📍 Room Sensor 2</h2>

<div class="label">
Temperature
</div>

<div
class="value"
id="temp2">
-- °C
</div>


<div class="label">
Humidity
</div>

<div
class="value"
id="hum2">
-- %
</div>

</div>



<div class="card average">

<h2>🏠 Room Average</h2>

<div class="label">
Temperature
</div>

<div
class="value"
id="avgTemp">
-- °C
</div>


<div class="label">
Humidity
</div>

<div
class="value"
id="avgHum">
-- %
</div>


<div
id="condition">
Checking environment...
</div>

</div>


</div>


<div class="footer">

ESP32 Smart Room Monitor<br>

Live sensor monitoring

</div>


</div>


<script>

function updateData() {

  fetch('/data', {
    cache: 'no-store'
  })

  .then(
    response => response.json()
  )

  .then(
    data => {

      document.getElementById(
        'temp1'
      ).innerHTML =
        data.temp1.toFixed(1)
        + ' °C';


      document.getElementById(
        'hum1'
      ).innerHTML =
        data.hum1.toFixed(1)
        + ' %';


      document.getElementById(
        'temp2'
      ).innerHTML =
        data.temp2.toFixed(1)
        + ' °C';


      document.getElementById(
        'hum2'
      ).innerHTML =
        data.hum2.toFixed(1)
        + ' %';


      document.getElementById(
        'avgTemp'
      ).innerHTML =
        data.averageTemp.toFixed(1)
        + ' °C';


      document.getElementById(
        'avgHum'
      ).innerHTML =
        data.averageHum.toFixed(1)
        + ' %';


      let avg =
        document.getElementById(
          'avgTemp'
        );

      let condition =
        document.getElementById(
          'condition'
        );


      avg.className =
        'value';


      if (
        data.averageTemp <= 32
      ) {

        avg.classList.add(
          'green'
        );

        condition.innerHTML =
          '🟢 Room temperature is comfortable';

      }

      else if (
        data.averageTemp <= 35
      ) {

        avg.classList.add(
          'yellow'
        );

        condition.innerHTML =
          '🟡 Room is getting warm';

      }

      else {

        avg.classList.add(
          'red'
        );

        condition.innerHTML =
          '🔴 Room temperature is high';

      }


      // Connection status

      if (data.wifi) {

        document.getElementById(
          'connection'
        ).innerHTML =
          '🟢 ESP32 connected to Wi-Fi';

      }

      else if (data.ap) {

        document.getElementById(
          'connection'
        ).innerHTML =
          '🟡 ESP32 Access Point Mode';

      }

      else {

        document.getElementById(
          'connection'
        ).innerHTML =
          '🔴 Wi-Fi disconnected';

      }

    }
  )

  .catch(
    error => {

      document.getElementById(
        'connection'
      ).innerHTML =
        '🔴 ESP32 not responding';

    }
  );
}


updateData();


setInterval(
  updateData,
  2500
);

</script>

</body>

</html>

)rawliteral";


  server.send(
    200,
    "text/html",
    html
  );
}


// ============================================================
// SERVER
// ============================================================

void startWebServer() {

  server.on(
    "/",
    HTTP_GET,
    handleRoot
  );

  server.on(
    "/data",
    HTTP_GET,
    handleData
  );


  server.onNotFound(
    []() {

      server.send(
        404,
        "text/plain",
        "ESP32 Smart Room: Page not found"
      );

    }
  );


  server.begin();


  Serial.println(
    "Web server started on port 80"
  );
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  // ----------------------------------------------------------
  // LED
  // ----------------------------------------------------------

  pinMode(
    RED_LED,
    OUTPUT
  );

  pinMode(
    YELLOW_LED,
    OUTPUT
  );

  pinMode(
    GREEN_LED,
    OUTPUT
  );

  ledsOff();


  // ----------------------------------------------------------
  // DHT
  // ----------------------------------------------------------

  dht1.begin();

  dht2.begin();


  // Give DHT11 time to initialize

  delay(2000);


  // ----------------------------------------------------------
  // STARTUP
  // ----------------------------------------------------------

  startupAnimation();


  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  connectWiFi();


  // ----------------------------------------------------------
  // WEB SERVER
  // ----------------------------------------------------------

  startWebServer();


  // ----------------------------------------------------------
  // SENSOR
  // ----------------------------------------------------------

  readSensors();


  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "SYSTEM READY"
  );

  Serial.println(
    "================================"
  );


  if (wifiOK) {

    Serial.println(
      "Open:"
    );

    Serial.println(
      "http://smartroom.local"
    );

    Serial.print(
      "OR IP: http://"
    );

    Serial.println(
      WiFi.localIP()
    );

  }

  else {

    Serial.println(
      "Connect to ESP32-SmartRoom"
    );

    Serial.println(
      "Password: smartroom123"
    );

    Serial.println(
      "Open: http://192.168.4.1"
    );
  }
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  // Web server
  server.handleClient();


  // ----------------------------------------------------------
  // SENSOR UPDATE
  // ----------------------------------------------------------

  if (
    millis() - lastSensorRead
    >= SENSOR_INTERVAL
  ) {

    lastSensorRead =
      millis();

    readSensors();
  }


  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  maintainWiFi();
}