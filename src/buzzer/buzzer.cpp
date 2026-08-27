#include "buzzer.h"

// ================= SETUP =================
void setupBuzzer() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

// ================= BEEP =================
void beep(int freq, int duration) {

    int halfPeriod = 1000000 / freq / 2;
    int cycles = (freq * duration) / 1000;

    for (int i = 0; i < cycles; i++) {

        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(halfPeriod);

        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(halfPeriod);
    }
}

// ================= STARTUP SOUND =================
void startupBeep() {
    beep(1000, 80);
    delay(30);
}