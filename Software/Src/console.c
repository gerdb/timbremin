/**
 *  Project     timbremin
 *  @file		console.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Console to access the configuration parameters
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

/* Includes -----------------------------------------------------------------*/

#include "../Drivers/BSP/STM32F4-Discovery/stm32f4_discovery.h"
#include "stm32f4xx_hal.h"
#include "theremin.h"
#include "console.h"
#include "config.h"
#include "printf.h"


/* local functions ----------------------------------------------------------*/
static void CONSOLE_Transmit(UART_HandleTypeDef *huart);
static void CONSOLE_Receive(UART_HandleTypeDef *huart);
static void CONSOLE_RxCheckBuffer(void);

/* local variables ----------------------------------------------------------*/
UART_HandleTypeDef UartHandle;

// Transmit buffer with read and write pointer
uint8_t CONSOLE_tx_buffer[CONSOLE_TX_SIZE];
uint8_t CONSOLE_txen = 0;
uint16_t CONSOLE_tx_wr_pointer = 0;
uint16_t CONSOLE_tx_rd_pointer = 0;

// Receive buffer with read and write pointer
uint8_t CONSOLE_rx_buffer[CONSOLE_RX_SIZE];
int CONSOLE_rx_wr_pointer = 0;
int CONSOLE_rx_rd_pointer = 0;

// Line buffer
char CONSOLE_LineBuffer[100];
int CONSOLE_LineCnt = 0;
// Line buffer with the previous line (recall with TAB key)
char CONSOLE_LineBufferLast[100];
int CONSOLE_LineCntLast = 0;

// Console mode
CONSOLE_eMode eDebugMode = CONSOLE_MODE_NONE;

/**
 * @brief  Initialize the console
 * @param  None
 * @retval None
 */
void CONSOLE_Init(UART_HandleTypeDef huart) {

	// Use the USARTx
	UartHandle = huart;

	//Rset buffer
	CONSOLE_rx_wr_pointer = 0;
	CONSOLE_rx_rd_pointer = 0;
	CONSOLE_txen = 0;

	HAL_UART_Receive_IT(&UartHandle, (uint8_t *) CONSOLE_rx_buffer,	CONSOLE_RX_SIZE);

	// Start console
	my_printf("\r\nTimbremin v1.0.0 - www.sebulli.com\r\n>");
}

/**
 * @brief  Check, if there is something in the receive buffer to decode
 * @param  None
 * @retval None
 */
static void CONSOLE_RxCheckBuffer(void) {

	char c;

	// Check if there are characters in the receive buffe
	if (CONSOLE_RxBufferNotEmpty()) {

		// Get the next character from the RX buffer
		CONSOLE_rx_rd_pointer++;
		CONSOLE_rx_rd_pointer &= CONSOLE_RX_MASK;
		c = CONSOLE_rx_buffer[CONSOLE_rx_rd_pointer];


		// It was the TAB key
		if (c == '\t')
		{
			// clear the current line
			for (int i=0; i<CONSOLE_LineCnt;i++)
			{
				CONSOLE_PutByte(&UartHandle, 127);
			}

			// And fill it with the last line
			for (int i=0; i<CONSOLE_LineCntLast;i++)
			{
				CONSOLE_LineBuffer[i] = CONSOLE_LineBufferLast[i];
				CONSOLE_PutByte(&UartHandle, CONSOLE_LineBuffer[i]);
			}
			CONSOLE_LineCnt = CONSOLE_LineCntLast;
		}
		// It was the ENTER key
		else if (c == 0x0d)
		{
			CONSOLE_LineCntLast = 0;

			// Copy all characters of this line into the buffer to recall them with TAB
			int endFound = 0;
			for (int i=0; i<CONSOLE_LineCnt && !endFound ;i++)
			{
				// Do it up to the '=' or '?'
				if ((CONSOLE_LineBuffer[i] == '=') || (CONSOLE_LineBuffer[i] == '?'))
				{
					endFound = 1;
				}
				CONSOLE_LineBufferLast[i] = CONSOLE_LineBuffer[i];
				CONSOLE_LineCntLast = i+1;
			}

			// Decode the line and outpu the result
			CONSOLE_LineBuffer[CONSOLE_LineCnt] = '\0';
			CONSOLE_PutByte(&UartHandle, ' ');
			my_printf(CONFIG_DecodeLine(CONSOLE_LineBuffer));

			// Next line of the console
			CONSOLE_PutByte(&UartHandle, '\r');
			CONSOLE_PutByte(&UartHandle, '\n');
			CONSOLE_PutByte(&UartHandle, '>');
			CONSOLE_LineCnt = 0;
		}
		else
		{
			// It was a normal character
			if (CONSOLE_LineCnt < 80 && c >=32 && c <127)
			{
				// Store it and output it
				CONSOLE_LineBuffer[CONSOLE_LineCnt] = c;
				CONSOLE_PutByte(&UartHandle, c);
				CONSOLE_LineCnt ++;
			}
			// It was a "BACKSPACE", delete the last character
			else if (CONSOLE_LineCnt > 0 && c == 127)
			{
				CONSOLE_PutByte(&UartHandle, c);
				CONSOLE_LineCnt --;
			}
		}
	}
}


/**
 * @brief  1ms Task
 *
 * @param  None
 * @return  None
 */
void CONSOLE_1msTask(void)
{
	static int consoleTastCnt = 0;
	uint32_t frq1=0;

	CONSOLE_RxCheckBuffer();

	// Generate 200ms Task for debug information
	consoleTastCnt++;
	if (consoleTastCnt > 200)
	{
		consoleTastCnt = 0;

		switch (eDebugMode)
		{
		case CONSOLE_MODE_OSCILLATOR_PITCH:
			if (aOsc[PITCH].usPeriodRawN > 0)
			{
				frq1 = (168000*8*aOsc[PITCH].ulCalibScale) / aOsc[PITCH].usPeriodRawN;
			}
			my_printf("%4dkHz %4dkHz %4d\r\n", (int)frq1, (int)((float)aOsc[PITCH].usPeriodRawN * aOsc[PITCH].fCalibfScale), aOsc[PITCH].usPeriodRaw);
			break;
		case CONSOLE_MODE_OSCILLATOR_VOL1:
			if (aOsc[VOL1].usPeriodRawN > 0)
			{
				frq1 = (168000*8*1) / aOsc[VOL1].usPeriodRawN;
			}
			my_printf("%4dkHz %4d\r\n",frq1, aOsc[VOL1].usPeriodRaw);
			break;
		case CONSOLE_MODE_OSCILLATOR_VOL2:
			if (aOsc[VOL2].usPeriodRawN > 0)
			{
				frq1 = (168000*8*1) / aOsc[VOL2].usPeriodRawN;
			}
			my_printf("%4dkHz %4d\r\n",frq1, aOsc[VOL2].usPeriodRaw);
			break;
		case CONSOLE_MODE_STOPWATCH:
			my_printf("%4d\r\n",ulStopwatch);
			break;

		default: ;
		}
	}

}


/**
 * @brief  Returns, whether the receive buffer is empty or not
 *
 * @param  None
 * @return =0: there was no byte in the receive buffer
 *         >0: there was at least one byte in the receive buffer.
 */
int CONSOLE_RxBufferNotEmpty(void) {

	return CONSOLE_rx_wr_pointer != CONSOLE_rx_rd_pointer;
}

/**
 * @brief  Write a new character into the transmit buffer
 *         This function is called by the printf function
 *
 * @param  huart handle to uart driver
 * @param  b character to send
 */
void CONSOLE_PutByte(UART_HandleTypeDef *huart, uint8_t b) {
	__HAL_UART_DISABLE_IT(huart, UART_IT_TXE);

	// Send the byte
	if (!CONSOLE_txen) {
		// Write the first byte directly into the USART
		CONSOLE_txen = 1;
		huart->Instance->DR = b;
	} else {


		while (((CONSOLE_tx_wr_pointer + 1) & CONSOLE_TX_MASK)
				== CONSOLE_tx_rd_pointer)
		{
			__HAL_UART_ENABLE_IT(huart, UART_IT_TXE);
			__HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
		}

		// Write the next character into the buffer
		CONSOLE_tx_wr_pointer++;
		CONSOLE_tx_wr_pointer &= CONSOLE_TX_MASK;
		if (CONSOLE_tx_wr_pointer == CONSOLE_tx_rd_pointer) {
			CONSOLE_tx_wr_pointer--;
			CONSOLE_tx_wr_pointer &= CONSOLE_TX_MASK;
		}

		CONSOLE_tx_buffer[CONSOLE_tx_wr_pointer] = b;
		//__HAL_UART_ENABLE_IT(huart, UART_IT_TXE);
	}

	__HAL_UART_ENABLE_IT(huart, UART_IT_TXE);
}

/**
 * @brief  This function handles UART interrupt request.
 * @param  huart: UART handle
 * @retval None
 */
void CONSOLE_IRQHandler(UART_HandleTypeDef *huart) {
	uint32_t tmp1 = 0, tmp2 = 0;

	tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_PE);
	tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_PE);
	/* UART parity error interrupt occurred ------------------------------------*/
	if ((tmp1 != RESET) && (tmp2 != RESET)) {
		__HAL_UART_CLEAR_FLAG(huart, UART_FLAG_PE);

		huart->ErrorCode |= HAL_UART_ERROR_PE;
	}

	tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_FE);
	tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
	/* UART frame error interrupt occurred -------------------------------------*/
	if ((tmp1 != RESET) && (tmp2 != RESET)) {
		__HAL_UART_CLEAR_FLAG(huart, UART_FLAG_FE);

		huart->ErrorCode |= HAL_UART_ERROR_FE;
	}

	tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_NE);
	tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
	/* UART noise error interrupt occurred -------------------------------------*/
	if ((tmp1 != RESET) && (tmp2 != RESET)) {
		__HAL_UART_CLEAR_FLAG(huart, UART_FLAG_NE);

		huart->ErrorCode |= HAL_UART_ERROR_NE;
	}

	tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_ORE);
	tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
	/* UART Over-Run interrupt occurred ----------------------------------------*/
	if ((tmp1 != RESET) && (tmp2 != RESET)) {
		__HAL_UART_CLEAR_FLAG(huart, UART_FLAG_ORE);

		huart->ErrorCode |= HAL_UART_ERROR_ORE;
	}

	tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE);
	tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE);
	/* UART in mode Receiver ---------------------------------------------------*/
	if ((tmp1 != RESET) && (tmp2 != RESET)) {
		__HAL_UART_CLEAR_FLAG(huart, UART_FLAG_RXNE);
		CONSOLE_Receive(huart);
	}

	tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_TXE);
	tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TXE);
	/* UART in mode Transmitter ------------------------------------------------*/
	if ((tmp1 != RESET) && (tmp2 != RESET)) {
		__HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TXE);
		CONSOLE_Transmit(huart);
	}

	if (huart->ErrorCode != HAL_UART_ERROR_NONE) {
		/* Set the UART state ready to be able to start again the process */
		huart->gState = HAL_UART_STATE_READY;

		HAL_UART_ErrorCallback(huart);
	}
}

/**
 * @brief  Transmit an amount of data in non blocking mode
 * @param  huart: UART handle
 * @retval None
 */
static void CONSOLE_Transmit(UART_HandleTypeDef *huart) {
	//Increment the read pointer of the TX buffer
	if (CONSOLE_tx_wr_pointer != CONSOLE_tx_rd_pointer) {
		CONSOLE_tx_rd_pointer++;
		CONSOLE_tx_rd_pointer &= CONSOLE_TX_MASK;
		huart->Instance->DR = CONSOLE_tx_buffer[CONSOLE_tx_rd_pointer];

	} else {
		/* Disable the UART Transmit Complete Interrupt */
		__HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
		// This was the last byte to send, disable the transmission.
		CONSOLE_txen = 0;
	}
}

/**
 * @brief  Receives an amount of data in non blocking mode
 * @param  huart: UART handle
 * @retval HAL status
 */
static void CONSOLE_Receive(UART_HandleTypeDef *huart) {
	uint8_t c;

	// Increment the buffer pointer, if it's possible
	CONSOLE_rx_wr_pointer++;
	CONSOLE_rx_wr_pointer &= CONSOLE_RX_MASK;
	if (CONSOLE_rx_wr_pointer == CONSOLE_rx_rd_pointer) {
		CONSOLE_rx_wr_pointer--;
		CONSOLE_rx_wr_pointer &= CONSOLE_RX_MASK;
	}

	// Write the received byte
	c = (uint8_t) (huart->Instance->DR & (uint8_t) 0x00FF);
	CONSOLE_rx_buffer[CONSOLE_rx_wr_pointer] = c;
}

/**
 * @brief  UART error callbacks
 * @param  UartHandle: UART handle
 * @note   This example shows a simple way to report transfer error, and you can
 *         add your own implementation.
 * @retval None
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle) {
	/* Turn LED3 on: Transfer error in reception/transmission process */
}

/**
 * @brief  Tx Transfer completed callback
 * @param  UartHandle: UART handle.
 * @note   This example shows a simple way to report end of IT Tx transfer, and
 *         you can add your own implementation.
 * @retval None
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle) {
	/* Set transmission flag: trasfer complete*/

}
