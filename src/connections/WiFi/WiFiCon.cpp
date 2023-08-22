#include "WiFiCon.h"

#include <WiFi.h>
const char* ssid = "mqtt";
const char* password = "liviaeuia";

void conectaWiFi() {
  IPAddress ip;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
  }
  ip = WiFi.localIP();
  Serial.println(F("WiFi connected"));
  Serial.println("");
  Serial.println(ip);
  Serial.print("Stream Link: http://");
  Serial.println(ip);
}