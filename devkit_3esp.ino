#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/* ===== PINS ===== */
#define IR_PIN 2

/* ===== WIFI ===== */
const char* ssid = "EquipGuard-ML";
const char* pass = "12345678";

/* ===== DATA ===== */
int ir = 0;
bool alertState = false;

/* ===== TIMING ===== */
unsigned long lastSampleTime = 0;
float samplingInterval = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);    
    pinMode(IR_PIN, INPUT);
    connectWiFi();
}

void loop() {
    unsigned long currentTime = millis();
    samplingInterval = currentTime - lastSampleTime;
    lastSampleTime   = currentTime;

    if (WiFi.status() != WL_CONNECTED) connectWiFi();

    /* ===== READ IR ===== */
    ir = digitalRead(IR_PIN);

    /* ===== INFERENCE ===== */
    unsigned long startTime = micros();

    alertState = (ir == 0);    // 0 = object detected → alert

    unsigned long endTime = micros();
    float inferenceTime = (endTime - startTime) / 1000.0;

    /* ===== SEND TO C6 ===== */
    float postLatency = 0;
    {
        HTTPClient http;
        http.begin("http://192.168.4.1/upload_ir");
        http.addHeader("Content-Type", "application/json");

        DynamicJsonDocument outDoc(128);
        outDoc["ir"]    = ir;
        outDoc["alert"] = alertState;

        String payload;
        serializeJson(outDoc, payload);

        unsigned long postStart = micros();
        int code = http.POST(payload);
        unsigned long postEnd = micros();
        postLatency = (postEnd - postStart) / 1000.0;

        http.end();
    }

    Serial.printf(
        "IR:%d | ALERT:%s | Infer:%.3fms Sample:%.1fms POST:%.1f us\n",
        ir,
        alertState ? "YES" : "NO",
        inferenceTime, samplingInterval, postLatency
    );

    delay(2000);
}

void connectWiFi() {
    WiFi.begin(ssid, pass);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
    } else {
        Serial.println("\nWiFi failed — will retry in loop");
    }
}