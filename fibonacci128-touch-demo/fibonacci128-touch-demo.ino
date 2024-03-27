/*
   Fibonacci128 Touch Demo: https://github.com/jasoncoon/fibonacci128-touch-demo
   Copyright (C) 2021 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <FastLED.h>  // https://github.com/FastLED/FastLED
#include <Button.h>   // https://github.com/madleech/Button
#include "Adafruit_FreeTouch.h" //https://github.com/adafruit/Adafruit_FreeTouch
#include "GradientPalettes.h"

FASTLED_USING_NAMESPACE

#define DATA_PIN      10
#define CLOCK_PIN     8
#define LED_TYPE      SK9822
#define COLOR_ORDER   BGR
#define NUM_LEDS      128

#include "Map.h"
#define MILLI_AMPS         1400 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND  120

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

CRGB leds[NUM_LEDS];

uint8_t brightnesses[] = { 96, 64, 32, 16 };
uint8_t currentBrightnessIndex = 1;

Adafruit_FreeTouch touch0 = Adafruit_FreeTouch(A0, OVERSAMPLE_4, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch touch1 = Adafruit_FreeTouch(A1, OVERSAMPLE_4, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch touch2 = Adafruit_FreeTouch(A3, OVERSAMPLE_4, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch touch3 = Adafruit_FreeTouch(A2, OVERSAMPLE_4, RESISTOR_0, FREQ_MODE_NONE);

#define touchPointCount 4

Button button0(5);
Button button1(4);

// These values were discovered using the commented-out Serial.print statements in handleTouch below

// minimum values for each touch pad, used to filter out noise
uint16_t touchMin[touchPointCount] = { 626, 356, 356, 632 };

// maximum values for each touch pad, used to determine when a pad is touched
uint16_t touchMax[touchPointCount] = { 1016, 1016, 1016, 1016 };

// raw capacitive touch sensor readings
uint16_t touchRaw[touchPointCount] = { 0, 0, 0, 0 };

// capacitive touch sensor readings, mapped/scaled one one byte each (0-255)
uint8_t touch[touchPointCount] = { 0, 0, 0, 0 };

// coordinates of the touch points
uint8_t touchPointX[touchPointCount] = { 228, 29, 29, 228 };
uint8_t touchPointY[touchPointCount] = { 29, 29, 231, 231 };

boolean activeWaves = false;

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];

uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );

// ten seconds per color palette makes a good demo
// 20-120 is better for deployment
uint8_t secondsPerPalette = 10;

static uint8_t hue = 0;

#include "Patterns.h"

typedef void (*SimplePatternList[])();
SimplePatternList patterns = { colorWavesFibonacci, prideFibonacci, outwardPalettes, colorTest, horizontalRainbow, verticalRainbow, diagonalRainbow, outwardRainbow, rotatingRainbow };

uint8_t currentPatternIndex = 0;
const uint8_t patternCount = ARRAY_SIZE(patterns);

void setup() {
  Serial.begin(115200);
  //  delay(3000);

  button0.begin();
  button1.begin();

  if (!touch0.begin())
    Serial.println("Failed to begin qt on pin A0");
  if (!touch1.begin())
    Serial.println("Failed to begin qt on pin A1");
  if (!touch2.begin())
    Serial.println("Failed to begin qt on pin A3");
  if (!touch3.begin())
    Serial.println("Failed to begin qt on pin A2");

  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setDither(false);
  // FastLED.setCorrection(TypicalSMD5050);
  FastLED.setBrightness(brightnesses[currentBrightnessIndex]);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  FastLED.setBrightness(brightnesses[currentBrightnessIndex]);
}

void loop() {
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random());

  handleTouch();

  if (button0.pressed()) {
    Serial.println("button 0 pressed");
    if (currentPatternIndex < patternCount - 1) currentPatternIndex++;
    else currentPatternIndex = 0;
    Serial.print("mode: ");
    Serial.println(currentPatternIndex);
  }

  if (button1.pressed()) {
    Serial.println("button 1 pressed");
    currentBrightnessIndex = (currentBrightnessIndex + 1) % ARRAY_SIZE(brightnesses);
    FastLED.setBrightness(brightnesses[currentBrightnessIndex]);
    Serial.print("brightness: ");
    Serial.println(brightnesses[currentBrightnessIndex]);
  }

  // change to a new cpt-city gradient palette
  EVERY_N_SECONDS( secondsPerPalette ) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  // static byte offset = 0;

  EVERY_N_MILLISECONDS(40) {
    // slowly blend the current palette to the next
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 8);
  }

  EVERY_N_MILLIS(30) {
    hue++;
  }

  if (!activeWaves){
    patterns[currentPatternIndex]();
  }

  touchDemo();

  // for (byte i = 0; i < NUM_LEDS; i++) {
  //   leds[i] = ColorFromPalette(gCurrentPalette, coordsX[i] + offset);
  // }

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

bool touchChanged = true;

void handleTouch() {
  for (uint8_t i = 0; i < touchPointCount; i++) {
    if (i == 0) touchRaw[i] = touch0.measure();
    else if (i == 1) touchRaw[i] = touch1.measure();
    else if (i == 2) touchRaw[i] = touch2.measure();
    else if (i == 3) touchRaw[i] = touch3.measure();

    // // uncomment to display raw touch values in the serial monitor/plotter
    // Serial.print(touchRaw[i]);
    // Serial.print(" ");

    if (touchRaw[i] < touchMin[i]) {
      touchMin[i] = touchRaw[i];
      touchChanged = true;
    }

    if (touchRaw[i] > touchMax[i]) {
      touchMax[i] = touchRaw[i];
      touchChanged = true;
    }

    touch[i] = map(touchRaw[i], touchMin[i], touchMax[i], 0, 255);

    // // uncomment to display mapped/scaled touch values in the serial monitor/plotter
    // Serial.print(touch[i]);
    // Serial.print(" ");
  }

  // // uncomment to display raw and/or mapped/scaled touch values in the serial monitor/plotter
  // Serial.println();

  // uncomment to display raw, scaled, min, max touch values in the serial monitor/plotter
  //  if (touchChanged) {
  //    for (uint8_t i = 0; i < 4; i++) {
  //      Serial.print(touchRaw[i]);
  //      Serial.print(" ");
  //      Serial.print(touch[i]);
  //      Serial.print(" ");
  //      Serial.print(touchMin[i]);
  //      Serial.print(" ");
  //      Serial.print(touchMax[i]);
  //      Serial.print(" ");
  //    }
  //
  //    Serial.println();
  //
  //    touchChanged = false;
  //  }
}

// adds a color to a pixel given it's XY coordinates and a "thickness" of the logical pixel
// since we're using a sparse logical grid for mapping, there isn't an LED at every XY coordinate
// thickness adds a little "fuzziness"
void addColorXY(int x, int y, CRGB color, uint8_t thickness = 0)
{
  // ignore coordinates outside of our one byte map range
  if (x < 0 || x > 255 || y < 0 || y > 255) return;

  // loop through all of the LEDs
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // get the XY coordinates of the current LED
    uint8_t ix = coordsX[i];
    uint8_t iy = coordsY[i];

    // are the current LED's coordinates within the square centered
    // at X,Y, with width and height of thickness?
    if (ix >= x - thickness && ix <= x + thickness &&
        iy >= y - thickness && iy <= y + thickness) {

      // add to the color instead of just setting it
      // so that colors blend
      // FastLED automatically prevents overflowing over 255
      leds[i] += color;
    }
  }
}

// algorithm from http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
void drawCircle(int x0, int y0, int radius, const CRGB color, uint8_t thickness = 0)
{
  int a = radius, b = 0;
  int radiusError = 1 - a;

  if (radius == 0) {
    addColorXY(x0, y0, color, thickness);
    return;
  }

  while (a >= b)
  {
    addColorXY(a + x0, b + y0, color, thickness);
    addColorXY(b + x0, a + y0, color, thickness);
    addColorXY(-a + x0, b + y0, color, thickness);
    addColorXY(-b + x0, a + y0, color, thickness);
    addColorXY(-a + x0, -b + y0, color, thickness);
    addColorXY(-b + x0, -a + y0, color, thickness);
    addColorXY(a + x0, -b + y0, color, thickness);
    addColorXY(b + x0, -a + y0, color, thickness);

    b++;
    if (radiusError < 0)
      radiusError += 2 * b + 1;
    else
    {
      a--;
      radiusError += 2 * (b - a + 1);
    }
  }
}

const uint8_t waveCount = 8;

// track the XY coordinates and radius of each wave
uint16_t radii[waveCount];
uint8_t waveX[waveCount];
uint8_t waveY[waveCount];
CRGB waveColor[waveCount];

const uint16_t maxRadius = 512;

void touchDemo() {
  // fade all of the LEDs a small amount each frame
  // increasing this number makes the waves fade faster
  fadeToBlackBy(leds, NUM_LEDS, 30);

  for (uint8_t i = 0; i < touchPointCount; i++) {
    // start new waves when there's a new touch
    if (touch[i] > 63 && radii[i] == 0) {
      radii[i] = 32;
      waveX[i] = touchPointX[i];
      waveY[i] = touchPointY[i];
      waveColor[i] = CHSV(random8(), 255, 255);
    }
  }

  activeWaves = false;

  for (uint8_t i = 0; i < waveCount; i++)
  {
    // increment radii if it's already been set in motion
    if (radii[i] > 0 && radii[i] < maxRadius) radii[i] = radii[i] + 8;

    // reset waves already at max
    if (radii[i] >= maxRadius) {
      activeWaves = true;
      radii[i] = 0;
    }

    if (radii[i] == 0)
      continue;

    activeWaves = true;

    CRGB color = waveColor[i];

    uint8_t x = waveX[i];
    uint8_t y = waveY[i];

    // draw waves starting from the corner closest to each touch sensor
    drawCircle(x, y, radii[i], color, 4);
  }
}
