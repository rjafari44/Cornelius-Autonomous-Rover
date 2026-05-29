#pragma once

#include <esp_now.h>
#include <WiFi.h>

// motor driver pins
constexpr int ENA{7};
constexpr int IN1{6};
constexpr int IN2{5};
constexpr int IN3{4};
constexpr int IN4{3};
constexpr int ENB{1};

// ultrasonic pins
constexpr int TRIG_PIN{10};
constexpr int ECHO_PIN{8};

// servo pin
constexpr int SERVO_PIN{0};

// adjustable constants
constexpr int MOTOR_SPEED{200};
constexpr int OBSTACLE_LIMIT{20};
constexpr unsigned long FAILSAFE_MS{500};

// servo center angle
constexpr int SERVO_CENTER{100};

// sweep interval
constexpr unsigned long SWEEP_INTERVAL{5000};

// ESP-NOW data struct
typedef struct {
  int x;
  int y;
  bool autonomous;
} ControlData;

// shared variables
extern ControlData rx;
extern unsigned long lastSweepTime;
extern bool hasPacket;
extern bool autonomousMode;
extern unsigned long lastPacketTime;

// function declarations
int lookLeft();
int lookRight();
int getDistance();
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopMotors();
void manualControl(int x, int y);
void autonomousDrive();
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len);