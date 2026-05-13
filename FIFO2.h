/*!
 * @defgroup FIFO
 * @brief Lab 9H First in first out queue, C++
 * @{*/
/**
 * @file      FIFO2.h
 * @brief     Provide functions for a first in first out queue
 * @details   ECE319H Lab 9H — receive buffer for Wemos D1 UART data
 * @author    change this to your names or look very silly
 * @date      change this to the last modification date or look very silly
 */

#ifndef __FIFO2_H__
#define __FIFO2_H__
#include <stdint.h>

/**
 * \brief FIFOSIZE the size of the FIFO, which can hold 0 to FIFOSIZE-1 elements.
 * The size must be a power of 2.
 * 64 entries gives headroom for longer Wemos response strings.
 */
#define FIFOSIZE 64  // maximum storage is FIFOSIZE-1 elements

/**
 * ECE319K Lab 9H FIFO — receive buffer for Wemos D1 UART responses
 * @brief C++ FIFO queue
 */
class Queue {
private:
  char Buf[FIFOSIZE];
  int PutI; // index to next empty slot
  int GetI; // index to oldest data

public:
  Queue();             // initialize queue
  bool IsEmpty(void);  // true if empty
  bool IsFull(void);   // true if full
  bool Put(char x);    // enter data into queue, returns false if full
  char Get();          // remove and return oldest data (255 if empty)
  void Print(void);    // display elements on LCD (debug)
};

#endif //  __FIFO2_H__
/** @}*/
