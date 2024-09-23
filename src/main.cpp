#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Preferences.h>

#define PIN_LED 8
#define PIN_BUTTON 9

Preferences preferences;
DNSServer dns;
AsyncWebServer server(80);

String ssid;
String password;
String macAddress;
uint8_t macArray[6];

bool enableDNS = true;

bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
uint32_t pressedTime = 0;

const char *TAG = "MJOLNIR";

bool convertMacStringToArray() {
    if (macAddress.length() == 17) {
        const char *macStr = macAddress.c_str();
        for (unsigned char &i: macArray) {
            sscanf(macStr, "%02x", &i);
            macStr += 3;
        }
        ESP_LOGD(TAG, "Conversions successfully, MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                 macArray[0], macArray[1],
                 macArray[2], macArray[3],
                 macArray[4], macArray[5]);
        return true;
    } else {
        ESP_LOGD(TAG, "Invalid MAC address");
        return false;
    }
}

bool createAP() {
    if (!convertMacStringToArray()) {
        digitalWrite(PIN_LED, HIGH);
        return false;
    }

    WiFi.enableAP(true);
    esp_err_t set_mac_err = esp_wifi_set_mac(WIFI_IF_AP, macArray);
    ESP_LOGD(TAG, "esp_wifi_set_mac: %s", esp_err_to_name(set_mac_err));
    if (set_mac_err == ESP_OK) {
        server.end();
        WiFi.softAPdisconnect();
        if (WiFi.softAP(ssid, password)) {
            esp_err_t set_power_err = esp_wifi_set_max_tx_power(34);
            ESP_LOGD(TAG, "esp_wifi_set_max_tx_power: %s", esp_err_to_name(set_power_err));
            digitalWrite(PIN_LED, LOW);
            ESP_LOGD(TAG, "AP created successfully");
            ESP_LOGD(TAG, "#     SSID: %s", ssid.c_str());
            ESP_LOGD(TAG, "# Password: %s", password.c_str());
            ESP_LOGD(TAG, "#      MAC: %s", macAddress.c_str());
            server.begin();
            enableDNS = false;
            return true;
        } else {
            digitalWrite(PIN_LED, HIGH);
            return false;
        }
    } else {
        digitalWrite(PIN_LED, HIGH);
        ESP_LOGD(TAG, "Failed to set MAC address");
        return false;
    }
}

void initWebServer() {
    if (!LittleFS.begin()) {
        ESP_LOGD(TAG, "Failed to initialize LittleFS");
        return;
    }

    server.serveStatic("/", LittleFS, "/");

    server.on("/", HTTP_GET,
              [](AsyncWebServerRequest *request) { request->send(LittleFS, "/index.html", "text/html"); });

    server.onNotFound([](AsyncWebServerRequest *request) { request->redirect("/"); });

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
        String _ssid_str;
        String _password_str;
        String _mac_str;

        for (int i = 0; i < request->params(); i++) {
            AsyncWebParameter *p = request->getParam(i);

            if (p->name() == "wifi_ssid") {
                _ssid_str = p->value();
            }

            if (p->name() == "wiif_password") {
                _password_str = p->value();
            }

            if (p->name() == "wifi_mac") {
                _mac_str = p->value();
            }
        }

        if (_ssid_str.length() < 1 || _ssid_str.length() > 63) {
            ESP_LOGD(TAG, "Request send error");
            request->send(200, "text/plain", "SSID format error");
        } else if (_password_str.length() < 8 && _password_str.length() != 0) {
            ESP_LOGD(TAG, "Request send error");
            request->send(200, "text/plain", "Password format error");
        } else if (_mac_str.length() != 17) {
            ESP_LOGD(TAG, "Request send error");
            request->send(200, "text/plain", "MAC length error");
        } else {
            ssid = _ssid_str;
            password = _password_str;
            macAddress = _mac_str;
            if (!createAP()) {
                ESP_LOGD(TAG, "Request send result");
                request->send(200, "text/plain", "创建热点失败，请检查MAC地址是否正确");
            } else {
                preferences.begin("wifi_config", false);
                preferences.putString("ssid", ssid);
                preferences.putString("password", password);
                preferences.putString("mac", macAddress);
                preferences.end();
                // server.end();
                // server.begin();
            }
        }
    });
    server.begin();
}

void setup() {
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    digitalWrite(PIN_LED, HIGH);

    if (!preferences.begin("wifi_config", false)) {
        ESP_LOGD(TAG, "Failed to initialize preferences");
    }

    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    macAddress = preferences.getString("mac", "");
    preferences.end();

    WiFi.mode(WIFI_AP);

    if (ssid.length() > 0 && macAddress.length() == 17) {
        if (!createAP()) {
            ESP_LOGD(TAG, "WiFi SSID or MAC error. Creating default AP");
            WiFi.softAP("WIFI MANAGER");
        }
    } else {
        ESP_LOGD(TAG, "No saved WiFi credentials. Creating default AP");
        WiFi.softAP("WIFI MANAGER");
    }

    initWebServer();
    dns.start(53, "*", WiFi.softAPIP());
}

void loop() {
    currentButtonState = digitalRead(PIN_BUTTON);

    if (lastButtonState == HIGH && currentButtonState == LOW) {
        pressedTime = millis();
    } else if (lastButtonState == LOW && currentButtonState == HIGH) {
        if (millis() - pressedTime > 1000) {
            ESP_LOGD(TAG, "Button long pressed, Clean WiFi credentials");
            preferences.begin("wifi_config", false);
            preferences.clear();
            preferences.end();
            delay(1000);
            ESP.restart();
        }
    }
    lastButtonState = currentButtonState;

    if (enableDNS) {
        dns.processNextRequest();
    }
    delay(50);
}