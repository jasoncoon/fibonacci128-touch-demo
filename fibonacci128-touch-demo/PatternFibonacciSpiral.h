// Fibonacci Spiral Bloom
// A mesmerizing animation that follows the natural Fibonacci spiral arms.
// Two overlapping spiral arm systems (e.g., 5-arm and 8-arm) create organic
// moiré interference patterns. Light pulses travel outward along the golden-angle
// ordering while colors shift through golden-ratio harmonics.
//
// Uses physicalToFibonacci[] to map each LED to its position in the Fibonacci
// sequence, revealing the spiral structure inherent in the hardware layout.

void fibonacciSpiral()
{
  // Pairs of Fibonacci spiral arm counts that create beautiful interference
  static const uint8_t armPairs[][2] = {{5, 8}, {8, 13}, {3, 5}, {5, 13}, {3, 8}, {8, 21}};
  static uint8_t pairIdx = 0;
  static uint8_t bloomPhase = 0;

  EVERY_N_SECONDS(8) {
    pairIdx = (pairIdx + 1) % 6;
  }

  fadeToBlackBy(leds, NUM_LEDS, 30);

  uint8_t arms1 = armPairs[pairIdx][0];
  uint8_t arms2 = armPairs[pairIdx][1];

  uint32_t ms = millis();
  uint8_t timeA = ms / 11;  // outward pulse phase
  uint8_t timeB = ms / 17;  // counter-pulse phase
  uint8_t timeHue = ms / 37; // slow color rotation

  // Breathing modulation - center brightens and dims organically
  uint8_t breath = beatsin8(6, 60, 255);

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t fibIdx = physicalToFibonacci[i];

    // Normalized position along spiral (0=center, 255=edge)
    uint8_t spiralPos = fibIdx * 2; // 0..254, wraps are fine for wave math

    // --- System 1: outward-traveling pulse along arms1 spiral arms ---
    uint8_t arm1Id = fibIdx % arms1;
    // Phase offset per arm so pulses are staggered
    uint8_t arm1Phase = arm1Id * (256 / arms1);
    uint8_t wave1 = sin8(spiralPos * 3 - timeA + arm1Phase);
    // Threshold to create distinct pulse peaks rather than uniform wash
    uint8_t bri1 = (wave1 > 140) ? (wave1 - 140) * 2 + 25 : 0;
    // Each arm has a distinct hue, golden-ratio spaced and slowly rotating
    uint8_t hue1 = arm1Phase + timeHue + (spiralPos >> 2);

    // --- System 2: counter-rotating pulse along arms2 spiral arms ---
    uint8_t arm2Id = fibIdx % arms2;
    uint8_t arm2Phase = arm2Id * (256 / arms2);
    uint8_t wave2 = sin8(spiralPos * 2 + timeB + arm2Phase);
    uint8_t bri2 = (wave2 > 150) ? (wave2 - 150) * 2 + 20 : 0;
    uint8_t hue2 = arm2Phase - timeHue + 128 + (spiralPos >> 3);

    // --- Breathing center bloom ---
    // LEDs closer to center (low radius) pulse with the breath
    uint8_t centerGlow = 0;
    if (radius[i] < 80) {
      uint8_t centerBri = scale8(80 - radius[i], breath);
      centerGlow = scale8(centerBri, 3); // gentle glow
      // Center bloom hue follows golden ratio offset from main
      CRGB bloom = CHSV(timeHue * 2 + 64, 200, centerGlow);
      leds[i] += bloom;
    }

    // --- Combine the two spiral systems ---
    CRGB c1 = CHSV(hue1, 240, bri1);
    CRGB c2 = CHSV(hue2, 220, bri2);
    leds[i] += c1;
    leds[i] += c2;
  }

  bloomPhase++;
}

// Fibonacci Spiral Chase - a simpler variant
// A single bright pulse chases along each spiral arm in sequence,
// creating an unmistakable spiral motion effect.

void fibonacciChase()
{
  static uint8_t chaseArms = 5;
  static uint8_t chaseHueBase = 0;

  EVERY_N_SECONDS(12) {
    // Cycle through Fibonacci arm counts
    static const uint8_t armOptions[] = {3, 5, 8, 13};
    static uint8_t armIdx = 0;
    armIdx = (armIdx + 1) % 4;
    chaseArms = armOptions[armIdx];
  }

  fadeToBlackBy(leds, NUM_LEDS, 80);

  // uint16_t ms = millis();
  uint8_t maxArmLen = 128 / chaseArms;

  for (uint8_t arm = 0; arm < chaseArms; arm++) {
    // Each arm's bright dot position oscillates along the arm length
    // Stagger the phase per arm so they chase in sequence
    uint8_t armPhaseOffset = arm * (256 / chaseArms);
    uint8_t dotPos = beatsin8(20, 0, maxArmLen - 1, 0, armPhaseOffset);

    // Walk through all LEDs on this arm
    for (uint8_t pos = 0; pos < maxArmLen; pos++) {
      uint8_t fibIdx = arm + pos * chaseArms;
      if (fibIdx >= 128) break;
      uint8_t physIdx = fibonacciToPhysical[fibIdx];

      // Distance from the bright dot
      int8_t dist = (int8_t)pos - (int8_t)dotPos;
      if (dist < 0) dist = -dist;

      if (dist < 5) {
        uint8_t bri = 255 - dist * 50;
        uint8_t hueVal = chaseHueBase + arm * (256 / chaseArms) + pos * 4;
        leds[physIdx] += ColorFromPalette(gCurrentPalette, hueVal, bri);
      }
    }
  }

  EVERY_N_MILLISECONDS(20) { chaseHueBase++; }
}
