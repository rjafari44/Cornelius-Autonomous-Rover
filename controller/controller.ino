#include <esp_now.h>
#include <WiFi.h>

uint8_t roverAddress[] = {0xA8, 0x46, 0x74, 0x5C, 0x1A, 0x7C};

#define JOY_X 4
#define JOY_Y 3

#define CENTER 2048
#define DEADZONE 80

typedef struct {
  int x;
  int y;
} ControlData;

ControlData data;

int lastX = CENTER;
int lastY = CENTER;

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

  // smoothing first
  lastX = (lastX * 3 + x) / 4;
  lastY = (lastY * 3 + y) / 4;

  int sx = lastX;
  int sy = lastY;

  // deadzone AFTER smoothing (important fix)
  if (abs(sx - CENTER) < DEADZONE) sx = CENTER;
  if (abs(sy - CENTER) < DEADZONE) sy = CENTER;

  data.x = sx;
  data.y = sy;

  esp_now_send(roverAddress, (uint8_t*)&data, sizeof(data));
}