#include "common.h"

// function for getting distance
int getDistance() {

  long sum{};
  int count{};
  long duration{};
  long avgDuration{};
  int distance{};

  for (int i{}; i < 3; i++) {

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH, 25000);

    if (duration > 0) {
      sum += duration;
      count++;
    }

    delay(5);
  }

  if (count == 0) {
    return 999;
  }

  avgDuration = (sum / count);
  distance = (avgDuration * 0.034 / 2 + 0.5);

  return distance;
}