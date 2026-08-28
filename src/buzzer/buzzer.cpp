#include "buzzer.h"

static const uint8_t BUZZER_CHANNEL = 7;
static const uint8_t BUZZER_RESOLUTION = 8;

// ================= SETUP =================
void setupBuzzer() {
    ledcSetup(BUZZER_CHANNEL, 1000, BUZZER_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    stopBuzzerTone();
}

void startBuzzerTone(int freq) {
    ledcSetup(BUZZER_CHANNEL, freq, BUZZER_RESOLUTION);
    ledcWrite(BUZZER_CHANNEL, 128);
}

void stopBuzzerTone() {
    ledcWrite(BUZZER_CHANNEL, 0);
}

// ================= BEEP =================
void beep(int freq, int duration) {
    startBuzzerTone(freq);
    delay(duration);
    stopBuzzerTone();
}

// ================= STARTUP SOUND =================
void startupBeep() {
    beep(1000, 80);
    delay(30);
}