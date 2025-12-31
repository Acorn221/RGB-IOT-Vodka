#include "LedController.h"

void LedController::applyRGB(uint8_t r, uint8_t g, uint8_t b) {
  currentR = r;
  currentG = g;
  currentB = b;
  ledcWrite(CHANNEL_RED, r);
  ledcWrite(CHANNEL_GREEN, g);
  ledcWrite(CHANNEL_BLUE, b);
}

uint8_t LedController::lerp(uint8_t start, uint8_t end, float t) {
  return start + (end - start) * t;
}

void LedController::begin() {
  ledcSetup(CHANNEL_RED, PWM_FREQ, PWM_RES);
  ledcSetup(CHANNEL_GREEN, PWM_FREQ, PWM_RES);
  ledcSetup(CHANNEL_BLUE, PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_RED, CHANNEL_RED);
  ledcAttachPin(PIN_GREEN, CHANNEL_GREEN);
  ledcAttachPin(PIN_BLUE, CHANNEL_BLUE);
  applyRGB(0, 0, 0);
}

void LedController::setRGB(uint8_t r, uint8_t g, uint8_t b) {
  stop();
  applyRGB(r, g, b);
}

void LedController::fadeTo(uint8_t r, uint8_t g, uint8_t b, uint16_t duration) {
  stop();
  fadeStartR = currentR;
  fadeStartG = currentG;
  fadeStartB = currentB;
  fadeTargetR = r;
  fadeTargetG = g;
  fadeTargetB = b;
  fadeDuration = duration;
  fadeStartTime = millis();
  fading = true;
}

void LedController::loadBuiltInSequence(const char* name, uint16_t speed) {
  if (strcmp(name, "rainbow") == 0) {
    sequence[0] = {255, 0, 0, speed, true};
    sequence[1] = {255, 127, 0, speed, true};
    sequence[2] = {255, 255, 0, speed, true};
    sequence[3] = {0, 255, 0, speed, true};
    sequence[4] = {0, 0, 255, speed, true};
    sequence[5] = {139, 0, 255, speed, true};
    sequenceLength = 6;
  } else if (strcmp(name, "flash") == 0) {
    sequence[0] = {255, 255, 255, speed, false};
    sequence[1] = {0, 0, 0, speed, false};
    sequenceLength = 2;
  } else if (strcmp(name, "breathe") == 0) {
    sequence[0] = {0, 0, 0, speed, true};
    sequence[1] = {255, 255, 255, speed, true};
    sequenceLength = 2;
  } else if (strcmp(name, "police") == 0) {
    sequence[0] = {255, 0, 0, (uint16_t)(speed / 2), false};
    sequence[1] = {0, 0, 0, (uint16_t)(speed / 4), false};
    sequence[2] = {255, 0, 0, (uint16_t)(speed / 2), false};
    sequence[3] = {0, 0, 0, (uint16_t)(speed / 4), false};
    sequence[4] = {0, 0, 255, (uint16_t)(speed / 2), false};
    sequence[5] = {0, 0, 0, (uint16_t)(speed / 4), false};
    sequence[6] = {0, 0, 255, (uint16_t)(speed / 2), false};
    sequence[7] = {0, 0, 0, (uint16_t)(speed / 4), false};
    sequenceLength = 8;
  } else {
    sequenceLength = 0;
  }
}

void LedController::startSequence(const char* name, uint16_t speed) {
  stop();
  loadBuiltInSequence(name, speed);
  if (sequenceLength == 0) return;

  sequenceLoop = true;
  sequenceIndex = 0;
  stepStartTime = millis();
  sequenceActive = true;
  startStep();
}

void LedController::startCustomSequence(SequenceStep* steps, uint8_t length, bool loop) {
  stop();
  sequenceLength = min(length, (uint8_t)MAX_SEQUENCE_STEPS);
  for (int i = 0; i < sequenceLength; i++) {
    sequence[i] = steps[i];
  }
  sequenceLoop = loop;
  sequenceIndex = 0;
  stepStartTime = millis();
  sequenceActive = true;
  startStep();
}

// Queue a built-in sequence
void LedController::queueSequence(const char* name, uint16_t speed) {
  if (queueCount >= MAX_QUEUE_SIZE) return;

  // If nothing playing, start immediately
  if (!sequenceActive && !fading) {
    startSequence(name, speed);
    sequenceLoop = false;  // Queued sequences don't loop
    return;
  }

  QueuedSequence& item = queue[queueTail];
  item.isBuiltIn = true;
  strncpy(item.name, name, sizeof(item.name) - 1);
  item.name[sizeof(item.name) - 1] = '\0';
  item.speed = speed;
  item.stepCount = 0;

  queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
  queueCount++;
}

// Queue a custom sequence
void LedController::queueCustomSequence(SequenceStep* steps, uint8_t length) {
  if (queueCount >= MAX_QUEUE_SIZE) return;

  // If nothing playing, start immediately
  if (!sequenceActive && !fading) {
    startCustomSequence(steps, length, false);
    return;
  }

  QueuedSequence& item = queue[queueTail];
  item.isBuiltIn = false;
  item.stepCount = min(length, (uint8_t)MAX_SEQUENCE_STEPS);
  for (int i = 0; i < item.stepCount; i++) {
    item.steps[i] = steps[i];
  }

  queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
  queueCount++;
}

void LedController::clearQueue() {
  queueHead = 0;
  queueTail = 0;
  queueCount = 0;
}

void LedController::clearAll() {
  clearQueue();
  stop();
  applyRGB(0, 0, 0);
}

void LedController::playNextFromQueue() {
  if (queueCount == 0) return;

  QueuedSequence& item = queue[queueHead];
  queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
  queueCount--;

  if (item.isBuiltIn) {
    loadBuiltInSequence(item.name, item.speed);
  } else {
    sequenceLength = item.stepCount;
    for (int i = 0; i < sequenceLength; i++) {
      sequence[i] = item.steps[i];
    }
  }

  if (sequenceLength == 0) {
    playNextFromQueue();  // Skip empty sequences
    return;
  }

  sequenceLoop = false;  // Queued sequences don't loop
  sequenceIndex = 0;
  stepStartTime = millis();
  sequenceActive = true;
  startStep();
}

void LedController::stop() {
  fading = false;
  sequenceActive = false;
}

void LedController::update() {
  // Handle fading
  if (fading) {
    unsigned long elapsed = millis() - fadeStartTime;
    if (elapsed >= fadeDuration) {
      applyRGB(fadeTargetR, fadeTargetG, fadeTargetB);
      fading = false;
    } else {
      float t = (float)elapsed / fadeDuration;
      applyRGB(
        lerp(fadeStartR, fadeTargetR, t),
        lerp(fadeStartG, fadeTargetG, t),
        lerp(fadeStartB, fadeTargetB, t)
      );
    }
  }

  // Handle sequence
  if (sequenceActive && !fading) {
    SequenceStep& step = sequence[sequenceIndex];
    unsigned long elapsed = millis() - stepStartTime;

    if (elapsed >= step.duration) {
      sequenceIndex++;
      if (sequenceIndex >= sequenceLength) {
        if (sequenceLoop) {
          sequenceIndex = 0;
        } else {
          sequenceActive = false;
          // Check queue for next sequence
          if (queueCount > 0) {
            playNextFromQueue();
            return;
          }
          // Queue empty, turn off
          applyRGB(0, 0, 0);
          return;
        }
      }
      stepStartTime = millis();
      startStep();
    }
  }

  // If nothing playing, check queue
  if (!sequenceActive && !fading && queueCount > 0) {
    playNextFromQueue();
  }
}

void LedController::startStep() {
  SequenceStep& step = sequence[sequenceIndex];
  if (step.fade) {
    fadeStartR = currentR;
    fadeStartG = currentG;
    fadeStartB = currentB;
    fadeTargetR = step.r;
    fadeTargetG = step.g;
    fadeTargetB = step.b;
    fadeDuration = step.duration;
    fadeStartTime = millis();
    fading = true;
  } else {
    applyRGB(step.r, step.g, step.b);
  }
}

// Getters
uint8_t LedController::getR() { return currentR; }
uint8_t LedController::getG() { return currentG; }
uint8_t LedController::getB() { return currentB; }
bool LedController::isFading() { return fading; }
bool LedController::isSequenceActive() { return sequenceActive; }
uint8_t LedController::getQueueLength() { return queueCount; }
