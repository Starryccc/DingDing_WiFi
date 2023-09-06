#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Preferences.h>

Preferences preferences;
DNSServer dns;
AsyncWebServer server(80);

String ssid;
String password;
String mac;
uint8_t mac_arr[6];

void parserMAC(String mac_string, uint8_t mac_arr[6])
{
  Serial.println("parserMAC");
  const char * _mac = mac_string.c_str();
  
  for (int i = 0; i < 6; i++)
  {
    sscanf(_mac, "%02x", &mac_arr[i]);
    _mac += 3;
  }
}

void openAP()
{
  Serial.println("Open AP");

  parserMAC(mac, mac_arr);

  WiFi.mode(WIFI_AP);
  WiFi.enableAP(true);
  
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_AP, mac_arr);
  Serial.print("Change mac status: ");
  Serial.println(esp_err_to_name(err));

  if (err != ESP_OK)
  {
    Serial.println("Failed to set MAC address");
    return;
  }

  Serial.printf("#     SSID: %s\n", ssid);
  Serial.printf("# Password: %s\n", password);
  Serial.printf("#      MAC: %s\n", mac.c_str());

  if (password.length() == 0)
  {
    WiFi.softAP(ssid);
  }
  else
  {
    WiFi.softAP(ssid, password);
  }
}

void runWebServer()
{
  Serial.println("Run Web server");

  if (!LittleFS.begin())
  {
    Serial.println("Failed to initialize LittleFS");
    return;
  }

  server.serveStatic("/", LittleFS, "/");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });

  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->redirect("/"); });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              String _ssid_str;
              String _password_str;
              String _mac_str;

              for (int i = 0; i < request->params(); i++)
              {
                AsyncWebParameter *p = request->getParam(i);

                if (p->name() == "wifi_ssid")
                {
                  _ssid_str = p->value();
                }

                if (p->name() == "wiif_password")
                {
                  _password_str = p->value();
                }

                if (p->name() == "wifi_mac")
                {
                  _mac_str = p->value();
                }
              }

              if (_ssid_str.length() < 1 || _ssid_str.length() > 63)
              {
                Serial.println("Request send error");
                request->send(200, "text/plain", "SSID format error");
              }

              else if (_password_str.length() < 8 && _password_str.length() != 0)
              {
                Serial.println("Request send error");
                request->send(200, "text/plain", "Password format error");
              }

              else if (_mac_str.length() != 17)
              {
                Serial.println("Request send error");
                request->send(200, "text/plain", "MAC length error");
              }

              else
              {
                Serial.println("Request send done");
                request->send(200, "text/plain", "Done");
                ssid = _ssid_str;
                password = _password_str;
                mac = _mac_str;
                preferences.begin("wifi_config", false);
                preferences.putString("ssid", ssid);
                preferences.putString("password", password);
                preferences.putString("mac", mac);
                preferences.end();
                openAP();
              } });
  server.begin();
}

void setup()
{
  Serial.begin(115200);

  if (!preferences.begin("wifi_config", false))
  {
    Serial.println("Failed to initialize Preferences");
  }

  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  mac = preferences.getString("mac", "");

  preferences.end();

  if (ssid == "")
  {
    Serial.println("No config saved");

    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32 WIFI MANAGER");
  }
  else
  {
    openAP();
  }

  runWebServer();
  dns.start(53, "*", WiFi.softAPIP());
}

void loop()
{
  dns.processNextRequest();
}