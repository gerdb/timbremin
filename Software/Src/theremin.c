/**
 *  Project     timbremin
 *  @file		theremin.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Theremin functionality
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
#include "audio_out.h"
#include "theremin.h"
#include "display.h"
#include "pots.h"
#include "config.h"
#include "usb_stick.h"
#include <math.h>
#include <stdlib.h>

uint16_t usCC[8];
int16_t ssWaveTable[4096];

// Linearization tables for pitch an volume
uint32_t ulVol1LinTable[1024];
uint32_t ulVol2LinTable[1024];
uint32_t ulPitchLinTable[2048];
float fLFOTable[1024];

float fNonLinTable[1024+1];

// Pitch
uint16_t usPitchCC;			// value of capture compare register
uint16_t usPitchLastCC; 	// last value (last task)
uint16_t usPitchPeriod;		// period of oscillator
int32_t slPitchOffset; 		// offset value (result of auto-tune)
int32_t slPitchPeriodeFilt;	// low pass filtered period
// Divider = 2exp(20)
// Wavetable: 4096
// DAC: 48kHz
// Audiofreq = 48000Hz * wavetablefrq / (2exp(20)) / 1024 tableentries * 2(channel left & right);
// wavetablefrq = Audiofreq / 48000Hz * 2exp(20) * 1024 /2
// wavetablefrq = Audiofreq * 11184.81066 ...
int32_t slPitch;			// pitch value
float fPitch;			// pitch value

float fPitchScale = 1.0f;
float fPitchShift = 1.0f;

// Volume
float fVol = 0.0;			// volume value
uint16_t usVolTim1CC;			// value of capture compare register
uint16_t usVolTim2CC;			// value of capture compare register
uint16_t usVolTim1LastCC;		// last value (last task)
uint16_t usVolTim2LastCC;		// last value (last task)
uint16_t usVolTim1Period;		// period of oscillator
uint16_t usVolTim2Period;		// period of oscillator
uint16_t usVolTim1Period_lastvalid = 0;		// period of oscillator
uint16_t usVolTim2Period_lastvalid = 0;		// period of oscillator
int32_t slVolTim1Offset;		// offset value (result of auto-tune)
int32_t slVolTim2Offset;		// offset value (result of auto-tune)
int32_t slVolTim1PeriodeFilt;	// low pass filtered period
int32_t slVolTim2PeriodeFilt;	// low pass filtered period
int32_t slVol1;				// volume value
int32_t slVol1FiltL;		// volume value, filtered (internal filter value)
int32_t slVol1Filt;			// volume value, filtered
int32_t slVol2;				// volume value
int32_t slVol2FiltL;		// volume value, filtered (internal filter value)
int32_t slVol2Filt;			// volume value, filtered

float fVolScale = 1.0f;
float fVolShift = 0.0f;


int32_t slPitchOld;
int32_t slPitchDiffSum;
int32_t slPitchDiffCnt = 0;
int32_t slVolTim1Old;
int32_t slVolTim1DiffSum;
int32_t slVolTim1DiffCnt = 0;
int32_t slVolTim2Old;
int32_t slVolTim2DiffSum;
int32_t slVolTim2DiffCnt = 0;


//int32_t slVol2;				// volume value


uint32_t ulWaveTableIndex = 0;
uint32_t ulLFOTableIndex = 0;


float fWavStepFilt = 0.0f;

// Auto-tune
int siAutotune = 0;			// Auto-tune down counter
uint32_t ulLedCircleSpeed;	// LED indicator speed
uint32_t ulLedCirclePos;	// LED indicator position
int32_t slMinPitchPeriode;	// minimum pitch value during auto-tune
int32_t slMinVol1Periode = 0;	// minimum volume value during auto-tune
int32_t slMinVol2Periode = 0;	// minimum volume value during auto-tune

uint16_t usDACValue;		// wave table output for audio DAC
int iWavMask = 0x0FFF;
int iWavLength = 4096;
int bUseNonLinTab = 0;
e_waveform eWaveform = SINE;

int vol_state_cnt = 0;
e_vol_sel vol_sel = VOLSEL_NONE;
int vol_active = 0;
uint16_t test[96];

int task = 0;

extern TIM_HandleTypeDef htim1;	// Handle of timer for input capture

#ifdef DEBUG
uint32_t ulStopwatch = 0;
#define STOPWATCH_START() DWT->CYCCNT = 0;
#define STOPWATCH_STOP() ulStopwatch = DWT->CYCCNT;
#else
#define STOPWATCH_START() ;
#define STOPWATCH_STOP() ;
#endif

/**
 * @brief Initialize the module
 *
 */
void THEREMIN_Init(void)
{
#ifdef DEBUG
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif

	// Read auto-tune values from virtual EEPRom
	slPitchOffset = CONFIG_Read_SLong(EEPROM_ADDR_PITCH_AUTOTUNE_H);
	slVolTim1Offset = CONFIG_Read_SLong(EEPROM_ADDR_VOLTIM1_AUTOTUNE_H);
	slVolTim2Offset = CONFIG_Read_SLong(EEPROM_ADDR_VOLTIM2_AUTOTUNE_H);

	// Get the VolumeShift value from the flash configuration
	fVolShift = ((float) (CONFIG.VolumeShift)) * 0.1f + 11.5f;

	// 8 Waveforms
	strPots[POT_WAVEFORM].iMaxValue = 8;

	// Calculate the LUT for volume and pitch
	THEREMIN_Calc_Volume1Table();
	THEREMIN_Calc_Volume2Table();
	THEREMIN_Calc_PitchTable();
	THEREMIN_Calc_WavTable();
	THEREMIN_Calc_LFOTable();

	HAL_GPIO_WritePin(SEL_VOL_OSC_A_GPIO_Port, SEL_VOL_OSC_A_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SEL_VOL_OSC_B_GPIO_Port, SEL_VOL_OSC_B_Pin, GPIO_PIN_RESET);

}

/**
 * @brief Create a sin table for general purpose like LFO
 *
 */
void THEREMIN_Calc_LFOTable(void)
{
	for (int i = 0; i < 1024; i++)
	{
		fLFOTable[i] = 0.95f + 0.05f * sin((i * 2 * M_PI) / 1024.0f);
	}
}

/**
 * @brief Recalculates the pitch LUT
 *
 */
void THEREMIN_Calc_PitchTable(void)
{
	floatint_ut u;
	uint32_t val;
	float f;

	for (int32_t i = 0; i < 2048; i++)
	{
		// Calculate back the x values of the table
		u.ui = (i << 17) + 0x3F800000;
		// And now calculate the precise log2 value instead of only the approximation
		// used for x values in THEREMIN_96kHzDACTask(void) when using the table.
		//	u.f = fPitch * 0.0000001f * fPitchShift;
		//	u.i = (int) (fPitchScale * (u.i - 1064866805) + 1064866805);
		//	slWavStep = (int32_t) (u.f*10000000.0f);

		f = expf(logf(u.f * 0.0000001f * fPitchShift) * fPitchScale)
				* 10000000.0f;

		// Convert it to integer
		val = (uint32_t) (f);
		// Limit it to maximum
		if (val > 500000000)
		{
			val = 500000000;
		}
		// Fill the pitch table
		ulPitchLinTable[i] = val;
	}
}



/**
 * @brief Recalculates the volume LUT
 *
 */
void THEREMIN_Calc_Volume1Table(void)
{
	floatint_ut u;
	uint32_t val;
	float f;

	for (int32_t i = 0; i < 1024; i++)
	{
		// Calculate back the x values of the table
		u.ui = (i << 17) + 0x3F800000;
		// And now calculate the precise log2 value instead of only the approximation
		// used for x values in THEREMIN_96kHzDACTask(void) when using the table.
		f = (16.0f /*fVolShift*/ - log2f(u.f)) * 64.0f ;/* * fVolScale*/;

		// Limit the float value before we square it;
		if (f > 1024.0f)
			f = 1024.0f;
		if (f < 0.0f)
			f = 0.0f;

		// Square the volume value
		val = (uint32_t)f;//((f * f) * 0.000976562f); /* =1/1024 */
		;
		// Limit it to maximum
		if (val > 1023)
		{
			val = 1023;
		}

		// Fill the volume table
		ulVol1LinTable[i] = val;
	}
}
/**
 * @brief Recalculates the volume LUT
 *
 */
void THEREMIN_Calc_Volume2Table(void)
{
	floatint_ut u;
	uint32_t val;
	float f;

	for (int32_t i = 0; i < 1024; i++)
	{
		// Calculate back the x values of the table
		u.ui = (i << 17) + 0x3F800000;
		// And now calculate the precise log2 value instead of only the approximation
		// used for x values in THEREMIN_96kHzDACTask(void) when using the table.
		f = (16.0f /*fVolShift*/ - log2f(u.f)) * 1.1f * 64.0f ;/* * fVolScale*/;

		// Limit the float value before we square it;
		if (f > 1024.0f)
			f = 1024.0f;
		if (f < 0.0f)
			f = 0.0f;

		// Square the volume value
		val = (uint32_t)f;//((f * f) * 0.000976562f); /* =1/1024 */
		;
		// Limit it to maximum
		if (val > 1023)
		{
			val = 1023;
		}

		// Fill the volume table
		ulVol2LinTable[i] = val;
	}
}

/**
 * @brief Sets the length ov the wave LUT
 * @length of the LUT, only 2expN values <= 4096 are valid
 *
 */
void THEREMIN_SetWavelength(int length)
{
	iWavLength = length;
	iWavMask = length - 1;
}

/**
 * @brief Recalculates the wave LUT
 *
 */
void THEREMIN_Calc_WavTable(void)
{
	int bLpFilt = 0;
	float a0=0.0f;
	float a1=0.0f;
	float a2=0.0f;
	float b1=0.0f;
	float b2=0.0f;

	// Mute as long as new waveform is beeing calculated
	bMute = 1;
	THEREMIN_SetWavelength(4096);
	bUseNonLinTab = 0;

	if (eWaveform != USBSTICK)
	{
		bWavLoaded = 0;
	}

	switch (eWaveform)
	{
	case SINE:
		for (int i = 0; i < 1024; i++)
		{
			ssWaveTable[i] = 32767 * sin((i * 2 * M_PI) / 1024.0f);
		}
		THEREMIN_SetWavelength(1024);
		break;

	case CAT:
		for (int i = 0; i < 4096; i++)
		{
			if (i < 1024)
			{
				ssWaveTable[i] = 0;
			}
			else
			{
				ssWaveTable[i] = 32767 * sin((i * 2 * M_PI) / 1024.0f);
			}
		}
		// http://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/
		// 48kHz, Fc=60, Q=0.7071, A=0dB

		a0 = 0.00001533600755608856f;
		a1 = 0.00003067201511217712f;
		a2 = 0.00001533600755608856f;
		b1 = -1.9888928005576803f;
		b2 = 0.9889541445879048f;
		bLpFilt = 1;
		break;

	case COSPULSE:
		for (int i = 0; i < 4096; i++)
		{
			if (i < 2048)
			{
				ssWaveTable[i] = - 32767 * cos((i * 2 * M_PI) / 2048.0f);
			}
			else
			{
				ssWaveTable[i] = -32767;
			}
		}
		break;


	case HARMON:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = 16384 * (0.8f*sin((i * 2.0f * M_PI) / 1024.0f) + 1.0f * sin((i * 6.0f * M_PI) / 1024.0f)) ;
		}
		break;

	case COMPRESSED:
		for (int i = 0; i < 1024; i++)
		{
			ssWaveTable[i] = 32767 * sin((i * 2 * M_PI) / 1024.0f);
		}

		fNonLinTable[512] = 0.0f;

		for (int i = 0; i < 512; i++)
		{
			float lin = ((float)i)/511.0;

			fNonLinTable[513 + i] = ((1.0-lin)*((float)i / 512.0f) + lin *powf(((float)i / 512.0f),0.8f)) * 32767.0f;
			fNonLinTable[511 - i] = -fNonLinTable[513 + i];
		}
		bUseNonLinTab = 1;
		THEREMIN_SetWavelength(1024);

		// http://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/
		// 48kHz, Fc=60, Q=0.7071, A=0dB

		a0 = 0.00001533600755608856f;
		a1 = 0.00003067201511217712f;
		a2 = 0.00001533600755608856f;
		b1 = -1.9888928005576803f;
		b2 = 0.9889541445879048f;
		bLpFilt = 1;

		break;

	case GLOTTAL:

		// Based on:
		// http://www.fon.hum.uva.nl/praat/manual/PointProcess__To_Sound__phonation____.html
		for (int i = 0; i < 768; i++)
		{
			ssWaveTable[i] = (int32_t)(621368.0 * (powf((float)i / 768.0f, 3) - powf((float)i / 768.0f, 4))) - 32768 ;
		}
		for (int i = 768; i < 1024; i++)
		{
			ssWaveTable[i] = -32768 ;
		}
		THEREMIN_SetWavelength(1024);

		// http://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/
		// 48kHz, Fc=400, Q=0.7071, A=0dB

		a0 = 0.0006607788720867079f;
		a1 = 0.0013215577441734157f;
		a2 = 0.0006607788720867079f;
		b1 = -1.9259833105871227f;
		b2 = 0.9286264260754695f;
		bLpFilt = 1;

		break;

	case THEREMIN:
		for (int i = 0; i < 1024; i++)
		{
			ssWaveTable[i] = 32767 * sin((i * 2 * M_PI) / 1024.0);
		}
		for (int i = 0; i < 1024; i++)
		{
			fNonLinTable[i] = 32767.0f-(65536.0f*((expf((((float)i/1024.0f)*4.5f)))-1)/(expf(4.5f)-1.0f));
		}
		fNonLinTable[1024] = -32768;
		bUseNonLinTab = 1;
		THEREMIN_SetWavelength(1024);

		break;


	/*
	case SAWTOOTH:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = (i & 0x03FF)*64-32768;
		}
		a0 = 0.00001533600755608856f;
		a1 = 0.00003067201511217712f;
		a2 = 0.00001533600755608856f;
		b1 = -1.9888928005576803f;
		b2 = 0.9889541445879048f;

		bLpFilt = 1;

		break;
	 */


	case USBSTICK:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = 0;
		}
		USB_STICK_ReadFiles();
		break;

	default:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = 0;
		}
	}

	// Additional low pass filter;
	if (bLpFilt)
	{
		float result = 0.0f;
		float sample = 0.0f;
		float x1=0.0f;
		float x2=0.0f;
		float y1=0.0f;
		float y2=0.0f;

		for (int run = 0; run < 2; run ++)
		{
			for (int i = 0; i < iWavLength; i++)
			{
				sample = ((float)ssWaveTable[i]) * 0.8f;

			    // the biquad filter
			    result = a0 * sample + a1 * x1 + a2 * x2 -b1 * y1 - b2 * y2;

			    // shift x1 to x2, sample to x1
			    x2 = x1;
			    x1 = sample;

			    //shift y1 to y2, result to y1
			    y2 = y1;
			    y1 = result;

			    if (run == 1)
			    {
			    	if (result > 32767.0)
			    	{
			    		ssWaveTable[i] = 32767;
			    	}
			    	else if (result < -32768.0)
			    	{
			    		ssWaveTable[i] = -32768;
			    	}
			    	else
			    	{
						ssWaveTable[i] = (int16_t)result;
			    	}
			    }
			}
		}
	}

	// Mute as long as new waveform is beeing calculated
	bMute = 0;

}

/**
 * @brief 96kHz DAC task called in interrupt
 *
 * At 168MHz the absolute maximum cycle count would be 168MHz/96kHz = 1750
 */
inline void THEREMIN_96kHzDACTask(void)
{
	int32_t p1, p2, tabix, tabsub;
	float p1f, p2f;
	floatint_ut u;
	int task48 = 0;
	float result = 0.0f;
	int iWavOut;
	//float fLFO;


	task48 = 1-task48;

	if (task48)
	{
		ulLFOTableIndex +=1;

		//fLFO = fLFOTable[ulLFOTableIndex & 0x03FF];

		if (fPitch >= 1.0f)
		{
			// cycles: 59..62
			// fast pow approximation by LUT with interpolation
			// float bias: 127, so 127 << 23bit mantissa is: 0x3F800000
			// We use (5 bit of) the exponent and 6 bit of the mantissa
			u.f = fPitch;
			tabix = ((u.ui - 0x3F800000) >> 17);
			tabsub = (u.ui & 0x0001FFFF) >> 2;
			p1f = (float)ulPitchLinTable[tabix];
			p2f = (float)ulPitchLinTable[tabix + 1];
			fWavStepFilt = (p1f + (((p2f - p1f) * tabsub) * 0.000030518f /*1/32768*/));
			//fWavStepFilt += ((p1f + (((p2f - p1f) * tabsub) * 0.000007629394531f))- fWavStepFilt) * 0.0001f;
			//fWavStepFilt = 81460152.0f;
			ulWaveTableIndex += (uint32_t)(fWavStepFilt  /*  * fLFO*/ );
		}

		// Wait 166us until starting using the volume values
		vol_state_cnt++;
		if (vol_state_cnt == 16)
		{
			vol_active = 1;
		}
		// Do it for 1ms
		else if (vol_state_cnt == 96)
		{
			vol_state_cnt = 0;
			vol_active = 0;
			// Toggle between both antennas
			if (vol_sel == VOLSEL_1)
			{
				vol_sel = VOLSEL_2;
				HAL_GPIO_WritePin(SEL_VOL_OSC_A_GPIO_Port, SEL_VOL_OSC_A_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(SEL_VOL_OSC_B_GPIO_Port, SEL_VOL_OSC_B_Pin, GPIO_PIN_SET);
			}
			else
			{
				vol_sel = VOLSEL_1;
				HAL_GPIO_WritePin(SEL_VOL_OSC_A_GPIO_Port, SEL_VOL_OSC_A_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(SEL_VOL_OSC_B_GPIO_Port, SEL_VOL_OSC_B_Pin, GPIO_PIN_RESET);
			}
		}

		//test = ulWavStep;



		//slVolFilt *= fLFO;

		// cycles: 29..38
		// WAV output to audio DAC
		tabix = ulWaveTableIndex >> 20; // use only the 12MSB of the 32bit counter
		tabsub = (ulWaveTableIndex >> 12) & 0x000000FF;
		p1 = ssWaveTable[tabix & iWavMask];
		p2 = ssWaveTable[(tabix + 1) & iWavMask];
		iWavOut = ((p1 + (((p2 - p1) * tabsub) / 256)) * slVol1Filt / 1024);

		if (bUseNonLinTab)
		{
			tabix = (iWavOut+32768) / 64;
			tabsub = iWavOut & 0x003F;
			p1f = fNonLinTable[tabix];
			p2f = fNonLinTable[tabix + 1];
			result = (p1f + (((p2f - p1f) * tabsub) * 0.015625f));
		}
		else
		{
			result = (float)iWavOut;
		}

		// Limit the output to 16bit
    	if (result > 32767.0)
    	{
    		usDACValue = 32767;
    	}
    	else if (result < -32768.0)
    	{
    		usDACValue = -32768;
    	}
    	else
    	{
    		usDACValue = (int16_t)result;
    	}
	}


	// cycles: 29
	// Get the input capture timestamps
	usPitchCC = htim1.Instance->CCR1;


	// cycles: 26
	// Calculate the period by the capture compare value and
	// the last capture compare value
	usPitchPeriod = usPitchCC - usPitchLastCC;
	if (vol_sel == VOLSEL_1)
	{

		// cycles: 39 .. 43
		// scale of the volume raw value by LUT with interpolation
		// float bias: 127, so 127 << 23bit mantissa is: 0x3F800000
		// We use for the 10bit table 4 bit of the exponent and 6 bit of the mantissa
		// So we have to shift the float value 17 bits to have a mantissa with 6 bit
		// (23bit - 17bit = 6 bit)
		// See also https://www.h-schmidt.net/FloatConverter/IEEE754.html
		if (slVol1 < 1)
		{
			slVol1 = 1;
		}
		if (slVol1 > 32767)
		{
			slVol1 = 32767;
		}
		// Fill the union with the float value
		u.f = (float) (slVol1);
		// Shift it 17 bits to right to get a 10bit value (4bit exponent, 6bit mantissa)
		tabix = ((u.ui - 0x3F800000) >> 17);
		// The rest of the value ( the 17 bits we have shifted out)
		tabsub = u.ui & 0x0001FFFF;
		p1 = ulVol1LinTable[tabix];
		p2 = ulVol1LinTable[tabix + 1];
		slVol1 = p1 + (((p2 - p1) * tabsub) >> 17);

		// cycles
		// Low pass filter the output to avoid aliasing noise.
		slVol1FiltL += slVol1 - slVol1Filt;
		slVol1Filt = slVol1FiltL / 1024;



		usVolTim1CC = htim1.Instance->CCR2;
		if (vol_active)
		{
			usVolTim1Period = usVolTim1CC - usVolTim1LastCC;
		}
		else
		{
			usVolTim1Period = 0;
		}

		test[vol_state_cnt] = usVolTim1Period;

		usVolTim1LastCC = usVolTim1CC;

		usVolTim2Period = 0;

		if (usVolTim1Period != 0)
		{
			//                                   11bit                10bit  10bit
			slVolTim1PeriodeFilt += ((int16_t) (usVolTim1Period - 2048) * 1024 * 1024
					- slVolTim1PeriodeFilt) / 1024;
			usVolTim1Period_lastvalid = usVolTim1Period;
		}
	}
	else
	{
		// cycles: 39 .. 43
		// scale of the volume raw value by LUT with interpolation
		// float bias: 127, so 127 << 23bit mantissa is: 0x3F800000
		// We use for the 10bit table 4 bit of the exponent and 6 bit of the mantissa
		// So we have to shift the float value 17 bits to have a mantissa with 6 bit
		// (23bit - 17bit = 6 bit)
		// See also https://www.h-schmidt.net/FloatConverter/IEEE754.html
		if (slVol2 < 1)
		{
			slVol2 = 1;
		}
		if (slVol2 > 32767)
		{
			slVol2 = 32767;
		}
		// Fill the union with the float value
		u.f = (float) (slVol2);
		// Shift it 17 bits to right to get a 10bit value (4bit exponent, 6bit mantissa)
		tabix = ((u.ui - 0x3F800000) >> 17);
		// The rest of the value ( the 17 bits we have shifted out)
		tabsub = u.ui & 0x0001FFFF;
		p1 = ulVol2LinTable[tabix];
		p2 = ulVol2LinTable[tabix + 1];
		slVol2 = p1 + (((p2 - p1) * tabsub) >> 17);

		// cycles
		// Low pass filter the output to avoid aliasing noise.
		slVol2FiltL += slVol2 - slVol2Filt;
		slVol2Filt = slVol2FiltL / 1024;

		usVolTim2CC = htim1.Instance->CCR3;
		if (vol_active)
		{
			usVolTim2Period = usVolTim2CC - usVolTim2LastCC;
		}
		else
		{
			usVolTim2Period = 0;
		}
		usVolTim2LastCC = usVolTim2CC;



		if (usVolTim2Period != 0 )
		{
			//                                   11bit                10bit  10bit
			slVolTim2PeriodeFilt += ((int16_t) (usVolTim2Period - 2048) * 1024 * 1024
					- slVolTim2PeriodeFilt) / 1024;
			usVolTim2Period_lastvalid = usVolTim2Period;

		}

		usVolTim1Period = 0;
	}



	// cycles: 9
	/*
	if (usPitchPeriod < 2000)
		usPitchPeriod *= 2;

	if (usVolTim1Period < 2000)
		usVolTim1Period *= 2;
	if (usVolTim2Period < 2000)
		usVolTim2Period *= 2;*/


//	for (int i=0;i<7;i++)
//	{
//		usCC[i] = usCC[i+1];
//	}
//	usCC[7] = usPitchPeriod;

	// cycles: 29
	// Low pass filter period values
	// factor *1024 is necessary, because of the /1024 integer division
	// factor *1024 is necessary, because we want to oversample the input signal
	if (usPitchPeriod != 0)
	{
		//                                   11bit                10bit  10bit
		slPitchPeriodeFilt += ((int16_t) (usPitchPeriod - 2048) * 1024 * 1024
				- slPitchPeriodeFilt) / 1024;
	}


	// cycles: 34
	fPitch = (float) ((slPitchPeriodeFilt - slPitchOffset) * 8);
	slVol1 = ((slVolTim1PeriodeFilt - slVolTim1Offset) / 4096);
	slVol2 = ((slVolTim2PeriodeFilt - slVolTim2Offset) / 4096);

	// cycles: 9
	// Store values for next task
	usPitchLastCC = usPitchCC;


	if (usPitchPeriod != 0) {
		slPitchDiffSum += abs(usPitchPeriod - slPitchOld);
		slPitchDiffCnt ++;
		slPitchOld = usPitchPeriod;
	}
	if (usVolTim1Period != 0) {
		slVolTim1DiffSum += abs(usVolTim1Period - slVolTim1Old);
		slVolTim1DiffCnt ++;
		slVolTim1Old = usVolTim1Period;
	}
	if (usVolTim2Period != 0) {
		slVolTim2DiffSum += abs(usVolTim2Period - slVolTim2Old);
		slVolTim2DiffCnt ++;
		slVolTim2Old = usVolTim2Period;
	}

}

/**
 * @brief 1ms task
 *
 */
void THEREMIN_1msTask(void)
{
	int bReqCalcPitchTable = 0;
	if (siAutotune == 0)
	{
		// Start autotune by pressing BUTTON_KEY
		if (BSP_PB_GetState(BUTTON_KEY) == GPIO_PIN_SET)
		{
			DISPLAY_Dark();

			// 1.0sec auto-tune
			siAutotune = 1000;

			// Reset LED indicator and pitch and volume values
			ulLedCircleSpeed = siAutotune;
			ulLedCirclePos = 0;
			slPitchOffset = 0;
			slPitchPeriodeFilt = 0x7FFFFFFF;
			slMinPitchPeriode = 0x7FFFFFFF;
			slVolTim1Offset = 0;
			slVolTim2Offset = 0;
			slVolTim1PeriodeFilt = 0x7FFFFFFF;
			slVolTim2PeriodeFilt = 0x7FFFFFFF;
			slMinVol1Periode = 0x7FFFFFFF;
			slMinVol2Periode = 0x7FFFFFFF;

			// Mute the output
			bMute = 1;
		}
	}
	else
	{
		// Find lowest pitch period
		if (slPitchPeriodeFilt < slMinPitchPeriode)
		{
			slMinPitchPeriode = slPitchPeriodeFilt;
		}
		// Find lowest volume period
		if (slVolTim1PeriodeFilt < slMinVol1Periode)
		{
			slMinVol1Periode = slVolTim1PeriodeFilt;
		}
		if (slVolTim2PeriodeFilt < slMinVol2Periode)
		{
			slMinVol2Periode = slVolTim2PeriodeFilt;
		}
		siAutotune--;

		// LED indicator
		ulLedCircleSpeed = siAutotune;
		ulLedCirclePos += ulLedCircleSpeed;
		BSP_LED_Off(((ulLedCirclePos / 32768) + 4 - 1) & 0x03);
		BSP_LED_On(((ulLedCirclePos / 32768) + 4) & 0x03);

		// Auto-tune is finished
		if (siAutotune == 0)
		{
			// activate output
			bMute = 0;

			DISPLAY_Dark();
			// Use minimum values for offset of pitch and volume
			slPitchOffset = slMinPitchPeriode;
			slVolTim1Offset = slMinVol1Periode;	// + 16384 * 128;
			slVolTim2Offset = slMinVol2Periode;	// + 16384 * 128;
#ifdef DEBUG
		printf("%d %d\n", usPitchPeriod, usVolTim1Period);
#endif
			CONFIG_Write_SLong(EEPROM_ADDR_PITCH_AUTOTUNE_H, slPitchOffset);
			CONFIG_Write_SLong(EEPROM_ADDR_VOLTIM1_AUTOTUNE_H, slVolTim1Offset);
			CONFIG_Write_SLong(EEPROM_ADDR_VOLTIM2_AUTOTUNE_H, slVolTim2Offset);
		}
	}

	// pitch scale pot
	if (POTS_HasChanged(POT_PITCH_SCALE))
	{
		// from 2^0.5 ... 2^2.0
		// 2^((POT-2048)/1024)
		fPitchScale = powf(2,
				((float) (POTS_GetScaledValue(POT_PITCH_SCALE) - 2048))
						* 0.000976562f /* 1/1024 */);

		// Request the calculation of a new pitch table
		bReqCalcPitchTable = 1;
	}

	// pitch shift pot
	if (POTS_HasChanged(POT_PITCH_SHIFT))
	{
		// from 2^0.25 ... 2^4.0
		// 2^((POT-2048)/1024)
		fPitchShift = powf(2,
				((float) (POTS_GetScaledValue(POT_PITCH_SHIFT) - 2048))
						* 0.001953125f /* 1/512 */);

		// Request the calculation of a new pitch table
		bReqCalcPitchTable = 1;
	}

	// Is it necessary to recalculate the pitch table?
	if (bReqCalcPitchTable)
	{
		THEREMIN_Calc_PitchTable();
	}

	// volume scale pot
	if (POTS_HasChanged(POT_VOL_SCALE))
	{
		// from 2^0.25 ... 2^4.0
		// 2^((POT-2048)/1024)
		fVolScale = powf(2,
				((float) (POTS_GetScaledValue(POT_VOL_SCALE) - 2048))
						* 0.000976562f /* 1/1024 */);
		THEREMIN_Calc_Volume1Table();
		THEREMIN_Calc_Volume2Table();
	}

	// waveform pot
	if (POTS_HasChanged(POT_WAVEFORM))
	{
		eWaveform = POTS_GetScaledValue(POT_WAVEFORM);
		THEREMIN_Calc_WavTable();
	}

	if (siAutotune == 0)
	{
		// fWavStepFilt * 96kHz * (1 >> 20 / 1024(WaveTable length))
		DISPLAY_PitchDisplay(fWavStepFilt * 0.0000894069671631);
	}
	else
	{
		DISPLAY_AutotuneDisplay();
	}




}

/**
 * @brief 1s task used for debugging purpose
 *
 */
void THEREMIN_1sTask(void)
{
#ifdef DEBUG
	int p1=0,p2=0,p3=0;

	if (siAutotune == 0)
	{
//		printf("%d %d\n", (int)slVolTim1PeriodeFilt, (int)slVolTim2PeriodeFilt);

		if (slPitchDiffCnt != 0)
		{
			p1 = slPitchDiffSum *10 / slPitchDiffCnt;
		}
		if (slVolTim1DiffCnt != 0)
		{
			p2 = slVolTim1DiffSum *10 / slVolTim1DiffCnt;
		}
		if (slVolTim2DiffCnt != 0)
		{
			p3 = slVolTim2DiffSum *10 / slVolTim2DiffCnt;
		}
		slPitchDiffSum = 0;
		slPitchDiffCnt = 0;
		slVolTim1DiffSum = 0;
		slVolTim1DiffCnt = 0;
		slVolTim2DiffSum = 0;
		slVolTim2DiffCnt = 0;

//		printf("%d (%d) %d (%d) %d (%d)\n", (int)usPitchPeriod, (int)p1,
//				(int)usVolTim1Period, (int)p2,
//				(int)usVolTim2Period, (int)p3);
		printf("%d %d %d %d\n",
				(int)usVolTim1Period_lastvalid,// (int)p2,
				(int)usVolTim2Period_lastvalid,// (int)p2,
				(int)((slVol1Filt+slVol2Filt)/2),
				(int)slVol1Filt-slVol2Filt);// (int)p3);


		//printf("Stopwatch %d\n", ulStopwatch);
	}
#endif
}




