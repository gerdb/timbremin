/**
 *  Project     timbremin
 *  @file		effect.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Echo and Reverb effect
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
#include <math.h>
#include <stdlib.h>
#include "audio_out.h"
#include "beep.h"
#include "theremin.h"
#include "effect.h"
#include "config.h"

/* local variables  ------------------------------------------------------- */

// Echo effect
int32_t slEchoDelay[4096];
int iEchoDelW = 0;
int iEchoDelR = 0;
int iEchoDelLength = 0;
int32_t slEchoIn;
int32_t slEchoDelayed;
int32_t slEchoOut;
int32_t slEchoFB = 0;
int32_t slEchoFW = 0;
int32_t slEchoNFW = 0;
int32_t slEchoIN = 0;
int32_t slEchoNIN = 0;




// The length of each delay block
#define DELAY1_LENGTH 12
#define DELAY2_LENGTH 42
#define DELAY3_LENGTH 902
#define DELAY4_LENGTH 17
#define DELAY5_LENGTH 59
#define DELAY6_LENGTH 1003
#define DELAY7_LENGTH 21
#define DELAY8_LENGTH 77
#define DELAY9_LENGTH 1272
#define DELAY10_LENGTH 39
#define DELAY11_LENGTH 101
#define DELAY12_LENGTH 1323

int32_t slD1[DELAY1_LENGTH];
int32_t slD2[DELAY2_LENGTH];
int32_t slD3[DELAY3_LENGTH];
int32_t slD4[DELAY4_LENGTH];
int32_t slD5[DELAY5_LENGTH];
int32_t slD6[DELAY6_LENGTH];
int32_t slD7[DELAY7_LENGTH];
int32_t slD8[DELAY8_LENGTH];
int32_t slD9[DELAY9_LENGTH];
int32_t slD10[DELAY10_LENGTH];
int32_t slD11[DELAY11_LENGTH];
int32_t slD12[DELAY12_LENGTH];

// Delay counter
uint32_t iDc1_in = 0;
uint32_t iDc2_in = 0;
uint32_t iDc3_in = 0;
uint32_t iDc4_in = 0;
uint32_t iDc5_in = 0;
uint32_t iDc6_in = 0;
uint32_t iDc7_in = 0;
uint32_t iDc8_in = 0;
uint32_t iDc9_in = 0;
uint32_t iDc10_in = 0;
uint32_t iDc11_in = 0;
uint32_t iDc12_in = 0;
// Initialize all output counter with +1, so it becomes a ring buffer
uint32_t iDc1_out = 1;
uint32_t iDc2_out = 1;
uint32_t iDc3_out = 1;
uint32_t iDc4_out = 1;
uint32_t iDc5_out = 1;
uint32_t iDc6_out = 1;
uint32_t iDc7_out = 1;
uint32_t iDc8_out = 1;
uint32_t iDc9_out = 1;
uint32_t iDc10_out = 1;
uint32_t iDc11_out = 1;
uint32_t iDc12_out = 1;
// Delays of the early reflections
uint32_t iDcEarly1 = 0;
uint32_t iDcEarly2 = 607;
uint32_t iDcEarly3 = 699;
uint32_t iDcEarly4 = 813;
uint32_t iDcEarly5 = 900;

// High pass filter
int32_t slReverbLP = 0;
int32_t slReverbHP = 0;

// Loop gain
int iKRT = 150;



/**
 * @brief Initialize the module
 *
 */
void EFFECT_Init(void)
{
}

/**
 * @brief slow task
 *
 */
void EFFECT_SlowTask(void)
{
	iEchoDelLength = 4 * aConfigWorkingSet[CFG_E_ECHO_DELAY].iVal;
	slEchoFB = (950 * aConfigWorkingSet[CFG_E_ECHO_FEEDBACK].iVal) / 1000;
	slEchoNFW = 1024 - (1024 * aConfigWorkingSet[CFG_E_ECHO_FORWARD].iVal) / 1000;
	slEchoFW =         (1024 * aConfigWorkingSet[CFG_E_ECHO_FORWARD].iVal) / 1000;
	slEchoNIN = 1024 - (1024 * aConfigWorkingSet[CFG_E_ECHO_INTENSITY].iVal) / 1000;
	slEchoIN =         (1024 * aConfigWorkingSet[CFG_E_ECHO_INTENSITY].iVal) / 1000;
}

/**
 * @brief calculate the Echo and reverb effect
 *
 */
inline void EFFECT_Task(void)
{
	// Mute the input of the reverb effect, but calculate the reverb
	// to prevents clicks after / before a beep
	if (bMute != 0 || bBeepActive )
	{
		slThereminOut = 0;
	}

	// Echo effect
	iEchoDelW++;
	iEchoDelW &= 0x00000FFF;
	iEchoDelR = (iEchoDelW - iEchoDelLength) & 0x00000FFF;

	slEchoIn = slThereminOut - (slEchoDelayed * slEchoFB) / 1024;
	slEchoDelay[iEchoDelW] = slEchoIn;
	slEchoDelayed = slEchoDelay[iEchoDelR];

	slEchoOut = (slEchoIN * ((slEchoNFW * slEchoIn + slEchoFW * slEchoDelayed)/1024) +
				 slEchoNIN * slThereminOut) / 1024;



	// https://valhalladsp.com/2010/08/25/rip-keith-barr/
	// http://www.spinsemi.com/knowledge_base/effects.html#Reverberation
	slD1[iDc1_in] =  slThereminOut - slD1[iDc1_out] / 2 + (iKRT * slD12[iDc12_out]) / 256;
	slD2[iDc2_in] = - slD2[iDc2_out] / 2 + slD1[iDc1_out] + slD1[iDc1_in] / 2;
	slD3[iDc3_in] =  slD2[iDc2_out] + slD2[iDc2_in] / 2;

	slD4[iDc4_in] =   slThereminOut - slD4[iDc4_out] / 2 + (iKRT * slD3[iDc3_out]) / 256;
	slD5[iDc5_in] = - slD5[iDc5_out] / 2 + slD4[iDc4_out] + slD4[iDc4_in] / 2;
	slD6[iDc6_in] =  slD5[iDc5_out] + slD5[iDc5_in] / 2;

	slD7[iDc7_in] =  slThereminOut - slD7[iDc7_out] / 2 + (iKRT * slD6[iDc6_out]) / 256;
	slD8[iDc8_in] = - slD8[iDc8_out] / 2 + slD7[iDc7_out] + slD7[iDc7_in] / 2;
	slD9[iDc9_in] =  slD8[iDc8_out] + slD8[iDc8_in] / 2;

	slD10[iDc10_in] =   slThereminOut - slD10[iDc10_out] / 2 + (iKRT * slD9[iDc9_out]) / 256;
	slD11[iDc11_in] = - slD11[iDc11_out] / 2 + slD10[iDc10_out] + slD10[iDc10_in] / 2;
	slD12[iDc12_in] =  slD11[iDc11_out] + slD11[iDc11_in] / 2;

	//slReverbLP += ( slD12[iDc12_out] * 256 - slReverbLP) / 512;
	//slReverbHP = slD12[iDc12_out] - slReverbLP / 256;

	// Shift all delays
	iDc1_in = iDc1_out;
	iDc2_in = iDc2_out;
	iDc3_in = iDc3_out;
	iDc4_in = iDc4_out;
	iDc5_in = iDc5_out;
	iDc6_in = iDc6_out;
	iDc7_in = iDc7_out;
	iDc8_in = iDc8_out;
	iDc9_in = iDc9_out;
	iDc10_in = iDc10_out;
	iDc11_in = iDc11_out;
	iDc12_in = iDc12_out;

	// Increment delay counter
	iDc1_out ++;
	iDc2_out ++;
	iDc3_out ++;
	iDc4_out ++;
	iDc5_out ++;
	iDc6_out ++;
	iDc7_out ++;
	iDc8_out ++;
	iDc9_out ++;
	iDc10_out ++;
	iDc11_out ++;
	iDc12_out ++;

	iDcEarly1 ++;
	iDcEarly2 ++;
	iDcEarly3 ++;
	iDcEarly4 ++;
	iDcEarly5 ++;

	if (iDc1_out == DELAY1_LENGTH) iDc1_out = 0;
	if (iDc2_out == DELAY2_LENGTH) iDc2_out = 0;
	if (iDc3_out == DELAY3_LENGTH) iDc3_out = 0;
	if (iDc4_out == DELAY4_LENGTH) iDc4_out = 0;
	if (iDc5_out == DELAY5_LENGTH) iDc5_out = 0;
	if (iDc6_out == DELAY6_LENGTH) iDc6_out = 0;
	if (iDc7_out == DELAY7_LENGTH) iDc7_out = 0;
	if (iDc8_out == DELAY8_LENGTH) iDc8_out = 0;
	if (iDc9_out == DELAY9_LENGTH) iDc9_out = 0;
	if (iDc10_out == DELAY10_LENGTH) iDc10_out = 0;
	if (iDc11_out == DELAY11_LENGTH) iDc11_out = 0;
	if (iDc12_out == DELAY11_LENGTH) iDc12_out = 0;

	if (iDcEarly1 == DELAY3_LENGTH) iDcEarly1 = 0;
	if (iDcEarly2 == DELAY3_LENGTH) iDcEarly2 = 0;
	if (iDcEarly3 == DELAY3_LENGTH) iDcEarly3 = 0;
	if (iDcEarly4 == DELAY3_LENGTH) iDcEarly4 = 0;
	if (iDcEarly5 == DELAY3_LENGTH) iDcEarly5 = 0;

	// Get the early reflections
	if (bMute == 0 && !bBeepActive )
	{
		//ssDACValueR =slD3[iDcEarly1];
		// Limit the output to 16bit
		int32_t slVal;

		slVal = (slThereminOut * 200 +
				slD3[iDcEarly1] * 50 +
				slD3[iDcEarly2] * 30 +
				slD3[iDcEarly3] * 20 +
				slD3[iDcEarly4] * 8 +
				slD3[iDcEarly5] * 5 ) / 256;

		slVal = slEchoOut;
		// Limit to 16 bit signed for DAC
		if (slVal > 32767)
		{
			slVal = 32767;
		}
		if (slVal < -32768)
		{
			slVal = -32768;
		}
		// Output it to the headphone (right channel)
		ssDACValueR = slVal;
		// and also to the speaker (left channel)
		ssDACValueL = slVal; //fLFO * 30000;
	}

}
