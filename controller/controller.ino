#include <esp_now.h>
#include <WiFi.h>

uint8_t roverAddress[] = {0xA8, 0x46, 0x74, 0x5C, 0x1A, 0x7C};

#define JOY_X 4
#define JOY_Y 3

#define CENTER_X 3400
#define CENTER_Y 3350
#define DEADZONE 80

typedef struct {
  int x;
  int y;
} ControlData;

ControlData data;

int lastX = CENTER_X;
int lastY = CENTER_Y;

unsigned long lastSend = 0;
const int SEND_INTERVAL = 30;

void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, roverAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;

  esp_now_add_peer(&peer);

  Serial.println("Controller ready");
}

void loop() {

  if (millis() - lastSend < SEND_INTERVAL) return;
  lastSend = millis();

  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);

  // smoothing
  lastX = (lastX * 3 + x) / 4;
  lastY = (lastY * 3 + y) / 4;

  int sx = lastX;
  int sy = lastY;

  // deadzone AFTER smoothing
  if (abs(sx - CENTER_X) < DEADZONE) sx = CENTER_X;
  if (abs(sy - CENTER_Y) < DEADZONE) sy = CENTER_Y;

  data.x = sx;
  data.y = sy;

  // DEBUG
  Serial.printf("TX -> X:%d  Y:%d\n", sx, sy);

  esp_now_send(roverAddress, (uint8_t*)&data, sizeof(data));
}