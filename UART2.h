/*!
 * @defgroup UART
 * @brief Asynchronous serial communication
 <table>
<caption id="UARTpins6">UART pins on the MSPM0G3507</caption>
<tr><th>Pin <th>Description
<tr><td>PA22 <td>UART1 Rx from other microcontroller PA8 Tx
</table>
 * @{*/
/**
 * @file      UART2.h
 * @brief     Initialize UART2, no transmit, interrupt on receive timeout
 * @details   UART2 initialization. 2375 bps baud,
 * 1 start, 8 data bits, 1 stop, no parity.<br>

 * @version   ECE319K v1.2
 * @author    your name
 * @copyright lab 8
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      October 29, 2023
 <table>
<caption id="UARTpins7">UART pins on the MSPM0G3507</caption>
<tr><th>Pin  <th>Description
<tr><td>PA22 <td>UART2 Rx from other microcontroller PA8 IR output
</table>
  ******************************************************************************/
#ifndef __UART2_H__
#define __UART2_H__

/**
 * initialize UART2 for 2375 bps baud rate.<br>
 * PA22 UART2 Rx from other microcontroller PA8 IR output<br>
 * no transmit, interrupt on receive timeout
 * @param none
 * @return none
 * @brief  Initialize UART2
*/
void UART2_Init(void);

/**
 * Get new serial port receive data from FIFO2<br>
 * Return 0 if the FIFO2 is empty<br>
 * Return nonzero data from the FIFO2 if available
 * @param none
 * @return char ASCII code from other computer
 * @brief get data from FIFO2
 */
char UART2_InChar(void);




#endif // __UART2_H__
/** @}*/
