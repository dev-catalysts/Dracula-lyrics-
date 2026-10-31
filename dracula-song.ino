/*
 * =========================================================================
 *  Dracula - synchronized lyric visualizer for ESP32 + SSD1306 OLED
 * =========================================================================
 *
 *  Original sketch credit: CircuitForge / @Afxlabz ("GOD MODE" upgrade)
 *  Song timing/lyrics: see lyrics.h
 *
 *  ANIMATIONS
 *    1. Blood Drip Portal     - rings + vertical drip columns
 *    2. Hypno Spiral          - multi-arm Archimedean spiral pulse
 *    3. Star Warp             - hyperspace jump star-streaks
 *    4. Plasma Tunnel + EQ    - rotating tunnel + live EQ bars
 *    5. DNA Helix             - dual sine-wave strand with nodes
 *    6. Particle Explosion    - point-burst with gravity trails
 *    7. Glitch Scanlines      - CRT corruption + horizontal tears
 *    8. Tesla Coil Arc        - radial arc discharges from centre
 *    9. Mobius Strip Rings    - layered concentric rotating ellipses
 *
 *  LED accent panel: a unique reactive pattern for every mode.
 *  Boot sequence: triple ping-pong chase + surge.
 *
 *  HARDWARE
 *    Board : ESP32 Dev Module (38-pin)
 *    OLED  : SSD1306 128x64 I2C
 *            SCL -> GPIO 18 | SDA -> GPIO 19 | VCC -> 3.3V | GND -> GND
 *    LEDs  : 4x single LED, each with a 220 ohm resistor to GND
 *            LED1 -> GPIO 25 | LED2 -> GPIO 26
 *            LED3 -> GPIO 32 | LED4 -> GPIO 33
 *
 *  Requires ESP32 Arduino Core 3.x (analogWrite support)
 *  Libraries: Adafruit GFX Library, Adafruit SSD1306
 * =========================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <string.h>

#include "lyrics.h"   // Effect enum, Lyric struct and the lyric timeline

// =========================================================================
//  DISPLAY
// =========================================================================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_SDA_PIN   19
#define OLED_SCL_PIN   18
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define CX  (SCREEN_WIDTH  / 2)
#define CY  (SCREEN_HEIGHT / 2)

// =========================================================================
//  LED PINS
// =========================================================================
#define LED1_PIN 25
#define LED2_PIN 26
#define LED3_PIN 32
#define LED4_PIN 33
static const uint8_t LED_PINS[4] = { LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN };

int   ledCurrent[4]  = {0,0,0,0};
int   ledTarget[4]   = {0,0,0,0};
float ledBreathPhase = 0.0f;
float ledBatPhase    = 0.0f;
int   ledChaseIdx    = 0;
int   ledStrobeTick  = 0;

// =========================================================================
//  ANIMATION MODES
// =========================================================================
enum BackgroundMode {
  BG_BLOOD_DRIP = 0,
  BG_HYPNO_SPIRAL,
  BG_STAR_WARP,
  BG_PLASMA_TUNNEL,
  BG_DNA_HELIX,
  BG_PARTICLES,
  BG_GLITCH_SCAN,
  BG_TESLA,
  BG_MOBIUS
};

// LED base targets [LED1..LED4]
static const uint8_t LED_BASE[9][4] = {
  { 180,   0, 180,   0 },  // BG_BLOOD_DRIP   (pulse pair)
  { 255, 100, 255, 100 },  // BG_HYPNO_SPIRAL  (rotating chase)
  {   0,   0, 255, 255 },  // BG_STAR_WARP     (streak flash)
  { 255, 255, 255, 255 },  // BG_PLASMA_TUNNEL (VU driven)
  { 120, 220, 220, 120 },  // BG_DNA_HELIX     (wave sync)
  { 255,   0,   0, 255 },  // BG_PARTICLES     (chaos burst)
  {  60,  60,  60,  60 },  // BG_GLITCH_SCAN   (twitch strobe)
  { 255, 255, 255, 255 },  // BG_TESLA         (arc flash)
  { 100, 200, 200, 100 },  // BG_MOBIUS        (breathe rings)
};

// =========================================================================
//  GLOBAL STATE
// =========================================================================
BackgroundMode currentBg    = BG_PLASMA_TUNNEL;
int            lastLyricIdx = -1;
unsigned long  startTime    = 0;
bool           isPlaying    = true;
unsigned long  TOTAL_LOOP_TIME = 0;

//  1. Blood Drip Portal
#define NUM_DRIP_COLS 10
struct DripCol {
  float y;
  float speed;
  int   x;
  int   len;
  float dropY;
  bool  dropping;
} drips[NUM_DRIP_COLS];
float portalPulse = 0.0f;

//  2. Hypno Spiral
float spiralAngle = 0.0f;
float spiralZoom  = 1.0f;

//  3. Star Warp
#define NUM_STARS 40
struct Star { float x, y, z, pz; } stars[NUM_STARS];

//  4. Plasma Tunnel + EQ
float tunnelAngle = 0.0f;
#define NUM_RINGS   6
float ringRadii[NUM_RINGS];
#define NUM_BARS   16
int   barHeights[NUM_BARS], targetHeights[NUM_BARS];
struct { float radius; bool active; } wave;

//  5. DNA Helix
float dnaPhase  = 0.0f;
float dnaScroll = 0.0f;

//  6. Particle Explosion
#define NUM_PARTICLES 30
struct Particle {
  float x, y, vx, vy;
  float life, maxLife;
  bool  active;
} particles[NUM_PARTICLES];
float particleTimer = 0.0f;

//  7. Glitch Scanlines
float glitchTimer  = 0.0f;
int   glitchOffset[8];
bool  glitchActive[8];

//  8. Tesla Coil
#define MAX_ARCS    6
#define ARC_PTS     8
struct TeslaArc {
  int   px[ARC_PTS], py[ARC_PTS];
  int   framesLeft;
  bool  active;
  float baseAngle;
} arcs[MAX_ARCS];
float teslaTime = 0.0f;

//  9. Mobius Rings
float mobiusAngle = 0.0f;

// =========================================================================
//  FORWARD DECLARATIONS
// =========================================================================
void drawBloodDrip(Effect eff, bool lyricActive);
void drawHypnoSpiral(Effect eff, bool lyricActive);
void drawStarWarp(Effect eff, bool lyricActive);
void drawPlasmaTunnel(Effect eff, bool lyricActive);
void drawShockwave();
void drawEQ(Effect eff, bool lyricActive);
void drawDNAHelix(Effect eff, bool lyricActive);
void drawParticles(Effect eff, bool lyricActive);
void drawGlitchScan(Effect eff, bool lyricActive);
void generateArc(int idx, float baseAngle);
void drawTesla(Effect eff, bool lyricActive);
void drawMobius(Effect eff, bool lyricActive);

// =========================================================================
//  LED HELPERS
// =========================================================================
void writeLEDs() {
  for (int i = 0; i < 4; i++)
    analogWrite(LED_PINS[i], constrain(ledCurrent[i], 0, 255));
}

inline int rampLED(int cur, int tgt) {
  int d = tgt - cur;
  if (abs(d) <= 6) return tgt;
  return cur + d / 4;
}

void updateLEDs(BackgroundMode bg, Effect eff, bool lyricActive, bool modeChanged) {
  if (modeChanged) {
    ledChaseIdx   = 0;
    ledStrobeTick = 0;
    for (int i = 0; i < 4; i++) ledTarget[i] = LED_BASE[bg][i];
  }

  //  Effect overrides (highest priority)
  if (eff == EFFECT_INVERT) {
    for (int i = 0; i < 4; i++) ledCurrent[i] = 255;
    writeLEDs(); return;
  }
  if (eff == EFFECT_SHAKE) {
    ledStrobeTick++;
    int v = (ledStrobeTick % 2 == 0) ? 255 : 0;
    for (int i = 0; i < 4; i++) ledCurrent[i] = v;
    writeLEDs(); return;
  }
  if (eff == EFFECT_ZOOM_IN) {
    // DRACULA DROP  rolling fire wave
    ledChaseIdx = (ledChaseIdx + 1) % 8;
    for (int i = 0; i < 4; i++) {
      int dist = abs((ledChaseIdx % 4) - i);
      ledCurrent[i] = (dist == 0) ? 255 : (dist == 1) ? 160 : 40;
    }
    writeLEDs(); return;
  }

  //  Per-mode patterns
  switch (bg) {

    case BG_BLOOD_DRIP: {
      // Slow heartbeat pulse on outer pair, offset on inner pair
      ledBreathPhase += 0.08f;
      int v1 = (int)(100.0f + 100.0f * sinf(ledBreathPhase));
      int v2 = (int)(100.0f + 100.0f * sinf(ledBreathPhase + (float)PI));
      ledCurrent[0] = v1; ledCurrent[3] = v1;
      ledCurrent[1] = v2; ledCurrent[2] = v2;
      break;
    }

    case BG_HYPNO_SPIRAL: {
      // Rotating chase synced to spiral angle
      ledChaseIdx = ((int)(spiralAngle * 2.0f / (float)PI)) % 4;
      for (int i = 0; i < 4; i++) {
        int dist = (i - ledChaseIdx + 4) % 4;
        ledCurrent[i] = (dist == 0) ? 255 : (dist == 1) ? 130 : (dist == 2) ? 40 : 10;
      }
      break;
    }

    case BG_STAR_WARP: {
      // Fast strobe pulse  warp flash
      ledStrobeTick++;
      int v = (ledStrobeTick % 3 == 0) ? 255 : (ledStrobeTick % 3 == 1) ? 80 : 0;
      for (int i = 0; i < 4; i++) ledCurrent[i] = v;
      break;
    }

    case BG_PLASMA_TUNNEL: {
      // VU from EQ bars
      int a0 = (barHeights[0]  + barHeights[1])  / 2;
      int a1 = (barHeights[6]  + barHeights[7])  / 2;
      int a2 = (barHeights[9]  + barHeights[10]) / 2;
      int a3 = (barHeights[14] + barHeights[15]) / 2;
      ledTarget[0] = constrain(map(a0, 0, 35, 20, 255), 0, 255);
      ledTarget[1] = constrain(map(a1, 0, 35, 20, 255), 0, 255);
      ledTarget[2] = constrain(map(a2, 0, 35, 20, 255), 0, 255);
      ledTarget[3] = constrain(map(a3, 0, 35, 20, 255), 0, 255);
      for (int i = 0; i < 4; i++) ledCurrent[i] = rampLED(ledCurrent[i], ledTarget[i]);
      break;
    }

    case BG_DNA_HELIX: {
      // Sine wave crossing  mirror pair
      ledBreathPhase += 0.06f;
      int v1 = (int)(120.0f + 120.0f * sinf(dnaPhase));
      int v2 = (int)(120.0f + 120.0f * sinf(dnaPhase + (float)PI));
      ledCurrent[0] = v1; ledCurrent[2] = v1;
      ledCurrent[1] = v2; ledCurrent[3] = v2;
      break;
    }

    case BG_PARTICLES: {
      // Random chaos  matches explosion energy
      if (lyricActive) {
        for (int i = 0; i < 4; i++)
          ledCurrent[i] = random(80, 255);
      } else {
        for (int i = 0; i < 4; i++) ledCurrent[i] = rampLED(ledCurrent[i], 60);
      }
      break;
    }

    case BG_GLITCH_SCAN: {
      // Twitchy random strobe
      ledStrobeTick++;
      if (ledStrobeTick % 3 == 0) {
        for (int i = 0; i < 4; i++)
          ledCurrent[i] = (random(3) == 0) ? random(150, 255) : random(0, 40);
      }
      break;
    }

    case BG_TESLA: {
      // Arc flash: brief full-on blast, then dim
      ledStrobeTick++;
      bool burst = (ledStrobeTick % 5 < 2);
      int v = burst ? 255 : 30;
      for (int i = 0; i < 4; i++) ledCurrent[i] = v;
      break;
    }

    case BG_MOBIUS: {
      // Slow breathe centre, edges offset by half-phase
      ledBreathPhase += lyricActive ? 0.05f : 0.025f;
      int pulse = (int)(90.0f + 90.0f * sinf(ledBreathPhase));
      int inv   = 180 - pulse;
      ledCurrent[0] = constrain(inv,   0, 255);
      ledCurrent[1] = constrain(pulse, 0, 255);
      ledCurrent[2] = constrain(pulse, 0, 255);
      ledCurrent[3] = constrain(inv,   0, 255);
      break;
    }

    default:
      for (int i = 0; i < 4; i++) ledCurrent[i] = rampLED(ledCurrent[i], ledTarget[i]);
      break;
  }

  writeLEDs();
}

//
//  Boot animation
//
void ledBootAnimation() {
  // Triple ping-pong chase
  for (int pass = 0; pass < 3; pass++) {
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) analogWrite(LED_PINS[j], (j == i) ? 255 : 10);
      delay(60);
    }
    for (int i = 2; i >= 1; i--) {
      for (int j = 0; j < 4; j++) analogWrite(LED_PINS[j], (j == i) ? 200 : 10);
      delay(45);
    }
  }
  // Surge
  for (int v = 0; v <= 255; v += 5) { for (int j = 0; j < 4; j++) analogWrite(LED_PINS[j], v); delay(3); }
  for (int v = 255; v >= 0; v -= 5) { for (int j = 0; j < 4; j++) analogWrite(LED_PINS[j], v); delay(3); }
}

// =========================================================================
//  SETUP
// =========================================================================
void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) { pinMode(LED_PINS[i], OUTPUT); analogWrite(LED_PINS[i], 0); }

#if defined(ARDUINO_ARCH_RP2040)
  Wire.setSDA(28); Wire.setSCL(29);
  Wire.begin();
#else
  // ESP32 Dev Module: explicitly set the I2C pins used by the OLED
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
#endif
  // 100 kHz is slower but more reliable for a small 4-wire OLED during testing.
  Wire.setClock(100000);
  delay(100);

  // The confirmed module uses 0x3C (0x3D is retained as a fallback).
  // Detect the address instead of assuming it.
  uint8_t oledAddress = 0;

  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0) {
    oledAddress = 0x3C;
  } else {
    Wire.beginTransmission(0x3D);
    if (Wire.endTransmission() == 0) {
      oledAddress = 0x3D;
    }
  }

  Serial.print("OLED I2C address: ");
  if (oledAddress) Serial.println(oledAddress, HEX);
  else Serial.println("NOT FOUND");

  if (oledAddress == 0 || !display.begin(SSD1306_SWITCHCAPVCC, oledAddress)) {
    Serial.println("OLED initialization FAILED. Check VCC/GND/SDA/SCL and SSD1306 library/controller.");
    while (true) {
      for (int j = 0; j < 4; j++) analogWrite(LED_PINS[j], 220); delay(200);
      for (int j = 0; j < 4; j++) analogWrite(LED_PINS[j], 0);   delay(200);
    }
  }

  // Immediate startup test: if you see this, the OLED wiring/driver is working.
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.setTextSize(2);
  display.setCursor(18, 16);
  display.print("OLED OK");
  display.setTextSize(1);
  display.setCursor(22, 40);
  display.print("Lyrics ready");
  display.display();
  delay(1500);
  display.clearDisplay();
  display.display();

  // Init stars
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x  = random(-CX, CX);
    stars[i].y  = random(-CY, CY);
    stars[i].z  = random(1, SCREEN_WIDTH);
    stars[i].pz = stars[i].z;
  }

  // Init drips
  for (int i = 0; i < NUM_DRIP_COLS; i++) {
    drips[i].x     = 6 + i * (SCREEN_WIDTH / NUM_DRIP_COLS);
    drips[i].y     = random(-SCREEN_HEIGHT, 0);
    drips[i].speed = random(8, 22) / 10.0f;
    drips[i].len   = random(6, 18);
    drips[i].dropY = -10;
    drips[i].dropping = false;
  }

  // Init plasma rings
  for (int i = 0; i < NUM_RINGS; i++) ringRadii[i] = i * (90.0f / NUM_RINGS);
  for (int i = 0; i < NUM_BARS; i++) { barHeights[i] = 0; targetHeights[i] = 0; }
  wave.active = false;

  // Init arcs
  for (int i = 0; i < MAX_ARCS; i++) arcs[i].active = false;

  // Init glitch offsets
  for (int i = 0; i < 8; i++) { glitchOffset[i] = 0; glitchActive[i] = false; }

  // Build lyric timeline
  unsigned long t = 0;
  for (int i = 0; i < numLyrics; i++) {
    t += lyrics[i].delayBeforeMs;
    lyrics[i].calculatedStartTime = t;
    t += lyrics[i].durationMs;
  }
  TOTAL_LOOP_TIME = t + 3000;

  ledBootAnimation();
  for (int i = 0; i < 4; i++) ledTarget[i] = LED_BASE[currentBg][i];
  startTime = millis();
}

// =========================================================================
//  MAIN LOOP
// =========================================================================
void loop() {
  if (!isPlaying) return;

  unsigned long now = millis() - startTime;
  if (now > TOTAL_LOOP_TIME) {
    startTime = millis(); now = 0;
    lastLyricIdx = -1; currentBg = BG_PLASMA_TUNNEL;
    for (int i = 0; i < 4; i++) ledTarget[i] = LED_BASE[currentBg][i];
  }

  display.clearDisplay();

  //  Find active lyric
  int currentLyricIdx = -1;
  for (int i = 0; i < numLyrics; i++) {
    if (now >= lyrics[i].calculatedStartTime &&
        now <= (lyrics[i].calculatedStartTime + lyrics[i].durationMs)) {
      currentLyricIdx = i; break;
    }
  }

  Effect currentEffect = EFFECT_NONE;
  bool   lyricActive   = false;
  float  progress      = 0.0f;
  bool   invertScreen  = false;
  bool   modeChanged   = false;

  if (currentLyricIdx != -1) {
    lyricActive = true;
    const Lyric& l = lyrics[currentLyricIdx];
    currentEffect = l.effect;
    progress = (float)(now - l.calculatedStartTime) / (float)l.durationMs;

    if (currentLyricIdx != lastLyricIdx) {
      BackgroundMode prevBg = currentBg;

      //  Scene mapping
      if      (currentLyricIdx <= 1)                           currentBg = BG_PLASMA_TUNNEL;
      else if (currentLyricIdx >= 2  && currentLyricIdx <= 4)  currentBg = BG_DNA_HELIX;
      else if (currentLyricIdx == 5)                           currentBg = BG_TESLA;
      else if (currentLyricIdx >= 6  && currentLyricIdx <= 7)  currentBg = BG_HYPNO_SPIRAL;
      else if (currentLyricIdx == 8)                           currentBg = BG_MOBIUS;
      else if (currentLyricIdx == 9)                           currentBg = BG_GLITCH_SCAN;
      else if (currentLyricIdx >= 10 && currentLyricIdx <= 11) currentBg = BG_STAR_WARP;
      else if (currentLyricIdx >= 12 && currentLyricIdx <= 14) currentBg = BG_PLASMA_TUNNEL;
      else if (currentLyricIdx >= 15 && currentLyricIdx <= 16) currentBg = BG_MOBIUS;
      else if (currentLyricIdx >= 17 && currentLyricIdx <= 18) currentBg = BG_DNA_HELIX;
      else if (currentLyricIdx >= 19 && currentLyricIdx <= 20) currentBg = BG_HYPNO_SPIRAL;
      else if (currentLyricIdx == 21)                          currentBg = BG_TESLA;
      else if (currentLyricIdx >= 22 && currentLyricIdx <= 23) currentBg = BG_BLOOD_DRIP;
      else if (currentLyricIdx >= 24 && currentLyricIdx <= 25) currentBg = BG_STAR_WARP;
      else if (currentLyricIdx >= 26 && currentLyricIdx <= 27) currentBg = BG_HYPNO_SPIRAL;
      else if (currentLyricIdx >= 28 && currentLyricIdx <= 29) currentBg = BG_MOBIUS;
      else if (currentLyricIdx >= 30 && currentLyricIdx <= 31) currentBg = BG_DNA_HELIX;
      else if (currentLyricIdx == 32)                          currentBg = BG_TESLA;
      else if (currentLyricIdx >= 33 && currentLyricIdx <= 34) currentBg = BG_GLITCH_SCAN;
      else if (currentLyricIdx >= 35 && currentLyricIdx <= 36) currentBg = BG_BLOOD_DRIP;
      else if (currentLyricIdx == 37)                          currentBg = BG_PARTICLES;
      else if (currentLyricIdx == 38)                          currentBg = BG_PLASMA_TUNNEL;

      modeChanged = (currentBg != prevBg);

      // Shockwave on heavy hits
      if (currentBg == BG_PLASMA_TUNNEL &&
          (l.effect == EFFECT_INVERT || l.effect == EFFECT_ZOOM_IN || l.effect == EFFECT_SHAKE)) {
        wave.active = true; wave.radius = 4.0f;
      }

      // Particle burst on big words
      if (currentBg == BG_PARTICLES ||
          l.effect == EFFECT_INVERT || l.effect == EFFECT_ZOOM_IN) {
        for (int i = 0; i < NUM_PARTICLES; i++) {
          particles[i].x    = CX + random(-10, 10);
          particles[i].y    = CY + random(-10, 10);
          float angle       = random(0, 360) * (float)PI / 180.0f;
          float spd         = random(5, 40) / 10.0f;
          particles[i].vx   = cosf(angle) * spd;
          particles[i].vy   = sinf(angle) * spd;
          particles[i].life    = random(15, 40);
          particles[i].maxLife = particles[i].life;
          particles[i].active  = true;
        }
      }

      lastLyricIdx = currentLyricIdx;
    }
  } else {
    if (now < lyrics[0].calculatedStartTime) {
      if (currentBg != BG_PLASMA_TUNNEL) { currentBg = BG_PLASMA_TUNNEL; modeChanged = true; }
    } else if (lastLyricIdx == 38) {
      BackgroundMode want = (now < lyrics[38].calculatedStartTime + lyrics[38].durationMs + 1500)
                            ? BG_PARTICLES : BG_GLITCH_SCAN;
      if (currentBg != want) { currentBg = want; modeChanged = true; }
    }
  }

  //  Render background
  switch (currentBg) {
    case BG_BLOOD_DRIP:    drawBloodDrip(currentEffect, lyricActive);    break;
    case BG_HYPNO_SPIRAL:  drawHypnoSpiral(currentEffect, lyricActive);  break;
    case BG_STAR_WARP:     drawStarWarp(currentEffect, lyricActive);     break;
    case BG_PLASMA_TUNNEL: drawPlasmaTunnel(currentEffect, lyricActive);
                           drawShockwave();
                           drawEQ(currentEffect, lyricActive);            break;
    case BG_DNA_HELIX:     drawDNAHelix(currentEffect, lyricActive);     break;
    case BG_PARTICLES:     drawParticles(currentEffect, lyricActive);    break;
    case BG_GLITCH_SCAN:   drawGlitchScan(currentEffect, lyricActive);   break;
    case BG_TESLA:         drawTesla(currentEffect, lyricActive);        break;
    case BG_MOBIUS:        drawMobius(currentEffect, lyricActive);       break;
  }

  //  Render lyrics on top
  if (currentLyricIdx != -1) {
    const Lyric& l = lyrics[currentLyricIdx];
    int textSize = l.size;
    int offsetX  = 0, offsetY = 0;

    char typeBuf[20];
    const char* displayWord = l.word;
    bool showCursor = false;

    if (l.effect == EFFECT_TYPEWRITER) {
      int total = strlen(l.word);
      int show  = 1 + (int)(progress * total);
      if (show > total) show = total;
      strncpy(typeBuf, l.word, show);
      typeBuf[show] = '\0';
      displayWord = typeBuf;
      showCursor  = (show < total);
    }

    if (l.effect == EFFECT_POP) {
      if (progress < 0.15f) textSize = l.size + 1;
    } else if (l.effect == EFFECT_BOUNCE) {
      float amp = 20.0f * expf(-5.0f * progress);
      offsetY = -(int)(amp * fabsf(cosf(progress * (float)PI * 4.0f)));
    } else if (l.effect == EFFECT_SHAKE) {
      offsetX = random(-3, 4); offsetY = random(-3, 4);
    } else if (l.effect == EFFECT_INVERT) {
      invertScreen = true;
      if (random(10) > 5) { offsetX = random(-2, 3); offsetY = random(-2, 3); }
    } else if (l.effect == EFFECT_GLITCH) {
      if (random(10) > 6) {
        offsetX = random(-6, 6);
        display.fillRect(0, random(SCREEN_HEIGHT), SCREEN_WIDTH, random(2, 6), SSD1306_WHITE);
      }
      if (random(10) > 8) invertScreen = true;
    } else if (l.effect == EFFECT_ZOOM_IN) {
      if      (progress < 0.05f) textSize = l.size > 1 ? l.size - 1 : 1;
      else if (progress < 0.10f) textSize = l.size;
      else                       textSize = l.size + 1;
      if (progress > 0.4f) { offsetX = random(-4, 5); offsetY = random(-4, 5); }
      if (progress > 0.3f && random(10) > 7) invertScreen = true;
    }

    display.setTextSize(textSize);
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(displayWord, 0, 0, &x1, &y1, &w, &h);

    if (w > SCREEN_WIDTH) {
      textSize = 2;
      display.setTextSize(textSize);
      display.getTextBounds(displayWord, 0, 0, &x1, &y1, &w, &h);
    }

    int drawX = (SCREEN_WIDTH  - (int)w) / 2 + offsetX;
    int drawY = (SCREEN_HEIGHT - (int)h) / 2 + offsetY;
    if (currentBg == BG_PLASMA_TUNNEL) drawY -= 5;

    // Frosted glass background box
    display.fillRoundRect(drawX - 6, drawY - 5, (int)w + 12, (int)h + 10, 3, SSD1306_BLACK);
    display.drawRoundRect(drawX - 6, drawY - 5, (int)w + 12, (int)h + 10, 3, SSD1306_WHITE);
    // Double border on impact effects
    if (l.effect == EFFECT_INVERT || l.effect == EFFECT_ZOOM_IN) {
      display.drawRoundRect(drawX - 8, drawY - 7, (int)w + 16, (int)h + 14, 4, SSD1306_WHITE);
    }

    if (l.effect == EFFECT_GLITCH && random(10) > 8) {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      display.fillRoundRect(drawX - 4, drawY - 3, (int)w + 8, (int)h + 6, 2, SSD1306_WHITE);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }

    display.setCursor(drawX, drawY);
    display.print(displayWord);

    if (showCursor && ((millis() / 150) % 2 == 0))
      display.fillRect(drawX + (int)w + 2, drawY, 2, (int)h, SSD1306_WHITE);
  }

  display.invertDisplay(invertScreen);
  display.display();
  updateLEDs(currentBg, currentEffect, lyricActive, modeChanged);
}

// =========================================================================
//  ANIMATION FUNCTIONS
// =========================================================================

//  1. Blood Drip Portal
void drawBloodDrip(Effect eff, bool lyricActive) {
  portalPulse += lyricActive ? 0.12f : 0.06f;

  // Pulsing outer rings (portal mouth)
  for (int r = 5; r <= 28; r += 5) {
    int offset = (int)(2.0f * sinf(portalPulse + r * 0.2f));
    display.drawCircle(CX, CY - 10, r + offset, SSD1306_WHITE);
  }
  // Inner highlight
  display.fillCircle(CX, CY - 10, 3, SSD1306_WHITE);

  // Drip columns
  float sm = (eff == EFFECT_SHAKE || eff == EFFECT_ZOOM_IN) ? 3.0f : 1.0f;
  for (int i = 0; i < NUM_DRIP_COLS; i++) {
    drips[i].y += drips[i].speed * sm;
    if (drips[i].y > SCREEN_HEIGHT + drips[i].len + 5) {
      drips[i].y     = random(-20, -2);
      drips[i].speed = random(8, 22) / 10.0f;
      drips[i].len   = random(6, 18);
    }
    int hy = (int)drips[i].y;
    int x  = drips[i].x;
    // Trail
    for (int j = 0; j < drips[i].len; j++) {
      int ty = hy - j;
      if (ty >= 0 && ty < SCREEN_HEIGHT) {
        if (j == 0)       display.fillRect(x - 1, ty, 3, 2, SSD1306_WHITE); // fat head
        else if (j < 4)   display.drawPixel(x, ty, SSD1306_WHITE);           // bright body
        else if (j % 2 == 0) display.drawPixel(x, ty, SSD1306_WHITE);        // fading tail
      }
    }
    // Drip tip teardrop
    if (hy >= 0 && hy < SCREEN_HEIGHT) display.fillCircle(x, hy, 1, SSD1306_WHITE);
  }
}

//  2. Hypno Spiral
void drawHypnoSpiral(Effect eff, bool lyricActive) {
  float spd = lyricActive ? 0.10f : 0.04f;
  if (eff == EFFECT_ZOOM_IN || eff == EFFECT_SHAKE) spd = 0.25f;
  spiralAngle += spd;

  // 3 interleaved Archimedean arms
  for (int arm = 0; arm < 3; arm++) {
    float armOffset = arm * (2.0f * (float)PI / 3.0f);
    int prevX = CX, prevY = CY;
    for (float t = 0.1f; t < 5.5f; t += 0.18f) {
      float a = t + spiralAngle + armOffset;
      float r = t * 5.5f;
      if (r > 62) break;
      int x = CX + (int)(cosf(a) * r);
      int y = CY + (int)(sinf(a) * r);
      if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
        display.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
      prevX = x; prevY = y;
    }
  }

  // Central star burst
  if (lyricActive) {
    for (int i = 0; i < 6; i++) {
      float a = spiralAngle * 2.0f + i * (float)PI / 3.0f;
      display.drawLine(CX, CY,
        CX + (int)(cosf(a) * 8),
        CY + (int)(sinf(a) * 8), SSD1306_WHITE);
    }
  }
  display.fillCircle(CX, CY, 2, SSD1306_WHITE);
}

//  3. Star Warp
void drawStarWarp(Effect eff, bool lyricActive) {
  float warpSpeed = lyricActive ? 12.0f : 5.0f;
  if (eff == EFFECT_ZOOM_IN || eff == EFFECT_SHAKE) warpSpeed = 25.0f;

  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].pz = stars[i].z;
    stars[i].z -= warpSpeed;
    if (stars[i].z <= 0) {
      stars[i].x  = random(-CX, CX);
      stars[i].y  = random(-CY, CY);
      stars[i].z  = SCREEN_WIDTH;
      stars[i].pz = stars[i].z;
    }
    // Project current + previous position to draw a streak
    float sx  = (stars[i].x  / stars[i].z)  * SCREEN_WIDTH  + CX;
    float sy  = (stars[i].y  / stars[i].z)  * SCREEN_HEIGHT + CY;
    float psx = (stars[i].x  / stars[i].pz) * SCREEN_WIDTH  + CX;
    float psy = (stars[i].y  / stars[i].pz) * SCREEN_HEIGHT + CY;

    if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 0 && sy < SCREEN_HEIGHT) {
      if (abs((int)(sx - psx)) > 0 || abs((int)(sy - psy)) > 0)
        display.drawLine((int)psx, (int)psy, (int)sx, (int)sy, SSD1306_WHITE);
      else
        display.drawPixel((int)sx, (int)sy, SSD1306_WHITE);
    }
  }
}

//  4. Plasma Tunnel
void drawPlasmaTunnel(Effect eff, bool lyricActive) {
  float speed = (eff == EFFECT_ZOOM_IN) ? 7.0f :
                (eff == EFFECT_SHAKE)   ? 4.0f :
                lyricActive ? 1.5f : 0.6f;
  tunnelAngle += 0.04f;

  int cx = CX, cy = CY;
  if (eff == EFFECT_SHAKE || eff == EFFECT_ZOOM_IN) {
    cx += random(-4, 4); cy += random(-4, 4);
  }

  // Diagonal rails (rotated 45 degrees)
  for (float a = tunnelAngle; a < tunnelAngle + 2.0f * (float)PI; a += (float)PI / 3.0f) {
    display.drawLine(cx + (int)(cosf(a) * 4),   cy + (int)(sinf(a) * 4),
                     cx + (int)(cosf(a) * 120), cy + (int)(sinf(a) * 120), SSD1306_WHITE);
  }

  // Expanding rings
  for (int i = 0; i < NUM_RINGS; i++) {
    ringRadii[i] += speed;
    if (ringRadii[i] > 110) ringRadii[i] = 1;
    int r = (int)ringRadii[i];
    if (r > 2) {
      display.drawCircle(cx, cy, r, SSD1306_WHITE);
      // Double-ring for thick look on big radii
      if (r > 30 && r % 20 < 4) display.drawCircle(cx, cy, r + 1, SSD1306_WHITE);
    }
  }
}

//  Shockwave
void drawShockwave() {
  if (!wave.active) return;
  display.drawCircle(CX, CY, (int)wave.radius,     SSD1306_WHITE);
  display.drawCircle(CX, CY, (int)wave.radius + 1, SSD1306_WHITE);
  display.drawCircle(CX, CY, (int)wave.radius + 2, SSD1306_WHITE);
  wave.radius += 9.0f;
  if (wave.radius > 140) wave.active = false;
}

//  EQ Bars
void drawEQ(Effect eff, bool lyricActive) {
  int barW = SCREEN_WIDTH / NUM_BARS;
  for (int i = 0; i < NUM_BARS; i++) {
    if (random(10) > 4) {
      if (lyricActive) {
        if      (eff == EFFECT_ZOOM_IN) targetHeights[i] = random(18, 36);
        else if (eff == EFFECT_SHAKE)   targetHeights[i] = random(10, 26);
        else                            targetHeights[i] = random(5, 16);
      } else {
        targetHeights[i] = random(1, 6);
      }
    }
    if      (barHeights[i] < targetHeights[i]) barHeights[i] += 4;
    else if (barHeights[i] > targetHeights[i]) barHeights[i] -= 3;
    if (barHeights[i] < 1) barHeights[i] = 1;
    // Hollow bars with dot cap
    display.drawRect(i * barW + 1, SCREEN_HEIGHT - barHeights[i],
                     barW - 1, barHeights[i], SSD1306_WHITE);
    display.fillRect(i * barW + 1, SCREEN_HEIGHT - barHeights[i],
                     barW - 1, 2, SSD1306_WHITE); // bright cap
  }
}

//  5. DNA Helix
void drawDNAHelix(Effect eff, bool lyricActive) {
  float spd = lyricActive ? 0.10f : 0.04f;
  if (eff == EFFECT_ZOOM_IN || eff == EFFECT_SHAKE) spd = 0.22f;
  dnaPhase  += spd;
  dnaScroll += spd * 2.0f;

  int amplitude = lyricActive ? 20 : 15;

  // Two strands
  int prevA1x = -1, prevA1y = -1;
  int prevA2x = -1, prevA2y = -1;
  int prevB1x = -1, prevB1y = -1;
  int prevB2x = -1, prevB2y = -1;

  for (int x = 0; x < SCREEN_WIDTH; x += 2) {
    float t  = (float)x / SCREEN_WIDTH * (float)PI * 4.0f + dnaPhase;
    int y1a  = CY + (int)(amplitude * sinf(t));
    int y1b  = CY + (int)(amplitude * sinf(t + (float)PI));

    // Draw strands
    if (prevA1x >= 0) display.drawLine(prevA1x, prevA1y, x, y1a, SSD1306_WHITE);
    if (prevB1x >= 0) display.drawLine(prevB1x, prevB1y, x, y1b, SSD1306_WHITE);

    // Cross-rungs every ~10px
    if (x % 10 == 0) {
      display.drawLine(x, y1a, x, y1b, SSD1306_WHITE);
      // Node dots at each rung end
      display.drawPixel(x, y1a, SSD1306_WHITE);
      display.drawPixel(x, y1b, SSD1306_WHITE);
      display.drawPixel(x - 1, y1a, SSD1306_WHITE);
      display.drawPixel(x + 1, y1b, SSD1306_WHITE);
    }

    prevA1x = x; prevA1y = y1a;
    prevB1x = x; prevB1y = y1b;
  }

  // Bright centre line
  display.drawFastHLine(0, CY, SCREEN_WIDTH, SSD1306_WHITE);
}

//  6. Particle Explosion
void drawParticles(Effect eff, bool lyricActive) {
  // Continuous spawn
  particleTimer++;
  if (particleTimer > 8 || (lyricActive && particleTimer > 3)) {
    particleTimer = 0;
    for (int i = 0; i < NUM_PARTICLES; i++) {
      if (!particles[i].active) {
        particles[i].x    = CX + random(-20, 20);
        particles[i].y    = CY + random(-20, 20);
        float angle       = random(0, 628) / 100.0f;
        float spd         = random(4, 25) / 10.0f;
        if (eff == EFFECT_ZOOM_IN) spd *= 3.0f;
        particles[i].vx   = cosf(angle) * spd;
        particles[i].vy   = sinf(angle) * spd - 0.5f;
        particles[i].life    = random(12, 35);
        particles[i].maxLife = particles[i].life;
        particles[i].active  = true;
        break;
      }
    }
  }

  for (int i = 0; i < NUM_PARTICLES; i++) {
    if (!particles[i].active) continue;
    particles[i].x  += particles[i].vx;
    particles[i].y  += particles[i].vy;
    particles[i].vy += 0.15f; // gravity
    particles[i].life--;
    if (particles[i].life <= 0 ||
        particles[i].x < 0 || particles[i].x >= SCREEN_WIDTH ||
        particles[i].y < 0 || particles[i].y >= SCREEN_HEIGHT) {
      particles[i].active = false; continue;
    }

    float frac = particles[i].life / particles[i].maxLife;
    int px = (int)particles[i].x, py = (int)particles[i].y;
    if (frac > 0.6f)      display.fillCircle(px, py, 2, SSD1306_WHITE);
    else if (frac > 0.3f) display.drawPixel(px, py, SSD1306_WHITE);
    else                  { if ((int)particles[i].life % 2 == 0) display.drawPixel(px, py, SSD1306_WHITE); }
  }

  // Gravity well cross at centre
  display.drawFastHLine(CX - 4, CY, 9, SSD1306_WHITE);
  display.drawFastVLine(CX, CY - 4, 9, SSD1306_WHITE);
}

//  7. Glitch Scanlines
void drawGlitchScan(Effect eff, bool lyricActive) {
  glitchTimer++;
  float intensity = lyricActive ? 0.7f : 0.3f;
  if (eff == EFFECT_ZOOM_IN || eff == EFFECT_SHAKE) intensity = 1.0f;

  // Scanline pattern base  thin horizontal lines
  for (int y = 0; y < SCREEN_HEIGHT; y += 4)
    display.drawFastHLine(0, y, SCREEN_WIDTH, SSD1306_WHITE);

  // Glitch horizontal tears
  for (int i = 0; i < 8; i++) {
    if (glitchActive[i]) {
      int gy = i * 8 + random(0, 4);
      if (gy < SCREEN_HEIGHT)
        display.fillRect(glitchOffset[i], gy, random(10, 60), random(1, 4), SSD1306_WHITE);
    }
    // Randomly toggle tears
    if (random(100) < (int)(intensity * 15)) {
      glitchActive[i]  = !glitchActive[i];
      glitchOffset[i]  = random(0, SCREEN_WIDTH - 60);
    }
  }

  // Vertical block glitch
  if (random(100) < (int)(intensity * 20)) {
    int bx = random(0, SCREEN_WIDTH - 10);
    int bh = random(4, 16);
    display.fillRect(bx, random(0, SCREEN_HEIGHT - bh), random(5, 20), bh, SSD1306_WHITE);
  }

  // Twitchy centre line
  int cy = CY + random(-3, 4);
  display.drawFastHLine(0, cy, SCREEN_WIDTH, SSD1306_WHITE);
  display.drawFastHLine(0, cy + 1, SCREEN_WIDTH, SSD1306_WHITE);

  // Pixel noise scatter
  int noise = (int)(intensity * 20);
  for (int i = 0; i < noise; i++)
    display.drawPixel(random(SCREEN_WIDTH), random(SCREEN_HEIGHT), SSD1306_WHITE);
}

//  8. Tesla Coil Arc
void generateArc(int idx, float baseAngle) {
  arcs[idx].baseAngle  = baseAngle;
  arcs[idx].px[0]      = CX;
  arcs[idx].py[0]      = CY;
  float len = random(20, 55);
  for (int i = 1; i < ARC_PTS; i++) {
    float frac = (float)i / (ARC_PTS - 1);
    float jitter = (float)PI / 5.0f * (1.0f - frac);
    float a = baseAngle + (random(-100, 101) / 100.0f) * jitter;
    float r = frac * len;
    arcs[idx].px[i] = CX + (int)(cosf(a) * r);
    arcs[idx].py[i] = CY + (int)(sinf(a) * r);
  }
  arcs[idx].framesLeft = random(2, 6);
  arcs[idx].active     = true;
}

void drawTesla(Effect eff, bool lyricActive) {
  teslaTime += 0.08f;

  int spawnChance = lyricActive ? 40 : 12;
  if (eff == EFFECT_SHAKE || eff == EFFECT_INVERT || eff == EFFECT_ZOOM_IN)
    spawnChance = 70;

  // Spawn new arcs
  if (random(100) < spawnChance) {
    float ba = random(0, 628) / 100.0f;
    for (int i = 0; i < MAX_ARCS; i++) {
      if (!arcs[i].active) { generateArc(i, ba); break; }
    }
  }

  // Draw arcs
  for (int i = 0; i < MAX_ARCS; i++) {
    if (!arcs[i].active) continue;
    for (int j = 0; j < ARC_PTS - 1; j++) {
      display.drawLine(arcs[i].px[j], arcs[i].py[j],
                       arcs[i].px[j+1], arcs[i].py[j+1], SSD1306_WHITE);
    }
    // Draw again shifted 1px for thickness
    for (int j = 0; j < ARC_PTS - 1; j++) {
      display.drawLine(arcs[i].px[j]+1, arcs[i].py[j],
                       arcs[i].px[j+1]+1, arcs[i].py[j+1], SSD1306_WHITE);
    }
    arcs[i].framesLeft--;
    if (arcs[i].framesLeft <= 0) arcs[i].active = false;
  }

  // Pulsing halo rings around centre
  for (int r = 3; r <= 14; r += 4) {
    int offset = (int)(2.0f * sinf(teslaTime * 3.0f + r));
    display.drawCircle(CX, CY, r + offset, SSD1306_WHITE);
  }
  display.fillCircle(CX, CY, 3, SSD1306_WHITE);

  // Sparkle corona
  int sparks = lyricActive ? 10 : 4;
  for (int i = 0; i < sparks; i++)
    display.drawPixel(random(SCREEN_WIDTH), random(SCREEN_HEIGHT), SSD1306_WHITE);
}

//  9. Mobius Rings
void drawMobius(Effect eff, bool lyricActive) {
  float spd = lyricActive ? 0.06f : 0.025f;
  if (eff == EFFECT_ZOOM_IN || eff == EFFECT_SHAKE) spd = 0.18f;
  mobiusAngle += spd;

  // Layered rotating ellipses at different tilt angles
  for (int layer = 0; layer < 5; layer++) {
    float tilt  = (float)layer * (float)PI / 5.0f + mobiusAngle;
    float rx    = 28 + layer * 4;
    float ry    = (int)(rx * fabsf(sinf(tilt + (float)layer)));
    if (ry < 2) ry = 2;

    float phase = mobiusAngle + layer * (float)PI / 2.5f;
    int prevX = -1, prevY = -1;
    for (float a = 0; a <= 2.0f * (float)PI; a += 0.15f) {
      int x = CX + (int)(cosf(a + phase) * rx);
      int y = CY + (int)(sinf(a + phase) * ry);
      if (prevX >= 0 && x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
        display.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
      prevX = x; prevY = y;
    }
  }

  // Rotating node dots on outermost ring
  for (int n = 0; n < 8; n++) {
    float a = n * (float)PI / 4.0f + mobiusAngle * 2.0f;
    int nx = CX + (int)(cosf(a) * 44);
    int ny = CY + (int)(sinf(a) * 20);
    if (nx >= 0 && nx < SCREEN_WIDTH && ny >= 0 && ny < SCREEN_HEIGHT)
      display.fillCircle(nx, ny, 2, SSD1306_WHITE);
  }

  display.fillCircle(CX, CY, 2, SSD1306_WHITE);
}
