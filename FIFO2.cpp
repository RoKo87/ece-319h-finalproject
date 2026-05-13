// FIFO2.cpp
// Runs on any microcontroller
// Receive FIFO for buffering incoming UART data from Wemos D1.
// Index-based circular buffer implementation.
// ECE319H Lab 9H

#include <stdint.h>
#include "FIFO2.h"
#include "../inc/ST7735.h"

// Constructor — initialize to empty
Queue::Queue() {
  PutI = 0;
  GetI = 0;
}

// Returns true if queue is empty
bool Queue::IsEmpty(void) {
  return PutI == GetI;
}

// Returns true if queue is full
bool Queue::IsFull(void) {
  return ((PutI + 1) % FIFOSIZE) == GetI;
}

// Insert character at rear; returns false if full (data lost)
bool Queue::Put(char x) {
  if (IsFull()) return false;
  Buf[PutI] = x;
  PutI = (PutI + 1) % FIFOSIZE;
  return true;
}

// Remove and return oldest character; returns 255 if empty
char Queue::Get() {
  if (IsEmpty()) return 255;
  char ret = Buf[GetI];
  GetI = (GetI + 1) % FIFOSIZE;
  return ret;
}

// Debug: display queue contents on LCD
void Queue::Print(void) {
  // output to ST7735 if needed for debugging
}
