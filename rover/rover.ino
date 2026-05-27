#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

// ================= MOTOR PINS =================
#define ENA  7
#define ENB  1
#define IN1  6
#define IN2  5
#define IN3  4
#define IN4  3

// ================= AUTONOMOUS HARDWARE PINS =================
#define SERVO_PIN   9    // D9
#define TRIG_PIN    10
#define ECHO_PIN    8

// ================= JOYSTICK CENTER =================
#define CENTER_X    3400
#define CENTER_Y    3350

// ================= TIMEOUT =================
unsigned long lastPacketTime = 0;
const unsigned long FAILSAFE_MS = 500;
bool hasPacket = false;

// ================= DATA STRUCT =================
typedef struct {
  int  x;
  int  y;
  bool autonomous;
} ControlData;

ControlData rx;

// ================= PWM =================
const int PWM_FREQ = 2000;
const int PWM_RES  = 8;

// ================= AUTONOMOUS STATE =================
bool      inAuto       = false;
Servo     scanServo;

// ================= PWM SETUP =================
void setupPWM() {
  ledcAttach(ENA, PWM_FREQ, PWM_RES);
  ledcAttach(ENB, PWM_FREQ, PWM_RES);
}

// ================= MOTOR CONTROL =================
void driveMotors(float left, float right) {
  left  = constrain(left,  -255.0f, 255.0f);
  right = constrain(right, -255.0f, 255.0f);

  // Left motor
  if (left > 5)       { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  }
  else if (left < -5) { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
  else                { digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  }

  // Right motor
  if (right > 5)       { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  }
  else if (right < -5) { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
  else                 { digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);  }

  ledcWrite(ENA, (int)abs(left));
  ledcWrite(ENB, (int)abs(right));
}

void stopMotors() {
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ================= ULTRASONIC =================
long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout ~500cm
  if (duration == 0) return 999; // no echo = clear path
  return duration / 58;
}

// ================= AUTONOMOUS SCAN & DRIVE =================
//
// Sweeps servo 0–180°, samples distance every 15° (13 readings).
// Picks the angle with the greatest clearance, then turns that way
// and drives forward. Re-scans when an obstacle is detected ahead.
//
const int  SCAN_STEP     = 15;   // degrees between samples
const long OBSTACLE_CM   = 30;   // stop & re-scan if closer than this
const int  DRIVE_SPEED   = 200;  // 0-255
const int  TURN_SPEED    = 180;
const int  TURN_MS_PER_DEG = 8;  // tune this for your rover's turn rate

int bestAngle = 90; // last known best heading (90 = straight)

void scanAndChoose() {
  Serial.println("AUTO: scanning...");
  int   bestDeg  = 90;
  long  bestDist = 0;

  for (int angle = 0; angle <= 180; angle += SCAN_STEP) {
    scanServo.write(angle);
    delay(300); // let servo settle and echo clear

    long d = readDistanceCm();
    Serial.printf("  %3d° -> %ld cm\n", angle, d);

    if (d > bestDist) {
      bestDist = d;
      bestDeg  = angle;
    }
  }

  // Return servo to centre
  scanServo.write(90);
  delay(200);

  bestAngle = bestDeg;
  Serial.printf("AUTO: best path at %d° (%ld cm)\n", bestAngle, bestDist);
}

// Turn rover to align with bestAngle (0°=full left, 90°=straight, 180°=full right)
void turnToAngle(int targetDeg) {
  int delta = targetDeg - 90; // negative = turn left, positive = turn right

  if (abs(delta) < SCAN_STEP / 2) return; // close enough to straight

  int turnTime = abs(delta) * TURN_MS_PER_DEG;

  if (delta > 0) {
    // Turn right: left motor forward, right motor back
    driveMotors(TURN_SPEED, -TURN_SPEED);
  } else {
    // Turn left
    driveMotors(-TURN_SPEED, TURN_SPEED);
  }
  delay(turnTime);
  stopMotors();
  delay(100);
}

void runAutonomous() {
  // Check distance straight ahead before moving
  scanServo.write(90);
  delay(150);
  long ahead = readDistanceCm();

  if (ahead < OBSTACLE_CM) {
    // Obstacle — stop and re-scan
    stopMotors();
    Serial.printf("AUTO: obstacle at %ld cm, re-scanning\n", ahead);
    scanAndChoose();
    turnToAngle(bestAngle);
  } else {
    // Clear — drive forward
    driveMotors(DRIVE_SPEED, DRIVE_SPEED);
  }
}

// ================= MANUAL CONTROL (strict 4-dir) =================
//
// Since the controller already snaps to 4 directions before sending,
// we just need to map those snapped values cleanly.
//
void controlFromJoystick(int x, int y) {
  int dx = x - CENTER_X;
  int dy = y - CENTER_Y;

  // Both axes centred → stop
  if (dx == 0 && dy == 0) {
    stopMotors();
    return;
  }

  const float FULL = 255.0f;

  if (dy > 0) {
    // Forward
    driveMotors(FULL, FULL);
  } else if (dy < 0) {
    // Backward
    driveMotors(-FULL, -FULL);
  } else if (dx > 0) {
    // Right: spin in place
    driveMotors(FULL, -FULL);
  } else {
    // Left: spin in place
    driveMotors(-FULL, FULL);
  }
}

// ================= ESP-NOW RECEIVE =================
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(ControlData)) return;
  memcpy(&rx, incomingData, sizeof(rx));
  lastPacketTime = millis();
  hasPacket      = true;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  setupPWM();
  stopMotors();

  // Ultrasonic pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Servo
  scanServo.attach(SERVO_PIN);
  scanServo.write(90); // centre on boot

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

  // Failsafe — no recent packet
  if (!hasPacket || millis() - lastPacketTime > FAILSAFE_MS) {
    stopMotors();
    delay(10);
    return;
  }

  // Mode switch detection
  if (rx.autonomous && !inAuto) {
    Serial.println("Entering AUTONOMOUS mode");
    stopMotors();
    inAuto = true;
    scanAndChoose();         // initial scan on mode entry
  } else if (!rx.autonomous && inAuto) {
    Serial.println("Returning to MANUAL mode");
    stopMotors();
    inAuto = false;
    scanServo.write(90);     // park servo at centre
  }

  if (inAuto) {
    runAutonomous();
    delay(50);
  } else {
    Serial.printf("RX -> X:%d Y:%d\n", rx.x, rx.y);
    controlFromJoystick(rx.x, rx.y);
    delay(10);
  }
}
