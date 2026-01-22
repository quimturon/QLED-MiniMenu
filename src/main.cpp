//llibreries de sistema
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
//llibreries de wifi i dades
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <EEPROM.h>
#include <esp_now.h>
//llibreries de pantalles
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
//fonts OLED
#include <Fonts/FreeMonoBoldOblique24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
//llibreries de llums
#include <Adafruit_NeoPixel.h>
//llibreries de inputs
#include <AiEsp32RotaryEncoder.h>
//llibreries propies
#include "ota/ota.h"
#include "wifi/wifi_manager.h"
#include "neopixel/leds.h"
#include "ntp/ntp.h"
#include "espnow/espnow.h"

// ================= MENU SETTING =================
DateTime lastUpdateOTA;

// --- OLED --- 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Wi-Fi ---
struct WiFiCred {
    const char* ssid;
    const char* pass;
};

//ntp
int previousTime = -1;

// --- OTA ---
String FW_VERSION;
String NEW_VERSION;
bool otaInProgress = false;
int needOTA = 0;
const char* releasesAPI  = "https://api.github.com/repos/quimturon/habitacio/releases/latest";
const char* firmwareURL = "https://github.com/quimturon/habitacio/releases/latest/download/firmware.bin";

// --- EEPROM ---
#define EEPROM_SIZE 160
#define SSID_ADDR 0       // Offset SSID
#define PASS_ADDR 64      // Offset password
#define VERSION_ADDR 128  // Offset firmware

// --- espNOW ---
uint8_t controladorAdress[] = {0x80, 0xF3, 0xDA, 0x65, 0x5C, 0xB8};

// --- ledStrips ---
uint8_t bri0;
uint8_t bri1;
uint8_t targetBri0;
uint8_t targetBri1;
uint8_t lastBri0;
uint8_t lastBri1;
int minBri = 5;
int maxBri = 255;
uint8_t briSteps = 50;
String nominalPreset[] = {"","","",""};

String callPreset(int stripIndex, int presetIndex) {
    if (presetIndex == 1) {
        nominalPreset[stripIndex] = "Rainbow";
        return "Rainbow";
    }else if (presetIndex == 2) {
        nominalPreset[stripIndex] = "Calid";
        return "Calid";
    }else if (presetIndex == 3) {
        nominalPreset[stripIndex] = "Blanc";
        return "Blanc";
    }else if (presetIndex == 4) {
        nominalPreset[stripIndex] = "Colorit";
        return "Colorit";
    }else{
        nominalPreset[stripIndex] = "Off";
        return "Off";
    }
}

// ================= PIN DEFINITIONS =================
#define ENC1_A 34
#define ENC1_B 35

#define ENC2_A 36
#define ENC2_B 39

#define ENC3_A 16
#define ENC3_B 17
#define ENC4_A 25
#define ENC4_B 26

#define ENCODER_STEPS 4

#define BUTTON1 33
#define BUTTON2 32
#define BUTTON3 13
#define BUTTON4 12
#define ENC1_BTN 14
#define ENC2_BTN 15
#define ENC3_BTN 2
#define ENC4_BTN 5

bool buttonState[] = {0,0,0,0,0,0,0,0,};
bool buttonState1 = 0;
bool buttonState2 = 0;
bool buttonState3 = 0;
bool buttonState4 = 0;
bool buttonState5 = 0;
bool buttonState6 = 0;
bool buttonState7 = 0;
bool buttonState8 = 0;

bool lastButtonState1 = HIGH;
bool lastButtonState2 = HIGH;
bool lastButtonState3 = HIGH;
bool lastButtonState4 = HIGH;
bool lastButtonState5 = HIGH;
bool lastButtonState6 = HIGH;
bool lastButtonState7 = HIGH;
bool lastButtonState8 = HIGH;

// ================= ROTARY ENCODERS =================
AiEsp32RotaryEncoder enc1(ENC1_A, ENC1_B, ENC1_BTN, -1, ENCODER_STEPS);
AiEsp32RotaryEncoder enc2(ENC2_A, ENC2_B, ENC2_BTN, -1, ENCODER_STEPS);
AiEsp32RotaryEncoder enc3(ENC3_A, ENC3_B, ENC3_BTN, -1, ENCODER_STEPS);
AiEsp32RotaryEncoder enc4(ENC4_A, ENC4_B, ENC4_BTN, -1, ENCODER_STEPS);

long encVal[5] = {0};

// ================= FUNCTION PROTOTYPES =================
void updateOLED();
void debugPrint(const String &msg);

// ================= ISR =================
void IRAM_ATTR readEncoder0() { enc1.readEncoder_ISR(); }
void IRAM_ATTR readEncoder1() { enc2.readEncoder_ISR(); }
void IRAM_ATTR readEncoder2() { enc3.readEncoder_ISR(); }
void IRAM_ATTR readEncoder3() { enc4.readEncoder_ISR(); }


void updateOLED(char* timebuf) {
  display.clearDisplay();
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE); display.setFont(&FreeSans18pt7b);
  display.setCursor(16, display.height()/2+14); display.print(timebuf);
  display.setCursor(SCREEN_WIDTH-6*6, SCREEN_HEIGHT-8);
  display.display();
  display.setFont();
}

void debugPrint(const String &msg){
  Serial.println(msg);
  display.setCursor(0, SCREEN_HEIGHT-8);
  display.fillRect(0, SCREEN_HEIGHT-8, SCREEN_WIDTH,8,SSD1306_BLACK);
  display.print(msg);
  display.display();
}

// --- Setup ---
void setup() {
    Serial.begin(115200);
    Serial.println("Iniciant ESP32...");

    FW_VERSION = readVersion();
    Serial.print("Versió llegida EEPROM: "); 
    Serial.println(FW_VERSION);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println("Iniciant ESP32...");
    display.setCursor(0,20);
    display.print("V");
    display.println(FW_VERSION);
    display.display();
    if (!setup_wifi()) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("ERROR WIFI");
      display.display();
      delay(5000);
      ESP.restart();  // o deixa'l offline si prefereixes
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.dim(true);
    display.display();
    delay(100);
    display.dim(false);
    display.display();
    display.println("WIFI OK");
    display.display();

    // Setup LEDs i ESP-NOW
    setupLEDs();
    targetBri0=0;
    targetBri1=0;
    bri0=0;
    bri1=0;

    xTaskCreatePinnedToCore(
        LEDTask,
        "LED Task",
        4000,
        NULL,
        1,
        NULL,
        0
    );

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, controladorAdress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(controladorAdress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("❌ Error afegint el peer");
        return;
    }
  }

    // OLED
    if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay(); display.display();
    }

    // Configura pins A/B dels encoders 3 i 4
    pinMode(ENC3_A, INPUT_PULLUP); pinMode(ENC3_B, INPUT_PULLUP);
    pinMode(ENC4_A, INPUT_PULLUP); pinMode(ENC4_B, INPUT_PULLUP);

    // Encoders
    enc1.begin(); enc1.setup(readEncoder0); enc1.setAcceleration(0);
    enc2.begin(); enc2.setup(readEncoder1); enc2.setAcceleration(0);
    enc3.begin(); enc3.setup(readEncoder2); enc3.setAcceleration(0);
    enc4.begin(); enc4.setup(readEncoder3); enc4.setAcceleration(0);

    // Botons extra
    pinMode(BUTTON1, INPUT_PULLUP);
    pinMode(BUTTON2, INPUT_PULLUP);
    pinMode(BUTTON3, INPUT_PULLUP);
    pinMode(BUTTON4, INPUT_PULLUP);

    pinMode(ENC1_BTN, INPUT_PULLUP);
    pinMode(ENC2_BTN, INPUT_PULLUP);
    pinMode(ENC3_BTN, INPUT_PULLUP);
    pinMode(ENC4_BTN, INPUT_PULLUP);


}

// --- Loop ---
void loop() {
    ensureWiFi();

    struct tm timeinfo;

    // --- Encoders ---
    bool encoderMoved = false;

    if(enc1.encoderChanged()) { // controla la tira 1
        encVal[0] = enc1.readEncoder();
        int delta = enc1.readEncoder();
        if(delta>0) ledStrips[0].targetBrightness = min(ledStrips[0].targetBrightness+briSteps,maxBri);
        else if(delta<0) ledStrips[0].targetBrightness = max(ledStrips[0].targetBrightness-briSteps,minBri);
        enviaBrillantor(0);
        enc1.reset();
    }

    if(enc2.encoderChanged()) { // controla la tira 1
        encVal[1] = enc2.readEncoder();
        int delta = enc2.readEncoder();
        if(delta>0) ledStrips[1].targetBrightness = min(ledStrips[1].targetBrightness+briSteps,maxBri);
        else if(delta<0) ledStrips[1].targetBrightness = max(ledStrips[1].targetBrightness-briSteps,minBri);
        enviaBrillantor(1);
        enc2.reset();
    }


    if (enc3.encoderChanged()) { encVal[2] = enc3.readEncoder(); encoderMoved = true; }
    if (enc4.encoderChanged()) { encVal[3] = enc4.readEncoder(); encoderMoved = true; }

    // --- Botons ---
    buttonState1 = digitalRead(BUTTON1);
    buttonState2 = digitalRead(BUTTON2);
    buttonState3 = digitalRead(BUTTON3);
    buttonState4 = digitalRead(BUTTON4);
    buttonState6 = digitalRead(ENC1_BTN);
    buttonState7 = digitalRead(ENC2_BTN);
    buttonState8 = digitalRead(ENC3_BTN);


    // Detectar canvi (només quan es prem)
    // Menu 1 = Firmware Update
    // Menu 2 = lights
    // Menu 3 = rtc
    if (lastButtonState1 == HIGH && buttonState1 == LOW) {
        
    }
    if (lastButtonState2 == HIGH && buttonState2 == LOW) {
        
    }
    if (lastButtonState3 == HIGH && buttonState3 == LOW) {
        
    }
    if (lastButtonState4 == HIGH && buttonState4 == LOW) {
        
    }
    if (lastButtonState5 == HIGH && buttonState5 == LOW) {
        
    }
    if (lastButtonState6 == HIGH && buttonState6 == LOW) {
        
    }
    if (lastButtonState7 == HIGH && buttonState7 == LOW) {
        
    }
    if (lastButtonState8 == HIGH && buttonState8 == LOW) {
        
    }

    // Guardar estat anterior
    lastButtonState1 = buttonState1;
    lastButtonState2 = buttonState2;
    lastButtonState3 = buttonState3;
    lastButtonState4 = buttonState4;
    lastButtonState5 = buttonState5;
    lastButtonState6 = buttonState6;
    lastButtonState7 = buttonState7;
    lastButtonState8 = buttonState8;

}
