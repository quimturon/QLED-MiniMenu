#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>


// Callback RX (NO cridar directament)
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

void sendLedState();
void sendMessage(const uint8_t *mac, const char *msg);

extern char receivedTime[6];
extern uint8_t remoteParetBrightness;
extern uint8_t remotePrestatgeBrightness;

#endif
