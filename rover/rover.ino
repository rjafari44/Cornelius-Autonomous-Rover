#include <WiFi.h>
#include <esp_now.h>

// ================= PINS =================
#define ENA 7
#define ENB 1
#define IN1 6
#define IN2 5
#define IN3 4
#define IN4 3

// ================= YOUR JOYSTICK CENTER =================
#define CENTER_X 3400
#define CENTER_Y 3350

#define DEADZONE 70

// ================= ASSUMED ADC RANGE =================
const int X_MIN = 0;
const int X_MAX = 4095;
const int Y_MIN = 0;
const int Y_MAX = 4095;

// ================= TIMEOUT =================
unsigned long lastPacketTime = 0;
const unsigned long FAILSAFE_MS = 300;

bool hasPacket = false;

// ================= DATA =================
typedef struct {
  int x;
  int y;
} ControlData;

ControlData rx;

// ================= PWM =================
const int PWM_FREQ = 2000;
const int PWM_RES = 8;

// ================= PWM INIT =================
void setupPWM() {
  ledcAttach(ENA, PWM_FREQ, PWM_RES);
  ledcAttach(ENB, PWM_FREQ, PWM_RES);
}

// ================= MOTOR CONTROL =================
void driveMotors(float left, float right) {

  left = constrain(left, -255.0f, 255.0f);
  right = constrain(right, -255.0f, 255.0f);

  // LEFT motor direction
  if (left > 5) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (left < -5) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  // RIGHT motor direction
  if (right > 5) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (right < -5) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }

  ledcWrite(ENA, (int)abs(left));
  ledcWrite(ENB, (int)abs(right));
}

// ================= STOP =================
void stopMotors() {
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ================= ESP-NOW =================
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(ControlData)) return;

  memcpy(&rx, incomingData, sizeof(rx));

  lastPacketTime = millis();
  hasPacket = true;
}

// ================= CORE FIX: ASYMMETRIC NORMALIZATION =================
float normalizeAxis(int val, int center, int minVal, int maxVal) {

  if (val >= center) {
    float upRange = maxVal - center;
    if (upRange < 1) return 0;

    float v = (float)(val - center) / upRange;

    // amplify small upward range (your problem side)
    v *= 1.7f;

    return constrain(v, 0.0f, 1.0f);
  } 
  else {
    float downRange = center - minVal;
    if (downRange < 1) return 0;

    float v = (float)(val - center) / downRange;

    // compress large downward range so both sides feel equal
    v *= 0.85f;

    return constrain(v, -1.0f, 0.0f);
  }
}

// ================= CONTROL =================
void controlFromJoystick(int x, int y) {

  // deadzone around center
  if (abs(x - CENTER_X) < DEADZONE) x = CENTER_X;
  if (abs(y - CENTER_Y) < DEADZONE) y = CENTER_Y;

  float dx = normalizeAxis(x, CENTER_X, X_MIN, X_MAX);
  float dy = normalizeAxis(y, CENTER_Y, Y_MIN, Y_MAX);

  // additional noise filter
  if (fabs(dx) < 0.05f) dx = 0;
  if (fabs(dy) < 0.05f) dy = 0;

  if (dx == 0 && dy == 0) {
    stopMotors();
    return;
  }

  // arcade mixing
  float forward = dy;
  float turn = dx * 0.85f;

  float left = forward + turn;
  float right = forward - turn;

  // normalize output (IMPORTANT)
  float maxMag = max(fabs(left), fabs(right));
  if (maxMag > 1.0f) {
    left /= maxMag;
    right /= maxMag;
  }

  driveMotors(left * 255.0f, right * 255.0f);
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  setupPWM();
  stopMotors();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Rover ready");
}

// ================= LOOP =================
void loop() {

  if (!hasPacket || millis() - lastPacketTime > FAILSAFE_MS) {
    stopMotors();
    delay(10);
    return;
  }

  Serial.printf("RX -> X:%d Y:%d\n", rx.x, rx.y);

  controlFromJoystick(rx.x, rx.y);

  delay(10);
}