#include "Robot_Mouth.h"

#define T_LENGTH_OPENING 250
#define T_LENGTH_CLOSING 250

#define MOUTH_WIDTH_RATIO 0.65f
#define MOUTH_MAX_HEIGHT_RATIO 0.35f

#define LIP_COLOR GC9A01A_WHITE
#define MOUTH_COLOR GC9A01A_RED 
#define TOOTH_COLOR GC9A01A_WHITE

Mouth::Mouth(
  Adafruit_GC9A01A& tftObj,
  GFXcanvas16& canvasObj,
  String mouthName)
  : tft(tftObj),
    canvas(canvasObj)
{
  name = mouthName;

  current_state = MouthStates::CLOSED;

  desiredOpenAmount = 0;
  currentOpenAmount = 0;

  desiredSmile = 0;
  currentSmile = 0;

  time_at_state_change = millis();
  time_at_last_draw = millis();
}

// =============
// State control 
// =============

void Mouth::setState(MouthStates newState)
{
  current_state = newState;
  time_at_state_change = millis();
}

void Mouth::openMouth()
{
  setState(MouthStates::OPENING);
}

void Mouth::closeMouth()
{
  setState(MouthStates::CLOSING);
}

void Mouth::talk()
{
  setState(MouthStates::TALKING);
}

void Mouth::stopTalking()
{
  setState(MouthStates::CLOSED);
}

// =================
// Parameter control
// =================

void Mouth::setOpenAmount(float amt)
{
  desiredOpenAmount = constrain(amt, 0.0f, 1.0f);
}

void Mouth::smile(float amt)
{
  desiredSmile = constrain(amt, 0.0f, 1.0f);
}

void Mouth::frown(float amt)
{
  desiredSmile = -constrain(amt, 0.0f, 1.0f);
}

// ===========
// State decay
// ===========

void Mouth::checkDecayState()
{
  switch (current_state)
  {
    case MouthStates::OPENING:
      if (millis() - time_at_state_change > T_LENGTH_OPENING)
      {
        current_state = MouthStates::OPEN;
        desiredOpenAmount = 1.0f;
      }
      break;

    case MouthStates::CLOSING:
      if (millis() - time_at_state_change > T_LENGTH_CLOSING)
      {
        current_state = MouthStates::CLOSED;
        desiredOpenAmount = 0.0f;
      }
      break;

    default:
      break;
  }
}

// =========
// Draw loop
// =========

void Mouth::showMouth()
{
  checkDecayState();

  unsigned long dt = millis() - time_at_last_draw;

  float t = constrain(
    0.001f * dt,
    0.10f,
    0.90f
  );

  currentOpenAmount =
    t * currentOpenAmount +
    (1.0f - t) * desiredOpenAmount;

  currentSmile =
    t * currentSmile +
    (1.0f - t) * desiredSmile;

  if (current_state == MouthStates::OPENING)
  {
    desiredOpenAmount = 1.0f;
  }

  if (current_state == MouthStates::CLOSING)
  {
    desiredOpenAmount = 0.0f;
  }

  if (current_state == MouthStates::TALKING)
  {
    desiredOpenAmount =
      0.2f + random(0, 80) / 100.0f;
  }

  drawMouth();

  tft.drawRGBBitmap(
    0,
    0,
    canvas.getBuffer(),
    canvas.width(),
    canvas.height());

  time_at_last_draw = millis();
}

// =================
// Drawing the mouth
// =================


#define LIP_THICKNESS 4   // <- new: controls how “boxed” the lips look

void Mouth::drawMouth()
{
  canvas.fillScreen(GC9A01A_YELLOW);

  int cx = tft.width() / 2;
  int cy = tft.height() * 0.75;

  int mouthWidth =
    tft.width() * MOUTH_WIDTH_RATIO;

  int mouthHeight =
    tft.height() *
    MOUTH_MAX_HEIGHT_RATIO *
    currentOpenAmount;

  int x = cx - mouthWidth / 2;
  int y = cy - mouthHeight / 2;

  // Ensure minimum visible mouth body so lips are always present
  int minHeight = 18;
  int h = max(mouthHeight, minHeight);

  int yFixed = cy - h / 2;

  // -------------------------
  // INNER MOUTH (fill)
  // -------------------------
  canvas.fillRoundRect(
    x,
    yFixed,
    mouthWidth,
    h,
    10,
    MOUTH_COLOR
  );

  // -------------------------
  // OUTER LIP BORDER (thick)
  // -------------------------
  for (int i = 0; i < LIP_THICKNESS; i++)
  {
    canvas.drawRoundRect(
      x - i,
      yFixed - i,
      mouthWidth + (i * 2),
      h + (i * 2),
      10 + i,
      LIP_COLOR
    );
  }

  // -------------------------
  // TEETH (only when open enough)
  // -------------------------
  if (currentOpenAmount > 0.25f)
  {
    int toothHeight = 6 + currentOpenAmount * 4;

    canvas.fillRect(
      x + 6,
      yFixed + 3,
      mouthWidth - 12,
      toothHeight,
      TOOTH_COLOR
    );
  }

  // -------------------------
  // CORNERS (smile/frown control)
  // -------------------------
  int cornerOffset = currentSmile * 12;

  for (int i = 0; i < LIP_THICKNESS; i++)
  {
    canvas.drawLine(
      x + i,
      cy,
      x - 8,
      cy - cornerOffset,
      LIP_COLOR
    );

    canvas.drawLine(
      x + mouthWidth - i,
      cy,
      x + mouthWidth + 8,
      cy - cornerOffset,
      LIP_COLOR
    );
  }
}