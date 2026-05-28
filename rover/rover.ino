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

  int distance1{};
  int distance2{};
  int avgDistance{};

  myServo.write(135);
  delay(500);

  distance1 = getDistance();

  myServo.write(180);
  delay(500);

  distance2 = getDistance();

  myServo.write(90);
  delay(300);

  avgDistance = (distance1 + distance2) / 2;

  return avgDistance;
}

// ================= LOOK RIGHT =================
int lookRight() {

  int distance1{};
  int distance2{};
  int avgDistance{};

  myServo.write(45);
  delay(500);

  distance1 = getDistance();

  myServo.write(0);
  delay(500);

  distance2 = getDistance();

  myServo.write(90);
  delay(300);

  avgDistance = (distance1 + distance2) / 2;

  return avgDistance;
}

// ================= MOTOR FUNCTIONS =================
void moveForward() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void moveBackward() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void turnLeft() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void turnRight() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

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

  // forward/backward priority
  if (abs(y) >= abs(x)) {

    if (y > 0) {
      moveForward();
    }
    else {
      moveBackward();
    }
  }
  else {

    if (x > 0) {
      turnRight();
    }
    else {
      turnLeft();
    }
  }
}

// ================= AUTONOMOUS =================
void autonomousDrive() {

  int distance{};
  int leftDist{};
  int rightDist{};

  distance = getDistance();

  if (distance > OBSTACLE_LIMIT) {

    moveForward();
    delay(200);

    return;
  }

  // obstacle detected
  stopMotors();

  delay(200);

  moveBackward();
  delay(300);

  stopMotors();

  leftDist = lookLeft();
  rightDist = lookRight();

  if (leftDist > rightDist) {
    turnLeft();
  }
  else {
    turnRight();
  }

  delay(400);

  stopMotors();
}

// ================= ESP-NOW CALLBACK =================
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(ControlData)) {
    return;
  }

  memcpy(&rx, incomingData, sizeof(rx));

  hasPacket = true;
  lastPacketTime = millis();
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  // motor pins
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // servo
  myServo.attach(SERVO_PIN);
  myServo.write(90);

  delay(1000);

  // stop motors initially
  stopMotors();

  // wifi
  WiFi.mode(WIFI_STA);

  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // esp-now
  if (esp_now_init() != ESP_OK) {

    Serial.println("ESP-NOW failed");

    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Rover Ready");
}

// ================= LOOP =================
void loop() {

  // failsafe
  if (!hasPacket ||
      millis() - lastPacketTime > FAILSAFE_MS) {

    stopMotors();

    delay(10);

    return;
  }

  autonomousMode = rx.autonomous;

  if (autonomousMode) {
    autonomousDrive();
  }
  else {
    manualControl(rx.x, rx.y);
  }
}