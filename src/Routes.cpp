#include "Routes.h"
#include "LedController.h"
#include "Config.h"
#include <ArduinoJson.h>

static bool parseColor(AsyncWebServerRequest* request, uint8_t& r, uint8_t& g, uint8_t& b) {
  if (request->hasParam("r") || request->hasParam("g") || request->hasParam("b")) {
    r = request->hasParam("r") ? request->getParam("r")->value().toInt() : 0;
    g = request->hasParam("g") ? request->getParam("g")->value().toInt() : 0;
    b = request->hasParam("b") ? request->getParam("b")->value().toInt() : 0;
    return true;
  }

  if (request->hasParam("color")) {
    String color = request->getParam("color")->value();
    color.toLowerCase();
    uint8_t value = request->hasParam("value") ? request->getParam("value")->value().toInt() : 255;

    if (color == "red") { r = value; g = 0; b = 0; }
    else if (color == "green") { r = 0; g = value; b = 0; }
    else if (color == "blue") { r = 0; g = 0; b = value; }
    else if (color == "white") { r = value; g = value; b = value; }
    else if (color == "off") { r = 0; g = 0; b = 0; }
    else return false;
    return true;
  }
  return false;
}

// Static buffer for POST body
static String postBody;

void setupRoutes(AsyncWebServer& server) {
  // Register specific routes FIRST, generic /led LAST

  // GET /led/off
  server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest* request) {
    led.setRGB(0, 0, 0);
    request->send(200, "application/json", "{\"status\":\"ok\",\"color\":\"off\"}");
  });

  // GET /led/fade
  server.on("/led/fade", HTTP_GET, [](AsyncWebServerRequest* request) {
    uint8_t r, g, b;
    if (!parseColor(request, r, g, b)) {
      request->send(400, "application/json", "{\"error\":\"Missing color params\"}");
      return;
    }

    uint16_t duration = 1000;
    if (request->hasParam("duration")) {
      duration = request->getParam("duration")->value().toInt();
    }

    led.fadeTo(r, g, b, duration);

    String json = "{\"status\":\"ok\",\"r\":" + String(r) + ",\"g\":" + String(g) + ",\"b\":" + String(b) + ",\"duration\":" + String(duration) + "}";
    request->send(200, "application/json", json);
  });

  // GET /led/sequence - start or queue built-in sequence
  server.on("/led/sequence", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->hasParam("name")) {
      request->send(400, "application/json", "{\"error\":\"Missing name param\"}");
      return;
    }

    String name = request->getParam("name")->value();
    uint16_t speed = 500;
    if (request->hasParam("speed")) {
      speed = request->getParam("speed")->value().toInt();
    }

    bool queue = request->hasParam("queue") && request->getParam("queue")->value() == "true";

    if (queue) {
      led.queueSequence(name.c_str(), speed);
    } else {
      led.startSequence(name.c_str(), speed);
    }

    String json = "{\"status\":\"ok\",\"sequence\":\"" + name + "\",\"speed\":" + String(speed) +
                  ",\"queued\":" + (queue ? "true" : "false") +
                  ",\"queueLength\":" + String(led.getQueueLength()) + "}";
    request->send(200, "application/json", json);
  });

  // POST /led/sequence - custom sequence with queue and clear support
  server.on("/led/sequence", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, postBody);
      postBody = "";

      if (err) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }

      // Handle clear field
      if (doc["clear"].is<bool>() && doc["clear"].as<bool>()) {
        led.clearAll();
      }

      // Get steps array
      JsonArray arr;
      if (doc.is<JsonArray>()) {
        arr = doc.as<JsonArray>();
      } else if (doc["steps"].is<JsonArray>()) {
        arr = doc["steps"].as<JsonArray>();
      }

      if (!arr || arr.size() == 0) {
        if (doc["clear"].is<bool>() && doc["clear"].as<bool>()) {
          request->send(200, "application/json", "{\"status\":\"ok\",\"cleared\":true}");
          return;
        }
        request->send(400, "application/json", "{\"error\":\"Expected steps array\"}");
        return;
      }

      SequenceStep steps[MAX_SEQUENCE_STEPS];
      uint8_t count = 0;

      for (JsonObject step : arr) {
        if (count >= MAX_SEQUENCE_STEPS) break;
        steps[count].r = step["r"] | 0;
        steps[count].g = step["g"] | 0;
        steps[count].b = step["b"] | 0;
        steps[count].duration = step["duration"] | 500;
        steps[count].fade = step["fade"] | false;
        count++;
      }

      bool shouldQueue = doc["queue"] | false;

      if (shouldQueue) {
        led.queueCustomSequence(steps, count);
      } else {
        bool loop = doc["loop"] | false;
        led.startCustomSequence(steps, count, loop);
      }

      String json = "{\"status\":\"ok\",\"steps\":" + String(count) +
                    ",\"queued\":" + (shouldQueue ? "true" : "false") +
                    ",\"queueLength\":" + String(led.getQueueLength()) + "}";
      request->send(200, "application/json", json);
    },
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        postBody = "";
      }
      for (size_t i = 0; i < len; i++) {
        postBody += (char)data[i];
      }
    }
  );

  // GET /led/stop
  server.on("/led/stop", HTTP_GET, [](AsyncWebServerRequest* request) {
    bool clearQueue = request->hasParam("clear") && request->getParam("clear")->value() == "true";

    if (clearQueue) {
      led.clearAll();
    } else {
      led.stop();
    }

    request->send(200, "application/json", "{\"status\":\"ok\",\"cleared\":" + String(clearQueue ? "true" : "false") + "}");
  });

  // GET /led/clear
  server.on("/led/clear", HTTP_GET, [](AsyncWebServerRequest* request) {
    led.clearAll();
    request->send(200, "application/json", "{\"status\":\"ok\",\"cleared\":true}");
  });

  // GET /led/status
  server.on("/led/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    String json = "{\"r\":" + String(led.getR()) +
                  ",\"g\":" + String(led.getG()) +
                  ",\"b\":" + String(led.getB()) +
                  ",\"fading\":" + (led.isFading() ? "true" : "false") +
                  ",\"sequence\":" + (led.isSequenceActive() ? "true" : "false") +
                  ",\"queueLength\":" + String(led.getQueueLength()) + "}";
    request->send(200, "application/json", json);
  });

  // GET /led - instant set (MUST BE LAST - catches /led prefix)
  server.on("/led", HTTP_GET, [](AsyncWebServerRequest* request) {
    uint8_t r, g, b;
    if (!parseColor(request, r, g, b)) {
      request->send(400, "application/json", "{\"error\":\"Missing color params\"}");
      return;
    }

    led.setRGB(r, g, b);

    String json = "{\"status\":\"ok\",\"r\":" + String(r) + ",\"g\":" + String(g) + ",\"b\":" + String(b) + "}";
    request->send(200, "application/json", json);
  });
}
