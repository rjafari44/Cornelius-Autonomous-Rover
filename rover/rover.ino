#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ESP32Servo.h>

// motor driver pins
constexpr int ENA{7};
constexpr int IN1{6};
constexpr int IN2{5};
constexpr int IN3{4};
constexpr int IN4{3};
constexpr int ENB{1};

// ultrasonic pins
constexpr int TRIG_PIN{10};
constexpr int ECHO_PIN{8};

// servo pin
constexpr int SERVO_PIN{0};

// adjustable constants
constexpr int MOTOR_SPEED{200};
constexpr int OBSTACLE_LIMIT{20};
constexpr unsigned long FAILSAFE_MS{500};

// servo center angle
constexpr int SERVO_CENTER{100};

// servo sweep constants
unsigned long lastSweepTime{0};
constexpr unsigned long SWEEP_INTERVAL{5000};

// servo object
Servo myServo;

// ESP-NOW data struct
typedef struct {
  int x;
  int y;
  bool autonomous;
} ControlData;

// struct object
ControlData rx;

// ESP-NOW & remote control variables
bool hasPacket{false};
bool autonomousMode{false};
unsigned long lastPacketTime{0};

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

// function for getting left angle values
int lookLeft() {

  int d1{};
  int d2{};
  int avg{};

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

// function for getting right angle values
int lookRight() {

  int d1{};
  int d2{};
  int avg{};

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

// function for moving forward
void moveForward() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

// function for moving backwards
void moveBackward() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

// function for turning left
void turnLeft() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

// function for turning right
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

// function for manually controlling the rover
void manualControl(int x, int y) {

  if (x == 0 && y == 0) {
    stopMotors();
    return;
  }

  if (abs(y) >= abs(x)) {

    if (y > 0) {
      moveBackward();
    }
    else {
      moveForward();   
    }       

  } else {

    if (x > 0) {
      turnRight();
    }
    else {
      turnLeft();
    }
  }
}

// ================= AUTONOMOUS (STABLE + SWEEP) =================
void autonomousDrive() {

  static unsigned long lastMoveTime{0};
  unsigned long now{millis()};

  int d1 = getDistance();
  delay(5);
  int d2 = getDistance();

  int distance = (d1 + d2) / 2;

  // safe path to move forward
  if (distance > OBSTACLE_LIMIT) {

    if (millis() - lastMoveTime > 200) {
      moveForward();
      lastMoveTime = millis();
    }

    // five second sweep
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

  // obstacle detected
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

// ESP-NOW function for data handling
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(ControlData)) return;

  memcpy(&rx, incomingData, sizeof(rx));

  hasPacket = true;
  lastPacketTime = millis();
}

// main setup
void setup() {

  Serial.begin(115200);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

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

// main loop
void loop() {

  if (!hasPacket || millis() - lastPacketTime > FAILSAFE_MS) {
    stopMotors();
    delay(10);
    return;
  }

  autonomousMode = rx.autonomous;

  // switch between autonomous and manual
  if (autonomousMode) {
    autonomousDrive();
  } else {
    manualControl(rx.x, rx.y);
  }
}