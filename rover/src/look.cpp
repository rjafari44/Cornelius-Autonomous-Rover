#include "common.h"
#include "servoDeclare.h"

// function for getting left angle values
int lookLeft() {

  int d1{};
  int d2{};
  int avg{};

  myServo.write(SERVO_CENTER + 35); // 135
  delay(400);
  d1 = getDistance();

  myServo.write(SERVO_CENTER + 80); // 180
  delay(400);
  d2 = getDistance();

  myServo.write(SERVO_CENTER);
  delay(300);

  avg = (d1 + d2) / 2;
  return avg;
}

// function for getting right angle values
int lookRight() {

  int d1{};
  int d2{};
  int avg{};

  myServo.write(SERVO_CENTER - 55); // 45
  delay(400);
  d1 = getDistance();

  myServo.write(SERVO_CENTER - 100); // 0
  delay(400);
  d2 = getDistance();

  myServo.write(SERVO_CENTER);
  delay(300);

  avg = (d1 + d2) / 2;
  return avg;
}