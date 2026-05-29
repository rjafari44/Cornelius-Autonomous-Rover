#include "common.h"
#include "servoDeclare.h"
#include <esp_wifi.h>

// servo object
Servo myServo;

// struct object
ControlData rx;

// shared variable definitions
unsigned long lastSweepTime{0};
bool hasPacket{false};
bool autonomousMode{false};
unsigned long lastPacketTime{0};

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
