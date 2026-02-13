// Sublime Demos - adapted from Novel Mutations Costume Controllers 2018
// Original: https://github.com/Intrinsically-Sublime/esp8266-fastled-webserver
// Further adapted by Marc MERLIN
// GPLv3
//
// Patterns: juggle, bpm, sinelon
// Adapted for Fibonacci128 - proximity-based or per-LED rendering.

// Juggle - colored dots weaving in and out of sync
// Dots bounce along a 1D path mapped to the LED array via radius.
// Number of dots, speed, hue increment, and fade rate change over time.

void sublimeJuggle()
{
  static uint8_t numdots = 4;
  static uint8_t faderate = 2;
  static uint8_t hueinc = 255 / 4 - 1;
  static uint8_t thishue = 0;
  static uint8_t curhue = 0;
  static uint8_t thissat = 255;
  static uint8_t thisbright = 255;
  static uint8_t basebeat = 5;

  static uint8_t lastSecond = 99;
  uint8_t secondHand = (millis() / 1000) % 30;

  if (lastSecond != secondHand) {
    lastSecond = secondHand;
    switch (secondHand) {
      case  0: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break;
      case 10: numdots = 4; basebeat = 10; hueinc = 16; faderate = 8; thishue = 128; break;
      case 20: numdots = 8; basebeat =  3; hueinc =  0; faderate = 8; thishue = random8(); break;
      case 30: break;
    }
  }

  curhue = thishue;
  fadeToBlackBy(leds, NUM_LEDS, faderate);

  // Each dot has a position in 0-255 space (mapped via radius)
  for (uint8_t d = 0; d < numdots; d++) {
    uint8_t dotPos = beatsin8(basebeat + d + numdots, 0, 255);
    CRGB color = CHSV(hue + curhue, thissat, thisbright);

    // Light LEDs whose radius is close to the dot position
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      int16_t diff = (int16_t)radius[i] - (int16_t)dotPos;
      if (diff < 0) diff = -diff;
      if (diff < 12) {
        uint8_t bri = 255 - diff * 21;
        leds[i] += CRGB(scale8(color.r, bri), scale8(color.g, bri), scale8(color.b, bri));
      }
    }
    curhue += hueinc;
  }
}

// BPM - colored stripes pulsing at a defined Beats-Per-Minute
// Radial bands pulse outward from center, colored through the palette.

void sublimeBpm()
{
  uint8_t beat = beatsin8(62, 64, 255);

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(gCurrentPalette, hue + (radius[i] >> 3), beat - hue + (radius[i] >> 2));
  }
}

// Sinelon - a colored dot sweeping back and forth with fading trails
// Dot position mapped via radius (sweeps from center to edge and back).

void sublimeSinelon()
{
  fadeToBlackBy(leds, NUM_LEDS, 20);

  uint8_t dotPos = beatsin8(13, 0, 255);
  CRGB color = ColorFromPalette(gCurrentPalette, hue, 255);

  // Light LEDs whose radius matches the sweep position
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int16_t diff = (int16_t)radius[i] - (int16_t)dotPos;
    if (diff < 0) diff = -diff;
    if (diff < 10) {
      uint8_t bri = 255 - diff * 25;
      leds[i] += CRGB(scale8(color.r, bri), scale8(color.g, bri), scale8(color.b, bri));
    }
  }
}

// Fire (Radial) - adapted from Sublime Demos fire() / Fire2012WithPalette
// Radial heat simulation: heat rises from edge (high radius) toward center.
// 1D fire along radius bands with angular Perlin noise for organic variation.

#define FIRE_BANDS 32

void sublimeFire()
{
  static uint8_t heat[FIRE_BANDS];

  uint8_t cooling = 55;
  uint8_t sparking = 120;

  // Step 1. Cool down every band a little
  for (uint8_t i = 0; i < FIRE_BANDS; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((cooling * 10) / FIRE_BANDS) + 2));
  }

  // Step 2. Heat drifts "up" (from edge=low index toward center=high index)
  for (uint8_t k = FIRE_BANDS - 1; k > 1; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3. Randomly ignite new sparks at the base (edge = low indices)
  if (random8() < sparking) {
    uint8_t j = random8(3);
    heat[j] = qadd8(heat[j], random8(160, 255));
  }

  // Step 4. Map heat to LEDs
  // radius 255 (edge) → heat[0] (base), radius 0 (center) → heat[FIRE_BANDS-1] (top)
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t bandIdx = ((255 - radius[i]) * (uint16_t)(FIRE_BANDS - 1)) / 255;
    uint8_t h = heat[bandIdx];
    // Angular Perlin noise for organic flame variation
    uint8_t noise = inoise8(angles[i] * 4, radius[i] * 2, (uint16_t)(millis() / 3));
    h = qsub8(h, noise >> 2);
    leds[i] = ColorFromPalette(HeatColors_p, h);
  }
}

// Fire (Vertical) - embers rising bottom-to-top in Y space with fading trails.
// Particle-based rendering like rain but with HeatColors_p palette.

#define FIRE_MAX_EMBERS 16

struct FireEmber {
  int16_t x, y;
  uint8_t heat;
  int8_t drift;
  bool active;
};

void sublimeVerticalFire()
{
  static FireEmber embers[FIRE_MAX_EMBERS];
  static bool fireInit = false;

  if (!fireInit) {
    memset(embers, 0, sizeof(embers));
    fireInit = true;
  }

  fadeToBlackBy(leds, NUM_LEDS, 50);

  // Update existing embers and spawn new ones
  for (uint8_t e = 0; e < FIRE_MAX_EMBERS; e++) {
    if (embers[e].active) {
      embers[e].y += random8(3, 7); // rise speed (variable)
      embers[e].x += embers[e].drift; // lateral drift
      embers[e].heat = qsub8(embers[e].heat, random8(1, 5)); // cool as rising
      if (embers[e].y > 280 || embers[e].heat < 30) embers[e].active = false;
    } else {
      if (random8() < 60) { // spawn frequency
        embers[e].x = random8(60, 196); // spawn across center horizontally
        embers[e].y = random8(0, 30); // start near bottom
        embers[e].heat = random8(200, 255);
        embers[e].drift = random8(3) - 1; // -1, 0, or 1
        embers[e].active = true;
      }
    }
  }

  const int32_t proxSq = 400L; // 20^2

  // Render embers
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int16_t lx = coordsX[i];
    int16_t ly = 255 - coordsY[i]; // flip

    for (uint8_t e = 0; e < FIRE_MAX_EMBERS; e++) {
      if (!embers[e].active) continue;
      int16_t dx = lx - embers[e].x;
      int16_t dy = ly - embers[e].y;
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = scale8((uint8_t)(255L * (proxSq - dSq) / proxSq), embers[e].heat);
        leds[i] += ColorFromPalette(HeatColors_p, embers[e].heat, bri);
      }
    }
  }
}

// Rain (The Matrix) - adapted from Sublime Demos theMatrix()/rain()
// Green drops falling top-to-bottom (decreasing Y) with fading trails.
// Adapted for Fibonacci128 - drops rendered via proximity to scattered LEDs.

#define RAIN_MAX_DROPS 16

struct RainDrop {
  int16_t x, y;
  uint8_t brightness;
  bool active;
};

void sublimeRain()
{
  static RainDrop drops[RAIN_MAX_DROPS];
  static bool rainInit = false;

  if (!rainInit) {
    memset(drops, 0, sizeof(drops));
    rainInit = true;
  }

  fadeToBlackBy(leds, NUM_LEDS, 60);

  // Update existing drops and spawn new ones
  for (uint8_t d = 0; d < RAIN_MAX_DROPS; d++) {
    if (drops[d].active) {
      drops[d].y -= 5; // fall speed
      drops[d].brightness = qsub8(drops[d].brightness, 2); // fade slightly as falling
      if (drops[d].y < 0 || drops[d].brightness < 40) drops[d].active = false;
    } else {
      if (random8() < 40) { // spawn frequency
        drops[d].x = random16(0, 255);
        drops[d].y = 255 + random8(20); // start just above top
        drops[d].brightness = random8(180, 255);
        drops[d].active = true;
      }
    }
  }

  const int32_t proxSq = 225L; // 15^2 - tighter for rain drops

  // Render drops
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int16_t lx = coordsX[i];
    int16_t ly = 255 - coordsY[i]; // flip

    for (uint8_t d = 0; d < RAIN_MAX_DROPS; d++) {
      if (!drops[d].active) continue;
      int16_t dx = lx - drops[d].x;
      int16_t dy = ly - drops[d].y;
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = scale8((uint8_t)(255L * (proxSq - dSq) / proxSq), drops[d].brightness);
        // Matrix green color
        leds[i] += CRGB(0, bri, scale8(bri, 40));
      }
    }
  }
}
