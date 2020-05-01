#include <WiFi.h>
#include <HTTPClient.h>
#include "HTTPSRedirect.h"
#include "SPIFFS.h"
#include "DebugMacros.h"
#include <TJpg_Decoder.h>
#include <Inkplate.h>
#include <GFXLayer.h>
#include <ArduinoJson.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Wire.h>

StaticJsonDocument<1024> doc;
StaticJsonDocument<32768> doc2;

#define DPRINT(...)    Serial.print(__VA_ARGS__)
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)

#define ARDUINOJSON_DECODE_UNICODE 1
#include <ArduinoJson.h>

// Fill ssid and password with your network credentials
const char* ssid     = "???";     // your network SSID (name of wifi network)
const char* password = "???"; // your network password

const char* host = "script.google.com";
// Replace with your own script id to make server side changes
const char *GScriptId = "???";
const int httpsPort = 443;

// certificate for https://google.com
// GlobalSign, valid until Wed Dec 15 2021, size: 1549 bytes
const char* rootCACertificateGoogle = \
                                      "-----BEGIN CERTIFICATE-----\n" \
                                      "MIIESjCCAzKgAwIBAgINAeO0mqGNiqmBJWlQuDANBgkqhkiG9w0BAQsFADBMMSAw\n" \
                                      "HgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEGA1UEChMKR2xvYmFs\n" \
                                      "U2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjAeFw0xNzA2MTUwMDAwNDJaFw0yMTEy\n" \
                                      "MTUwMDAwNDJaMEIxCzAJBgNVBAYTAlVTMR4wHAYDVQQKExVHb29nbGUgVHJ1c3Qg\n" \
                                      "U2VydmljZXMxEzARBgNVBAMTCkdUUyBDQSAxTzEwggEiMA0GCSqGSIb3DQEBAQUA\n" \
                                      "A4IBDwAwggEKAoIBAQDQGM9F1IvN05zkQO9+tN1pIRvJzzyOTHW5DzEZhD2ePCnv\n" \
                                      "UA0Qk28FgICfKqC9EksC4T2fWBYk/jCfC3R3VZMdS/dN4ZKCEPZRrAzDsiKUDzRr\n" \
                                      "mBBJ5wudgzndIMYcLe/RGGFl5yODIKgjEv/SJH/UL+dEaltN11BmsK+eQmMF++Ac\n" \
                                      "xGNhr59qM/9il71I2dN8FGfcddwuaej4bXhp0LcQBbjxMcI7JP0aM3T4I+DsaxmK\n" \
                                      "FsbjzaTNC9uzpFlgOIg7rR25xoynUxv8vNmkq7zdPGHXkxWY7oG9j+JkRyBABk7X\n" \
                                      "rJfoucBZEqFJJSPk7XA0LKW0Y3z5oz2D0c1tJKwHAgMBAAGjggEzMIIBLzAOBgNV\n" \
                                      "HQ8BAf8EBAMCAYYwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMBIGA1Ud\n" \
                                      "EwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFJjR+G4Q68+b7GCfGJAboOt9Cf0rMB8G\n" \
                                      "A1UdIwQYMBaAFJviB1dnHB7AagbeWbSaLd/cGYYuMDUGCCsGAQUFBwEBBCkwJzAl\n" \
                                      "BggrBgEFBQcwAYYZaHR0cDovL29jc3AucGtpLmdvb2cvZ3NyMjAyBgNVHR8EKzAp\n" \
                                      "MCegJaAjhiFodHRwOi8vY3JsLnBraS5nb29nL2dzcjIvZ3NyMi5jcmwwPwYDVR0g\n" \
                                      "BDgwNjA0BgZngQwBAgIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly9wa2kuZ29vZy9y\n" \
                                      "ZXBvc2l0b3J5LzANBgkqhkiG9w0BAQsFAAOCAQEAGoA+Nnn78y6pRjd9XlQWNa7H\n" \
                                      "TgiZ/r3RNGkmUmYHPQq6Scti9PEajvwRT2iWTHQr02fesqOqBY2ETUwgZQ+lltoN\n" \
                                      "FvhsO9tvBCOIazpswWC9aJ9xju4tWDQH8NVU6YZZ/XteDSGU9YzJqPjY8q3MDxrz\n" \
                                      "mqepBCf5o8mw/wJ4a2G6xzUr6Fb6T8McDO22PLRL6u3M4Tzs3A2M1j6bykJYi8wW\n" \
                                      "IRdAvKLWZu/axBVbzYmqmwkm5zLSDW5nIAJbELCQCZwMH56t2Dvqofxs6BBcCFIZ\n" \
                                      "USpxu6x6td0V7SvJCCosirSmIatj/9dSSVDQibet8q/7UK4v4ZUN80atnZz1yg==\n" \
                                      "-----END CERTIFICATE-----\n" \
                                      "";
// certificate for https://redd.it
// DigiCert Global Root CA, valid until Wed Mar 08 2023, size: 1647 bytes
const char* rootCACertificateReddit = \
                                      "-----BEGIN CERTIFICATE-----\n" \
                                      "MIIElDCCA3ygAwIBAgIQAf2j627KdciIQ4tyS8+8kTANBgkqhkiG9w0BAQsFADBh\n" \
                                      "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
                                      "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
                                      "QTAeFw0xMzAzMDgxMjAwMDBaFw0yMzAzMDgxMjAwMDBaME0xCzAJBgNVBAYTAlVT\n" \
                                      "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxJzAlBgNVBAMTHkRpZ2lDZXJ0IFNIQTIg\n" \
                                      "U2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
                                      "ANyuWJBNwcQwFZA1W248ghX1LFy949v/cUP6ZCWA1O4Yok3wZtAKc24RmDYXZK83\n" \
                                      "nf36QYSvx6+M/hpzTc8zl5CilodTgyu5pnVILR1WN3vaMTIa16yrBvSqXUu3R0bd\n" \
                                      "KpPDkC55gIDvEwRqFDu1m5K+wgdlTvza/P96rtxcflUxDOg5B6TXvi/TC2rSsd9f\n" \
                                      "/ld0Uzs1gN2ujkSYs58O09rg1/RrKatEp0tYhG2SS4HD2nOLEpdIkARFdRrdNzGX\n" \
                                      "kujNVA075ME/OV4uuPNcfhCOhkEAjUVmR7ChZc6gqikJTvOX6+guqw9ypzAO+sf0\n" \
                                      "/RR3w6RbKFfCs/mC/bdFWJsCAwEAAaOCAVowggFWMBIGA1UdEwEB/wQIMAYBAf8C\n" \
                                      "AQAwDgYDVR0PAQH/BAQDAgGGMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEFBQcwAYYY\n" \
                                      "aHR0cDovL29jc3AuZGlnaWNlcnQuY29tMHsGA1UdHwR0MHIwN6A1oDOGMWh0dHA6\n" \
                                      "Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RDQS5jcmwwN6A1\n" \
                                      "oDOGMWh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RD\n" \
                                      "QS5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8v\n" \
                                      "d3d3LmRpZ2ljZXJ0LmNvbS9DUFMwHQYDVR0OBBYEFA+AYRyCMWHVLyjnjUY4tCzh\n" \
                                      "xtniMB8GA1UdIwQYMBaAFAPeUDVW0Uy7ZvCj4hsbw5eyPdFVMA0GCSqGSIb3DQEB\n" \
                                      "CwUAA4IBAQAjPt9L0jFCpbZ+QlwaRMxp0Wi0XUvgBCFsS+JtzLHgl4+mUwnNqipl\n" \
                                      "5TlPHoOlblyYoiQm5vuh7ZPHLgLGTUq/sELfeNqzqPlt/yGFUzZgTHbO7Djc1lGA\n" \
                                      "8MXW5dRNJ2Srm8c+cftIl7gzbckTB+6WohsYFfZcTEDts8Ls/3HB40f/1LkAtDdC\n" \
                                      "2iDJ6m6K7hQGrn2iWZiIqBtvLfTyyRRfJs8sjX7tN8Cp1Tm5gr8ZDOo0rwAhaPit\n" \
                                      "c+LJMto4JQtV05od8GiG7S5BNO98pVAdvzr508EIDObtHopYJeS4d60tbvVS3bR0\n" \
                                      "j6tJLp07kzQoH3jOlOrHvdPJbRzeXDLz\n" \
                                      "-----END CERTIFICATE-----\n" \
                                      "";
// Write to Google Scripts
String url = String("/macros/s/") + GScriptId + "/exec";

String payload_base =  "{\"command\": \"appendRow\", \
                    \"sheet_name\": \"Sheet1\", \
                    \"values\": ";
String payload = "";

String title = "";
String text = "";
String img_url = "";

float batVoltage = 0.00;
bool pgood = 0;
bool pchg = 0;
int system_state = 0;

#define STATE_DISCHARGING 0
#define STATE_CHARGING 1
#define STATE_LOW_BATTERY 2

int count = 0;
HTTPSRedirect* client = nullptr;

Inkplate display(INKPLATE_4BIT);

void setup() {
  Serial.begin(9600);

  display.begin();
  display.setRotation(1);
  display.selectDisplayMode(INKPLATE_4BIT);
  display.setTextColor(7, 15);
  display.setFont(&FreeSansBold18pt7b);
	
  if (!SPIFFS.begin()) {
    DPRINTLN("SPIFFS initialisation failed!");
    esp_deep_sleep_start();
  }
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(renderer);

  delay(250);
  
  WiFi.disconnect();

  DPRINTLN();
  DPRINT("Connecting to wifi: ");
  DPRINTLN(ssid);

  uint8_t tries = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DPRINT(".");
    WiFi.begin(ssid, password);
    tries++;
    if (tries > 10) {
      esp_restart();
    }
  }
  DPRINTLN();

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  client->setCACert(rootCACertificateGoogle);
  
  DPRINTLN("SETUP COMPLETE");
}

void loop() {
  if (Serial.available()) {      
    String jsonInput = Serial.readStringUntil('\n');

    DeserializationError error = deserializeJson(doc, jsonInput);

    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    } else {

      // Fetch values.
      batVoltage = doc["v"];
      count = doc["count"];
	  pgood = doc["pgood"];
	  pchg = doc["pchg"];
	  
	  if(!pchg){
		system_state = STATE_CHARGING;
	  }else{
		if(!pgood){
		
		}else{
		//Not connected to power supply
		
		if(batVoltage<3.50){
			system_state = STATE_LOW_BATTERY;
		}
		}
	  }
      DPRINT("Connecting to ");
      DPRINTLN(host);

      // Try to connect for a maximum of 5 times
      bool flag = false;
      for (int i = 0; i < 5; i++) {
        int retval = client->connect(host, httpsPort);
        if (retval == 1) {
          flag = true;
          break;
        }
        else
          DPRINTLN("Connection failed. Retrying...");
      }

      if (!flag) {
        DPRINT("Could not connect to server: ");
        DPRINTLN(host);
        DPRINTLN("Exiting...");
        return;
      }

      // fetch newest data
      client->GET(url, host);
      String json = client->getResponseBody();

      delete client;

      DeserializationError error = deserializeJson(doc2, json);

      // Test if parsing succeeds.
      if (error) {
        DPRINT(F("deserializeJson() failed: "));
        DPRINTLN(error.c_str());
        return;
      }

      // Fetch values.
      title = doc2["title"].as<String>();
      text = doc2["text"].as<String>();
      img_url = doc2["url"].as<String>();

      if (img_url != "") {
        WiFiClientSecure *image_client = new WiFiClientSecure;
        if (image_client) {
          image_client -> setCACert(rootCACertificateReddit);

          {
            // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
            HTTPClient https;

            DPRINT("[HTTPS] begin...\n");
            if (https.begin(*image_client, img_url)) {  // HTTPS
              DPRINT("[HTTPS] GET...\n");
              // start connection and send HTTP header
              int httpCode = https.GET();

              // httpCode will be negative on error
              if (httpCode > 0) {
                // HTTP header has been send and Server response header has been handled
                Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

                // file found at server
                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                  int len = https.getSize();
                  DPRINTLN(String(len));
                  if (len < 1024 * 1024) { //Max 1024kB allowed
                    // create buffer for read
                    // get tcp stream
                    WiFiClient * stream = https.getStreamPtr();

                    // read all data from server
                    while (https.connected() && (len > 0 || len == -1)) {
                      // get available data size
                      size_t size = stream->available();

                      if (size) {
                        uint8_t *buff = (uint8_t*) ps_malloc(len * sizeof(uint8_t));
                        int c = stream->readBytes(buff, len); //returns length of that chunk
                        DPRINTLN("Recv " + String(c) + " bytes");
                        uint16_t wd = 0, hd = 0;
                        TJpgDec.getJpgSize(&wd, &hd, buff, c);
                        DPRINTLN("Size [E] w: " + String(wd) + ", h: " + String(hd));
                        if (wd > 600 & hd > 800) {
                          TJpgDec.drawJpg(0, 0, buff, len);
						  display.setTextColor(5,15);
						  display.setCursor(0, 53);
                          display.setFont(&FreeSansBold18pt7b);
                          display.println(title);
                          display.setCursor(0, 50);
                          display.setTextColor(12,15);
                          display.println(title);
                          //drawCentreString(title);
                        } else {
                          DPRINTLN("Too small");
                          TJpgDec.drawJpg(0, 0, buff, len);
                          display.setTextColor(5,15);
						  display.setCursor(0, 53);
                          display.setFont(&FreeSansBold18pt7b);
                          display.println(title);
                          display.setCursor(0, 50);
                          display.setTextColor(12,15);
                          display.println(title);
                        }
                        free(buff);
                        len = len - c;
                      }
                      https.end();
                      DPRINTLN();
                      DPRINTLN("[HTTP] connection closed");
                    }
                  }
                } else {
                  Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
                }

                https.end();
              } else {
                Serial.printf("[HTTPS] Unable to connect\n");
				display.setTextColor(0, 15);
				display.setCursor(0, 50);
				display.setFont(&FreeSansBold18pt7b);
				display.println(F("Network error"));
				display.setCursor(0, 150);
				display.setFont(&FreeSans12pt7b);
				display.println(F("Unable to connect"));
              }
            }
          }
        }
        delete image_client;
      } else {
        //Text type
        display.setTextColor(0, 15);
        display.setCursor(0, 50);
        display.setFont(&FreeSansBold18pt7b);
        display.println(title);
        display.setCursor(0, 150);
        display.setFont(&FreeSans12pt7b);
        display.println(text);
      }
	  display.setTextColor(0, 15);
      display.setFont(&FreeSansBold18pt7b);
      display.setCursor(100 + random(100) , 700);
      display.print(String(batVoltage) + "V");
      display.setCursor(400 + random(100), 700);
      display.print(String(count));
	  
	  render_state();
	  
      display.display();
      jsonInput = "";
      esp_deep_sleep_start();
    }
  }
}
void render_state(){
	if(system_state==STATE_CHARGING){
		display.fillRoundRect(75, 725, 450, 64, 7);
		display.fillRoundRect(75 + 4, 725 + 4, 450 - 8, 64 - 8, 15);
		TJpgDec.drawFsJpg(90, 725 + 8, "/charging.jpg");
		display.setTextColor(0, 15);
		display.setFont(&FreeSansBold18pt7b);
        display.setCursor(420, 740);
        display.print(F("Charging"));
	}else if(system_state==STATE_LOW_BATTERY){
		display.fillRoundRect(75, 725, 450, 64, 7);
		display.fillRoundRect(75 + 4, 725 + 4, 450 - 8, 64 - 8, 15);
		TJpgDec.drawFsJpg(90, 725 + 8, "/low_battery.jpg");
		display.setTextColor(0, 15);
		display.setFont(&FreeSansBold18pt7b);
        display.setCursor(180, 740);
        display.print(F("Low battery detected"));
	}
}
bool renderer(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t* bitmap)
{
  for (int i = y; i < h + y; i++) {
    for (int j = x; j < w + x; j++) {
      uint8_t color = (bitmap[0] + bitmap[1] + bitmap[2]) / 3;
      display.drawPixel(j, i, color / 16);
      bitmap += 3;
    }
  }
  return 1;
}
/*void drawCentreString(String text)
  {
  display.setCursor(0, 20);
  int16_t x1, y1;
  uint16_t w1, h1;
  display.getTextBounds(text , 0, 0, &x1, &y1, &w1, &h1);
  if (w1 >= 600) {
    DPRINTLN("exceeded");
  } else {
    display.println(text);
  }
  }
*/
