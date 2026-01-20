#include "myheader.h"

/* A chart is provided on the README for how the different Movements are achieved with the TT motors */

const int motorSpeed {200};      // Global variable of integer type for speed (PWN, 0–255)
const float turnMultiplier{1}; // Global variable of float type for turning speed multiplier

// function for moving both motors forward, returns nothing
void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
}

// function for moving both motors backwards, returns nothing
void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
}

// function for turning left at a set speed, returns nothing
void turnLeft() {
  int turnSpeed{}; // variable of integer type for turn speed

  turnSpeed = motorSpeed * turnMultiplier; // calculate turning speed with the global variables

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, turnSpeed);
  analogWrite(ENB, turnSpeed);
}

// function for turning right at a set speed, returns nothing
void turnRight() {
  int turnSpeed{}; // variable of integer type for turn speed

  turnSpeed = motorSpeed * turnMultiplier; // calculate turning speed with the global variables

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, turnSpeed);
  analogWrite(ENB, turnSpeed);
}

// function for stopping motor movement, returns nothing
void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}