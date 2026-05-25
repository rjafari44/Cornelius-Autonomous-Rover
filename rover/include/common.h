#pragma once

#include <esp_now.h>
#include <WiFi.h>

// Motor driver pins (updated for ESP32-C3)
constexpr int ENA = 7;
constexpr int IN1 = 6;
constexpr int IN2 = 5;
constexpr int IN3 = 4;
constexpr int IN4 = 3;
constexpr int ENB = 1;

// Ultrasonic pins
constexpr int trigPin = 10;
constexpr int echoPin = 8;

// Servo pin
constexpr int SERVO_PIN = 0;  // GPIO0 with PWM

// Motor control constants
constexpr int MOTOR_SPEED = 200;
constexpr float TURN_MULTIPLIER = 1.0;
constexpr int OBSTACLE_LIMIT = 20;
constexpr unsigned long FORWARD_TIMEOUT = 3000;

// Data structure for ESP-NOW (must match controller)
typedef struct {
  int x;      // Joystick X axis (0-4095)
  int y;      // Joystick Y axis (0-4095)
  bool autonomous;  // Toggle for autonomous mode
} ControlData;

// Mode tracking
extern bool autonomousMode;

// Functions for motor movements
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopMotors();
void driveMotors(int leftSpeed, int rightSpeed);

// Functions for gathering distances
int getDistance();
int lookLeft();
int lookRight();

// Autonomous mode
void runAutonomousMode();

// Manual control
void controlFromJoystick(int x, int y);