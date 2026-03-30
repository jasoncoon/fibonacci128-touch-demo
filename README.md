# Fibonacci128 Touch Demo

Arduino firmware for One Inch Fibonacci128 with touch pads

More info: [One Inch Fibonacci128](https://www.evilgeniuslabs.org/one-inch-fibonacci128)

![One Inch Fibonacci128](1if128.png)

### Updating your 1" Fibonacci128:
https://github.com/jasoncoon/fibonacci128-touch-demo/releases

### Dependencies

I developed and tested this sketch with the following board and library versions. The sketch may work with other versions, but these are known to work.

Board: Adafruit QT Py (SAMD21) 
* Adafruit SAMD Boards version 1.7.5
* https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

Libraries:
* FastLED v3.5.0: https://github.com/FastLED/FastLED
* Adafruit FreeTouch Library v1.1.1: https://github.com/adafruit/Adafruit_FreeTouch

### Pattern Catalog (SimplePatternList — 18 patterns)


| # | Function | File | Description |
|---|----------|------|-------------|
| 0 | `colorWavesFibonacci` | Patterns.h | Mark Kriegsman's color waves rendered in Fibonacci spiral order |
| 1 | `prideFibonacci` | Patterns.h | Pride2015 rainbow animation rendered in Fibonacci spiral order |
| 2 | `outwardPalettes` | Patterns.h | Current palette mapped outward along Fibonacci index |
| 3 | `outwardRainbow` | Patterns.h | HSV rainbow mapped outward along Fibonacci index |
| 4 | `cube` | PatternCube.h | Wireframe 3D cube projected onto LEDs with back-face culling and organic rotation |
| 5 | `fibonacciChase` | PatternFibonacciSpiral.h | Bright pulses chase along each Fibonacci spiral arm in sequence |
| 6 | `wave` | PatternWave.h | Quadwave dots along sine curves with random orientation, mirroring, and slow rotation |
| 7 | `sublimeVerticalFire` | PatternSublime.h | Particle-based embers rising bottom-to-top with HeatColors palette |
| 8 | `sublimeRain` | PatternSublime.h | Matrix-style green drops falling top-to-bottom with fading trails |
| 9 | `phyllotaxisBloomAnimation` | PatternPhyllotaxisBloom.h | Golden-angle hue distribution with dual 5-arm/8-arm interference waves drifting at phi-ratio speeds — sunflower parastichy rendered as bioluminescent ripples |
| 10 | `flock` | PatternsAurora.h | Boids flocking simulation with separation/cohesion/alignment + predator |
| 11 | `attract` | PatternsAurora.h | Boids orbiting a gravitational attractor at center with swirling trails |
| 12 | `incrementalDrift` | PatternsAurora.h | Concentric rings of dots orbiting at incrementally different speeds |
| 13 | `incrementalDrift2` | PatternsAurora.h | Two mirrored groups of orbiting dots creating rose/flower patterns |
| 14 | `pendulumWave` | PatternsAurora.h | 32 pendulum dots swinging at incrementally different frequencies |
| 15 | `spiral` | PatternsAurora.h | Two rotating spiral arms with oscillating tightness and width |
| 16 | `swirl` | PatternsAurora.h | Six symmetric bouncing dots with fading trails |
| 17 | `electricMandala` | PatternsAurora.h | Perlin noise with kaleidoscope symmetry (4-fold rotation + diagonal mirror) and slow rotation |

**Additional pattern functions** (defined but not in `patterns[]`):

| # | Function | File | Description |
|---|----------|------|-------------|
| - | `fibonacciSpiral` | PatternFibonacciSpiral.h | Two overlapping Fibonacci spiral arm systems (e.g. 5+8 arms) creating moiré interference with breathing center bloom |
| - | `sublimeFire` | PatternSublime.h | Radial fire simulation — heat rises from edge toward center with angular Perlin noise |
| - | `sublimeJuggle` | PatternSublime.h | Colored dots weaving in/out of sync along radial paths, parameters shift every 10s |
| - | `sublimeBpm` | PatternSublime.h | Radial bands pulsing outward at 62 BPM, colored through current palette |
| - | `sublimeSinelon` | PatternSublime.h | Single colored dot sweeping center-to-edge and back with fading trails |
| - | `radar` | PatternsAurora.h | Sweeping radar arm with long fading trail, colored by radius |
| - | `plasma` | PatternsAurora.h | Classic RGB plasma using cos_wave lookup table with gamma correction |
| - | `rotatingPalettes` | Patterns.h | Current palette mapped by angle (rotating) |
| - | `rotatingRainbow` | Patterns.h | HSV rainbow mapped by angle (rotating) |
| - | `horizontalRainbow` | Patterns.h | HSV rainbow mapped by X coordinate |
| - | `verticalRainbow` | Patterns.h | HSV rainbow mapped by Y coordinate |
| - | `diagonalRainbow` | Patterns.h | HSV rainbow mapped by X+Y |
| - | `colorTest` | Patterns.h | Cycles through solid R/G/B/W/Black every 2s (diagnostic) |

### Key Design Patterns

**Pattern system:** A `patterns[]` array of function pointers (18 entries) drives animation selection. The current index is persisted to EEPROM and advances on each device reset.

**Proximity-based rendering:** Since LEDs are scattered (not on a grid), most patterns work by computing virtual object positions (dots, edges, particles) then lighting each LED based on squared-distance proximity (`dSq < proxSq`). This avoids sqrt and enables soft falloff.

**PatternRotation system:** A reusable struct (`Patterns.h:781`) that adds slow, organic rotation to patterns. Uses sinusoidal angular velocity (speeds up, slows, pauses, reverses) with randomized parameters, refreshed every 8 seconds.

**Touch system:** Three `Adafruit_FreeTouch` sensors (pins A3, A6, A7) are calibrated via min/max arrays and mapped to XY coordinates in 0–255 space. Touch events produce expanding circle overlays via `touchDemo()`. When waves are active, the current pattern is suppressed.

**LED mapping:** Four coordinate arrays in `Map.h` map physical LED indices:
- `coordsX[]`/`coordsY[]` — 2D Cartesian position (0–255)
- `angles[]` — angular position (0–255 = 0°–360°)
- `radius[]` — distance from center (0–255)
- `physicalToFibonacci[]`/`fibonacciToPhysical[]` — bidirectional mapping between physical LED index and Fibonacci spiral order

**Color palettes:** 34 cpt-city gradient palettes in `GradientPalettes.h` are blended smoothly with `nblendPaletteTowardPalette()`. Target palette auto-cycles every 10 seconds.