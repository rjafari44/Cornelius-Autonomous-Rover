#include "include/common.h"

bool autonomousMode = false;
ControlData receivedData;

// ---------------- ESP-NOW CALLBACK ----------------
void OnDataRecv(const esp_now_recv_info *recv_info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(ControlData)) return;

  memcpy(&receivedData, incomingData, sizeof(receivedData));

  Serial.println("RX");

  autonomousMode = receivedData.autonomous;

  if (!autonomousMode) {
    controlFromJoystick(receivedData.x, receivedData.y);
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("\n===== BOOT START =====");

  // Motor pins
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Ultrasonic pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.println("Pins OK");

  // ---------------- WIFI / ESP-NOW INIT ----------------
  WiFi.disconnect(true);
  delay(100);

  WiFi.mode(WIFI_STA);
  delay(100);

  Serial.println("WiFi STA OK");

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    while (true) delay(1000);
  }

  Serial.println("ESP-NOW INIT OK");

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Callback registered");

  Serial.println("===== ROVER READY =====");
}

// ---------------- LOOP ----------------
void loop() {

  if (autonomousMode) {
    runAutonomousMode();
  }

  delay(10);
}

// ---------------- JOYSTICK CONTROL ----------------
void controlFromJoystick(int x, int y) {

  int forward = map(y, 0, 4095, -255, 255);
  int turn = map(x, 0, 4095, -255, 255);

  if (abs(forward) < 100) forward = 0;
  if (abs(turn) < 100) turn = 0;

  int leftSpeed = constrain(forward + turn, -255, 255);
  int rightSpeed = constrain(forward - turn, -255, 255);

  driveMotors(leftSpeed, rightSpeed);
}