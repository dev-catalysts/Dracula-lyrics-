#pragma once
#include <Arduino.h>

// =========================================================================
//  EFFECTS & LYRICS
// =========================================================================
enum Effect {
  EFFECT_NONE = 0,
  EFFECT_POP,
  EFFECT_SHAKE,
  EFFECT_INVERT,
  EFFECT_GLITCH,
  EFFECT_ZOOM_IN,
  EFFECT_TYPEWRITER,
  EFFECT_BOUNCE
};

struct Lyric {
  unsigned long delayBeforeMs;
  unsigned long durationMs;
  const char*   word;
  Effect        effect;
  uint8_t       size;
  unsigned long calculatedStartTime;
};

// =========================================================================
//  LYRICS TIMELINE
// =========================================================================
Lyric lyrics[] = {
  // Phrase 1  "The morning light is turning BLUE"
  {   0, 300, "The",      EFFECT_NONE,       2, 0 },
  {   0, 300, "morning",  EFFECT_POP,        2, 0 },
  {   0, 300, "light",    EFFECT_NONE,       2, 0 },
  {   0, 300, "is",       EFFECT_NONE,       2, 0 },
  {   0, 300, "turning",  EFFECT_NONE,       2, 0 },
  { 100, 500, "BLUE",     EFFECT_INVERT,     3, 0 },
  // Phrase 2  "the feeling is BIZARRE"
  {   0, 300, "the",      EFFECT_NONE,       2, 0 },
  {   0, 300, "feeling",  EFFECT_BOUNCE,     2, 0 },
  {   0, 300, "is",       EFFECT_NONE,       2, 0 },
  {   0, 800, "BIZARRE",  EFFECT_TYPEWRITER, 2, 0 },
  // Phrase 3  "The night is almost over,"
  { 200, 300, "The",      EFFECT_NONE,       2, 0 },
  {   0, 300, "night",    EFFECT_POP,        2, 0 },
  {   0, 300, "is",       EFFECT_NONE,       2, 0 },
  {   0, 300, "almost",   EFFECT_NONE,       2, 0 },
  {   0, 500, "over",     EFFECT_NONE,       2, 0 },
  // Phrase 4  "I still don't know where you ARE"
  { 200, 300, "I",        EFFECT_NONE,       2, 0 },
  {   0, 300, "still",    EFFECT_BOUNCE,     2, 0 },
  {   0, 300, "don't",    EFFECT_NONE,       2, 0 },
  {   0, 300, "know",     EFFECT_NONE,       2, 0 },
  {   0, 200, "where",    EFFECT_NONE,       2, 0 },
  {   0, 200, "you",      EFFECT_NONE,       2, 0 },
  {   0, 600, "ARE",      EFFECT_SHAKE,      3, 0 },
  // Phrase 5  "The shadows, yeah, they keep me pretty like a movie STAR"
  { 200, 300, "The",      EFFECT_NONE,       2, 0 },
  {   0, 400, "shadows",  EFFECT_SHAKE,      2, 0 },
  {   0, 300, "yeah",     EFFECT_NONE,       2, 0 },
  {   0, 300, "they",     EFFECT_NONE,       2, 0 },
  {   0, 300, "keep",     EFFECT_NONE,       2, 0 },
  {   0, 300, "me",       EFFECT_NONE,       2, 0 },
  {   0, 300, "pretty",   EFFECT_POP,        2, 0 },
  {   0, 300, "like",     EFFECT_NONE,       2, 0 },
  {   0, 300, "a",        EFFECT_NONE,       2, 0 },
  {   0, 300, "movie",    EFFECT_NONE,       2, 0 },
  {   0, 800, "STAR",     EFFECT_INVERT,     3, 0 },
  // Phrase 6  "Daylight makes me feel like"
  { 300, 500, "Daylight", EFFECT_TYPEWRITER, 2, 0 },
  {   0, 300, "makes",    EFFECT_NONE,       2, 0 },
  {   0, 300, "me",       EFFECT_NONE,       2, 0 },
  {   0, 300, "feel",     EFFECT_BOUNCE,     2, 0 },
  {   0, 200, "like",     EFFECT_NONE,       2, 0 },
  // THE DROP
  { 100,2800, "DRACULA",  EFFECT_ZOOM_IN,    3, 0 },
};
const int numLyrics = sizeof(lyrics) / sizeof(Lyric);
