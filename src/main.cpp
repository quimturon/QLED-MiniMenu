//llibreries de sistema
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
//llibreries de wifi i dades
#include "esp_wifi.h"
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
#include "buzzer/buzzer.h"

// ================= MENU SETTING =================
DateTime lastUpdateOTA;

// --- OLED --- 
String debugMsg = "";
String debugMsg2 = "";
bool alarmRinging = false;
unsigned long lastAlarmBeep = 0;
bool buzzerState = false;
uint8_t alarmSavedBri0 = 0;
uint8_t alarmSavedBri1 = 0;
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
const char* firmwareURL = "https://github.com/quimturon/QLED-MiniMenu/releases/latest/download/firmware.bin";

// --- EEPROM ---
#define EEPROM_SIZE 160
#define SSID_ADDR 0       // Offset SSID
#define PASS_ADDR 64      // Offset password
#define VERSION_ADDR 128  // Offset firmware

// --- espNOW ---
uint8_t controladorAdress[] = {0x30, 0x76, 0xF5, 0xA5, 0x9B, 0x84};

// --- ledStrips ---
uint8_t bri0;
uint8_t bri1;
uint8_t bri2;
uint8_t bri3;
uint8_t targetBri0;
uint8_t targetBri1;
uint8_t targetBri2;

uint8_t targetBri3;
uint8_t lastBri0;
uint8_t lastBri1;
uint8_t lastBri2;
uint8_t lastBri3;
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
#define ENC1_BTN 15
#define ENC2_BTN 14
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

#define BUZZER_PIN 23

// ================= ROTARY ENCODERS =================
AiEsp32RotaryEncoder enc1(ENC1_A, ENC1_B, ENC1_BTN, -1, ENCODER_STEPS);
AiEsp32RotaryEncoder enc2(ENC2_A, ENC2_B, ENC2_BTN, -1, ENCODER_STEPS);
AiEsp32RotaryEncoder enc3(ENC3_A, ENC3_B, ENC3_BTN, -1, ENCODER_STEPS);
AiEsp32RotaryEncoder enc4(ENC4_A, ENC4_B, ENC4_BTN, -1, ENCODER_STEPS);

long encVal[5] = {0};


// ================= ISR =================
void IRAM_ATTR readEncoder0() { enc1.readEncoder_ISR(); }
void IRAM_ATTR readEncoder1() { enc2.readEncoder_ISR(); }
void IRAM_ATTR readEncoder2() { enc3.readEncoder_ISR(); }
void IRAM_ATTR readEncoder3() { enc4.readEncoder_ISR(); }


void updateOLED(String buf="") {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(34, 8);
    display.setFont();
    display.println(receivedTime);
    display.display();
}

void debugPrint(const String &msg){
  Serial.println(msg);
  display.setCursor(0, SCREEN_HEIGHT-8);
  display.fillRect(0, SCREEN_HEIGHT-8, SCREEN_WIDTH,8,SSD1306_BLACK);
  display.print(msg);
  display.display();
}

void startAlarm() {
    alarmSavedBri0 = targetBri0;
    alarmSavedBri1 = targetBri1;
    alarmRinging = true;
    lastAlarmBeep = 0;
    buzzerState = false;
    startBuzzerTone(1000);
    targetBri0 = 255;
    targetBri1 = 255;
}

void stopAlarm() {
    alarmRinging = false;
    buzzerState = false;
    stopBuzzerTone();
    targetBri0 = alarmSavedBri0;
    targetBri1 = alarmSavedBri1;
        for (int attempt = 0; attempt < 3; attempt++) {
            sendMessage(controladorAdress, "ALARM_OFF");
        }
}

void updateAlarmBuzzer() {
    if (alarmRinging && !buzzerState) {
        buzzerState = true;
        startBuzzerTone(1000);
    }
}

// --- Setup ---
void setup() {
    
    Serial.begin(115200);
    Serial.println("Iniciant ESP32...");

    FW_VERSION = readVersion();
    Serial.print("Versió llegida EEPROM: "); 
    Serial.println(FW_VERSION);

    // OLED
    if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay(); display.display();
    }
    Serial.println("Pantalles inicialitzades");
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println("Iniciant ESP32...");
    display.setCursor(0,20);
    display.print("V");
    display.println(FW_VERSION);
    display.display();
    setupBuzzer();
    startupBeep();

    setup_wifi();
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ Error inicialitzant ESP-NOW");
        display.println("ESP-NOW ERROR");
        display.display();
        while (true) delay(100);
    }
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, controladorAdress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_now_del_peer(controladorAdress);  // elimina si existeix
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("❌ Error afegint peer");

    } else {
        Serial.println("✅ Peer afegit correctament");
    }
    esp_now_register_recv_cb(onDataRecv);
    Serial.println("✅ ESP-NOW inicialitzat");
    Serial.print("Canal: ");
    Serial.println(WiFi.channel());

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
    Serial.println("Tasca LED creada");

    
    
    // Botons extra
    pinMode(BUTTON1, INPUT_PULLUP);
    pinMode(BUTTON2, INPUT_PULLUP);
    pinMode(BUTTON3, INPUT_PULLUP);
    pinMode(BUTTON4, INPUT_PULLUP);
    
    pinMode(ENC1_BTN, INPUT_PULLUP);
    pinMode(ENC2_BTN, INPUT_PULLUP);
    pinMode(ENC3_BTN, INPUT_PULLUP);
    pinMode(ENC4_BTN, INPUT_PULLUP);
    
    // Encoders
    pinMode(ENC1_A, INPUT);
    pinMode(ENC1_B, INPUT);

    pinMode(ENC2_A, INPUT);
    pinMode(ENC2_B, INPUT);

    pinMode(ENC3_A, INPUT_PULLUP);
    pinMode(ENC3_B, INPUT_PULLUP);

    pinMode(ENC4_A, INPUT_PULLUP);
    pinMode(ENC4_B, INPUT_PULLUP);

    enc1.begin();
    enc1.setup(readEncoder0);
    enc1.setAcceleration(0);

    enc2.begin();
    enc2.setup(readEncoder1);
    enc2.setAcceleration(0);

    enc3.begin();
    enc3.setup(readEncoder2);
    enc3.setAcceleration(0);

    enc4.begin();
    enc4.setup(readEncoder3);
    enc4.setAcceleration(0);
}

// --- Loop ---
void loop() {
    struct tm timeinfo;

    ensureWiFi();
    updateAlarmBuzzer();

    if (alarmRinging) {
        bool alarmButtonPressed =
            digitalRead(BUTTON1) == LOW || digitalRead(BUTTON2) == LOW ||
            digitalRead(BUTTON3) == LOW || digitalRead(BUTTON4) == LOW ||
            digitalRead(ENC1_BTN) == LOW || digitalRead(ENC2_BTN) == LOW ||
            digitalRead(ENC3_BTN) == LOW || digitalRead(ENC4_BTN) == LOW ||
            enc1.encoderChanged() || enc2.encoderChanged() ||
            enc3.encoderChanged() || enc4.encoderChanged();

        if (alarmButtonPressed) {
            enc1.reset();
            enc2.reset();
            enc3.reset();
            enc4.reset();
            stopAlarm();
        }

        return;
    }

    // --- Encoders ---
    bool encoderMoved = false;

    if(enc1.encoderChanged()) { // controla la tira 1
        encVal[0] = enc1.readEncoder();
        int delta = enc1.readEncoder();
        if(delta<0){
            debugMsg = "Enviant +briParet...";
            esp_now_send(controladorAdress, (uint8_t*)"+briParet", strlen("+briParet")+1);
            Serial.println("Enviat +briParet");
        }
        else if(delta>0) {
            debugMsg = "Enviant -briParet...";
            esp_now_send(controladorAdress, (uint8_t*)"-briParet", strlen("-briParet")+1);
            Serial.println("Enviat -briParet");
        }
        enc1.reset();
    }

    if(enc2.encoderChanged()) { // controla la tira 1
        encVal[1] = enc2.readEncoder();
        int delta = enc2.readEncoder();
        if(delta<0){
            debugMsg = "Enviant -briPrestatge...";
            esp_now_send(controladorAdress, (uint8_t*)"-briPrestatge", strlen("-briPrestatge")+1);
            Serial.println("Enviat -briPrestatge");
        }
        else if(delta>0) {
            debugMsg = "Enviant +briDespatx...";
            esp_now_send(controladorAdress, (uint8_t*)"+briPrestatge", strlen("+briPrestatge")+1);
            Serial.println("Enviat +briPrestatge");
        }
        enc2.reset();
    }


    if (enc3.encoderChanged()) { 
        encVal[2] = enc3.readEncoder();
        int delta = enc3.readEncoder();
        if(delta<0){
            briPlusTauleta();
        }
        else if(delta>0) {
            briMinusTauleta();
        }
        enc3.reset();
        sendLedState();
    }
    if (enc4.encoderChanged()) { 
        encVal[3] = enc4.readEncoder();
        int delta = enc4.readEncoder();
        if(delta<0){
            briPlusPrestatge();
        }
        else if(delta>0) {
            briMinusPrestatge();
        }
        enc4.reset();
        sendLedState();
    }

    // --- Botons ---
    buttonState1 = digitalRead(BUTTON1);
    buttonState2 = digitalRead(BUTTON2);
    buttonState3 = digitalRead(BUTTON3);
    buttonState4 = digitalRead(BUTTON4);
    buttonState5 = digitalRead(ENC1_BTN);
    buttonState6 = digitalRead(ENC2_BTN);
    buttonState7 = digitalRead(ENC3_BTN);
    buttonState8 = digitalRead(ENC4_BTN);

    static unsigned long allButtonsPressedAt = 0;
    static bool otaTriggered = false;
    bool allButtonsPressed = buttonState1 == LOW && buttonState2 == LOW &&
                              buttonState3 == LOW && buttonState4 == LOW;

    if (allButtonsPressed) {
        if (allButtonsPressedAt == 0) {
            allButtonsPressedAt = millis();
        } else if (!otaTriggered && millis() - allButtonsPressedAt >= 3000UL) {
            otaTriggered = true;
            String newVersion;
            display.clearDisplay();
            display.setTextSize(1);
            display.setCursor(0, 0);
            display.println("Comprovant actualitzacio...");
            display.display();
            Serial.println("Els quatre botons estan premuts 3 segons: comprovant OTA...");
            if (checkForUpdate(newVersion)) {
                performOTA(newVersion);
            } else {
                Serial.println("No hi ha cap actualitzacio OTA disponible.");
            }
        }
    } else {
        allButtonsPressedAt = 0;
        otaTriggered = false;
    }

    // Detectar canvi (només quan es prem)
    // Menu 1 = Firmware Update
    // Menu 2 = lights
    // Menu 3 = rtc
    if (lastButtonState1 == HIGH && buttonState1 == LOW) {
        debugMsg = "Enviant toggleParet...";
        esp_now_send(controladorAdress, (uint8_t*)"toggleParet", strlen("toggleParet")+1);
        Serial.println("Enviat toggleParet");
    }
    if (lastButtonState2 == HIGH && buttonState2 == LOW) {
        debugMsg = "Enviant togglePrestatge...";
        esp_now_send(controladorAdress, (uint8_t*)"togglePrestatge", strlen("togglePrestatge")+1);
        Serial.println("Enviat togglePrestatge");
    }
    if (lastButtonState3 == HIGH && buttonState3 == LOW) {
        toggleTauleta();
        sendLedState();
    }
    if (lastButtonState4 == HIGH && buttonState4 == LOW) {
        togglePrestatge();
        sendLedState();
    }
    if (lastButtonState5 == HIGH && buttonState5 == LOW) {
        debugMsg = "Enviant presetParet...";
        esp_now_send(controladorAdress, (uint8_t*)"presetParet", strlen("presetParet")+1);
        Serial.println("Enviat presetParet");
    }
    if (lastButtonState6 == HIGH && buttonState6 == LOW) {
        debugMsg = "Enviant presetPrestatge...";
        esp_now_send(controladorAdress, (uint8_t*)"presetPrestatge", strlen("presetPrestatge")+1);
        Serial.println("Enviat presetPrestatge");
    }
    if (lastButtonState7 == HIGH && buttonState7 == LOW) {
        presetTauleta();
        sendLedState();
    }
    if (lastButtonState8 == HIGH && buttonState8 == LOW) {
        presetPrestatge();
        sendLedState();
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

    updateOLED();
}
