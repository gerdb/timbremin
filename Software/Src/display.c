/**
 *  Project     timbremin
 *  @file		display.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		LED pitch display
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

#include "../Drivers/BSP/STM32F4-Discovery/stm32f4_discovery.h"
#include "stm32f4xx_hal.h"
#include "display.h"


/**
 * @brief generates a Knight Rider effect during the auto tune procedure
 *
 */
void DISPLAY_AutotuneDisplay(void)
{
	static int timer1 = 0;
	static int counter = 0;

	// Speed: Next LED every 40ms
	timer1 ++;
	if (timer1 == 40)
	{
		timer1 = 0;

		//Count from 0 to 9
		counter ++;
		if (counter == 10)
		{
			counter = 0;
		}
	}

	// Generate a Knight Rider effect
	if (counter == 0)
	{
		HAL_GPIO_WritePin(PITCH_LED_0_GPIO_Port, PITCH_LED_0_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_11_GPIO_Port, PITCH_LED_11_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_1_GPIO_Port, PITCH_LED_1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PITCH_LED_10_GPIO_Port, PITCH_LED_10_Pin, GPIO_PIN_RESET);
	}

	if (counter == 1)
	{
		HAL_GPIO_WritePin(PITCH_LED_1_GPIO_Port, PITCH_LED_1_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_10_GPIO_Port, PITCH_LED_10_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_0_GPIO_Port, PITCH_LED_0_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PITCH_LED_11_GPIO_Port, PITCH_LED_11_Pin, GPIO_PIN_RESET);
	}

	if (counter == 2)
	{
		HAL_GPIO_WritePin(PITCH_LED_2_GPIO_Port, PITCH_LED_2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_9_GPIO_Port, PITCH_LED_9_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_1_GPIO_Port, PITCH_LED_1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PITCH_LED_10_GPIO_Port, PITCH_LED_10_Pin, GPIO_PIN_RESET);
	}

	if (counter == 3)
	{
		HAL_GPIO_WritePin(PITCH_LED_3_GPIO_Port, PITCH_LED_3_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_8_GPIO_Port, PITCH_LED_8_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_2_GPIO_Port, PITCH_LED_2_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PITCH_LED_9_GPIO_Port, PITCH_LED_9_Pin, GPIO_PIN_RESET);
	}

	if (counter == 4)
	{
		HAL_GPIO_WritePin(PITCH_LED_4_GPIO_Port, PITCH_LED_4_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_7_GPIO_Port, PITCH_LED_7_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_3_GPIO_Port, PITCH_LED_3_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PITCH_LED_8_GPIO_Port, PITCH_LED_8_Pin, GPIO_PIN_RESET);
	}

	if (counter == 5)
	{
		HAL_GPIO_WritePin(PITCH_LED_5_GPIO_Port, PITCH_LED_5_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_6_GPIO_Port, PITCH_LED_6_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_4_GPIO_Port, PITCH_LED_4_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PITCH_LED_7_GPIO_Port, PITCH_LED_7_Pin, GPIO_PIN_RESET);
	}

	if (counter == 6)
	{
		HAL_GPIO_WritePin(PITCH_LED_4_GPIO_Port, PITCH_LED_4_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_7_GPIO_Port, PITCH_LED_7_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_5_GPIO_Port, PITCH_LED_5_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PITCH_LED_6_GPIO_Port, PITCH_LED_6_Pin, GPIO_PIN_RESET);
	}

	if (counter == 7)
	{
		HAL_GPIO_WritePin(PITCH_LED_3_GPIO_Port, PITCH_LED_3_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_8_GPIO_Port, PITCH_LED_8_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_4_GPIO_Port, PITCH_LED_4_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PITCH_LED_7_GPIO_Port, PITCH_LED_7_Pin, GPIO_PIN_RESET);
	}

	if (counter == 8)
	{
		HAL_GPIO_WritePin(PITCH_LED_2_GPIO_Port, PITCH_LED_2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_9_GPIO_Port, PITCH_LED_9_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_3_GPIO_Port, PITCH_LED_3_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PITCH_LED_8_GPIO_Port, PITCH_LED_8_Pin, GPIO_PIN_RESET);
	}

	if (counter == 9)
	{
		HAL_GPIO_WritePin(PITCH_LED_1_GPIO_Port, PITCH_LED_1_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_10_GPIO_Port, PITCH_LED_10_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PITCH_LED_2_GPIO_Port, PITCH_LED_2_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PITCH_LED_9_GPIO_Port, PITCH_LED_9_Pin, GPIO_PIN_RESET);
	}

}


/**
 * @brief Displays the pitch with 12 LEDs
 *
 * @param fAudioFrequency Audio frequency in Hz
 *
 */
void DISPLAY_PitchDisplay(float fAudioFrequency)
{

	// Shift it 4 octaves up
	// So we can display notes down to C2 or 16Hz
	for (int i=0;i<4; i++)
	{
		// Shift it one octave if frequency is lower than c1
		// So we can display notes down to c or 130Hz
		if (fAudioFrequency <= 254.284f)
		{
			// double the frequency
			fAudioFrequency = fAudioFrequency *2.0f;
		}
	}

	// Shift it 3 octaves down
	// So we can display notes up to h4 or 3951Hz
	for (int i=0;i<3; i++)
	{
		// Shift it one octave down, if frequency is higher than h1
		// So we can display notes up to h2 or 987Hz
		if (fAudioFrequency > 508.567f)
		{
			// halve the frequency
			fAudioFrequency = fAudioFrequency *0.5f;
		}
	}

	// Convert frequency to a note
	e_note note = NOTE_NONE;
	if (fAudioFrequency > 254.284f)
	{
		if (fAudioFrequency <= 269.4045f)
		{
			note = NOTE_C;
		}
		else if (fAudioFrequency <= 285.424f)
		{
			note = NOTE_CIS;
		}
		else if (fAudioFrequency <= 302.396f)
		{
			note = NOTE_D;
		}
		else if (fAudioFrequency <= 320.3775f)
		{
			note = NOTE_DIS;
		}
		else if (fAudioFrequency <= 339.428f)
		{
			note = NOTE_E;
		}
		else if (fAudioFrequency <= 359.611f)
		{
			note = NOTE_F;
		}
		else if (fAudioFrequency <= 380.9945f)
		{
			note = NOTE_FIS;
		}
		else if (fAudioFrequency <= 403.65f)
		{
			note = NOTE_G;
		}
		else if (fAudioFrequency <= 427.6525f)
		{
			note = NOTE_GIS;
		}
		else if (fAudioFrequency <= 453.082f)
		{
			note = NOTE_A;
		}
		else if (fAudioFrequency <= 480.0235f)
		{
			note = NOTE_AIS;
		}
		else if (fAudioFrequency <= 508.567f)
		{
			note = NOTE_H;
		}
	}

	// Switch on the corresponding LED
	// Note: c
	HAL_GPIO_WritePin(PITCH_LED_0_GPIO_Port, PITCH_LED_0_Pin, note == NOTE_C);
	// Note: cis
	HAL_GPIO_WritePin(PITCH_LED_1_GPIO_Port, PITCH_LED_1_Pin, note == NOTE_CIS);
	// Note: d
	HAL_GPIO_WritePin(PITCH_LED_2_GPIO_Port, PITCH_LED_2_Pin, note == NOTE_D);
	// Note: dis
	HAL_GPIO_WritePin(PITCH_LED_3_GPIO_Port, PITCH_LED_3_Pin, note == NOTE_DIS);
	// Note: e
	HAL_GPIO_WritePin(PITCH_LED_4_GPIO_Port, PITCH_LED_4_Pin, note == NOTE_E);
	// Note: f
	HAL_GPIO_WritePin(PITCH_LED_5_GPIO_Port, PITCH_LED_5_Pin, note == NOTE_F);
	// Note: fis
	HAL_GPIO_WritePin(PITCH_LED_6_GPIO_Port, PITCH_LED_6_Pin, note == NOTE_FIS);
	// Note: g
	HAL_GPIO_WritePin(PITCH_LED_7_GPIO_Port, PITCH_LED_7_Pin, note == NOTE_G);
	// Note: gis
	HAL_GPIO_WritePin(PITCH_LED_8_GPIO_Port, PITCH_LED_8_Pin, note == NOTE_GIS);
	// Note: a
	HAL_GPIO_WritePin(PITCH_LED_9_GPIO_Port, PITCH_LED_9_Pin, note == NOTE_A);
	// Note: ais
	HAL_GPIO_WritePin(PITCH_LED_10_GPIO_Port, PITCH_LED_10_Pin, note == NOTE_AIS);
	// Note: h
	HAL_GPIO_WritePin(PITCH_LED_11_GPIO_Port, PITCH_LED_11_Pin, note == NOTE_H);
}

/**
 * @brief switchs off every LED
 *
 */
void DISPLAY_Dark(void)
{
	HAL_GPIO_WritePin(PITCH_LED_0_GPIO_Port, PITCH_LED_0_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_1_GPIO_Port, PITCH_LED_1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_2_GPIO_Port, PITCH_LED_2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_3_GPIO_Port, PITCH_LED_3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_4_GPIO_Port, PITCH_LED_4_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_5_GPIO_Port, PITCH_LED_5_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_6_GPIO_Port, PITCH_LED_6_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_7_GPIO_Port, PITCH_LED_7_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_8_GPIO_Port, PITCH_LED_8_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_9_GPIO_Port, PITCH_LED_9_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_10_GPIO_Port, PITCH_LED_10_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PITCH_LED_11_GPIO_Port, PITCH_LED_11_Pin, GPIO_PIN_RESET);
}
