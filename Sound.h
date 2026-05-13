// Sound.h
// Runs on MSPM0G3507
// Uses built-in 12-bit DAC on PA15
#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>

// ---- Note frequency constants (Hz), matching Lab5 style ----
#define rest (float)0.0
#define C    (float)261.6
#define Cs   (float)277.2
#define D    (float)293.7
#define Ds   (float)311.1
#define E    (float)329.6
#define F    (float)349.2
#define Fs   (float)370.0
#define G    (float)392.0
#define Gs   (float)415.3
#define A    (float)440.0
#define As   (float)466.2
#define B    (float)493.9

// ---- Note struct (matches Lab5 style) ----
// base:   frequency in Hz (use constants above; 0.0 = rest)
// octave: 0 = as-is, +1 = up one octave, -1 = down one octave
// beats:  duration in beats (quarter note = 1.0, eighth = 0.5, etc.)
typedef struct Note {
  float base;
  int   octave;
  float beats;
} Note;

// ---- Sound API ----

// Initialize SysTick and DAC (SysTick starts disabled)
// priority: 0 (highest) to 3 (lowest)
// wf:       pointer to 12-bit wave table (values 0-4095)
// len:      number of entries in wave table
void Sound_Init(uint32_t priority, const uint16_t wf[], uint32_t len);

// Start continuous playback at given SysTick period
// period: 80,000,000 / (frequency * tableLength)
void Sound_Start(uint32_t period);

// Stop playback and silence output
void Sound_Stop(void);

// Play an array of notes at the given tempo (BPM)
// canLoop: 1 = loop forever, 0 = play once then stop
// Blocking — does not return until done (or forever if canLoop=1)
int PlaySound(Note sound[], int len, int tempo, int canLoop);

#endif
