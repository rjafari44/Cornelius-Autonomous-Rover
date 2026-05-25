#include <esp_now.h>
#include <WiFi.h>

uint8_t roverAddress[] = {0xA8, 0x46, 0x74, 0x5C, 0x1A, 0x7C};

#define JOY_X 4
#define JOY_Y 3
#define MODE_BUTTON 1

typedef struct {
  int x;
  int y;
  bool autonomous;
} ControlData;

ControlData myData;

bool autonomousMode = false;
bool lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(MODE_BUTTON, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  Serial.print("Controller MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peerInfo{};
  memcpy(peerInfo.peer_addr, roverAddress, 6);

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Controller ready!");
}

void loop() {

  bool buttonState = digitalRead(MODE_BUTTON);

  if (buttonState == LOW && lastButtonState == HIGH) {
    autonomousMode = !autonomousMode;

    Serial.println(
      autonomousMode ? "Autonomous Mode"
                     : "Manual Mode"
    );

    delay(200);
  }

  lastButtonState = buttonState;

  myData.x = analogRead(JOY_X);
  myData.y = analogRead(JOY_Y);
  myData.autonomous = autonomousMode;

  esp_err_t result = esp_now_send(
    roverAddress,
    (uint8_t *)&myData,
    sizeof(myData)
  );

  if (result == ESP_OK) {
    Serial.printf(
      "Sent: X=%d Y=%d Mode=%s\n",
      myData.x,
      myData.y,
      autonomousMode ? "AUTO" : "MANUAL"
    );
  } else {
    Serial.println("Send failed");
  }

  delay(50);
}