#include "espnow/espnow.h"
#include "neopixel/leds.h"   // 👈 aquí cridem les funcions dels LEDs


extern uint8_t lastBri0;
extern uint8_t lastBri1;
extern uint8_t bri0;
extern uint8_t bri1;
extern uint8_t targetBri0;
extern uint8_t targetBri1;
extern uint8_t briSteps;

void espNowSendText(const uint8_t *mac, const char *msg) {
    esp_now_send(mac, (const uint8_t *)msg, strlen(msg) + 1);
}

// ===============================
// RX CALLBACK
// ===============================
void onDataRecv(const uint8_t *mac,
                      const uint8_t *incomingData,
                      int len) {

    // 🔴 IMPORTANT: assumim STRING només si acaba amb \0
    if (incomingData[len - 1] != '\0') {
        Serial.println("⚠️ Paquet no-string rebut (ignorat)");
        return;
    }

    String msg = String((char *)incomingData);
    msg.trim();

    Serial.printf("📩 ESP-NOW rebut: %s\n", msg.c_str());

    // ===============================
    // COMANDES GLOBALS
    // ===============================
    if (msg == "toggleAll") {
        toggleTauleta();
        togglePrestatge();
    }
    else if (msg == "+briAll") {
        briPlusTauleta();
        briPlusPrestatge();
    }
    else if (msg == "-briAll") {
        briMinusTauleta();
        briMinusPrestatge();
    }
    else if (msg == "presetAll") {
        presetTauleta();
        presetPrestatge();
    }

    // ===============================
    // TIRA 0 (TAULETA)
    // ===============================
    else if (msg == "toggleTauleta") {
        toggleTauleta();
    }
    else if (msg == "+briTauleta") {
        briPlusTauleta();
    }
    else if (msg == "-briTauleta") {
        briMinusTauleta();
    }
    else if (msg == "presetTauleta") {
        presetTauleta();
    }

    // ===============================
    // TIRA 1 (PRESTATGE)
    // ===============================
    else if (msg == "togglePrestatge") {
        togglePrestatge();
    }
    else if (msg == "+briPrestatge") {
        briPlusPrestatge();
    }
    else if (msg == "-briPrestatge") {
        briMinusPrestatge();
    }
    else if (msg == "presetPrestatge") {
        presetPrestatge();
    }

    else {
        Serial.println("⚠️ Comanda desconeguda");
    }
}

