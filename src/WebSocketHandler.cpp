#include "WebSocketHandler.h"
#include "LedController.h"
#include "Config.h"
#include <ArduinoJson.h>

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WS client %u connected\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WS client %u disconnected\n", client->id());
  } else if (type == WS_EVT_DATA) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
      client->text("{\"error\":\"Invalid JSON\"}");
      return;
    }

    // Handle clear field on any command
    if (doc["clear"].is<bool>() && doc["clear"].as<bool>()) {
      led.clearAll();
    }

    const char* cmd = doc["cmd"];
    if (!cmd) {
      // If only clear was sent, that's ok
      if (doc["clear"].is<bool>() && doc["clear"].as<bool>()) {
        client->text("{\"status\":\"ok\",\"cleared\":true}");
        return;
      }
      client->text("{\"error\":\"Missing cmd\"}");
      return;
    }

    if (strcmp(cmd, "set") == 0) {
      uint8_t r = doc["r"] | 0;
      uint8_t g = doc["g"] | 0;
      uint8_t b = doc["b"] | 0;

      if (doc["color"].is<const char*>()) {
        const char* color = doc["color"];
        uint8_t value = doc["value"] | 255;
        if (strcmp(color, "red") == 0) { r = value; g = 0; b = 0; }
        else if (strcmp(color, "green") == 0) { r = 0; g = value; b = 0; }
        else if (strcmp(color, "blue") == 0) { r = 0; g = 0; b = value; }
        else if (strcmp(color, "white") == 0) { r = value; g = value; b = value; }
      }

      led.setRGB(r, g, b);
      client->text("{\"status\":\"ok\",\"cmd\":\"set\"}");

    } else if (strcmp(cmd, "fade") == 0) {
      uint8_t r = doc["r"] | 0;
      uint8_t g = doc["g"] | 0;
      uint8_t b = doc["b"] | 0;
      uint16_t duration = doc["duration"] | 1000;
      led.fadeTo(r, g, b, duration);
      client->text("{\"status\":\"ok\",\"cmd\":\"fade\"}");

    } else if (strcmp(cmd, "sequence") == 0) {
      const char* name = doc["name"] | "rainbow";
      uint16_t speed = doc["speed"] | 500;
      bool queue = doc["queue"] | false;

      if (queue) {
        led.queueSequence(name, speed);
      } else {
        led.startSequence(name, speed);
      }

      String response = "{\"status\":\"ok\",\"cmd\":\"sequence\",\"queued\":" +
                        String(queue ? "true" : "false") +
                        ",\"queueLength\":" + String(led.getQueueLength()) + "}";
      client->text(response);

    } else if (strcmp(cmd, "custom") == 0) {
      // Custom sequence: {"cmd":"custom","steps":[...],"queue":true/false}
      JsonArray arr = doc["steps"].as<JsonArray>();
      if (!arr || arr.size() == 0) {
        client->text("{\"error\":\"Missing steps array\"}");
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

      bool queue = doc["queue"] | false;

      if (queue) {
        led.queueCustomSequence(steps, count);
      } else {
        bool loop = doc["loop"] | false;
        led.startCustomSequence(steps, count, loop);
      }

      String response = "{\"status\":\"ok\",\"cmd\":\"custom\",\"steps\":" + String(count) +
                        ",\"queued\":" + String(queue ? "true" : "false") +
                        ",\"queueLength\":" + String(led.getQueueLength()) + "}";
      client->text(response);

    } else if (strcmp(cmd, "stop") == 0) {
      led.stop();
      client->text("{\"status\":\"ok\",\"cmd\":\"stop\"}");

    } else if (strcmp(cmd, "clear") == 0) {
      led.clearAll();
      client->text("{\"status\":\"ok\",\"cmd\":\"clear\"}");

    } else if (strcmp(cmd, "status") == 0) {
      String json = "{\"r\":" + String(led.getR()) +
                    ",\"g\":" + String(led.getG()) +
                    ",\"b\":" + String(led.getB()) +
                    ",\"fading\":" + (led.isFading() ? "true" : "false") +
                    ",\"sequence\":" + (led.isSequenceActive() ? "true" : "false") +
                    ",\"queueLength\":" + String(led.getQueueLength()) + "}";
      client->text(json);

    } else {
      client->text("{\"error\":\"Unknown cmd\"}");
    }
  }
}

void setupWebSocket(AsyncWebServer& server, AsyncWebSocket& ws) {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
}
