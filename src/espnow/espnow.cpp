#include "espnow/espnow.h"
#include "neopixel/leds.h"

extern void updateOLED(String buf);
extern String debugMsg2;
extern uint8_t controladorAdress[];
extern bool alarmRinging;
extern void startAlarm();

char receivedTime[6] = "--:--";
uint8_t remotePreset0 = 2;
uint8_t remotePreset1 = 2;
uint8_t remoteParetBrightness = 0;
uint8_t remotePrestatgeBrightness = 0;

void sendMessage(const uint8_t *mac, const char *msg) {
    esp_now_send(mac, (const uint8_t *)msg, strlen(msg) + 1);
}

void sendLedState() {
    char state[80];
    snprintf(state, sizeof(state), "STATE,%u,%u,%u,%u,%u,%u,%u,%u,%s",
             remoteParetBrightness, 0,
             remotePrestatgeBrightness, 0,
             ledStrips[0].targetBrightness, ledStrips[0].preset,
             ledStrips[1].targetBrightness, ledStrips[1].preset,
             receivedTime);
    esp_now_send(controladorAdress, (const uint8_t *)state, strlen(state) + 1);
}

void setStripBrightness(uint8_t stripIndex, int value) {
    uint8_t brightness = constrain(value, 0, 255);
    ledStrips[stripIndex].targetBrightness = brightness;
    if (brightness > 0) {
        if (stripIndex == 0) {
            remotePreset0 = ledStrips[stripIndex].preset;
        } else {
            remotePreset1 = ledStrips[stripIndex].preset;
        }
    }
}

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (incomingData == nullptr || len <= 0 || incomingData[len - 1] != '\0') {
        return;
    }

    String msg = String((char *)incomingData);
    msg.trim();

    if (msg == "ALARM_ON") {
        startAlarm();
        return;
    }

    if (msg == "toggleAll") {
        toggleTauleta();
        togglePrestatge();
        sendLedState();
        return;
    }
    if (msg == "+briAll") {
        briPlusTauleta();
        briPlusPrestatge();
        sendLedState();
        return;
    }
    if (msg == "-briAll") {
        briMinusTauleta();
        briMinusPrestatge();
        sendLedState();
        return;
    }

    if (msg.startsWith("setTauleta=")) {
        setStripBrightness(0, msg.substring(11).toInt());
        sendLedState();
        return;
    }
    if (msg.startsWith("setGeneral=")) {
        setStripBrightness(1, msg.substring(11).toInt());
        sendLedState();
        return;
    }

    if (msg.startsWith("STATE,")) {
        int values[8];
        char receivedClock[6] = "--:--";
        if (sscanf(msg.c_str(), "STATE,%d,%d,%d,%d,%d,%d,%d,%d,%5[0-9:]",
                   &values[0], &values[1], &values[2], &values[3],
                   &values[4], &values[5], &values[6], &values[7],
                   receivedClock) >= 8) {
            remoteParetBrightness = constrain(values[0], 0, 255);
            remotePrestatgeBrightness = constrain(values[2], 0, 255);
            if (strlen(receivedClock) == 5) {
                strcpy(receivedTime, receivedClock);
            }
        }
        return;
    }

    if (msg == "toggleTauleta") {
        toggleTauleta();
    } else if (msg == "+briTauleta") {
        briPlusTauleta();
    } else if (msg == "-briTauleta") {
        briMinusTauleta();
    } else if (msg == "presetTauleta") {
        presetTauleta();
    } else if (msg == "togglePrestatge") {
        togglePrestatge();
    } else if (msg == "+briPrestatge") {
        briPlusPrestatge();
    } else if (msg == "-briPrestatge") {
        briMinusPrestatge();
    } else if (msg == "presetPrestatge") {
        presetPrestatge();
    } else if (msg == "toggleGeneral") {
        togglePrestatge();
    } else if (msg == "+briGeneral") {
        briPlusPrestatge();
    } else if (msg == "-briGeneral") {
        briMinusPrestatge();
    } else if (msg == "presetGeneral") {
        presetPrestatge();
    } else {
        return;
    }

    sendLedState();
}
