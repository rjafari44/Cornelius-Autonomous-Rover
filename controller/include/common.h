#pragma once

#include <esp_now.h>
#include <WiFi.h>

// rover MAC address
extern uint8_t roverAddress[6];

// pins
constexpr int JOY_X{4};
constexpr int JOY_Y{3};
constexpr int JOY_BTN{9};    // switching modes
constexpr int LED_GREEN{10}; // forward indicator
constexpr int LED_RED{8};    // backward indicator

// joystick calibration since mine isn't exactly 2048
constexpr int CENTER_X{3400};
constexpr int CENTER_Y{3350};
constexpr int DEADZONE{200}; // large deadzone for not sticking

// send interval
constexpr int SEND_INTERVAL{30};

// ESP-NOW data struct
typedef struct {
  int  x;
  int  y;
  bool autonomous; // true = autonomous, false = manual
} ControlData;

// shared variables
extern ControlData data;
extern int lastX;
extern int lastY;
extern bool autonomous;
extern unsigned long lastSend;
extern bool lastBtnState;

// function declarations
int proportional(int raw, int center, int deadzone);
void updateLEDs(int mappedX, int mappedY);