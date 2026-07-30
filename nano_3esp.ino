#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/* ===== PINS ===== */
#define AQI_PIN    8
#define BUZZER_PIN 10

/* ===== WIFI ===== */
const char* ssid = "EquipGuard-ML";
const char* pass = "12345678";

/* ===== DATA ===== */
float airQuality = 0;
bool alertState  = false;
bool machineAlert = false;
bool irAlert      = false;       // received from C6 (originally from Board 3)

/* ===== TIMING ===== */
unsigned long lastSampleTime = 0;
float samplingInterval = 0;

void setup() {
    Serial.begin(115200);

    pinMode(AQI_PIN,    INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    connectWiFi();
}

void loop() {
    unsigned long currentTime = millis();
    samplingInterval = currentTime - lastSampleTime;
    lastSampleTime   = currentTime;

    if (WiFi.status() != WL_CONNECTED) connectWiFi();

    /* ===== READ LOCAL ===== */
    airQuality = analogRead(AQI_PIN);

    /* ===== GET MACHINE + IR DATA FROM C6 ===== */
    float getLatency = 0;
    {
        HTTPClient http;
        http.begin("http://192.168.4.1/sensors");

        unsigned long getStart = micros();
        int code = http.GET();
        unsigned long getEnd = micros();
        getLatency = (getEnd - getStart) / 1000.0;

        if (code == 200) {
            DynamicJsonDocument doc(512);
            DeserializationError err = deserializeJson(doc, http.getString());
            if (!err) {
                machineAlert = doc["machine_alert"];
                irAlert      = doc["ir_alert"];       // NEW — pulled from C6
            }
        }
        http.end();
    }

    /* ===== INFERENCE ===== */
    unsigned long startTime = micros();

    bool aqiAlert = (airQuality > 1600);
    alertState    = (machineAlert || aqiAlert || irAlert);   // IR included via C6

    unsigned long endTime = micros();
    float inferenceTime = (endTime - startTime) / 1000.0;

    /* ===== ACTUATOR ===== */
    digitalWrite(BUZZER_PIN, alertState ? HIGH : LOW);

    /* ===== SEND DATA BACK TO C6 ===== */
    float postLatency = 0;
    {
        HTTPClient http;
        http.begin("http://192.168.4.1/upload");
        http.addHeader("Content-Type", "application/json");

        DynamicJsonDocument outDoc(256);
        outDoc["airQuality"] = airQuality;
        outDoc["alert"]      = alertState;

        String payload;
        serializeJson(outDoc, payload);

        unsigned long postStart = micros();
        http.POST(payload);
        unsigned long postEnd = micros();
        postLatency = (postEnd - postStart) / 1000.0;

        http.end();
    }

    Serial.printf(
        "AQI:%.1f | Machine:%d IR:%d | ALERT:%s | Infer:%.3fms Sample:%.1fms GET:%.1fms POST:%.1fms\n",
        airQuality, machineAlert, irAlert,
        alertState ? "YES" : "NO",
        inferenceTime, samplingInterval, getLatency, postLatency
    );

    delay(2000);
}

void connectWiFi() {
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) delay(500);
}