
// Rotating Cube - adapted from Aurora PatternCube
// Original: https://github.com/pixelmatix/aurora
// Copyright (c) 2014 Jason Coon, Noel Bundy, Windell H Oskay
// Adapted for Fibonacci128 radial LED layout - wireframe cube projected onto scattered LEDs.
// Instead of drawing lines on a grid, each LED is colored by its proximity to projected edges.

void cube()
{
  static float Angx = 20.0f, Angy = 10.0f;
  static uint8_t cubeHue = 0;
  static uint8_t cubeStep = 0;

  const float cubeW = 80.0f;       // cube half-width in 3D space
  const float focal = 100.0f;      // camera focal length
  const float Ox = 127.5f, Oy = 127.5f; // center of 0-255 coordinate space

  // Organic rotation speeds via beat functions
  float AngxSpeed = beatsin8(3, 1, 5) / 100.0f;
  float AngySpeed = beatsin8(5, 1, 5, 0, 64) / 100.0f; // phase offset = cosine-like

  float zCamera = (float)beatsin8(2, 160, 200);

  Angx += AngxSpeed;
  Angy += AngySpeed;
  if (Angx >= TWO_PI) Angx -= TWO_PI;
  if (Angy >= TWO_PI) Angy -= TWO_PI;

  // Rotation matrix
  float cosx = cos(Angx), sinx = sin(Angx);
  float cosy = cos(Angy), siny = sin(Angy);

  // 8 cube vertices in local space
  static const float lx[8] = {-1, 1, 1,-1,-1, 1, 1,-1};
  static const float ly[8] = { 1, 1,-1,-1, 1, 1,-1,-1};
  static const float lz[8] = { 1, 1, 1, 1,-1,-1,-1,-1};

  // Project vertices to 2D (integer coords for fast LED distance calc)
  int16_t sxi[8], syi[8];
  for (uint8_t i = 0; i < 8; i++) {
    float vx = lx[i] * cubeW, vy = ly[i] * cubeW, vz = lz[i] * cubeW;
    float ax = cosy * vx + (-siny) * vz;
    float ay = sinx * siny * vx + cosx * vy + sinx * cosy * vz;
    float az = cosx * siny * vx + (-sinx) * vy + cosx * cosy * vz + zCamera;
    sxi[i] = (int16_t)(Ox + focal * ax / az);
    syi[i] = (int16_t)(Oy - focal * ay / az);
  }

  // 12 edges (vertex index pairs)
  static const uint8_t edgeVerts[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0},   // front face
    {4,5}, {5,6}, {6,7}, {7,4},   // back face
    {0,4}, {1,5}, {2,6}, {3,7}    // connecting
  };

  // 6 faces (4 vertex indices each, wound consistently)
  static const uint8_t faceVerts[6][4] = {
    {1,0,3,2}, {0,4,7,3}, {4,0,1,5},
    {4,5,6,7}, {1,2,6,5}, {2,3,7,6}
  };

  // Determine front-facing edges via face normal cross product
  bool edgeVisible[12];
  memset(edgeVisible, 0, sizeof(edgeVisible));

  for (uint8_t f = 0; f < 6; f++) {
    int32_t cross = (int32_t)(sxi[faceVerts[f][1]] - sxi[faceVerts[f][0]]) *
                              (syi[faceVerts[f][2]] - syi[faceVerts[f][0]]) -
                    (int32_t)(syi[faceVerts[f][1]] - syi[faceVerts[f][0]]) *
                              (sxi[faceVerts[f][2]] - sxi[faceVerts[f][0]]);
    if (cross >= 0) { // front-facing
      for (uint8_t ei = 0; ei < 4; ei++) {
        uint8_t v0 = faceVerts[f][ei];
        uint8_t v1 = faceVerts[f][(ei + 1) % 4];
        for (uint8_t e = 0; e < 12; e++) {
          if ((edgeVerts[e][0] == v0 && edgeVerts[e][1] == v1) ||
              (edgeVerts[e][0] == v1 && edgeVerts[e][1] == v0)) {
            edgeVisible[e] = true;
            break;
          }
        }
      }
    }
  }

  fadeToBlackBy(leds, NUM_LEDS, 180);

  const int32_t thresholdSq = 900L; // 30^2 - proximity threshold squared

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int16_t px = coordsX[i];
    int16_t py = coordsY[i];

    int32_t minDistSq = 999999L;
    bool nearestVis = false;

    for (uint8_t e = 0; e < 12; e++) {
      int16_t x1 = sxi[edgeVerts[e][0]], y1 = syi[edgeVerts[e][0]];
      int16_t x2 = sxi[edgeVerts[e][1]], y2 = syi[edgeVerts[e][1]];

      int32_t dx = x2 - x1, dy = y2 - y1;
      int32_t lenSq = dx * dx + dy * dy;

      int32_t closestX, closestY;
      if (lenSq < 1) {
        closestX = x1; closestY = y1;
      } else {
        int32_t dot = (int32_t)(px - x1) * dx + (int32_t)(py - y1) * dy;
        if (dot <= 0) {
          closestX = x1; closestY = y1;
        } else if (dot >= lenSq) {
          closestX = x2; closestY = y2;
        } else {
          closestX = x1 + (dot * dx) / lenSq;
          closestY = y1 + (dot * dy) / lenSq;
        }
      }

      int32_t ex = px - closestX, ey = py - closestY;
      int32_t distSq = ex * ex + ey * ey;

      if (distSq < minDistSq) {
        minDistSq = distSq;
        nearestVis = edgeVisible[e];
      }
    }

    if (minDistSq < thresholdSq) {
      // Quadratic brightness falloff - no sqrt needed
      uint8_t bri = (uint8_t)(255L * (thresholdSq - minDistSq) / thresholdSq);
      if (!nearestVis) bri /= 3; // back-face edges are dimmer
      leds[i] += ColorFromPalette(gCurrentPalette, cubeHue, bri * .5);
      if (nearestVis && leds[i].getLuma() < 64) {
        leds[i] += CHSV(0, 0, 64);
      }
    }
  }

  cubeStep++;
  if (cubeStep >= 8) {
    cubeStep = 0;
    cubeHue += 3;
  }
}
