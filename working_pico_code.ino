#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <ArduinoJson.h>

/* ===== PINS ===== */
#define VIBRATION_PIN  14
#define CURRENT_PIN    4
#define VOLTAGE_PIN    3

WebServer server(80);
Adafruit_BMP085 bmp;

/* ===== SENSOR DATA ===== */
float temp = 25.0;
float pressure = 900.0;
int vibration = 0;
float current = 0, voltage = 0;
bool alert = false;

/* ===== NANO DATA ===== */
float nano_airQuality = 0;
int nano_ir = 0;
bool nano_alert = false;

unsigned long lastUpdate = 0;

/* ===== TIMING VARIABLES ===== */
unsigned long lastSampleTime = 0;
float samplingInterval = 0;

unsigned long requestStart = 0;
float commLatency = 0;

/* ===== TRAINED MODEL PARAMETERS ===== */
float mean_temp = 28.267232;
float std_temp  = 1.288181;

float mean_pressure = 912.611580;
float std_pressure  = 0.652943;

float mean_current = 0.130842;
float std_current  = 0.008495;

float mean_voltage = 3.294491;
float std_voltage  = 0.534438;

float threshold = 7.3184;

int anomalyCounter = 0;

/* ===== SETUP ===== */
void setup() {

    Serial.begin(115200);
    delay(1500);

    pinMode(VIBRATION_PIN, INPUT);
    pinMode(CURRENT_PIN, INPUT);
    pinMode(VOLTAGE_PIN, INPUT);

    #define SDA_PIN 6
    #define SCL_PIN 7

    Wire.begin(SDA_PIN, SCL_PIN);

    if (!bmp.begin()) {
        Serial.println("BMP180 not found!");
        while (1);
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP("EquipGuard-ML", "12345678");

    server.on("/", handleDashboard);
    server.on("/sensors", handleSensors);
    server.on("/upload", HTTP_POST, handleNanoUpload);

    server.begin();

    Serial.println("EquipGuard-ML C6 Started");
}

/* ===== LOOP ===== */
void loop() {

    server.handleClient();

    if (millis() - lastUpdate > 2500) {
        readSensors();
        lastUpdate = millis();
    }

}

/* ===== SENSOR READ + ANOMALY ===== */
void readSensors() {

    /* ===== SAMPLING INTERVAL ===== */
    unsigned long currentTime = millis();
    samplingInterval = currentTime - lastSampleTime;
    lastSampleTime = currentTime;

    /* ===== INFERENCE TIMER START ===== */
    unsigned long startTime = micros();

    temp = bmp.readTemperature();
    pressure = bmp.readPressure() / 100.0;

    vibration = digitalRead(VIBRATION_PIN);

    current = analogRead(CURRENT_PIN) * (3.3 / 4095.0);
    voltage = analogRead(VOLTAGE_PIN) * (3.3 / 4095.0) * 5;

    /* ===== VIBRATION OVERRIDE ===== */
    if (vibration == 1) {

        alert = true;
        anomalyCounter = 3;

    }
    else {

        /* ===== Z-SCORE MODEL ===== */

        float z_temp     = abs((temp - mean_temp) / std_temp);
        float z_pressure = abs((pressure - mean_pressure) / std_pressure);
        float z_current  = abs((current - mean_current) / std_current);
        float z_voltage  = abs((voltage - mean_voltage) / std_voltage);

        float score = z_temp + z_pressure + z_current + z_voltage;

        Serial.print("Score: ");
        Serial.println(score);

        if (score > threshold)
            anomalyCounter++;
        else
            anomalyCounter = 0;

        alert = (anomalyCounter >= 3);

    }

    /* ===== INFERENCE TIMER END ===== */

    unsigned long endTime = micros();
    float inferenceTime = (endTime - startTime) / 1000.0;

    Serial.print("Inference Time (ms): ");
    Serial.println(inferenceTime);

    Serial.print("Sampling Interval (ms): ");
    Serial.println(samplingInterval);
}

/* ===== RECEIVE FROM NANO ===== */

void handleNanoUpload() {

    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));

    nano_airQuality = doc["airQuality"];
    nano_ir = doc["ir"];
    nano_alert = doc["alert"];

    server.send(200, "text/plain", "OK");
}

/* ===== JSON API ===== */

void handleSensors() {

    /* ===== COMMUNICATION LATENCY START ===== */

    requestStart = micros();

    DynamicJsonDocument doc(512);

    doc["temp"] = temp;
    doc["pressure"] = pressure;
    doc["vibration"] = vibration;
    doc["current"] = current;
    doc["voltage"] = voltage;
    doc["machine_alert"] = alert;

    doc["airQuality"] = nano_airQuality;
    doc["ir"] = nano_ir;
    doc["nano_alert"] = nano_alert;

    String out;
    serializeJson(doc, out);

    /* ===== COMMUNICATION LATENCY END ===== */

    unsigned long requestEnd = micros();
    commLatency = requestEnd - requestStart;

    Serial.print("Communication Latency (ms): ");
    Serial.println(commLatency);

    server.send(200, "application/json", out);
}

/* ===== DASHBOARD UI ===== */

void handleDashboard() {

    const char* page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>EquipGuard Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body{
background:#0f172a;
font-family:Arial;
color:white;
text-align:center;
}

.grid{
display:grid;
grid-template-columns:repeat(2,1fr);
gap:15px;
margin-top:30px;
}

.card{
background:#1e293b;
padding:20px;
border-radius:10px;
}

h3{
color:#94a3b8;
}

p{
font-size:28px;
}
</style>
</head>

<body>

<h2>EquipGuard Dashboard</h2>

<div class="grid">

<div class="card"><h3>Temperature</h3><p id="temp">--</p></div>
<div class="card"><h3>Pressure</h3><p id="pressure">--</p></div>
<div class="card"><h3>Vibration</h3><p id="vibration">--</p></div>
<div class="card"><h3>Current</h3><p id="current">--</p></div>
<div class="card"><h3>Voltage</h3><p id="voltage">--</p></div>
<div class="card"><h3>Air Quality</h3><p id="aqi">--</p></div>
<div class="card"><h3>IR</h3><p id="ir">--</p></div>

</div>

<script>

setInterval(()=>{

fetch('/sensors')
.then(r=>r.json())
.then(d=>{

temp.innerHTML=d.temp+" °C";
pressure.innerHTML=d.pressure;
vibration.innerHTML=d.vibration;
current.innerHTML=d.current+" A";
voltage.innerHTML=d.voltage+" V";
aqi.innerHTML=d.airQuality;
ir.innerHTML=d.ir;

});

},2000);

</script>

</body>
</html>
)rawliteral";

    server.send(200, "text/html", page);
}