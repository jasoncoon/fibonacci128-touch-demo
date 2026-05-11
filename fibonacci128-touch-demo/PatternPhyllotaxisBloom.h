// Phyllotaxis Bloom - inspired by the golden angle arrangement of seeds in sunflowers.
//
// Each LED is colored using the golden angle (137.5°) in hue space, exactly as
// sunflower seeds are arranged 137.5° apart in physical space. This spacing is
// irrational relative to 360°, so colors never cluster or repeat — producing the
// maximally uniform, non-repeating distribution seen in nature.
//
// Two interference waves (5-arm and 8-arm — consecutive Fibonacci numbers) travel
// along the spiral arms at phi-ratio speeds (time divisors 8 and 13 — also
// consecutive Fibonacci numbers). They never phase-lock, producing ~40 drifting
// bright nodes where waves reinforce — like bioluminescent ripples on a living spiral.
//
// The 5×8 arm counts are exactly the parastichy numbers you count on a sunflower.

#ifndef PatternPhyllotaxisBloom_H
#define PatternPhyllotaxisBloom_H


void phyllotaxisBloomAnimation()
{
  uint32_t ms = millis();

  // Phi-ratio time bases: consecutive Fibonacci divisors (8, 13) keep the two
  // waves drifting at ~phi relative speed — they never lock into a repeating pattern.
  uint8_t timeA   = (uint8_t)(ms / 8);   // 5-arm wave: travels outward
  uint8_t timeB   = (uint8_t)(ms / 13);  // 8-arm wave: travels inward (phi-ratio slower)
  uint8_t timeHue = (uint8_t)(ms / 80);  // hue rotation: ~20s full spectrum cycle

  // Golden angle in 0-255 hue space: 137.508/360 * 256 = 97.8 → 98.
  // fibIdx * 98 places each successive Fibonacci LED 137.5° away in hue,
  // making 5-arm and 8-arm spiral arms visible as distinct color bands.
  const uint8_t GOLDEN_ANGLE_8 = 98;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t fibIdx = physicalToFibonacci[i];

    // Hue: golden-angle distribution reveals the spiral arm structure as color bands.
    // 5 Fibonacci steps ≈ 5×137.5° = 687.5° ≡ 327.5° — one full lap minus a small gap,
    // so every 5th LED is nearly the same hue, tracing the 5 spiral arms.
    // Similarly for 8 arms. Exactly the parastichy visible in a sunflower.
    uint8_t hue = (uint8_t)(fibIdx * GOLDEN_ANGLE_8) + timeHue;

    // 5-arm spiral wave: spatial frequency 5 places one crest per spiral arm.
    // All 5 arms brighten and dim together as timeA advances outward.
    uint8_t wave1 = sin8(fibIdx * 5 - timeA);

    // 8-arm counter-wave: travels inward at phi-ratio slower speed.
    // Spatial freq 8 = one crest per 8-arm spiral arm.
    uint8_t wave2 = sin8(fibIdx * 8 + timeB);

    // Multiplicative interference: bright only where both waves crest simultaneously.
    // Produces ~40 drifting nodes (5×8 intersections) that travel along the arms —
    // like the growing tip of each spiral arm lighting up in sequence.
    uint8_t bri = scale8(wave1, wave2);

    // Soft glow floor: dark regions stay gently lit, like a starfield behind the bloom.
    bri = qadd8(bri, 15);

    // Saturation dip at brightness peaks: white-tipped "petal" effect.
    // Bright nodes bloom toward white before fading back to saturated color.
    uint8_t sat = 255 - scale8(bri, 80);

    leds[i] = CHSV(hue, sat, bri);
  }
}

#endif
