// TODO: Figure out a way to rename the device without having to go into boot mode...

#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <SPI.h>
#include <ArduinoJson.h>

#include "Robot_Mouth.h"

// ================= DISPLAY SETUP =================


// --- MOUTH TFT (ASSUMED SEPARATE DISPLAY) ---
#define MOUTH_CS   D1
#define MOUTH_DC   D3
#define MOUTH_RST  -1

Adafruit_GC9A01A mouthTFT(MOUTH_CS, MOUTH_DC, MOUTH_RST);
GFXcanvas16 mouthCanvas(240, 240);

Mouth Mouth1(mouthTFT, mouthCanvas, "Mouth");

// ================= SERIAL STATE =================

String receivedData = "";
bool newDataAvailable = false;
TaskHandle_t SerialTaskHandle = NULL;

// ================= SERIAL TASK =================

void serialListenerTask(void *pvParameters) {
  while (1) {
    if (Serial.available() > 0) {
      char c = Serial.read();

      if (c == '\n') {
        newDataAvailable = true;
      } else if (c != '\r') {
        receivedData += c;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ================= COMMAND HANDLER =================

void handleCommand(JsonDocument& doc) {

  const char* cmd = doc["cmd"];
  if (!cmd) return;

  // -------- MOUTH COMMANDS --------

  else if (strcmp(cmd, "state") == 0) {
    const char* value = doc["value"];
    if (!value) return;

    if (strcmp(value, "closed") == 0) Mouth1.setState(MouthStates::CLOSED);
    else if (strcmp(value, "open") == 0) Mouth1.setState(MouthStates::OPEN);
    else if (strcmp(value, "opening") == 0) Mouth1.setState(MouthStates::OPENING);
    else if (strcmp(value, "closing") == 0) Mouth1.setState(MouthStates::CLOSING);
    else if (strcmp(value, "talking") == 0) Mouth1.setState(MouthStates::TALKING);
  }

  else if (strcmp(cmd, "open") == 0) {
    JsonVariant v = doc["value"];
    Mouth1.setOpenAmount(v["amt"] | 0.0);
  }

  else if (strcmp(cmd, "smile") == 0) {
    JsonVariant v = doc["value"];
    Mouth1.smile(v["amt"] | 0.0);
  }

  else if (strcmp(cmd, "frown") == 0) {
    JsonVariant v = doc["value"];
    Mouth1.frown(v["amt"] | 0.0);
  }

  else if (strcmp(cmd, "talk") == 0) {
    Mouth1.talk();
  }

  else if (strcmp(cmd, "stopTalk") == 0) {
    Mouth1.stopTalking();
  }

  // -------- UNKNOWN --------

  else {
    Serial.print("Unknown cmd: ");
    Serial.println(cmd);
  }
}

// ================= SETUP =================

void setup() {
  Serial.begin(9600);
  delay(1000);

  xTaskCreatePinnedToCore(
    serialListenerTask,
    "SerialListener",
    3000,
    NULL,
    1,
    &SerialTaskHandle,
    0
  );

  Serial.println("System starting...");
  mouthTFT.begin();
  mouthTFT.fillScreen(GC9A01A_GREEN);
  mouthTFT.setRotation(0);
  Serial.println("Displays initialized.");
}

// ================= LOOP =================

void loop() {

  if (newDataAvailable) {

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, receivedData);

    if (!error) {
      handleCommand(doc);
    } else {
      Serial.print("JSON error: ");
      Serial.println(error.c_str());
    }

    receivedData = "";
    newDataAvailable = false;
  }

  Mouth1.showMouth();
}