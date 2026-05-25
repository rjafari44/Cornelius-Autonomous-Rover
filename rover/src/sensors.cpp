#include "../include/common.h"

int getDistance() {
  long sum = 0;
  int count = 0;
  long duration = 0;

  for (int i = 0; i < 3; i++) {

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH, 25000);

    if (duration > 0) {
      sum += duration;
      count++;
    }

    delay(5);
  }

  if (count == 0) return 999;

  long avgDuration = sum / count;

  int distance = (avgDuration * 0.034 / 2) + 0.5;

  return distance;
}