#include "ota.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <EEPROM.h>
#include <Adafruit_SSD1306.h>

// ================= CONFIG =================
#define EEPROM_SIZE 160
#define VERSION_ADDR 128

static const char* versionURL =
    "https://github.com/quimturon/QLED-MiniMenu/releases/latest/download/version.txt";

// === Globals definits al main.cpp ===
extern Adafruit_SSD1306 display;
extern String FW_VERSION;

// ================= EEPROM =================
void saveVersion(const String& version) {
    EEPROM.begin(EEPROM_SIZE);
    int i = 0;
    for (; version[i] != '\0' && i < 31; i++)
        EEPROM.write(VERSION_ADDR + i, version[i]);
    EEPROM.write(VERSION_ADDR + i, '\0');
    EEPROM.commit();
}

String readVersion() {
    EEPROM.begin(EEPROM_SIZE);
    char buffer[32];
    int i = 0;
    while (EEPROM.read(VERSION_ADDR + i) != '\0' && i < 31) {
        buffer[i] = EEPROM.read(VERSION_ADDR + i);
        i++;
    }
    buffer[i] = '\0';
    return String(buffer);
}

// ================= VERSION COMPARE =================
static bool isVersionNewer(const String& current, const String& latest) {
    int majorC=0, minorC=0, patchC=0;
    int majorL=0, minorL=0, patchL=0;

    String currentVersion = current;
    String latestVersion = latest;
    currentVersion.trim();
    latestVersion.trim();
    if (currentVersion.startsWith("v") || currentVersion.startsWith("V")) {
        currentVersion.remove(0, 1);
    }
    if (latestVersion.startsWith("v") || latestVersion.startsWith("V")) {
        latestVersion.remove(0, 1);
    }

    if (sscanf(currentVersion.c_str(), "%d.%d.%d", &majorC, &minorC, &patchC) != 3 ||
        sscanf(latestVersion.c_str(), "%d.%d.%d", &majorL, &minorL, &patchL) != 3) {
        Serial.printf("OTA: versio invalida, actual='%s', remota='%s'\n",
                      current.c_str(), latest.c_str());
        return false;
    }

    if (majorL > majorC) return true;
    if (majorL < majorC) return false;
    if (minorL > minorC) return true;
    if (minorL < minorC) return false;
    return patchL > patchC;
}


// ================= CHECK UPDATE =================
bool checkForUpdate(String &newVersion) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("OTA: WiFi no connectat");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    IPAddress serverIP;
    if (WiFi.hostByName("github.com", serverIP) != 1) {
        Serial.println("OTA: no es resol github.com (DNS)");
        return false;
    }
    Serial.print("OTA: github.com = ");
    Serial.println(serverIP);

    client.setTimeout(10000);

    for (int attempt = 1; attempt <= 2; attempt++) {
        HTTPClient http;
        if (!http.begin(client, versionURL)) {
            Serial.printf("OTA: no es pot obrir la connexio (intent %d)\n", attempt);
            continue;
        }

        http.addHeader("User-Agent", "ESP32");
        http.useHTTP10(true);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setTimeout(10000);
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            newVersion = http.getString();
            http.end();
            break;
        }

        Serial.printf("OTA: error versio HTTP %d (intent %d)\n", httpCode, attempt);
        http.end();
        newVersion = "";
    }

    if (newVersion.isEmpty()) {
        return false;
    }

    newVersion.trim();
    if (newVersion.startsWith("v") || newVersion.startsWith("V")) {
        newVersion.remove(0, 1);
    }

    Serial.printf("OTA: versio actual='%s', versio remota='%s'\n",
                  FW_VERSION.c_str(), newVersion.c_str());

    return isVersionNewer(FW_VERSION, newVersion);
}

// ================= OTA =================
void performOTA(const String &newVersion) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("Inici OTA...");
    display.display();
    
    FW_VERSION = newVersion;

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client,
            "https://github.com/quimturon/QLED-MiniMenu/releases/latest/download/firmware.bin");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (http.GET() != HTTP_CODE_OK) {
        http.end();
        return;
    }

    int total = http.getSize();
    WiFiClient *stream = http.getStreamPtr();

    if (!Update.begin(total)) {
        http.end();
        return;
    }

    uint8_t buffer[256];
    int written = 0;

    display.drawRect(0, 20, 128, 10, SSD1306_WHITE);
    display.display();

    while (http.connected() && written < total) {
        size_t available = stream->available();
        if (available) {
            int r = stream->readBytes(buffer, min((int)available, 256));
            Update.write(buffer, r);
            written += r;
        }

        display.fillRect(0, 20, 128, 30, SSD1306_BLACK);
        display.drawRect(0, 20, 128, 10, SSD1306_WHITE);

        int w = map(written, 0, total, 0, 128);
        display.fillRect(0, 20, w, 10, SSD1306_WHITE);

        display.setCursor(0, 35);
        display.printf("%d %%", (written * 100) / total);
        display.display();
        delay(10);
    }

    if (Update.end()) {
        saveVersion(newVersion);
        delay(2000);
        ESP.restart();
    }

    http.end();
}
