#include "common.h"

// rover MAC address
uint8_t roverAddress[] = {0xA8, 0x46, 0x74, 0x5C, 0x1A, 0x7C};

// struct object
ControlData data;

// joystick states
int  lastX      = CENTER_X;
int  lastY      = CENTER_Y;
bool autonomous = false;

unsigned long lastSend  = 0;

// button state tracking
bool lastBtnState = HIGH;

// main setup
void setup() {
  Serial.begin(115200);

  pinMode(JOY_BTN, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, roverAddress, 6);
  peer.channel = 1;   // explicit channel which must match rover
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("Controller ready — hold joystick button 3s to toggle auto/manual");
}

// main loop
void loop() {

  // button press detection (falling edge, runs every iteration)
  bool btnState = digitalRead(JOY_BTN);

  if (lastBtnState == HIGH && btnState == LOW) {
    autonomous = !autonomous;

    Serial.printf("Mode toggled -> %s\n", autonomous ? "AUTONOMOUS" : "MANUAL");

    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED,   HIGH);
    delay(100);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED,   LOW);
  }
  lastBtnState = btnState;

  // throttle send rate (no early return — button must always run)
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    int x = analogRead(JOY_X);
    int y = analogRead(JOY_Y);

    lastX = (lastX * 3 + x) / 4;
    lastY = (lastY * 3 + y) / 4;

    int mappedX = proportional(lastX, CENTER_X, DEADZONE);
    int mappedY = proportional(lastY, CENTER_Y, DEADZONE);

    updateLEDs(mappedX, mappedY);

    data.x          = mappedX;
    data.y          = mappedY;
    data.autonomous = autonomous;

    Serial.printf("TX -> X:%d  Y:%d  Mode:%s\n",
                  mappedX, mappedY,
                  autonomous ? "AUTO" : "MANUAL");

    esp_now_send(roverAddress, (uint8_t*)&data, sizeof(data));
  }
}
