#include "leds.h"
#include "espnow/espnow.h"
#include <esp_now.h>

#define NUM_STRIPS 2
#define LEDS_PER_STRIP 42
#define NUM_LEDS (NUM_STRIPS * LEDS_PER_STRIP)
#define STRIP1_START 0
#define STRIP1_END 41
#define STRIP2_START 42
#define STRIP2_END 83

extern uint8_t bri0;
extern uint8_t bri1;
extern uint8_t targetBri0;
extern uint8_t targetBri1;
extern uint8_t lastBri0;
extern uint8_t lastBri1;
extern int minBri;
extern int maxBri;
extern uint8_t briSteps;

LEDStrip ledStrips[NUM_STRIPS] = {
    {255, 255, 1, 50},
    {255, 255, 1, 50}
};

Adafruit_NeoPixel ledStrip(NUM_LEDS, 19, NEO_GRBW + NEO_KHZ800);

void renderStrip(int stripIndex, int firstLed, int lastLed, uint16_t hue) {
    uint32_t color;

    switch (ledStrips[stripIndex].preset) {
        case 1:
            for (int ledIndex = firstLed; ledIndex <= lastLed; ledIndex++) {
                color = ledStrip.ColorHSV(
                    (hue + (ledIndex - firstLed) * 65536 / LEDS_PER_STRIP) % 65536,
                    255,
                    ledStrips[stripIndex].brightness
                );
                strip.setPixelColor(ledIndex, color);
            }
            return;
        case 3:
            color = ledStrip.Color(
                ledStrips[stripIndex].brightness,
                ledStrips[stripIndex].brightness,
                ledStrips[stripIndex].brightness,
                ledStrips[stripIndex].brightness
            );
            break;
        case 4:
            color = ledStrip.ColorHSV(
                hue % 65536,
                255,
                ledStrips[stripIndex].brightness
            );
            break;
        case 2:
        default:
            color = ledStrip.Color(
                (uint16_t)100 * ledStrips[stripIndex].brightness / 255,
                0,
                0,
                ledStrips[stripIndex].brightness
            );
            break;
    }

    for (int ledIndex = firstLed; ledIndex <= lastLed; ledIndex++) {
        ledStrip.setPixelColor(ledIndex, color);
    }
}

void setupLEDs() {
    ledStrip.begin();
    ledStrip.setBrightness(255);
    ledStrip.clear();
    ledStrip.show();

    for(int i=0;i<NUM_STRIPS;i++){
        ledStrips[i].brightness = 0;
        ledStrips[i].targetBrightness = 0;
        ledStrips[i].preset = 2;
    }

}

void LEDTask(void *pvParameters) {
    uint16_t hue = 0;
    while(true) {
        for(int s=0;s<NUM_STRIPS;s++){
            // Fading de brillantor
            if(ledStrips[s].brightness < ledStrips[s].targetBrightness) ledStrips[s].brightness++;
            else if(ledStrips[s].brightness > ledStrips[s].targetBrightness) ledStrips[s].brightness--;
        }
        renderStrip(0, STRIP1_START, STRIP1_END, hue);
        renderStrip(1, STRIP2_START, STRIP2_END, hue);
        ledStrip.show();
        hue += 256;
        if (ledStrips[0].brightness != ledStrips[0].targetBrightness) {vTaskDelay(5/portTICK_PERIOD_MS);}
        else {vTaskDelay(5/portTICK_PERIOD_MS);}
    }
}

// Funcions enviaBrillantor i onDataRecv: pots actualitzar-les per enviar/recebre info de totes les tiras
void enviaBrillantor(int stripIndex) {
    if(stripIndex < 0 || stripIndex >= NUM_STRIPS) return;
    sendLedState();
}

void toggleTauleta() {
    if(ledStrips[0].targetBrightness == 0) {
        ledStrips[0].targetBrightness = lastBri0;
    } else {
        lastBri0 = ledStrips[0].targetBrightness;
        ledStrips[0].targetBrightness = 0;
    }
}
void togglePrestatge() {
    if(ledStrips[1].targetBrightness == 0) {
        ledStrips[1].targetBrightness = lastBri1;
    } else {
        lastBri1 = ledStrips[1].targetBrightness;
        ledStrips[1].targetBrightness = 0;
    }
}
void briPlusTauleta() {
    ledStrips[0].targetBrightness = min(ledStrips[0].targetBrightness + briSteps, maxBri);
}
void briMinusTauleta() {
    ledStrips[0].targetBrightness = max(ledStrips[0].targetBrightness - briSteps, 5);
}
void briPlusPrestatge() {
    ledStrips[1].targetBrightness = min(ledStrips[1].targetBrightness + briSteps, maxBri);
}
void briMinusPrestatge() {
    ledStrips[1].targetBrightness = max(ledStrips[1].targetBrightness - briSteps, 5);
}
void presetTauleta() {
    ledStrips[0].preset += 1;
    if(ledStrips[0].preset > 4) ledStrips[0].preset = 1;
}
void presetPrestatge() {
    ledStrips[1].preset += 1;
    if(ledStrips[1].preset > 4) ledStrips[1].preset = 1;
}
