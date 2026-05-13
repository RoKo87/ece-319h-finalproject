/*!
 * @defgroup UART1
 * @brief UART1 transmit to Wemos D1
 <table>
<caption id="UART1pins">UART pins on the MSPM0G3507</caption>
<tr><th>Pin <th>Description
<tr><td>PA8 <td>UART1 Tx to Wemos D1 Rx
</table>
 * @{*/
/**
 * @file      UART1.h
 * @brief     Initialize UART1 transmit to Wemos D1
 * @details   UART1 initialization. 2375 bps baud,
 * 1 start, 8 data bits, 1 stop, no parity.<br>

 * @version   ECE319K v1.2
 * @author    your name
 * @copyright lab 9H
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      April 2026
 <table>
<caption id="UART1pins2">UART pins on the MSPM0G3507</caption>
<tr><th>Pin  <th>Description
<tr><td>PA8 <td>UART1 Tx to Wemos D1 Rx
</table>
  ******************************************************************************/
#ifndef __UART1_H__
#define __UART1_H__

/**
 * Initialize UART1 on PA8.<br>
 * PA8 UART1 Tx to Wemos D1 Rx<br>
 * no synchronization on transmit, no receiving<br>
 * Baud rate = 2375 bps
 * @param none
 * @return none
 * @brief  Initialize UART1 transmit
*/
void UART1_Init(void);

/**
 * Output 8-bit to UART1 transmitter<br>
 * Uses blind synchronization (busy-wait on Tx FIFO)<br>
 * start=0, bit0..bit7, stop=1
 * @param data is an 8-bit ASCII character to be transferred
 * @return none
 * @brief output one character to UART1
 */
void UART1_OutChar(char data);

/**
 * Output null-terminated string to UART1<br>
 * Calls UART1_OutChar for each character
 * @param str pointer to null-terminated ASCII string
 * @return none
 * @brief output string to UART1
 */
void UART1_OutString(char *str);

#endif // __UART1_H__
/** @}*/
