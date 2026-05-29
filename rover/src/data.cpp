#include "common.h"

// ESP-NOW function for data handling
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(ControlData)) return;

  memcpy(&rx, incomingData, sizeof(rx));

  hasPacket = true;
  lastPacketTime = millis();
}