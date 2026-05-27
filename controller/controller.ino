#include <esp_now.h>
#include <WiFi.h>

uint8_t roverAddress[] = {0xA8, 0x46, 0x74, 0x5C, 0x1A, 0x7C};

// ================= PINS =================
#define JOY_X       4
#define JOY_Y       3
#define JOY_BTN     1    // Joystick press (active LOW)
#define LED_GREEN   10   // Forward indicator / autonomous indicator
#define LED_RED     8    // Backward indicator / autonomous indicator

// ================= JOYSTICK CALIBRATION =================
#define CENTER_X    3400
#define CENTER_Y    3350
#define DEADZONE    200  // Wider deadzone to enforce strict 4-dir snapping

// ================= HOLD DURATION TO TOGGLE MODE =================
#define MODE_HOLD_MS  3000

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

// Button hold tracking
bool          btnHeld       = false;
unsigned long btnPressTime  = 0;
bool          toggleFired   = false;  // prevent repeat firing while held

// ================= HELPERS =================

// Snap joystick to exactly one of 5 states: CENTER, UP, DOWN, LEFT, RIGHT
// Returns snapped (x, y) pair via pointers
void snapDirection(int rawX, int rawY, int &outX, int &outY) {
  int dx = rawX - CENTER_X;
  int dy = rawY - CENTER_Y;

  // If both axes are inside the deadzone, treat as centered
  if (abs(dx) < DEADZONE && abs(dy) < DEADZONE) {
    outX = CENTER_X;
    outY = CENTER_Y;
    return;
  }

  // Dominant axis wins — only one direction at a time
  if (abs(dx) >= abs(dy)) {
    // Horizontal dominant → LEFT or RIGHT
    outX = (dx > 0) ? 4095 : 0;
    outY = CENTER_Y;
  } else {
    // Vertical dominant → UP or DOWN
    outX = CENTER_X;
    outY = (dy > 0) ? 4095 : 0;
  }
}

// ================= LED CONTROL =================
void updateLEDs(int snappedX, int snappedY) {
  if (autonomous) {
    // Both LEDs on in autonomous mode
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED,   HIGH);
    return;
  }

  // Manual mode: indicate direction
  int dy = snappedY - CENTER_Y;

  if (dy > 0) {
    // Forward
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED,   LOW);
  } else if (dy < 0) {
    // Backward
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED,   HIGH);
  } else {
    // Stopped or turning — LEDs off
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
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("Controller ready — hold joystick button 3s to toggle auto/manual");
}

// ================= LOOP =================
void loop() {

  // ---- Button hold detection ----
  bool btnPressed = (digitalRead(JOY_BTN) == LOW);

  if (btnPressed) {
    if (!btnHeld) {
      // Button just went down
      btnHeld      = true;
      btnPressTime = millis();
      toggleFired  = false;
    } else if (!toggleFired && (millis() - btnPressTime >= MODE_HOLD_MS)) {
      // Held long enough — toggle mode
      autonomous  = !autonomous;
      toggleFired = true;
      Serial.printf("Mode toggled -> %s\n", autonomous ? "AUTONOMOUS" : "MANUAL");

      // Brief flash confirmation
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED,   HIGH);
      delay(300);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED,   LOW);
      delay(150);
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED,   HIGH);
      delay(300);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED,   LOW);
    }
  } else {
    btnHeld     = false;
    toggleFired = false;
  }

  // ---- Throttle send rate ----
  if (millis() - lastSend < SEND_INTERVAL) return;
  lastSend = millis();

  // ---- Read & smooth joystick ----
  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);

  lastX = (lastX * 3 + x) / 4;
  lastY = (lastY * 3 + y) / 4;

  // ---- Snap to 4 directions ----
  int snappedX, snappedY;
  snapDirection(lastX, lastY, snappedX, snappedY);

  // ---- Update LEDs ----
  updateLEDs(snappedX, snappedY);

  // ---- Pack and send ----
  data.x          = snappedX;
  data.y          = snappedY;
  data.autonomous = autonomous;

  Serial.printf("TX -> X:%d  Y:%d  Mode:%s\n",
                snappedX, snappedY,
                autonomous ? "AUTO" : "MANUAL");

  esp_now_send(roverAddress, (uint8_t*)&data, sizeof(data));
}
