#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <creds.h>

// =============================================

// RGB Pin definitions
#define PIN_RED   16
#define PIN_BLUE  17
#define PIN_GREEN 18

// PWM Config
#define PWM_FREQ     5000
#define PWM_RES      8
#define CHANNEL_RED  0
#define CHANNEL_GREEN 1
#define CHANNEL_BLUE 2

AsyncWebServer server(80);

// Timer state for duration handling
unsigned long ledOffTime = 0;
bool ledTimerActive = false;

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(CHANNEL_RED, r);
  ledcWrite(CHANNEL_GREEN, g);
  ledcWrite(CHANNEL_BLUE, b);
}

void setup() {
  Serial.begin(115200);

  // Setup PWM channels (older ESP32 core API)
  ledcSetup(CHANNEL_RED, PWM_FREQ, PWM_RES);
  ledcSetup(CHANNEL_GREEN, PWM_FREQ, PWM_RES);
  ledcSetup(CHANNEL_BLUE, PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_RED, CHANNEL_RED);
  ledcAttachPin(PIN_GREEN, CHANNEL_GREEN);
  ledcAttachPin(PIN_BLUE, CHANNEL_BLUE);

  // Start with LEDs off
  setRGB(0, 0, 0);

  // Connect to WiFi
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // GET /led?color=red&value=255&duration=1000
  // GET /led?r=255&g=128&b=0&duration=1000
  server.on("/led", HTTP_GET, [](AsyncWebServerRequest *request) {
    uint8_t r = 0, g = 0, b = 0;
    int duration = 0;
    String colorName = "";

    // Check for RGB mode
    if (request->hasParam("r") || request->hasParam("g") || request->hasParam("b")) {
      if (request->hasParam("r")) r = request->getParam("r")->value().toInt();
      if (request->hasParam("g")) g = request->getParam("g")->value().toInt();
      if (request->hasParam("b")) b = request->getParam("b")->value().toInt();
      colorName = "rgb";
    }
    // Check for color name mode
    else if (request->hasParam("color")) {
      String color = request->getParam("color")->value();
      color.toLowerCase();
      uint8_t value = 255;
      if (request->hasParam("value")) {
        value = request->getParam("value")->value().toInt();
      }

      if (color == "red") { r = value; colorName = "red"; }
      else if (color == "green") { g = value; colorName = "green"; }
      else if (color == "blue") { b = value; colorName = "blue"; }
      else if (color == "white") { r = g = b = value; colorName = "white"; }
      else if (color == "off") { r = g = b = 0; colorName = "off"; }
      else {
        request->send(400, "application/json", "{\"error\":\"Unknown color\"}");
        return;
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing color or r/g/b params\"}");
      return;
    }

    // Get duration
    if (request->hasParam("duration")) {
      duration = request->getParam("duration")->value().toInt();
    }

    // Set the LEDs
    setRGB(r, g, b);

    // Set timer if duration specified
    if (duration > 0) {
      ledOffTime = millis() + duration;
      ledTimerActive = true;
    } else {
      ledTimerActive = false;
    }

    // Response
    String json = "{\"status\":\"ok\"";
    if (colorName == "rgb") {
      json += ",\"r\":" + String(r) + ",\"g\":" + String(g) + ",\"b\":" + String(b);
    } else {
      json += ",\"color\":\"" + colorName + "\"";
    }
    if (duration > 0) json += ",\"duration\":" + String(duration);
    json += "}";

    request->send(200, "application/json", json);
  });

  // GET /led/off
  server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    setRGB(0, 0, 0);
    ledTimerActive = false;
    request->send(200, "application/json", "{\"status\":\"ok\",\"color\":\"off\"}");
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle LED timer
  if (ledTimerActive && millis() >= ledOffTime) {
    setRGB(0, 0, 0);
    ledTimerActive = false;
  }
}
