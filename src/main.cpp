#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <creds.h>

#include "LedController.h"
#include "Routes.h"
#include "WebSocketHandler.h"

// Global instances
LedController led;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setup() {
  Serial.begin(115200);
  led.begin();

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

  // Setup routes and WebSocket
  setupRoutes(server);
  setupWebSocket(server, ws);

  server.begin();
  Serial.println("HTTP server started");
  Serial.println("WebSocket at /ws");
}

void loop() {
  led.update();
  ws.cleanupClients();
}
