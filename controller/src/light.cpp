#include "common.h"

// LED control function
void updateLEDs(int mappedX, int mappedY) {
  if (autonomous) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED,   HIGH);
    return;
  }

  if (mappedY > 0) {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED,   HIGH);
  } else if (mappedY < 0) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED,   LOW);
  } else {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED,   LOW);
  }
}