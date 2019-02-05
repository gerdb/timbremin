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

// Chorus effect
int32_t slChorusDelay[4096];
int iChorusDelW = 0;
int iChorusDelR1 = 0;
int iChorusDelR2 = 0;
int iChorusDelR2n = 0;
int iChorusDelLength1 = 0;
int iChorusDelLength2 = 0;

int32_t slChorusIn;
int32_t slChorusDelayed2;
float fChorusOut;
int32_t slChorusFB = 0;
int32_t slChorusINT = 0;
int32_t slChorusNINT = 0;
float fLFOSin = 0.0f;
float fLFOCos = 1.0f;
float fLFOFrq = 0.0f;
float fLFOCorr;
float fLFOModulationGain = 0.0f;
int32_t slLFOModulationOffset = 0;

float fLPout = 0.0f;
float fLPfeedback = 0.0f;
float fLPdamp2 = 0.0f;
float fLPdamp1 = 0.0f;

float fLBCF1 = 0.0f;
float fLBCF2 = 0.0f;
float fLBCF3 = 0.0f;
float fLBCF4 = 0.0f;
float fLBCF5 = 0.0f;
float fLBCF6 = 0.0f;
float fLBCF7 = 0.0f;
float fLBCF8 = 0.0f;

float fLP1 = 0.0f;
float fLP2 = 0.0f;
float fLP3 = 0.0f;
float fLP4 = 0.0f;
float fLP5 = 0.0f;
float fLP6 = 0.0f;
float fLP7 = 0.0f;
float fLP8 = 0.0f;

float fAP1out = 0.0f;
float fAP2out = 0.0f;
float fAP3out = 0.0f;
float fAP4out = 0.0f;

float fReverb_out = 0.0f;
float fReverbINT = 0.0f;
float fReverbNINT = 0.0f;

// The length of each delay block
#define DELAY1_LENGTH 1557
#define DELAY2_LENGTH 1617
#define DELAY3_LENGTH 1491
#define DELAY4_LENGTH 1422
#define DELAY5_LENGTH 1277
#define DELAY6_LENGTH 1356
#define DELAY7_LENGTH 1188
#define DELAY8_LENGTH 1116

#define ALLPASS1_LENGTH 225
#define ALLPASS2_LENGTH 556
#define ALLPASS3_LENGTH 441
#define ALLPASS4_LENGTH 341


float fD1[DELAY1_LENGTH];
float fD2[DELAY2_LENGTH];
float fD3[DELAY3_LENGTH];
float fD4[DELAY4_LENGTH];
float fD5[DELAY5_LENGTH];
float fD6[DELAY6_LENGTH];
float fD7[DELAY7_LENGTH];
float fD8[DELAY8_LENGTH];

float fAP1[ALLPASS1_LENGTH];
float fAP2[ALLPASS2_LENGTH];
float fAP3[ALLPASS3_LENGTH];
float fAP4[ALLPASS4_LENGTH];


// Delay counter
// Initialize all output counter with +1, so it becomes a ring buffer
uint32_t iDc1 = 0;
uint32_t iDc2 = 0;
uint32_t iDc3 = 0;
uint32_t iDc4 = 0;
uint32_t iDc5 = 0;
uint32_t iDc6 = 0;
uint32_t iDc7 = 0;
uint32_t iDc8 = 0;

uint32_t iAPc1 = 0;
uint32_t iAPc2 = 0;
uint32_t iAPc3 = 0;
uint32_t iAPc4 = 0;


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
	// Delay length from 0..4000 = 0..41ms
	iChorusDelLength1 = 2 * aConfigWorkingSet[CFG_E_CHORUS_DELAY].iVal;
	// Feedback max 0.7
	slChorusFB = (700 * aConfigWorkingSet[CFG_E_CHORUS_FEEDBACK].iVal) / 1000;

	slChorusNINT = 1024 - (1024 * aConfigWorkingSet[CFG_E_CHORUS_INTENSITY].iVal) / 1000;
	slChorusINT =         (1024 * aConfigWorkingSet[CFG_E_CHORUS_INTENSITY].iVal) / 1000;

	// Frequency from 0 .. 20.0Hz
	fLFOFrq = aConfigWorkingSet[CFG_E_CHORUS_FREQUENCY].iVal * 0.02 * (2.0f * M_PI / 48000.0f) ;

	//iChorusDelLength2 = fLFOSin * fLFOModulationGain + fLFOModulationOffset;

	// Modulation gain: Sin from ±0 to ±20%
	// *0.05f = 0.2 /1024f * 256.0f (Scaled by factor 256)
	fLFOModulationGain = iChorusDelLength1 * aConfigWorkingSet[CFG_E_CHORUS_MODULATION].iVal * 0.05f;
	// Scaled by factor 256
	slLFOModulationOffset = iChorusDelLength1 * 256;


	// https://ccrma.stanford.edu/~jos/pasp/Freeverb.html
	// Freeverb parameter
	fLPfeedback = aConfigWorkingSet[CFG_E_REVERB_ROOMSIZE].iVal * 0.00028f + 0.7f;
	fLPdamp2 = aConfigWorkingSet[CFG_E_REVERB_DAMPING].iVal * 0.0004f;
	fLPdamp1 = 1.0f - fLPdamp2;

	fReverbINT = aConfigWorkingSet[CFG_E_REVERB_INTENSITY].iVal * 0.001f;
	fReverbNINT = 1.0f - fReverbINT;
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

	// ******************** Chorus effect ********************

	// The LFO, a sine generator with amplitude ±1.0
	fLFOSin += fLFOFrq * fLFOCos;
	fLFOCos -= fLFOFrq * fLFOSin;
	fLFOCorr = 1.0f +((1.0f - (fLFOSin * fLFOSin + fLFOCos * fLFOCos))*0.0001f);
	fLFOSin *= fLFOCorr;
	fLFOCos *= fLFOCorr;

	// Modulate the delay
	iChorusDelLength2 = (int32_t)(fLFOSin * fLFOModulationGain) + slLFOModulationOffset;

	// Write index of delay ring buffer
	iChorusDelW++;
	iChorusDelW &= 0x00000FFF;
	// Read index of delay ring buffer for first, fix delay
	iChorusDelR1 = (iChorusDelW - iChorusDelLength1) & 0x00000FFF;
	// Read the fix delayed signal

	slChorusIn = slThereminOut - (slChorusDelay[iChorusDelR1] * slChorusFB) / 1024;
	slChorusDelay[iChorusDelW] = slChorusIn;


	// Read index of delay ring buffer for second, modulated delay
	iChorusDelR2  = (iChorusDelW - (iChorusDelLength2 / 256) - 0) & 0x00000FFF;
	iChorusDelR2n = (iChorusDelW - (iChorusDelLength2 / 256) - 1) & 0x00000FFF;
	// Read the delayed entry and interpolate between 2 points for less aliasing distortion
	slChorusDelayed2 = (
					   slChorusDelay[iChorusDelR2]  * (255 - (iChorusDelLength2 & 0x00FF))
					 + slChorusDelay[iChorusDelR2n] * (      (iChorusDelLength2 & 0x00FF))
					 ) / 256;

	fChorusOut = (slChorusNINT * slChorusIn + slChorusINT * slChorusDelayed2) * 0.000976562f;




	// ******************** Reverb effect ********************

	// Reverb algorithm: Freeverb https://ccrma.stanford.edu/~jos/pasp/Freeverb.html

	// 8 lowpass-feedback-comb-filter
	fLBCF1 = fD1[iDc1];
	fLP1 =  fLBCF1 * fLPdamp2 + fLP1 * fLPdamp1;
	fD1[iDc1] = fChorusOut + fLP1 * fLPfeedback;

	fLBCF2 = fD2[iDc2];
	fLP2 =  fLBCF2 * fLPdamp2 + fLP2 * fLPdamp1;
	fD2[iDc2] = fChorusOut + fLP2 * fLPfeedback;

	fLBCF3 = fD3[iDc3];
	fLP3 =  fLBCF3 * fLPdamp2 + fLP3 * fLPdamp1;
	fD3[iDc3] = fChorusOut + fLP3 * fLPfeedback;

	fLBCF4 = fD4[iDc4];
	fLP4 =  fLBCF4 * fLPdamp2 + fLP4 * fLPdamp1;
	fD4[iDc4] = fChorusOut + fLP4 * fLPfeedback;

	fLBCF5 = fD5[iDc5];
	fLP5 =  fLBCF5 * fLPdamp2 + fLP5 * fLPdamp1;
	fD5[iDc5] = fChorusOut + fLP5 * fLPfeedback;

	fLBCF6 = fD6[iDc6];
	fLP6 =  fLBCF6 * fLPdamp2 + fLP6 * fLPdamp1;
	fD6[iDc6] = fChorusOut + fLP6 * fLPfeedback;

	fLBCF7 = fD7[iDc7];
	fLP7 =  fLBCF7 * fLPdamp2 + fLP7 * fLPdamp1;
	fD7[iDc7] = fChorusOut + fLP7 * fLPfeedback;

	// Sum of all 8 lowpass-feedback-comb-filters
	float fLBCFsum = (fLBCF1 + fLBCF2 + fLBCF3 + fLBCF4 + fLBCF5 + fLBCF6 + fLBCF7 + fLBCF8) * 0.125f;

	// 4 allpass filter
	fAP1out = fAP1[iAPc1] - 0.5f * fLBCFsum;
	fAP1[iAPc1] = 0.5f * fAP1out + fLBCFsum;

	fAP2out = fAP2[iAPc2] - 0.5f * fAP1out;
	fAP2[iAPc2] = 0.5f * fAP2out + fAP1out;

	fAP3out = fAP3[iAPc3] - 0.5f * fAP2out;
	fAP3[iAPc3] = 0.5f * fAP3out + fAP2out;

	fAP4out = fAP4[iAPc4] - 0.5f * fAP3out;
	fAP4[iAPc4] = 0.5f * fAP4out + fAP3out;

	fLPout += (fAP4out - fLPout) * 0.001f;
	fReverb_out = fReverbINT * (fAP4out - fLPout) + fReverbNINT * fChorusOut;


	// Increment delay counter
	iDc1 ++;
	iDc2 ++;
	iDc3 ++;
	iDc4 ++;
	iDc5 ++;
	iDc6 ++;
	iDc7 ++;
	iDc8 ++;

	iAPc1 ++;
	iAPc2 ++;
	iAPc3 ++;
	iAPc4 ++;

	if (iDc1 == DELAY1_LENGTH) iDc1 = 0;
	if (iDc2 == DELAY2_LENGTH) iDc2 = 0;
	if (iDc3 == DELAY3_LENGTH) iDc3 = 0;
	if (iDc4 == DELAY4_LENGTH) iDc4 = 0;
	if (iDc5 == DELAY5_LENGTH) iDc5 = 0;
	if (iDc6 == DELAY6_LENGTH) iDc6 = 0;
	if (iDc7 == DELAY7_LENGTH) iDc7 = 0;
	if (iDc8 == DELAY8_LENGTH) iDc8 = 0;

	if (iAPc1 == ALLPASS1_LENGTH) iAPc1 = 0;
	if (iAPc2 == ALLPASS2_LENGTH) iAPc2 = 0;
	if (iAPc3 == ALLPASS3_LENGTH) iAPc3 = 0;
	if (iAPc4 == ALLPASS4_LENGTH) iAPc4 = 0;

	// Get the early reflections
	if (bMute == 0 && !bBeepActive )
	{
		// Limit the output to 16bit
		int32_t slVal;
		slVal = fReverb_out;
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
