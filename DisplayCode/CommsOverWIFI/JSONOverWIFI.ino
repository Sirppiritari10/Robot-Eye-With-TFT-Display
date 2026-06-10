//TODO: Figure out a way to rename the device without having to go into boot mode to upload new code // NOTE: probably wont do it since it seems it messes with arduino IDE too much, a workaround in python is better

/*Drawing sprites:
 * in the beginning have global bitmap variables that you can copy (probs from a python script that converts images to bitmap)
 * In the eyes draw commands instead of drawing shapes, draw these bitmaps
 *  The eye sclera(the white part aka the background) is a sprite (maybe with some tasteful shading probs), 
 *  The iris(colorful part) is also a sprite
 *  The pupil(black part) is procedural, a circle
 * For displaying images, have a separate rendering mode for rendering lone sprites (question marks, emojis etc.)
 *  These images are sent over usb as raw bitmaps.
*/




// ==========For display code===========
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include <ArduinoJson.h> // JSON library for easy communication


// == WIFI config==
#include <WiFi.h>
#include <WebSocketsServer.h>

const char* WIFI_SSID = "iPhone (Simon)";
const char* WIFI_PASSWORD = "123456789";

WebSocketsServer webSocket(8765);

String espIP;
bool receivedFirstCommand = false;

#include "Robot_Eye.h"
#include "Robot_Eye_Pair.h"

  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
#define TFT1_CS        D5  
#define TFT2_CS        D4 
#define TFT_RST        D3 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         D2

// OPTION 1 (recommended) is to use the HARDWARE SPI pins, which are unique
// to each board and not reassignable. For Arduino Uno: MOSI = pin 11 and
// SCLK = pin 13. This is the fastest mode of operation and is required if
// using the breakout board's mic
// For 1.44" and 1.8" TFT with ST7735 use:
Adafruit_ST7735 tft1 = Adafruit_ST7735(TFT1_CS, TFT_DC, TFT_RST);
Adafruit_ST7735 tft2 = Adafruit_ST7735(TFT2_CS, TFT_DC, -1); // help gotten from https://forums.adafruit.com/viewtopic.php?t=171191

//Canvas is initialized the same size as the tft1 display, assuming both displays are of the same size this is fine
GFXcanvas16 canvas(tft1.width(), tft1.height());// offscreen canvas to avoid flickering

Eye Eye1 = Eye(tft1, canvas, "L");
Eye Eye2 = Eye(tft2, canvas, "R");
Eye_Pair Eyes = Eye_Pair(Eye1, Eye2);


// ======================================

// =====Api commands through wifi======

void onWebSocketEvent(
    uint8_t clientNum,
    WStype_t type,
    uint8_t * payload,
    size_t length)
{
    switch(type)
    {
        case WStype_CONNECTED:
        {
            IPAddress ip = webSocket.remoteIP(clientNum);

            Serial.printf(
                "Client %u connected from %s\n",
                clientNum,
                ip.toString().c_str()
            );

            webSocket.sendTXT(
                clientNum,
                "ESP32 connected"
            );
            break;
        }

        case WStype_DISCONNECTED:
        {
            Serial.printf(
                "Client %u disconnected\n",
                clientNum
            );
            break;
        }

        case WStype_TEXT:
        {
            String msg = String((char*)payload);

            Serial.print("WS RX: ");
            Serial.println(msg);

            StaticJsonDocument<256> doc;

            DeserializationError error =
                deserializeJson(doc, msg);

            if(error)
            {
                Serial.print("JSON parse failed: ");
                Serial.println(error.c_str());

                webSocket.sendTXT(
                    clientNum,
                    "JSON parse failed"
                );

                return;
            }

            handleCommand(doc);

            webSocket.sendTXT(
                clientNum,
                "OK"
            );

            break;
        }

        default:
            break;
    }
}

void connectWiFi()
{
    WiFi.mode(WIFI_STA);


    IPAddress local_IP(192,168,1,50);
    IPAddress gateway(192,168,1,1);
    IPAddress subnet(255,255,255,0);


    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    Serial.print("Connecting to WiFi");

    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected");

    espIP = WiFi.localIP().toString();

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void drawIPScreen()
{
    tft1.fillScreen(ST77XX_BLACK);
    tft2.fillScreen(ST77XX_BLACK);

    tft1.setTextColor(ST77XX_WHITE);
    tft2.setTextColor(ST77XX_WHITE);

    tft1.setTextSize(1);
    tft2.setTextSize(1);

    tft1.setCursor(10, 20);
    tft2.setCursor(10, 20);

    tft1.print("Waiting for command...");
    tft2.print("Waiting for command...");

    tft1.setCursor(10, 40);
    tft2.setCursor(10, 40);

    tft1.print("IP:");
    tft2.print("IP:");

    tft1.setCursor(10, 55);
    tft2.setCursor(10, 55);

    tft1.print(espIP);
    tft2.print(espIP);
}

void handleCommand(JsonDocument& doc) {
  receivedFirstCommand = true;
  
  const char* cmd = doc["cmd"];

  if (!cmd) return;

  // ---- STATE COMMAND ----
  if (strcmp(cmd, "state") == 0) {
  const char* value = doc["value"];
  if (value) {
    Eyes.setEyePairState(StringToEyePairState(value));
  }
  }

  // ---- LOOK IN DIRECTION ----
  else if (strcmp(cmd, "look") == 0) {

  JsonVariant v = doc["value"];

  if (!v.isNull()) {
    float x = v["x"] | 0.0;
    float y = v["y"] | 0.0;

    Eyes.look(x, y);
  }
  }

  // ---- SET MOOD -----
  else if (strcmp(cmd, "mood") == 0) {

  const char* value = doc["value"];

  if (value) {
    Moods mood = getMood(value);
    Eyes.setMood(mood);
  }
  }
  
  // ---- SET PUPIL RADIUS ----
  else if (strcmp(cmd, "pupilR") == 0) {

  JsonVariant v = doc["value"];
  float R = v["r"] | 0.0;
  Eyes.setPupilRadius(R);
  
  }

  // ---- MANUAL BLINK ----
  else if (strcmp(cmd, "blink") == 0){
    //Ignore values
    Eyes.blink();
  }

  // ---- MANUAL GLANCE ---- // if no x/y for the direction are given excecutes in random direction & magnitude
  else if (strcmp(cmd, "glance") == 0)
  {
    JsonVariant value = doc["value"];

    if (value.is<JsonObject>() &&
        value["x"].is<float>() &&
        value["y"].is<float>())
    {
        float x = value["x"];
        float y = value["y"];

        Eyes.glance(x, y);
    }
    else
    {
        Eyes.glance();
    }
}
  // ---- UNKNOWN COMMAND ----
  else {
    Serial.print("Unknown cmd: ");
    Serial.println(cmd);
  }
}

// ======================================






void setup()
{
    Serial.begin(115200);

    delay(1000);

    connectWiFi();

    Serial.print("REAL IP: ");
    Serial.println(WiFi.localIP());

    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);

    Serial.println("WebSocket server started");

    Serial.println("Initializing displays:");

    tft1.initR(INITR_144GREENTAB);
    tft2.initR(INITR_144GREENTAB);

    tft1.setRotation(3);
    tft2.setRotation(1);

    Serial.println(F("Displays Initialized"));

    Eyes.setIdle(true);
}




// Pääsilmukka, joka pyörii itsenäisesti corella 1
void loop() {
  
  webSocket.loop();
  

  // Shows the ip address of the displays on screen if they aren't connected, remofe if you want its not necessary if you have access to serial and you can see it from there
  if (!receivedFirstCommand)
  {
    drawIPScreen();
    delay(200);  // prevents flicker / excessive redraw
    return;
  }

  Eyes.showEyes();
  
}