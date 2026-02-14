// Wave - adapted from Aurora PatternWave
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon
// Quadwave dots along a sine-like curve with random orientation and optional mirror.
// Adapted for Fibonacci128 - proximity-based LED rendering, random rotation per cycle.

#define WAVE_DOTS 32

void wave()
{
  static byte waveTheta = 0;
  static byte waveHue = 0;
  static uint8_t waveRotation = 0;
  static uint8_t waveCount = 1;
  static bool waveInit = false;
  static PatternRotation waveRot = {0, 0, 0, 0, false};

  if (!waveInit) {
    waveRotation = random(0, 4);
    waveCount = random(1, 3);
    waveInit = true;
  }

  // Re-randomize orientation every 10 seconds
  EVERY_N_SECONDS(10) {
    waveRotation = random(0, 4);
    waveCount = random(1, 3);
  }

  // Re-randomize rotation speeds every 8 seconds
  EVERY_N_SECONDS(8) {
    waveRot.randomizeSpeeds();
  }

  // Update rotation (sinusoidal speed: speeds up, slows, pauses, reverses)
  float wvAngle = waveRot.update();
  float wvCosA = cosf(wvAngle), wvSinA = sinf(wvAngle);

  fadeToBlackBy(leds, NUM_LEDS, 60); // DimAll(254)

  // Precompute dot positions along the wave
  int16_t dotX[WAVE_DOTS * 2]; // extra space for mirrored wave
  int16_t dotY[WAVE_DOTS * 2];
  CRGB dotColor[WAVE_DOTS * 2];
  uint8_t numDots = 0;

  for (uint8_t i = 0; i < WAVE_DOTS; i++) {
    uint8_t pos = i * 8; // 0..248
    uint8_t n;

    switch (waveRotation) {
      case 0: // horizontal, theta forward
        n = quadwave8(pos * 2 + waveTheta);
        dotX[numDots] = pos; dotY[numDots] = n;
        dotColor[numDots] = ColorFromPalette(gCurrentPalette, pos + waveHue);
        numDots++;
        if (waveCount == 2) {
          dotX[numDots] = pos; dotY[numDots] = 255 - n;
          dotColor[numDots] = ColorFromPalette(gCurrentPalette, pos + waveHue);
          numDots++;
        }
        break;
      case 1: // vertical, theta forward
        n = quadwave8(pos * 2 + waveTheta);
        dotX[numDots] = n; dotY[numDots] = pos;
        dotColor[numDots] = ColorFromPalette(gCurrentPalette, pos + waveHue);
        numDots++;
        if (waveCount == 2) {
          dotX[numDots] = 255 - n; dotY[numDots] = pos;
          dotColor[numDots] = ColorFromPalette(gCurrentPalette, pos + waveHue);
          numDots++;
        }
        break;
      case 2: // horizontal, theta reverse
        n = quadwave8(pos * 2 - waveTheta);
        dotX[numDots] = pos; dotY[numDots] = n;
        dotColor[numDots] = ColorFromPalette(gCurrentPalette, pos + waveHue);
        numDots++;
        if (waveCount == 2) {
          dotX[numDots] = pos; dotY[numDots] = 255 - n;
          dotColor[numDots] = ColorFromPalette(gCurrentPalette, pos + waveHue);
          numDots++;
        }
        break;
      case 3: // vertical, theta reverse
        n = quadwave8(pos * 2 - waveTheta);
        dotX[numDots] = n; dotY[numDots] = pos;
        dotColor[numDots] = ColorFromPalette(gCurrentPalette, pos + waveHue);
        numDots++;
        if (waveCount == 2) {
          dotX[numDots] = 255 - n; dotY[numDots] = pos;
          dotColor[numDots] = ColorFromPalette(gCurrentPalette, pos + waveHue);
          numDots++;
        }
        break;
    }
  }

  const int32_t proxSq = 400L; // 20^2

  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    // Rotate coordinates around center before proximity check
    float fx = (float)coordsX[j] - 128.0f;
    float fy = (float)coordsY[j] - 128.0f;
    int16_t lx = (int16_t)(128.0f + wvCosA * fx - wvSinA * fy);
    int16_t ly = (int16_t)(128.0f + wvSinA * fx + wvCosA * fy);

    for (uint8_t d = 0; d < numDots; d++) {
      int16_t dx = lx - dotX[d];
      int16_t dy = ly - dotY[d];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[j] += CRGB(scale8(dotColor[d].r, bri), scale8(dotColor[d].g, bri), scale8(dotColor[d].b, bri));
      }
    }
  }

  waveTheta++;
  waveHue++;
}
