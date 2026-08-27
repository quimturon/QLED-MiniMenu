#include "espnow/espnow.h"
#include "neopixel/leds.h"   // 👈 aquí cridem les funcions dels LEDs

extern void updateOLED(String buf);

extern String debugMsg2;
extern uint8_t lastBri0;
extern uint8_t lastBri1;
extern uint8_t lastBri2;
extern uint8_t lastBri3;
extern uint8_t bri0;
extern uint8_t bri1;
extern uint8_t bri2;
extern uint8_t bri3;
extern uint8_t targetBri0;
extern uint8_t targetBri1;
extern uint8_t targetBri2;
extern uint8_t targetBri3;
extern uint8_t briSteps;
extern uint8_t controladorAdress[];

uint8_t remotePreset0 = 2;
uint8_t remotePreset1 = 2;

void sendLedState() {
    char state[80];
    snprintf(state, sizeof(state), "STATE,%u,%u,%u,%u,%u,%u,%u,%u",
             bri0, remotePreset0, bri1, remotePreset1,
             ledStrips[0].targetBrightness, ledStrips[0].preset,
             ledStrips[1].targetBrightness, ledStrips[1].preset);
    esp_now_send(controladorAdress, (const uint8_t *)state, strlen(state) + 1);
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

    if (msg.startsWith("STATE,")) {
        int values[8];
        if (sscanf(msg.c_str(), "STATE,%d,%d,%d,%d,%d,%d,%d,%d",
                   &values[0], &values[1], &values[2], &values[3],
                   &values[4], &values[5], &values[6], &values[7]) == 8) {
            bri0 = constrain(values[0], 0, 255);
            remotePreset0 = constrain(values[1], 1, 4);
            bri1 = constrain(values[2], 0, 255);
            remotePreset1 = constrain(values[3], 1, 4);
        }
        return;
    }

    Serial.printf("📩 ESP-NOW rebut: %s\n", msg.c_str());

    // ===============================
    // TIRA 0 (TAULETA)
    // ===============================
    if (msg == "toggleTauleta") {
        toggleTauleta();
        debugMsg2 = "Tauleta Togg";
        updateOLED("Rebut Tauleta Togg");
        Serial.println("Rebut ESPNOW: Toggle Tauleta");
    }
    else if (msg == "+briTauleta") {
        briPlusTauleta();
        updateOLED("Rebut Tauleta Bri +");
        Serial.println("Rebut ESPNOW: Brillantor Tauleta +");
    }
    else if (msg == "-briTauleta") {
        briMinusTauleta();
        updateOLED("Rebut Tauleta Bri -");
        Serial.println("Rebut ESPNOW: Brillantor Tauleta -");
    }
    else if (msg == "presetTauleta") {
        presetTauleta();
        updateOLED("Rebut Tauleta Preset");
        Serial.println("Rebut ESPNOW: Preset Tauleta");
    }

    // ===============================
    // TIRA 1 (PRESTATGE)
    // ===============================
    else if (msg == "togglePrestatge") {
        togglePrestatge();
        debugMsg2 = "Prestatge Togg";
        updateOLED("Rebut Prestatge Togg");
        Serial.println("Rebut ESPNOW: Toggle Prestatge");
    }
    else if (msg == "+briPrestatge") {
        briPlusPrestatge();
        updateOLED("Rebut Prestatge Bri +");
        Serial.println("Rebut ESPNOW: Brillantor Prestatge +");
    }
    else if (msg == "-briPrestatge") {
        briMinusPrestatge();
        updateOLED("Rebut Prestatge Bri -");
        Serial.println("Rebut ESPNOW: Brillantor Prestatge -");
    }
    else if (msg == "presetPrestatge") {
        presetPrestatge();
        updateOLED("Rebut Prestatge Preset");
        Serial.println("Rebut ESPNOW: Preset Prestatge");
    }
    else if (msg.startsWith("briPrestatge=")) {
        String valueStr = msg.substring(strlen("briPrestatge="));
        int value = valueStr.toInt();  // convertir a número
        if (value >= 0 && value <= 255) {  // assegurar que està dins del rang de uint8_t
            bri3 = value;
            Serial.printf("ESPNOW: briPrestatge actualitzat a %d\n", bri3);
        } else {
            Serial.println("⚠️ Valor de briPrestatge fora de rang (0-255)");
        }
    }
    else if (msg.startsWith("briDespatx=")) {
        String valueStr = msg.substring(strlen("briDespatx="));
        int value = valueStr.toInt();  // convertir a número
        if (value >= 0 && value <= 255) {  // assegurar que està dins del rang de uint8_t
            bri2 = value;
            Serial.printf("ESPNOW: briDespatx actualitzat a %d\n", bri2);
        } else {
            Serial.println("⚠️ Valor de briDespatx fora de rang (0-255)");
        }
    }

    else {
        Serial.println("⚠️ Comanda desconeguda");
    }

    sendLedState();
}

