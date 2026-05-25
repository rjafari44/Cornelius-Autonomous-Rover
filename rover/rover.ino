#include <WiFi.h>
#include <esp_now.h>

// ================= PINS =================
#define ENA 7
#define ENB 1
#define IN1 6
#define IN2 5
#define IN3 4
#define IN4 3

// ================= JOYSTICK =================
#define CENTER 2048
#define DEADZONE 70

// ================= FAILSAFE =================
volatile unsigned long lastPacketTime = 0;
const unsigned long FAILSAFE_MS = 300;

volatile bool hasPacket = false;

// ================= DATA =================
typedef struct {
  int x;
  int y;
} ControlData;

volatile ControlData rx;

// ================= PWM =================
const int PWM_FREQ = 2000;
const int PWM_RES  = 8;

void setupPWM() {
  ledcAttach(ENA, PWM_FREQ, PWM_RES);
  ledcAttach(ENB, PWM_FREQ, PWM_RES);
}

// ================= MOTOR =================
void stopMotors() {

  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void driveMotors(int left, int right) {

  left  = constrain(left, -255, 255);
  right = constrain(right, -255, 255);

  // LEFT
  if (left > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  }
  else if (left < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }
  else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  // RIGHT
  if (right > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }
  else if (right < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
  else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }

  ledcWrite(ENA, abs(left));
  ledcWrite(ENB, abs(right));
}

// ================= ESP NOW =================
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(ControlData)) return;

  ControlData temp;
  memcpy(&temp, incomingData, sizeof(temp));

  // reject garbage packets
  if (temp.x < 0 || temp.x > 4095) return;
  if (temp.y < 0 || temp.y > 4095) return;

  rx = temp;

  lastPacketTime = millis();
  hasPacket = true;
}

// ================= CONTROL =================
void controlFromJoystick(int x, int y) {

  if (abs(x - CENTER) < DEADZONE) x = CENTER;
  if (abs(y - CENTER) < DEADZONE) y = CENTER;

  float dx = (x - CENTER) / 2048.0f;
  float dy = (y - CENTER) / 2048.0f;

  if (fabs(dx) < 0.05f) dx = 0;
  if (fabs(dy) < 0.05f) dy = 0;

  if (dx == 0 && dy == 0) {
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

  // IMPORTANT: attach PWM BEFORE using ledcWrite
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

  // ALWAYS force stop first
  stopMotors();

  // no valid packets yet
  if (!hasPacket) {
    delay(10);
    return;
  }

  // failsafe timeout
  if (millis() - lastPacketTime > FAILSAFE_MS) {

    Serial.println("FAILSAFE STOP");

    hasPacket = false;

    delay(10);
    return;
  }

  // safely copy shared packet
  ControlData localRx;

  noInterrupts();
  localRx = rx;
  interrupts();

  // DEBUG
  Serial.printf("RX -> X:%d  Y:%d\n", localRx.x, localRx.y);

  controlFromJoystick(localRx.x, localRx.y);

  delay(10);
}