#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

// ================= CONFIG =================
#define BUZZER_PIN 23

// ================= FUNCIONS =================
void setupBuzzer();
void beep(int freq, int duration);
void startupBeep();

#endif