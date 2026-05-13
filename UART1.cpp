/* UART1.cpp
 * Your name
 * Date:
 * PA8 UART1 Tx to Wemos D1 Rx
 */

#include <ti/devices/msp/msp.h>
#include "UART1.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"

#define PA8INDEX 18 // UART1_TX  SPI0_CS0  UART0_RTS  TIMA0_C0  TIMA1_C0N

// power Domain PD0
// for 80MHz bus clock, UART1 clock is ULPCLK 40MHz
// initialize UART1 for 2375 baud rate
// no receive, blind synchronization on transmit
void UART1_Init(void) {
  // RSTCLR to UART1 peripheral
  //   bits 31-24 unlock key 0xB1
  //   bit 1 is Clear reset sticky bit
  //   bit 0 is reset peripheral
  UART1->GPRCM.RSTCTL = 0xB1000003;
  // Enable power to UART1 peripheral
  //   bits 31-24 unlock key 0x26
  //   bit 0 is Enable Power
  UART1->GPRCM.PWREN = 0x26000001;
  Clock_Delay(24); // time for UART1 to power up

  // configure PA8 as UART1 transmit function
  // bit 7  PC connected
  // bits 5-0 = 2 for UART1_Tx
  IOMUX->SECCFG.PINCM[PA8INDEX] = 0x00000082;

  UART1->CLKSEL = 0x08; // bus clock (ULPCLK)
  UART1->CLKDIV = 0x00; // no divide
  UART1->CTL0 &= ~0x01; // disable UART1 while configuring
  UART1->CTL0 = 0x00020010; // FEN=1 (FIFO enable), HSE=00 (16x), TXE=1, RXE=0

  // 40,000,000 / 16 = 2,500,000 Hz sample clock
  // 2,500,000 / 115200 = 21.701 → IBRD=21, FBRD=round(0.701×64)=45
  UART1->IBRD = 21;
  UART1->FBRD = 45;
  UART1->LCRH = 0x00000030; // 8-bit, 1 stop, no parity
  UART1->CPU_INT.IMASK = 0;  // no interrupts needed for blind Tx
  UART1->CTL0 |= 0x01;      // enable UART1
}

//------------UART1_OutChar------------
// Output 8-bit to serial port
// blind synchronization: busy-wait until Tx FIFO has room
// 10 bit frame (start + 8 data + stop), 2375 baud
// Input: data is an 8-bit ASCII character to be transferred
// Output: none
void UART1_OutChar(char data) {
  while (UART1->STAT & 0x80) {} // wait while Tx FIFO full
  UART1->TXDATA = data;
}

//------------UART1_OutString------------
// Output null-terminated string to Wemos D1
// Calls UART1_OutChar for each character, ends with newline
// Input: str pointer to null-terminated ASCII string
// Output: none
void UART1_OutString(char *str) {
  while (*str) {
    UART1_OutChar(*str++);
  }
  UART1_OutChar('\n'); // Wemos expects newline to process the command
}
