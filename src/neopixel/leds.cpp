#include "leds.h"
#include "espnow/espnow.h"
#include <esp_now.h>

#define NUM_STRIPS 2
#define NUM_LEDS 42
#define LED_PINS {19, 18}

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
    {Adafruit_NeoPixel(NUM_LEDS, 19, NEO_GRBW + NEO_KHZ800), 255, 255, 1},
    {Adafruit_NeoPixel(100, 18, NEO_GRB + NEO_KHZ800), 255, 255, 1}
};

void setupLEDs() {
    for(int i=0;i<NUM_STRIPS;i++){
        ledStrips[i].strip.begin();
        ledStrips[i].strip.show();
        ledStrips[i].brightness = 0;
        ledStrips[i].targetBrightness = 0;
        ledStrips[i].strip.setBrightness(ledStrips[i].brightness);
        ledStrips[i].preset = 2;
    }

    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ Error inicialitzant ESP-NOW");
        return;
    }

    esp_now_register_recv_cb(onDataRecv);
}

void LEDTask(void *pvParameters) {
    uint16_t hue = 0;
    while(true) {
        for(int s=0;s<NUM_STRIPS;s++){
            // Fading de brillantor
            if(ledStrips[s].brightness < ledStrips[s].targetBrightness) ledStrips[s].brightness++;
            else if(ledStrips[s].brightness > ledStrips[s].targetBrightness) ledStrips[s].brightness--;
            ledStrips[s].strip.setBrightness(ledStrips[s].brightness);

            // Prests
            switch(ledStrips[s].preset){
                case 1: // RAINBOW
                    for(int i=0;i<NUM_LEDS;i++){
                        uint32_t color = ledStrips[s].strip.ColorHSV((hue+i*65536/NUM_LEDS)%65536,255,255);
                        ledStrips[s].strip.setPixelColor(i,color);
                    }
                    break;
                case 2: // WARM
                    for(int i=0;i<NUM_LEDS;i++) ledStrips[s].strip.setPixelColor(i,ledStrips[s].strip.Color(100,0,0,255));
                    break;
                case 3: // STATIC
                    for(int i=0;i<NUM_LEDS;i++) ledStrips[s].strip.setPixelColor(i,ledStrips[s].strip.Color(255,255,255,255));
                    break;
                case 4: // COLORCYCLE
                    static uint16_t hueCycle = 0;
                    uint32_t color = ledStrips[s].strip.ColorHSV(hueCycle%65536,255,255);
                    for(int i=0;i<NUM_LEDS;i++) ledStrips[s].strip.setPixelColor(i,color);
                    hueCycle += 128;
                    break;
            }

            ledStrips[s].strip.show();
        }
        hue += 256;
        if (ledStrips[0].brightness != ledStrips[0].targetBrightness) {vTaskDelay(5/portTICK_PERIOD_MS);}
        else {vTaskDelay(5/portTICK_PERIOD_MS);}
    }
}

// Funcions enviaBrillantor i onDataRecv: pots actualitzar-les per enviar/recebre info de totes les tiras
void enviaBrillantor(int stripIndex) {
    uint8_t controladorAdress[] = {0x80, 0xF3, 0xDA, 0x65, 0x5C, 0xB8};

    if(stripIndex < 0 || stripIndex >= NUM_STRIPS) return;

    char msg[20];
    sprintf(msg, "bri%d:%d", stripIndex, ledStrips[stripIndex].targetBrightness);
    esp_now_send(controladorAdress, (uint8_t *)msg, strlen(msg)+1);

    Serial.printf("Brillantor enviada tira %d: %d\n", stripIndex, ledStrips[stripIndex].targetBrightness);
}

void toggleTauleta() {
    if(ledStrips[0].targetBrightness == 0) {
            for(int s=0; s<NUM_STRIPS; s++) {
                ledStrips[0].targetBrightness = lastBri0;
            }
        } else {
            // Si alguna està encesa, apaguem-les totes
            for(int s=0; s<NUM_STRIPS; s++) {
                lastBri0 = ledStrips[0].targetBrightness;
                ledStrips[0].targetBrightness = 0;
            }
        }
}
void togglePrestatge() {
    if(ledStrips[1].targetBrightness == 0) {
            for(int s=0; s<NUM_STRIPS; s++) {
                ledStrips[1].targetBrightness = lastBri1;
            }
        } else {
            // Si alguna està encesa, apaguem-les totes
            for(int s=0; s<NUM_STRIPS; s++) {
                lastBri1 = ledStrips[1].targetBrightness;
                ledStrips[1].targetBrightness = 0;
            }
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
