#include "common.h"

// function for manually controlling the rover
void manualControl(int x, int y) {

  if (x == 0 && y == 0) {
    stopMotors();
    return;
  }

  if (abs(y) >= abs(x)) {

    if (y > 0) {
      moveBackward();
    }
    else {
      moveForward();
    }

  } else {

    if (x > 0) {
      turnRight();
    }
    else {
      turnLeft();
    }
  }
}

// function for autonomous control
void autonomousDrive() {

  static unsigned long lastMoveTime{0};
  unsigned long now{millis()};

  int d1 = getDistance();
  delay(5);
  int d2 = getDistance();

  int distance = (d1 + d2) / 2;

  // safe path to move forward
  if (distance > OBSTACLE_LIMIT) {

    if (millis() - lastMoveTime > 200) {
      moveForward();
      lastMoveTime = millis();
    }

    // five second sweep
    if (now - lastSweepTime >= SWEEP_INTERVAL) {

      lastSweepTime = now;

      stopMotors();
      delay(80);

      int leftDist = lookLeft();
      int rightDist = lookRight();

      if (leftDist > rightDist + 10) {
        turnLeft();
        delay(200);
      }
      else if (rightDist > leftDist + 10) {
        turnRight();
        delay(200);
      }

      stopMotors();
    }

    return;
  }

  // obstacle detected
  stopMotors();
  delay(150);

  moveBackward();
  delay(300);

  stopMotors();

  int leftDist = lookLeft();
  int rightDist = lookRight();

  if (leftDist < OBSTACLE_LIMIT && rightDist < OBSTACLE_LIMIT) {
    moveBackward();
    delay(300);
    return;
  }

  if (leftDist > rightDist) {
    turnLeft();
  } else {
    turnRight();
  }

  delay(350);

  stopMotors();
}