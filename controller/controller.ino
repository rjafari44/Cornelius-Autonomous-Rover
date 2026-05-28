#include <esp_now.h>
#include <WiFi.h>

uint8_t roverAddress[] = {0xA8, 0x46, 0x74, 0x5C, 0x1A, 0x7C};

// ================= PINS =================
#define JOY_X       4
#define JOY_Y       3
#define JOY_BTN     9   // Joystick press (active LOW) — try A1 if GPIO1 unresponsive
#define LED_GREEN   10   // Forward indicator / autonomous indicator
#define LED_RED     8    // Backward indicator / autonomous indicator

// ================= JOYSTICK CALIBRATION =================
#define CENTER_X    3400
#define CENTER_Y    3350
#define DEADZONE    200  // Wider deadzone to enforce strict 4-dir snapping


// ================= DATA =================
typedef struct {
  int  x;
  int  y;
  bool autonomous;  // true = autonomous, false = manual
} ControlData;

ControlData data;

// ================= STATE =================
int  lastX       = CENTER_X;
int  lastY       = CENTER_Y;
bool autonomous  = false;

unsigned long lastSend   = 0;
const int SEND_INTERVAL  = 30;

// Button state tracking
bool lastBtnState = HIGH;

// ================= HELPERS =================

// Map raw joystick to a proportional -255..255 value,
// scaling each side of center independently so both directions
// hit the same max magnitude despite the off-center neutral point.
int proportional(int raw, int center, int deadzone) {
  int delta = raw - center;
  if (abs(delta) < deadzone) return 0;

  if (delta > 0) {
    // Map (center+deadzone)..4095  →  0..255
    return map(delta, deadzone, 4095 - center, 0, 255);
  } else {
    // Map -(center-deadzone)..0  →  0..-255
    return -map(-delta, deadzone, center, 0, 255);
  }
}

// ================= LED CONTROL =================
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

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(JOY_BTN,   INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED,   OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED,   LOW);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, roverAddress, 6);
  peer.channel = 1;   // explicit channel — must match rover
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("Controller ready — hold joystick button 3s to toggle auto/manual");
}

// ================= LOOP =================
void loop() {

  // ---- Button press detection (falling edge, runs every iteration) ----
  bool btnState = digitalRead(JOY_BTN);

  if (lastBtnState == HIGH && btnState == LOW) {
    autonomous = !autonomous;
    Serial.printf("Mode toggled -> %s\n", autonomous ? "AUTONOMOUS" : "MANUAL");
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED,   HIGH);
    delay(100);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED,   LOW);
  }
  lastBtnState = btnState;

  // ---- Throttle send rate (no early return — button must always run) ----
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    int x = analogRead(JOY_X);
    int y = analogRead(JOY_Y);

    lastX = (lastX * 3 + x) / 4;
    lastY = (lastY * 3 + y) / 4;

    int mappedX = proportional(lastX, CENTER_X, DEADZONE);
    int mappedY = proportional(lastY, CENTER_Y, DEADZONE);

    updateLEDs(mappedX, mappedY);

    data.x          = mappedX;
    data.y          = mappedY;
    data.autonomous = autonomous;

    Serial.printf("TX -> X:%d  Y:%d  Mode:%s\n",
                  mappedX, mappedY,
                  autonomous ? "AUTO" : "MANUAL");

    esp_now_send(roverAddress, (uint8_t*)&data, sizeof(data));
  }
}
