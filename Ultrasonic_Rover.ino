#include "myheader.h"

constexpr int OBSTACLE_LIMIT{20};              // set detection limit (20 cm)
constexpr unsigned long FORWARD_TIMEOUT{3000}; // variable to limit movement after set time

// function for setting up the arduino code, runs once, returns nothing
void setup() {
  // setting the l298n pins as output
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // setup the ultrasonic pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // setup the servo 
  myServo.attach(SERVO_PIN);
  myServo.write(90); // center
  delay(1000);
}

// function for running the arduino code, loops, returns nothing
void loop() {
  int distance{};               // variable of integer type for distance to an object
  int leftDist{};               // variable of integer type for distance to an object on the left side
  int rightDist{};              // variable of integer type for distance to an object on the right side
  bool obstacleDetected{};      // variable of boolean type for object detected
  bool forwardTimeoutReached{}; // variable of boolean type for forward movement
  static unsigned long forwardStartTime{};         // variable to track time since movement

  distance = getDistance();  // get distance to object

  // if obstacle is detected within detection limit, set true, otherwise false
  obstacleDetected = (distance <= OBSTACLE_LIMIT);

  // if time since movement is not zero and time elapsed is more than 3 seconds, set true, otherwise false
  forwardTimeoutReached = (forwardStartTime != 0 && millis() - forwardStartTime >= FORWARD_TIMEOUT);

  // Start forward motion if path is clear and not already moving
  if (distance > OBSTACLE_LIMIT && forwardStartTime == 0) {
    moveForward();
    forwardStartTime = millis();
  }

  // If timeout reached, re-check path and keep going if clear
  if (forwardTimeoutReached) {
    forwardStartTime = millis(); // reset timer
    distance = getDistance();    // re-check distance
    if (distance > OBSTACLE_LIMIT) {
      moveForward();
      return;
    }
    obstacleDetected = true; // path is blocked, fall through to avoidance
  }

  // Run obstacle avoidance only if an obstacle is actually detected
  if (obstacleDetected) {
    stopMotors();
    forwardStartTime = 0; // reset timer

    // Back up a bit
    delay(200);
    moveBackward();
    delay(300);
    stopMotors();

    // Look left and right
    leftDist = lookLeft();
    rightDist = lookRight();

    // Decide which way to turn
    if (leftDist > rightDist) {
      turnLeft();
    } else {
      turnRight();
    }

    delay(400);
    stopMotors();
  }
}