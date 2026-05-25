#include "../include/common.h"

void runAutonomousMode() {

  static unsigned long forwardStartTime = 0;

  int distance = getDistance();
  bool obstacleDetected = (distance <= OBSTACLE_LIMIT);

  bool forwardTimeoutReached =
    (forwardStartTime != 0 &&
     millis() - forwardStartTime >= FORWARD_TIMEOUT);

  // move forward if clear
  if (!obstacleDetected && forwardStartTime == 0) {
    moveForward();
    forwardStartTime = millis();
  }

  // timeout safety reset
  if (forwardTimeoutReached) {
    forwardStartTime = millis();

    distance = getDistance();

    if (distance > OBSTACLE_LIMIT) {
      moveForward();
      return;
    }

    obstacleDetected = true;
  }

  // obstacle handling (NO scanning)
  if (obstacleDetected) {

    stopMotors();
    forwardStartTime = 0;

    delay(200);

    moveBackward();
    delay(300);

    stopMotors();

    // simple random-ish escape (instead of lookLeft/right)
    if (millis() % 2 == 0) {
      turnLeft();
    } else {
      turnRight();
    }

    delay(400);
    stopMotors();
  }
}