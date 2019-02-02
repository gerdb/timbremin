/**
 *  Project     timbremin
 *  @file		console.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for console.c
 *
 *  @copyright	GPL3
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CONSOLE_H_
#define CONSOLE_H_

/* Typedefs ------------------------------------------------------------------*/

/* Global variables  ---------------------------------------------------------*/
extern UART_HandleTypeDef UartHandle;


/* Defines -------------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

// Transmit buffer with read and write pointer
#define CONSOLE_TX_SIZE 1024
#define CONSOLE_TX_MASK (CONSOLE_TX_SIZE-1)

// Receive buffer with read and write pointer
#define CONSOLE_RX_SIZE 1024
#define CONSOLE_RX_MASK (CONSOLE_RX_SIZE-1)


/* Function Prototypes --------------------------------------------------------*/
void CONSOLE_Init(UART_HandleTypeDef huart);
void CONSOLE_RxBufferTask(void);
int CONSOLE_RxBufferNotEmpty(void);
void CONSOLE_PutByte(UART_HandleTypeDef *huart, uint8_t b);
void CONSOLE_IRQHandler(UART_HandleTypeDef *huart);

#endif /* USART_H_ */
