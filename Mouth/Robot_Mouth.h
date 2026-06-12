#ifndef Robot_Mouth_H
#define Robot_Mouth_H
#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

enum class MouthStates {
  CLOSED,
  OPEN,
  OPENING,
  CLOSING,
  TALKING
};

class Mouth {
public:
  Mouth(
    Adafruit_GC9A01A& tftObj,
    GFXcanvas16& canvasObj,
    String mouthName = "Mouth"
  );

  void showMouth();

  void setState(MouthStates newState);

  void openMouth();
  void closeMouth();

  void setOpenAmount(float amt);    // 0..1
  void smile(float amt);            // 0..1
  void frown(float amt);            // 0..1

  void talk();
  void stopTalking();

private:
  void checkDecayState();
  void drawMouth();

  String name;

  MouthStates current_state;

  float desiredOpenAmount;
  float currentOpenAmount;

  float desiredSmile;
  float currentSmile;

  unsigned long time_at_state_change;
  unsigned long time_at_last_draw;

  Adafruit_GC9A01A& tft;
  GFXcanvas16& canvas;
};

#endif