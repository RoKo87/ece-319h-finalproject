/* UART2.cpp
 * Your name
 * Data:
 * PA22 UART2 Rx from other microcontroller PA8 IR output<br>
 */


#include <ti/devices/msp/msp.h>
#include "UART2.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "FIFO2.h"

uint32_t LostData;
Queue FIFO2;

// power Domain PD0
// for 80MHz bus clock, UART2 clock is ULPCLK 40MHz
// initialize UART2 for 2375 baud rate
// no transmit, interrupt on receive timeout
void UART2_Init(void){
    // RSTCLR to GPIOA and UART2 peripherals
   // write this
   UART2->GPRCM.RSTCTL = 0xB1000003; // reset UART0
   UART2->GPRCM.PWREN = 0x26000001; // activate UART0
   Clock_Delay(24); // time for uart to activate
   IOMUX->SECCFG.PINCM[PA22INDEX] = 0x00000082;
   UART2->CLKSEL = 0x08; // bus clock
   UART2->CLKDIV = 0x00; // no divide
   UART2->CTL0 &= ~0x01; // disable UART0
   UART2->CTL0 = 0x00020018; // enable fifos, tx and rx

   UART2->IBRD = 21; // 115200 baud at 40MHz
   UART2->FBRD = 45;
   UART2->LCRH = 0x00000030; // 8bit, 1 stop, no parity
   UART2->CPU_INT.IMASK = 0x01; // Enable RTOUT interrupt
   UART2->IFLS = (4 << 8);
   UART2->CTL0 |= 0x01; // enable UART0
}
//------------UART2_InChar------------
// Get new serial port receive data from FIFO2
// Input: none
// Output: Return 0 if the FIFO2 is empty
//         Return nonzero data from the FIFO1 if available
char UART2_InChar(void){char out;
// write this
  if (FIFO2.IsEmpty()) return 0;
  else return FIFO2.Get();
}

extern "C" void UART2_IRQHandler(void);
void UART2_IRQHandler(void){ uint32_t status; char letter;
  status = UART2->CPU_INT.IIDX; // reading clears bit in RTOUT
  if(status == 0x01){   // 0x01 receive timeout
    GPIOB->DOUTTGL31_0 = BLUE; // toggle PB22 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = BLUE; // toggle PB22 (minimally intrusive debugging)
    // read all data, putting in FIFO
    while((UART2->STAT&0x04) == 0){
          letter = (char)(UART2->RXDATA);
          FIFO2.Put(letter);
    }

    // finish writing this
    status = 1;
    GPIOB->DOUTTGL31_0 = BLUE; // toggle PB22 (minimally intrusive debugging)
  }
}
