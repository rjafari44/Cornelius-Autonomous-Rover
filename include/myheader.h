#pragma once

// include the necessary header files for the servo and arduino nano
#include <Servo.h>
#include <Arduino.h>

// set motor driver pins of integer type
constexpr int ena = 3;
constexpr int in1 = 4;
constexpr int in2 = 8;
constexpr int in3 = 6;
constexpr int in4 = 7;
constexpr int enb = 5;

// set ultrasonic pins of integer type
constexpr int trigPin = 10;
constexpr int echoPin = 11;

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
