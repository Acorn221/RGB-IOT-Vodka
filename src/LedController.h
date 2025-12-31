#pragma once

#include <Arduino.h>
#include "Config.h"

class LedController {
public:
  void begin();
  void setRGB(uint8_t r, uint8_t g, uint8_t b);
  void fadeTo(uint8_t r, uint8_t g, uint8_t b, uint16_t duration);
  void startSequence(const char* name, uint16_t speed = 500);
  void startCustomSequence(SequenceStep* steps, uint8_t length, bool loop = true);
  void stop();
  void update();

  // Queue methods
  void queueSequence(const char* name, uint16_t speed = 500);
  void queueCustomSequence(SequenceStep* steps, uint8_t length);
  void clearQueue();
  void clearAll();  // Clear queue + stop current + LEDs off

  // Getters for status
  uint8_t getR();
  uint8_t getG();
  uint8_t getB();
  bool isFading();
  bool isSequenceActive();
  uint8_t getQueueLength();

private:
  // Current state
  uint8_t currentR = 0, currentG = 0, currentB = 0;

  // Fade state
  bool fading = false;
  uint8_t fadeStartR, fadeStartG, fadeStartB;
  uint8_t fadeTargetR, fadeTargetG, fadeTargetB;
  unsigned long fadeStartTime = 0;
  uint16_t fadeDuration = 0;

  // Sequence state
  bool sequenceActive = false;
  SequenceStep sequence[MAX_SEQUENCE_STEPS];
  uint8_t sequenceLength = 0;
  uint8_t sequenceIndex = 0;
  unsigned long stepStartTime = 0;
  bool sequenceLoop = true;

  // Queue state
  QueuedSequence queue[MAX_QUEUE_SIZE];
  uint8_t queueHead = 0;
  uint8_t queueTail = 0;
  uint8_t queueCount = 0;

  void applyRGB(uint8_t r, uint8_t g, uint8_t b);
  uint8_t lerp(uint8_t start, uint8_t end, float t);
  void startStep();
  void playNextFromQueue();
  void loadBuiltInSequence(const char* name, uint16_t speed);
};

// Global instance declaration
extern LedController led;
