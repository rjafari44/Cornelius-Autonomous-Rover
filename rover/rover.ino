#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ESP32Servo.h>

// ================= PINS =================
constexpr int ENA = 7;
constexpr int IN1 = 6;
constexpr int IN2 = 5;
constexpr int IN3 = 4;
constexpr int IN4 = 3;
constexpr int ENB = 1;

constexpr int trigPin = 10;
constexpr int echoPin = 8;

constexpr int SERVO_PIN = 0;

// ================= CONSTANTS =================
constexpr int MOTOR_SPEED = 200;
constexpr int OBSTACLE_LIMIT = 20;
constexpr unsigned long FAILSAFE_MS = 500;

// SERVO CENTER FIX
constexpr int SERVO_CENTER = 100;

// ================= SWEEP TIMER =================
unsigned long lastSweepTime = 0;
constexpr unsigned long SWEEP_INTERVAL = 5000;

// ================= SERVO =================
Servo myServo;

// ================= ESP-NOW =================
typedef struct {
  int x;
  int y;
  bool autonomous;
} ControlData;

ControlData rx;

bool hasPacket = false;
bool autonomousMode = false;

unsigned long lastPacketTime = 0;

// ================= DISTANCE =================
int getDistance() {

  long sum{};
  int count{};
  long duration{};
  long avgDuration{};
  int distance{};

  for (int i{}; i < 3; i++) {

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

  if (count == 0) {
    return 999;
  }

  avgDuration = (sum / count);
  distance = (avgDuration * 0.034 / 2 + 0.5);

  return distance;
}

// ================= LOOK LEFT =================
int lookLeft() {

  int d1{}, d2{}, avg{};

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

// ================= LOOK RIGHT =================
int lookRight() {

  int d1{}, d2{}, avg{};

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

// ================= MOTOR FUNCTIONS =================
void moveForward() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void moveBackward() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void turnLeft() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void turnRight() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void stopMotors() {

  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ================= MANUAL CONTROL =================
void manualControl(int x, int y) {

  if (x == 0 && y == 0) {
    stopMotors();
    return;
  }

  if (abs(y) >= abs(x)) {

    if (y > 0) moveBackward();   // swapped
    else moveForward();          // swapped

  } else {

    if (x > 0) turnRight();
    else turnLeft();
  }
}

// ================= AUTONOMOUS (STABLE + SWEEP) =================
void autonomousDrive() {

  static unsigned long lastMoveTime = 0;

  unsigned long now = millis();

  int d1 = getDistance();
  delay(5);
  int d2 = getDistance();

  int distance = (d1 + d2) / 2;

  // SAFE PATH → MOVE FORWARD
  if (distance > OBSTACLE_LIMIT) {

    if (millis() - lastMoveTime > 200) {
      moveForward();
      lastMoveTime = millis();
    }

    // ================= 5 SECOND SWEEP =================
    if (now - lastSweepTime >= SWEEP_INTERVAL) {

      lastSweepTime = now;

      stopMotors();
      delay(80);

      int leftDist = lookLeft();
      int rightDist = lookRight();

      if (leftDist > rightDist + 10) {
        turnLeft();
        delay(200);
      }
      else if (rightDist > leftDist + 10) {
        turnRight();
        delay(200);
      }

      stopMotors();
    }

    return;
  }

  // OBSTACLE DETECTED
  stopMotors();
  delay(150);

  moveBackward();
  delay(300);

  stopMotors();

  int leftDist = lookLeft();
  int rightDist = lookRight();

  if (leftDist < OBSTACLE_LIMIT && rightDist < OBSTACLE_LIMIT) {
    moveBackward();
    delay(300);
    return;
  }

  if (leftDist > rightDist) {
    turnLeft();
  } else {
    turnRight();
  }

  delay(350);

  stopMotors();
}

// ================= ESP-NOW =================
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(ControlData)) return;

  memcpy(&rx, incomingData, sizeof(rx));

  hasPacket = true;
  lastPacketTime = millis();
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_CENTER);

  delay(800);

  stopMotors();

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Rover Ready");
}

// ================= LOOP =================
void loop() {

  if (!hasPacket || millis() - lastPacketTime > FAILSAFE_MS) {
    stopMotors();
    delay(10);
    return;
  }

  autonomousMode = rx.autonomous;

  if (autonomousMode) {
    autonomousDrive();
  } else {
    manualControl(rx.x, rx.y);
  }
}