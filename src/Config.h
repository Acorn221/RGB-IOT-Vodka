#pragma once

#include <Arduino.h>

// RGB Pin definitions
#define PIN_RED   16
#define PIN_BLUE  17
#define PIN_GREEN 18

// PWM Config
#define PWM_FREQ      5000
#define PWM_RES       8
#define CHANNEL_RED   0
#define CHANNEL_GREEN 1
#define CHANNEL_BLUE  2

// Sequence config
#define MAX_SEQUENCE_STEPS 16
#define MAX_QUEUE_SIZE 8

// Sequence step structure
struct SequenceStep {
  uint8_t r, g, b;
  uint16_t duration;
  bool fade;
};

// Queued sequence structure
struct QueuedSequence {
  bool isBuiltIn;
  char name[16];                          // for built-in sequences
  uint16_t speed;                         // for built-in sequences
  SequenceStep steps[MAX_SEQUENCE_STEPS]; // for custom sequences
  uint8_t stepCount;                      // for custom sequences
};
