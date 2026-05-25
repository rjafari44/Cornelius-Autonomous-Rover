#include <WiFi.h>
#include <esp_now.h>

// ================= PINS (UNCHANGED) =================
#define ENA 7
#define ENB 1
#define IN1 6
#define IN2 5
#define IN3 4
#define IN4 3

// ================= JOYSTICK MODEL =================
#define CENTER 2048
#define DEADZONE 70

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

// ================= PWM (ESP32-C3 SAFE) =================
const int PWM_FREQ = 2000;
const int PWM_RES = 8;

// ESP32-C3 uses pin-based LEDC (NO channels needed)
void setupPWM() {
  ledcAttach(ENA, PWM_FREQ, PWM_RES);
  ledcAttach(ENB, PWM_FREQ, PWM_RES);
}

// ================= MOTOR =================
void driveMotors(int left, int right) {

  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);

  // LEFT motor
  if (left > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (left < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  // RIGHT motor
  if (right > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (right < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }

  // PWM OUTPUT (FIXED)
  ledcWrite(ENA, abs(left));
  ledcWrite(ENB, abs(right));
}

void stopMotors() {
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ================= ESP-NOW CALLBACK =================
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(ControlData)) return;

  memcpy(&rx, incomingData, sizeof(rx));
  lastPacketTime = millis();
  hasPacket = true;
}

// ================= CONTROL =================
void controlFromJoystick(int x, int y) {

  if (abs(x - CENTER) < DEADZONE) x = CENTER;
  if (abs(y - CENTER) < DEADZONE) y = CENTER;

  float dx = (x - CENTER) / 2048.0f;
  float dy = (y - CENTER) / 2048.0f;

  if (fabs(dx) < 0.05) dx = 0;
  if (fabs(dy) < 0.05) dy = 0;

  if (dx == 0 && dy == 0) {
    stopMotors();
    return;
  }

  float left  = dy + dx;
  float right = dy - dx;

  left  = constrain(left, -1.0f, 1.0f);
  right = constrain(right, -1.0f, 1.0f);

  driveMotors(left * 255, right * 255);
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopMotors();

  setupPWM();   // 🔥 IMPORTANT FIX

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

  // 🚨 DO NOT RUN MOTORS BEFORE FIRST PACKET
  if (!hasPacket) {
    stopMotors();
    return;
  }

  // FAILSAFE
  if (millis() - lastPacketTime > FAILSAFE_MS) {
    stopMotors();
    return;
  }

  controlFromJoystick(rx.x, rx.y);
  delay(10);
}