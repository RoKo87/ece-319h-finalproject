// Sound.cpp
// Runs on MSPM0G3507
// Uses built-in 12-bit DAC on PA15

#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "Sound.h"
#include "../inc/DAC.h"

// ---- Wave table state ----
typedef struct Wave {
  const uint16_t *waveptr;
  uint32_t        len;
} Wave;
static Wave wave;

static uint32_t windex = 0; // current sample index within wave table

// ---- Song playback state (driven by ISR) ----
static Note    *sPtr   = 0;   // current song array (0 = stopped)
static int      sLen   = 0;
static int      sindex   = 0;
static int      sTempo = 60;
static int      sLoop  = 0;   // 1 = loop forever
static int      isRest  = 0;   // 1 = current note is a rest
static volatile uint32_t numpd = 0; // samples remaining for current note

// ---- Internal helpers ----

// Compute SysTick period from a base frequency and octave shift
int CalcPeriod(double base, int octave) {
  if (wave.len == 0) wave.len = 32;
  int period = (int)(80000000 / (base * wave.len));
  if (octave <= 0) period = period << (-1 * octave);
  else             period = period >> octave;
  return period;
}

// Load sPtr[sindex] into SysTick and set noteCount; safe to call from ISR
static void NoteHandle(void) {
  Note n    = sPtr[sindex];
  isRest  = (n.base == 0.0f);

  int pd = isRest ? 9557 : CalcPeriod(n.base, n.octave);
  if (isRest) DAC_Out(0); // silence immediately for rests

  int cyclen = (int)(80000000 * n.beats * (60.0 / sTempo));
  numpd  = (uint32_t)(cyclen / pd);

  SysTick->LOAD = (uint32_t)pd - 1;
  SysTick->VAL  = 0;
  SysTick->CTRL = 0x07; // keep/start SysTick running
}

// ---- Public API ----

void Sound_Init(uint32_t priority, const uint16_t wf[], uint32_t len) {
  wave.waveptr = wf;
  wave.len     = len;
  DAC_Init();
  IOMUX->SECCFG.PINCM[PA15INDEX] = 0x00000000; // PA15 as analog (DAC output, no digital buffers)
  __disable_irq();
  SysTick->CTRL = 0x0;
  SysTick->LOAD = 9955;
  SysTick->VAL  = 0x0;
  SysTick->CTRL = 0x04; // interrupts disabled
  NVIC_SetPriority(SysTick_IRQn, priority);
  __enable_irq();
}

void Sound_Stop(void) {
  __disable_irq();
  SysTick->CTRL = 0x04; // disable interrupt, keep clock source
  __enable_irq();
  DAC_Out(0); // return to midpoint (silence)
  numpd = 0;
  sPtr   = 0;
}

void Sound_Start(uint32_t period) {
  __disable_irq();
  SysTick->LOAD = period - 1;
  SysTick->VAL  = 0x0;
  SysTick->CTRL = 0x07; // interrupts enabled
  __enable_irq();
}

// Begin non-blocking song playback; returns immediately.
// The SysTick ISR advances through notes and auto-stops when done.
int PlaySound(Note sound[], int len, int tempo, int canLoop) {
  sPtr   = sound;
  sLen   = len;
  sindex   = 0;
  sTempo = (tempo == 0) ? 60 : tempo;
  sLoop  = canLoop;
  windex    = 0;
  __disable_irq();
  NoteHandle();
  __enable_irq();
  return 0;
}

// ---- SysTick ISR ----
extern "C" void SysTick_Handler(void);
void SysTick_Handler(void) {
  if (sPtr) {
    // output sample (skip during rests)
    if (!isRest) {
      DAC_Out(wave.waveptr[windex]);
      windex = (windex + 1) % wave.len; //next in wave
    }
    // advance note when count expires
    if (numpd > 0 && --numpd == 0) {
      sindex++; //next in sound
      if (sLoop && sindex >= sLen) sindex = 0; // loop back
      if (sindex < sLen) NoteHandle(); // next note
      else Sound_Stop(); // song finished
    }
  }
}

// ---- Song arrays ----

// int sound0L = 30;
// Note sound0[30] =
//   {{rest, 0, 0.5f}, {B, 0, 0.4f}, {rest, 0, 0.1f}, {B, 0, 0.4f}, {rest, 0, 0.1f}, {B, 0, 0.5f},
//    {A, 0, 0.5f}, {Fs, 0, 0.4f}, {rest, 0, 0.1f}, {Fs, 0, 1.0f},
//    {E, 0, 0.4f}, {rest, 0, 0.1f}, {E, 0, 0.5f}, {D, 0, 0.5f}, {E, 0, 0.5f},
//    {Fs, 0, 1.0f}, {rest, 0, 0.5f}, {E, 0, 0.5f},
//    {Fs, 0, 0.4f}, {rest, 0, 0.1f}, {Fs, 0, 0.5f}, {D, 0, 0.5f}, {E, 0, 0.5f},
//    {Fs, 0, 0.75f}, {D, 0, 0.75f}, {E, 0, 0.5f},
//    {Fs, 0, 0.75f}, {D, 0, 0.75f}, {E, 0, 0.5f}, {Fs, 0, 1.0f}};
