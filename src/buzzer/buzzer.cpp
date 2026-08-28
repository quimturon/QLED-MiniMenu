#include "buzzer.h"

// ================= SETUP =================
void setupBuzzer() {
    ledcAttach(BUZZER_PIN, 1000, 8);
    stopBuzzerTone();
}

void startBuzzerTone(int freq) {
    ledcWriteTone(BUZZER_PIN, freq);
}

void stopBuzzerTone() {
    ledcWriteTone(BUZZER_PIN, 0);
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