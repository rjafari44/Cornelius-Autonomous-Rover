#include "common.h"

/* since the joysticks are not exactly centered, there needs to be some scaling:
- map raw joystick to a proportional -255..255 value,
- scaling each side of center independently so both directions
- hit the same max magnitude despite the off-center neutral point.
*/
int proportional(int raw, int center, int deadzone) {
  int delta = raw - center;
  if (abs(delta) < deadzone) return 0;

  if (delta > 0) {
    // map (center+deadzone)..4095  →  0..255
    return map(delta, deadzone, 4095 - center, 0, 255);
  } else {
    // map -(center-deadzone)..0  →  0..-255
    return -map(-delta, deadzone, center, 0, 255);
  }
}