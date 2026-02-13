
// Flocking Animation - adapted from Aurora PatternFlock
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Daniel Shiffman
// Based on Craig Reynolds' "Flocking" behavior (Separation, Cohesion, Alignment)
// Adapted for Fibonacci128 - boids simulated in 0-255 space, LEDs lit by proximity.

struct FVec {
  float x, y;
  FVec() : x(0), y(0) {}
  FVec(float _x, float _y) : x(_x), y(_y) {}
  FVec operator+(const FVec& v) const { return FVec(x + v.x, y + v.y); }
  FVec operator-(const FVec& v) const { return FVec(x - v.x, y - v.y); }
  FVec& operator+=(const FVec& v) { x += v.x; y += v.y; return *this; }
  FVec& operator-=(const FVec& v) { x -= v.x; y -= v.y; return *this; }
  FVec operator*(float s) const { return FVec(x * s, y * s); }
  FVec& operator*=(float s) { x *= s; y *= s; return *this; }
  FVec& operator/=(float s) { x /= s; y /= s; return *this; }
  float mag() const { return sqrtf(x * x + y * y); }
  float magSq() const { return x * x + y * y; }
  float dist(const FVec& v) const { float dx = v.x - x, dy = v.y - y; return sqrtf(dx * dx + dy * dy); }
  FVec& normalize() { float m = mag(); if (m > 0) { x /= m; y /= m; } return *this; }
  void limit(float max) { if (magSq() > max * max) { normalize(); *this *= max; } }
};

#define FLOCK_COUNT 8

struct FBoid {
  FVec loc, vel, acc;
  float maxspeed, maxforce, desiredsep, neighbordist;
  bool enabled;

  void init(float x, float y, float ms, float mf, float ds, float nd) {
    acc = FVec(0, 0);
    vel = FVec((random(256) - 128) / 256.0f, (random(256) - 128) / 256.0f);
    loc = FVec(x, y);
    maxspeed = ms; maxforce = mf;
    desiredsep = ds; neighbordist = nd;
    enabled = true;
  }

  void applyForce(FVec f) { acc += f; }

  void update() {
    vel += acc;
    vel.limit(maxspeed);
    loc += vel;
    acc *= 0;
  }

  void wrap() {
    if (loc.x < 0) loc.x += 256;
    if (loc.y < 0) loc.y += 256;
    if (loc.x >= 256) loc.x -= 256;
    if (loc.y >= 256) loc.y -= 256;
  }

  void repelFrom(FVec obstacle, float radius) {
    FVec futPos = loc + vel;
    FVec d = obstacle - futPos;
    if (d.mag() <= radius) {
      FVec repel = loc - obstacle;
      repel.normalize();
      repel *= (maxforce * 7);
      applyForce(repel);
    }
  }

  FVec seek(FVec target) {
    FVec desired = target - loc;
    desired.normalize();
    desired *= maxspeed;
    FVec steer = desired - vel;
    steer.limit(maxforce);
    return steer;
  }

  // Combined separation, alignment, cohesion in one pass for efficiency
  void flock(FBoid boids[], uint8_t count) {
    FVec sep(0, 0), ali(0, 0), coh(0, 0);
    int sepCount = 0, flockCount = 0;

    for (uint8_t i = 0; i < count; i++) {
      if (!boids[i].enabled) continue;
      float d = loc.dist(boids[i].loc);
      if (d > 0 && d < desiredsep) {
        FVec diff = loc - boids[i].loc;
        diff.normalize();
        diff /= d;
        sep += diff;
        sepCount++;
      }
      if (d > 0 && d < neighbordist) {
        ali += boids[i].vel;
        coh += boids[i].loc;
        flockCount++;
      }
    }

    if (sepCount > 0) {
      sep /= (float)sepCount;
      if (sep.mag() > 0) { sep.normalize(); sep *= maxspeed; sep -= vel; sep.limit(maxforce); }
    }
    if (flockCount > 0) {
      ali /= (float)flockCount;
      ali.normalize(); ali *= maxspeed;
      FVec aliSteer = ali - vel; aliSteer.limit(maxforce); ali = aliSteer;
      coh /= (float)flockCount;
      coh = seek(coh);
    }

    sep *= 1.5f;
    applyForce(sep);
    applyForce(ali);
    applyForce(coh);
  }

  void run(FBoid boids[], uint8_t count) {
    flock(boids, count);
    update();
  }
};

static FBoid flockBoids[FLOCK_COUNT];
static FBoid flockPredator;
static bool flockInitialized = false;

void flock()
{
  // Initialize boids on first call
  if (!flockInitialized) {
    for (uint8_t i = 0; i < FLOCK_COUNT; i++) {
      // Speeds/distances scaled for 256x256 space (original was ~32x32)
      flockBoids[i].init(128, 128, 3.0f, 0.12f, 32.0f, 64.0f);
    }
    flockPredator.init(64, 64, 3.08f, 0.16f, 0.0f, 128.0f);
    flockInitialized = true;
  }

  fadeToBlackBy(leds, NUM_LEDS, 40); // equivalent to DimAll(230)

  static uint8_t flockHue = 0;
  EVERY_N_MILLIS(200) { flockHue++; }

  // Random wind gusts
  bool applyWind = random(256) > 250;
  FVec wind(0, 0);
  if (applyWind) {
    wind.x = (random(256) - 128) / 256.0f * 0.12f;
    wind.y = (random(256) - 128) / 256.0f * 0.12f;
  }

  // Simulate boids
  CRGB boidColor = ColorFromPalette(gCurrentPalette, flockHue);
  for (uint8_t b = 0; b < FLOCK_COUNT; b++) {
    flockBoids[b].repelFrom(flockPredator.loc, 80.0f);
    flockBoids[b].run(flockBoids, FLOCK_COUNT);
    flockBoids[b].wrap();
    if (applyWind) {
      flockBoids[b].applyForce(wind);
      applyWind = false;
    }
  }

  // Simulate predator
  flockPredator.run(flockBoids, FLOCK_COUNT);
  flockPredator.wrap();
  CRGB predColor = ColorFromPalette(gCurrentPalette, flockHue + 128);

  // Convert boid positions to integers for fast proximity check
  int16_t bxi[FLOCK_COUNT], byi[FLOCK_COUNT];
  for (uint8_t b = 0; b < FLOCK_COUNT; b++) {
    bxi[b] = (int16_t)flockBoids[b].loc.x;
    byi[b] = (int16_t)flockBoids[b].loc.y;
  }
  int16_t pxi = (int16_t)flockPredator.loc.x;
  int16_t pyi = (int16_t)flockPredator.loc.y;

  const int32_t proxSq = 400L; // 20^2 proximity threshold

  // Light LEDs near boids (single pass over LEDs for all boids)
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int16_t lx = coordsX[i];
    int16_t ly = coordsY[i];

    for (uint8_t b = 0; b < FLOCK_COUNT; b++) {
      int16_t dx = lx - bxi[b];
      int16_t dy = ly - byi[b];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[i] += CRGB(scale8(boidColor.r, bri), scale8(boidColor.g, bri), scale8(boidColor.b, bri));
      }
    }

    // Predator in contrasting color
    int16_t dx = lx - pxi;
    int16_t dy = ly - pyi;
    int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
    if (dSq < proxSq) {
      uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
      leds[i] += CRGB(scale8(predColor.r, bri), scale8(predColor.g, bri), scale8(predColor.b, bri));
    }
  }
}

// Attract - adapted from Aurora PatternAttract
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Daniel Shiffman (Nature of Code)
// Boids orbit a gravitational attractor at the center, creating swirling trails.
// Adapted for Fibonacci128 - reuses FVec/FBoid, proximity-based LED rendering.

#define ATTRACT_COUNT 10

void attract()
{
  static FBoid attractBoids[ATTRACT_COUNT];
  static uint8_t attractColors[ATTRACT_COUNT];
  static bool attractInit = false;

  if (!attractInit) {
    int dir = random(0, 2) == 0 ? -1 : 1;
    for (uint8_t i = 0; i < ATTRACT_COUNT; i++) {
      attractBoids[i].acc = FVec(0, 0);
      // Start in a vertical column near center (scaled for 0-255 space)
      attractBoids[i].loc = FVec(127, 255 - i * (256 / ATTRACT_COUNT));
      // Horizontal velocity, random magnitude, all same direction (scaled 8x from original)
      attractBoids[i].vel = FVec(dir * (1.6f + random(0, 48) / 10.0f), 0);
      attractBoids[i].maxspeed = 12.0f;  // 1.5 * 8
      attractBoids[i].maxforce = 10.0f;
      attractBoids[i].desiredsep = 0;
      attractBoids[i].neighbordist = 0;
      attractBoids[i].enabled = true;
      attractColors[i] = i * (240 / ATTRACT_COUNT);
    }
    attractInit = true;
  }

  // Oscillating dim for trailing effect (matches original)
  uint8_t dim = beatsin8(2, 150, 230);
  fadeToBlackBy(leds, NUM_LEDS, 255 - dim);

  // Gravitational attractor at center
  // G=0.5, M=10 in original 32x32 space; scaled by 512 for 256x256 space
  const FVec attractorLoc(128, 128);
  const float GM = 2560.0f;

  // Precompute boid colors and integer positions
  CRGB boidColors[ATTRACT_COUNT];
  int16_t bxi[ATTRACT_COUNT], byi[ATTRACT_COUNT];

  for (uint8_t i = 0; i < ATTRACT_COUNT; i++) {
    // Gravitational force toward center
    FVec force = attractorLoc - attractBoids[i].loc;
    float d = force.mag();
    if (d < 40.0f) d = 40.0f;   // min clamp (5 * 8)
    if (d > 256.0f) d = 256.0f; // max clamp (32 * 8)
    force.normalize();
    force *= GM / (d * d);

    attractBoids[i].applyForce(force);
    attractBoids[i].update();

    bxi[i] = (int16_t)attractBoids[i].loc.x;
    byi[i] = (int16_t)attractBoids[i].loc.y;
    boidColors[i] = ColorFromPalette(gCurrentPalette, attractColors[i]);
  }

  const int32_t proxSq = 400L; // 20^2

  // Single pass over LEDs for all boids
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    int16_t lx = coordsX[j];
    int16_t ly = coordsY[j];

    for (uint8_t i = 0; i < ATTRACT_COUNT; i++) {
      int16_t dx = lx - bxi[i];
      int16_t dy = ly - byi[i];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[j] += CRGB(scale8(boidColors[i].r, bri), scale8(boidColors[i].g, bri), scale8(boidColors[i].b, bri));
      }
    }
  }
}

// Incremental Drift - adapted from Aurora PatternIncrementalDrift
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon
// Concentric rings of dots orbiting at incrementally different speeds.
// Adapted for Fibonacci128 - orbits scaled to 0-255 space, proximity-based LED rendering.

// beatcos8: like beatsin8 but cosine (not in standard FastLED)
uint8_t beatcos8(accum88 bpm, uint8_t lo, uint8_t hi, uint32_t tb = 0, uint8_t po = 0) {
  uint8_t beat = beat8(bpm, tb);
  uint8_t bc = cos8(beat + po);
  uint8_t rng = hi - lo;
  return lo + scale8(bc, rng);
}

#define DRIFT_RINGS 16

void incrementalDrift()
{
  uint8_t dim = beatsin8(2, 210, 230);
  fadeToBlackBy(leds, NUM_LEDS, 255 - dim);

  // Precompute dot positions and colors for all rings
  int16_t dotX[DRIFT_RINGS - 1], dotY[DRIFT_RINGS - 1];
  CRGB ringColor[DRIFT_RINGS - 1];
  uint8_t numRings = 0;

  for (int i = 2; i <= DRIFT_RINGS; i++) {
    ringColor[numRings] = ColorFromPalette(gCurrentPalette, (i - 2) * (240 / DRIFT_RINGS));
    uint8_t ri = i * 8; // orbit radius scaled for 0-255 space
    uint8_t speed = (DRIFT_RINGS + 1 - i) * 2; // inner rings orbit faster
    dotX[numRings] = (int16_t)beatcos8(speed, 128 - ri, 128 + ri);
    dotY[numRings] = (int16_t)beatsin8(speed, 128 - ri, 128 + ri);
    numRings++;
  }

  const int32_t proxSq = 400L; // 20^2

  // Single pass over LEDs, check proximity to all orbiting dots
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    int16_t lx = coordsX[j];
    int16_t ly = coordsY[j];

    for (uint8_t r = 0; r < numRings; r++) {
      int16_t dx = lx - dotX[r];
      int16_t dy = ly - dotY[r];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[j] += CRGB(scale8(ringColor[r].r, bri), scale8(ringColor[r].g, bri), scale8(ringColor[r].b, bri));
      }
    }
  }
}

// Incremental Drift Rose - adapted from Aurora PatternIncrementalDrift2
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon
// Two mirrored groups of orbiting dots creating a rose/flower pattern.
// Adapted for Fibonacci128 - orbits scaled to 0-255 space, proximity-based LED rendering.

#define DRIFT2_COUNT 32

void incrementalDrift2()
{
  uint8_t dim = beatsin8(2, 150, 230);
  fadeToBlackBy(leds, NUM_LEDS, 255 - dim);

  // Precompute dot positions and colors
  int16_t dotX[DRIFT2_COUNT], dotY[DRIFT2_COUNT];
  CRGB dotColor[DRIFT2_COUNT];

  for (uint16_t i = 0; i < DRIFT2_COUNT; i++) {
    uint8_t lo, hi;

    if (i < DRIFT2_COUNT / 2) {
      lo = i * 8;
      hi = min(255, (DRIFT2_COUNT - i) * 8);
      dotX[i] = (int16_t)beatcos8((i + 1) * 2, lo, hi);
      dotY[i] = (int16_t)beatsin8((i + 1) * 2, lo, hi);
      dotColor[i] = ColorFromPalette(gCurrentPalette, i * 14);
    } else {
      lo = (DRIFT2_COUNT - i) * 8;
      hi = min(255, (i + 1) * 8);
      dotX[i] = (int16_t)beatsin8((DRIFT2_COUNT - i) * 2, lo, hi);
      dotY[i] = (int16_t)beatcos8((DRIFT2_COUNT - i) * 2, lo, hi);
      dotColor[i] = ColorFromPalette(gCurrentPalette, (DRIFT2_COUNT - 1 - i) * 14);
    }
  }

  const int32_t proxSq = 400L; // 20^2

  // Single pass over LEDs, check proximity to all orbiting dots
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    int16_t lx = coordsX[j];
    int16_t ly = coordsY[j];

    for (uint8_t d = 0; d < DRIFT2_COUNT; d++) {
      int16_t dx = lx - dotX[d];
      int16_t dy = ly - dotY[d];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[j] += CRGB(scale8(dotColor[d].r, bri), scale8(dotColor[d].g, bri), scale8(dotColor[d].b, bri));
      }
    }
  }
}

// Pendulum Wave - adapted from Aurora PatternPendulumWave
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Lunarch Studios (CC0 1.0)
// Dots at evenly spaced positions swing at incrementally different frequencies.
// Adapted for Fibonacci128 - 32 pendulums across x-axis, proximity-based LED rendering.

#define PENDULUM_COUNT 32

void pendulumWave()
{
  fadeToBlackBy(leds, NUM_LEDS, 85); // DimAll(170)

  // Precompute pendulum positions and colors
  int16_t penX[PENDULUM_COUNT], penY[PENDULUM_COUNT];
  CRGB penColor[PENDULUM_COUNT];

  for (uint8_t i = 0; i < PENDULUM_COUNT; i++) {
    penX[i] = i * 8; // evenly spaced across 0-248
    penY[i] = beatsin16(i + 1, 0, 255); // each swings at frequency i+1 BPM
    penColor[i] = ColorFromPalette(gCurrentPalette, i * 7);
  }

  const int32_t proxSq = 400L; // 20^2

  // Single pass over LEDs
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    int16_t lx = coordsX[j];
    int16_t ly = coordsY[j];

    for (uint8_t p = 0; p < PENDULUM_COUNT; p++) {
      int16_t dx = lx - penX[p];
      int16_t dy = ly - penY[p];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[j] += CRGB(scale8(penColor[p].r, bri), scale8(penColor[p].g, bri), scale8(penColor[p].b, bri));
      }
    }
  }
}

// Radar - adapted from Aurora PatternRadar
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon
// Sweeping radar arm with fading trail, colored by radius through the palette.
// Adapted for Fibonacci128 - uses angles[] array for natural radial sweep.

void radar()
{
  static byte radarTheta = 0;
  static byte radarHueOffset = 0;

  fadeToBlackBy(leds, NUM_LEDS, 3); // very slow fade for long radar trail

  EVERY_N_MILLIS(25) {
    radarTheta += 2;
    radarHueOffset += 1;
  }

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Shortest angular distance from sweep line (0-128)
    int16_t diff = (int16_t)angles[i] - (int16_t)radarTheta;
    if (diff < 0) diff += 256;
    if (diff > 128) diff = 256 - diff;

    if (diff < 8) { // narrow sweep arc (~11 degrees)
      uint8_t bri = 255 - diff * 32;
      // Color shifts with radius and time, matching original's per-ring hue offset
      byte colorIdx = 255 - (radius[i] + radarHueOffset);
      CRGB color = ColorFromPalette(gCurrentPalette, colorIdx);
      leds[i] += CRGB(scale8(color.r, bri), scale8(color.g, bri), scale8(color.b, bri));
    }
  }
}

// Spiral - adapted from Aurora PatternSpiral
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Stefan Petrick (Funky Clouds)
// Rotating spiral arms with oscillating tightness and width.
// Adapted for Fibonacci128 - uses angles[]/radius[] for natural spiral rendering.

void spiral()
{
  fadeToBlackBy(leds, NUM_LEDS, 40); // DimAll(224)

  // Oscillating parameters for organic movement (inspired by original's 5 oscillators)
  uint8_t baseAngle = beat8(7);            // rotation speed
  uint8_t tightness = beatsin8(3, 1, 3);   // spiral tightness oscillates
  uint8_t colorPhase = beat8(11);           // color cycling
  uint8_t armWidth = beatsin8(5, 10, 20);  // arm width breathes

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t r = radius[i];
    uint8_t a = angles[i];

    // Two spiral arms, 180 degrees apart
    for (uint8_t arm = 0; arm < 2; arm++) {
      uint8_t spiralAngle = baseAngle + r * tightness + arm * 128;

      // Shortest angular distance
      int16_t diff = (int16_t)a - (int16_t)spiralAngle;
      if (diff < 0) diff += 256;
      if (diff > 128) diff = 256 - diff;

      if (diff < armWidth) {
        uint8_t bri = (uint8_t)(255L * (armWidth - diff) / armWidth);
        byte colorIdx = r + colorPhase + arm * 80;
        CRGB color = ColorFromPalette(gCurrentPalette, colorIdx);
        leds[i] += CRGB(scale8(color.r, bri), scale8(color.g, bri), scale8(color.b, bri));
      }
    }
  }
}

// Swirl - adapted from Aurora PatternSwirl
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Mark Kriegsman (SmartMatrixSwirl)
// Six symmetric bouncing dots with fading trails.
// Adapted for Fibonacci128 - proximity-based LED rendering replaces blur2d.

#define SWIRL_DOTS 6

void swirl()
{
  // Oscillating fade replaces blur2d (lossy blur trends toward black)
  uint8_t blurAmount = beatsin8(2, 10, 128);
  fadeToBlackBy(leds, NUM_LEDS, 256 - blurAmount);

  // Two out-of-sync sine waves for base positions (scaled to 0-255)
  uint8_t si = beatsin8(27, 16, 240);
  uint8_t sj = beatsin8(41, 16, 240);
  // Reflections
  uint8_t ni = 255 - si;
  uint8_t nj = 255 - sj;

  // 6 symmetric dot positions
  int16_t dotPosX[SWIRL_DOTS] = { si, sj, ni, nj, si, ni };
  int16_t dotPosY[SWIRL_DOTS] = { sj, si, nj, ni, nj, sj };

  // Each dot shifts color at a different rate
  uint16_t ms = millis();
  uint8_t colorIdx[SWIRL_DOTS];
  colorIdx[0] = ms / 11;
  colorIdx[1] = ms / 13;
  colorIdx[2] = ms / 17;
  colorIdx[3] = ms / 29;
  colorIdx[4] = ms / 37;
  colorIdx[5] = ms / 41;

  CRGB dotColors[SWIRL_DOTS];
  for (uint8_t d = 0; d < SWIRL_DOTS; d++) {
    dotColors[d] = ColorFromPalette(gCurrentPalette, colorIdx[d]);
  }

  const int32_t proxSq = 400L; // 20^2

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int16_t lx = coordsX[i];
    int16_t ly = coordsY[i];

    for (uint8_t d = 0; d < SWIRL_DOTS; d++) {
      int16_t dx = lx - dotPosX[d];
      int16_t dy = ly - dotPosY[d];
      int32_t dSq = (int32_t)dx * dx + (int32_t)dy * dy;
      if (dSq < proxSq) {
        uint8_t bri = (uint8_t)(255L * (proxSq - dSq) / proxSq);
        leds[i] += CRGB(scale8(dotColors[d].r, bri), scale8(dotColors[d].g, bri), scale8(dotColors[d].b, bri));
      }
    }
  }
}

// Electric Mandala - adapted from Aurora PatternElectricMandala
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Stefan Petrick (FunkyNoise)
// Perlin noise with kaleidoscope symmetry (diagonal mirror + 4-fold rotation).
// Adapted for Fibonacci128 - symmetry applied as coordinate folds before noise lookup.

void electricMandala()
{
  static uint32_t emNoiseX, emNoiseY, emNoiseZ;
  static uint16_t emScaleX = 6000, emScaleY = 6000;
  static int16_t emDx = 0, emDy = 0, emDz = 0;
  static bool emInitialized = false;
  static uint8_t emSmooth[NUM_LEDS];
  static PatternRotation emRot = {0, 0, 0, 0, false};

  if (!emInitialized) {
    emNoiseX = random16();
    emNoiseY = random16();
    emNoiseZ = random16();
    emDx = random8();
    emDy = random8();
    emDz = random8();
    memset(emSmooth, 128, NUM_LEDS);
    emInitialized = true;
  }

  // Randomize noise drift and scale every 5 seconds
  EVERY_N_SECONDS(5) {
    emDx = random16(500) - 250;
    emDy = random16(500) - 250;
    emDz = random16(500) - 250;
    emScaleX = random16(10000) + 2000;
    emScaleY = random16(10000) + 2000;
  }

  // Re-randomize rotation speeds every 8 seconds
  EVERY_N_SECONDS(8) {
    emRot.randomizeSpeeds();
  }

  emNoiseX += emDx;
  emNoiseY += emDy;
  emNoiseZ += emDz;

  // Update rotation (sinusoidal speed: speeds up, slows, pauses, reverses)
  float emAngle = emRot.update();
  float emCosA = cosf(emAngle), emSinA = sinf(emAngle);

  const uint8_t smoothing = 200;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Rotate coordinates around center before kaleidoscope folds
    float fx = (float)coordsX[i] - 128.0f;
    float fy = (float)coordsY[i] - 128.0f;
    int16_t rx = (int16_t)(128.0f + emCosA * fx - emSinA * fy);
    int16_t ry = (int16_t)(128.0f + emSinA * fx + emCosA * fy);

    // Distance from center — folds all quadrants (Caleidoscope1)
    int16_t dx = abs(rx - 128);
    int16_t dy = abs(ry - 128);

    // Diagonal mirror (Caleidoscope3): ensures noise[x][y] == noise[y][x]
    int16_t kx = dx < dy ? dx : dy;
    int16_t ky = dx < dy ? dy : dx;

    // Scale to match original ~16-pixel matrix half-width
    kx >>= 3;
    ky >>= 3;

    uint32_t ioffset = (uint32_t)emScaleX * kx;
    uint32_t joffset = (uint32_t)emScaleY * ky;
    uint8_t data = inoise16(emNoiseX + ioffset, emNoiseY + joffset, emNoiseZ) >> 8;

    // Temporal smoothing (200/256 old + 56/256 new)
    uint8_t olddata = emSmooth[i];
    data = scale8(olddata, smoothing) + scale8(data, 256 - smoothing);
    emSmooth[i] = data;

    // Noise value used as both palette index and brightness (matches original)
    leds[i] = ColorFromPalette(gCurrentPalette, data, data);
  }
}

// Aurora Plasma - adapted from PatternPlasma by Robert Atkins / Jason Coon
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, (c) 2013 Robert Atkins
// Overlapping sin/cos waves colored through the current palette.
// Adapted for Fibonacci128 - iterates over LED coordinates instead of grid.

void plasma()
{
  static int auroraPlasmaTime = 0;
  static int auroraPlasmaFrames = 0;
  static PatternRotation apRot = {0, 0, 0, 0, false};

  // Re-randomize rotation speeds every 8 seconds
  EVERY_N_SECONDS(8) {
    apRot.randomizeSpeeds();
  }

  // Update rotation (sinusoidal speed: speeds up, slows, pauses, reverses)
  float apAngle = apRot.update();
  float apCosA = cosf(apAngle), apSinA = sinf(apAngle);

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Rotate coordinates around center before plasma calculation
    float fx = (float)coordsX[i] - 128.0f;
    float fy = (float)coordsY[i] - 128.0f;
    int16_t rx = (int16_t)(128.0f + apCosA * fx - apSinA * fy);
    int16_t ry = (int16_t)(128.0f + apSinA * fx + apCosA * fy);

    // Scale rotated coords to ~0-31 range to match original matrix dimensions
    uint8_t x = (uint8_t)constrain(rx, 0, 255) >> 3;
    uint8_t y = (uint8_t)constrain(ry, 0, 255) >> 3;

    int16_t v = 0;
    uint8_t wibble = sin8(auroraPlasmaTime);
    v += sin16(x * wibble * 2 + auroraPlasmaTime);
    v += cos16(y * (128 - wibble) * 2 + auroraPlasmaTime);
    v += sin16(y * x * cos8(-auroraPlasmaTime) / 2);

    leds[i] = ColorFromPalette(gCurrentPalette, (v >> 8) + 127);
  }

  auroraPlasmaTime += 1;
  auroraPlasmaFrames++;

  if (auroraPlasmaFrames >= 2048) {
    auroraPlasmaTime = 0;
    auroraPlasmaFrames = 0;
  }
}
