#include "buzzer.h"

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#define BUZZER_ARDUINO_V3 1
#else
#define BUZZER_ARDUINO_V3 0
#endif

static const uint8_t BUZZER_CHANNEL = 7;
static const uint8_t BUZZER_RESOLUTION = 8;

// ================= SETUP =================
void setupBuzzer() {
#if BUZZER_ARDUINO_V3
    ledcAttach(BUZZER_PIN, 1000, 8);
#else
    ledcSetup(BUZZER_CHANNEL, 1000, BUZZER_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
#endif
    stopBuzzerTone();
}

void startBuzzerTone(int freq) {
#if BUZZER_ARDUINO_V3
    ledcWriteTone(BUZZER_PIN, freq);
#else
    ledcWriteTone(BUZZER_CHANNEL, freq);
#endif
}

void stopBuzzerTone() {
#if BUZZER_ARDUINO_V3
    ledcWriteTone(BUZZER_PIN, 0);
#else
    ledcWriteTone(BUZZER_CHANNEL, 0);
#endif
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