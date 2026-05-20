#pragma once

// include the necessary header files for the servo and arduino nano
#include <Servo.h>
#include <Arduino.h>

// set motor driver pins of integer type
constexpr int ENA = 3;
constexpr int IN1 = 4;
constexpr int IN2 = 8;
constexpr int IN3 = 6;
constexpr int IN4 = 7;
constexpr int ENB = 5;

// set ultrasonic pins of integer type
constexpr int trigPin = 10;
constexpr int echoPin = 11;

// set servo pin
constexpr int SERVO_PIN{9};

// declare the Servo object externally
inline Servo myServo;

// functions for gathering distances from the servo + ultrasonic combo
int getDistance();
int lookLeft();
int lookRight();

// functions for motor movements
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopMotors();
