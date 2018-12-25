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
#include "beep.h"
#include "volume.h"
#include <math.h>
#include <stdlib.h>

uint16_t usCC[8];
int16_t ssWaveTable[4096];

// Linearization tables for pitch an volume
uint32_t ulVol1LinTable[1024];
uint32_t ulVol2LinTable[1024];
uint32_t ulPitchLinTable[2048];

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
float fPitchFrq;		// pitch frequency scaled to 96kHz task

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
int32_t slVolTim1Offset;		// offset value (result of auto-tune)
int32_t slVolTim2Offset;		// offset value (result of auto-tune)
int slVolTim1PeriodeFilt_cnt;	// low pass filtered period
int slVolTim2PeriodeFilt_cnt;	// low pass filtered period
int32_t slVolTimPeriodeFiltDiff;	// low pass filtered period
int32_t slVolTim1PeriodeFilt;	// low pass filtered period
int32_t slVolTim2PeriodeFilt;	// low pass filtered period
int32_t slVolTim1MeanPeriode;	// low pass filtered period
int32_t slVolTim2MeanPeriode;	// low pass filtered period
int32_t slVol1;					// volume value
int32_t slVol2;					// volume value
int32_t slVolFiltL;				// volume value, filtered (internal filter value)
int32_t slVolFilt;				// volume value, filtered


int32_t slVolumeRaw;
int32_t slVolume;
int32_t slTimbre;

float fOscSin = 0.0f;
float fOscCos = 1.0f;
float fOscCorr = 1.0f;

float fVolScale = 1.0f;
float fVolShift = 0.0f;


int32_t slPitchOld;
int32_t slVolTim1Old;
int32_t slVolTim2Old;


//int32_t slVol2;				// volume value

uint32_t ulWaveTableIndex = 0;


float fWavStepFilt = 0.0f;

// Auto-tune
int siAutotune = 0;			// Auto-tune down counter
uint32_t ulLedCircleSpeed;	// LED indicator speed
uint32_t ulLedCirclePos;	// LED indicator position
int32_t slMinPitchPeriode;	// minimum pitch value during auto-tune
int32_t slMinVol1Periode = 0;	// minimum volume value during auto-tune
int32_t slMinVol2Periode = 0;	// minimum volume value during auto-tune
int iTuned = 0;

int iWavMask = 0x0FFF;
int iWavLength = 4096;
int bUseNonLinTab = 0;
e_waveform eWaveform = SINE;

e_vol_sel vol_sel = VOLSEL_NONE;

int32_t test1;
int32_t test2;
int task = 0;



int taskTableCnt = 0;
void (*taskTable[96]) () = {0};

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
	int i;

	// Fill the task table
	taskTable[0] = THEREMIN_Task_Select_VOL1;
	taskTable[2] = THEREMIN_Task_Volume_Timbre;
	taskTable[4] = THEREMIN_Task_Volume_Nonlin;
	taskTable[16] = THEREMIN_Task_Activate_VOL1;
	for (i = 18; i< 46; i+=2)
	{
		taskTable[i] = THEREMIN_Task_Capture_VOL1;
	}
	taskTable[46] = THEREMIN_Task_Calculate_VOL1;

	taskTable[48] = THEREMIN_Task_Select_VOL2;
	taskTable[64] = THEREMIN_Task_Activate_VOL2;
	for (i = 66; i< 94; i+=2)
	{
		taskTable[i] = THEREMIN_Task_Capture_VOL2;
	}
	taskTable[94] = THEREMIN_Task_Calculate_VOL2;

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
	THEREMIN_Calc_VolumeTable(VOLSEL_1);
	THEREMIN_Calc_VolumeTable(VOLSEL_2);
	THEREMIN_Calc_PitchTable();
	THEREMIN_Calc_WavTable();

	HAL_GPIO_WritePin(SEL_VOL_OSC_A_GPIO_Port, SEL_VOL_OSC_A_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SEL_VOL_OSC_B_GPIO_Port, SEL_VOL_OSC_B_Pin, GPIO_PIN_RESET);


	// Beep for "switched on"
	BEEP_Play(0.0f,50,500);
	BEEP_Play(NOTE_C6,100,50);
	BEEP_Play(NOTE_G6,100,50);
	BEEP_Play(NOTE_A6,100,50);
	BEEP_Play(NOTE_F6,300,500);

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
void THEREMIN_Calc_VolumeTable(e_vol_sel vol_sel)
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

		//Scale it to 20cm and then to 1024 units
		f = pow(f,2.7450703915f) * 0.0000007370395f * 1024.0f/20.0f;
		if (vol_sel == VOLSEL_2)
		{
			f*= 1.1;
		}
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
		if (vol_sel == VOLSEL_1)
		{
			ulVol1LinTable[i] = val;
		}
		else if (vol_sel == VOLSEL_2)
		{
			ulVol2LinTable[i] = val;
		}
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

	// Mute as long as new waveform is being calculated
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

	// Mute as long as new waveform is being calculated
	bMute = 0;

}


/**
 * @brief 96kHz DAC task called in interrupt
 *
 * At 168MHz the absolute maximum cycle count would be 168MHz/96kHz = 1750
 */
inline void THEREMIN_96kHzDACTask_A(void)
{
	int32_t p1, p2, tabix, tabsub;
	float p1f, p2f;
	floatint_ut u;
	int iWavOut;


	float result = 0.0f;


	if (bBeepActive)
	{
		BEEP_Task();
		return;
	}


	// Get the input capture value and calculate the period time
	usPitchCC = htim1.Instance->CCR1;
	usPitchPeriod = usPitchCC - usPitchLastCC;
	usPitchLastCC = usPitchCC;



	// cycles:21
	fPitchFrq = ((float) (slPitchPeriodeFilt - slPitchOffset))*0.00000001f;


	// cycles: 12
	//fPitchFrq = fPitch * 0.00000001f;


	// cycles: 10
	if (fPitchFrq > 0.4f)
	{
		fPitchFrq = 0.4f;
	}
	if (fPitchFrq < -0.0f)
	{
		fPitchFrq = -0.0f;
	}

	STOPWATCH_START();
	fOscSin += fPitchFrq * fOscCos;
	fOscCos -= fPitchFrq * fOscSin;
	fOscCorr = 1.0f +(1 -(fOscSin * fOscSin + fOscCos * fOscCos)*0.01f);
	fOscCos *= fOscCorr;
	fOscSin *= fOscCorr;
	result = fOscSin * (float)slVolFilt;
	//result = fabs(fOscSin) * (float)slVolFilt;
	STOPWATCH_STOP();


	/*
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
		fWavStepFilt = (p1f + (((p2f - p1f) * tabsub) * 0.000030518f )); // *1/32768
		//fWavStepFilt += ((p1f + (((p2f - p1f) * tabsub) * 0.000007629394531f))- fWavStepFilt) * 0.0001f;
		//fWavStepFilt = 81460152.0f;
		ulWaveTableIndex += (uint32_t)(fWavStepFilt  );
	}
	*/

	/*
	// cycles: 29..38
	// WAV output to audio DAC
	tabix = ulWaveTableIndex >> 20; // use only the 12MSB of the 32bit counter
	tabsub = (ulWaveTableIndex >> 12) & 0x000000FF;
	p1 = ssWaveTable[tabix & iWavMask];
	p2 = ssWaveTable[(tabix + 1) & iWavMask];
	iWavOut = ((p1 + (((p2 - p1) * tabsub) / 256)) * slVolFilt / 1024);
	*/
	/*
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
	*/



	//result = (float)iWavOut;

	// cycles
	// Low pass filter the output to avoid aliasing noise.
	slVolFiltL += slVolume - slVolFilt;
	slVolFilt = slVolFiltL / 1024;


	// Limit the output to 16bit
	if (bMute)
	{
		usDACValueR = 0;
	}
	else if (result > 32767.0)
	{
		usDACValueR = 32767;
	}
	else if (result < -32768.0)
	{
		usDACValueR = -32768;
	}
	else
	{
		usDACValueR = (int16_t)result;
	}

	// Output also to the speaker
	usDACValueL = usDACValueR;
}

/**
 * @brief 96kHz DAC task called in interrupt
 *
 * At 168MHz the absolute maximum cycle count would be 168MHz/96kHz = 1750
 */
inline void THEREMIN_96kHzDACTask_B(void)
{


}

/**
 * @brief 96kHz DAC task called in interrupt
 *
 * At 168MHz the absolute maximum cycle count would be 168MHz/96kHz = 1750
 */
inline void THEREMIN_96kHzDACTask_Common(void)
{
	// cycles: 29
	// Low pass filter period values
	// factor *1024 is necessary, because of the /1024 integer division
	// factor *1024 is necessary, because we want to over sample the input signal
	if (usPitchPeriod != 0)
	{
		//                                   11bit                10bit  10bit
		slPitchPeriodeFilt += ((int16_t) (usPitchPeriod - 2048) * 1024 * 1024
				- slPitchPeriodeFilt) / 1024;
	}






	// Call next Task of task table
	taskTableCnt++;
	if (taskTableCnt>=96)
	{
		taskTableCnt = 0;
	}
	if (taskTable[taskTableCnt] != 0)
	{
		taskTable[taskTableCnt]();
	}
}


/**
 * Different tasks called from the task table every 96kHz
 * The work is divided into 96 parts, so the computation time stays
 * below 1/96kHz
 */

/**
 * Switch on volume oscillator 1
 */
void THEREMIN_Task_Select_VOL1(void)
{
	// Select oscillator of VOL1
	vol_sel = VOLSEL_1;
	HAL_GPIO_WritePin(SEL_VOL_OSC_A_GPIO_Port, SEL_VOL_OSC_A_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SEL_VOL_OSC_B_GPIO_Port, SEL_VOL_OSC_B_Pin, GPIO_PIN_RESET);
}

/**
 * Switch on volume oscillator 2
 */
void THEREMIN_Task_Select_VOL2(void)
{
	// Select oscillator of VOL2
	vol_sel = VOLSEL_2;
	HAL_GPIO_WritePin(SEL_VOL_OSC_A_GPIO_Port, SEL_VOL_OSC_A_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SEL_VOL_OSC_B_GPIO_Port, SEL_VOL_OSC_B_Pin, GPIO_PIN_SET);
}

/**
 * Oscillator is stable. Use now the frequency signal
 */
void THEREMIN_Task_Activate_VOL1(void)
{
	usVolTim1LastCC = htim1.Instance->CCR2; // Read capture compare to be prepared
	slVolTim1PeriodeFilt_cnt = 0;	// Reset mean filter counter
}

/**
 * Oscillator is stable. Use now the frequency signal
 */
void THEREMIN_Task_Activate_VOL2(void)
{
	usVolTim2LastCC = htim1.Instance->CCR3; // Read capture compare to be prepared
	slVolTim2PeriodeFilt_cnt = 0;	// Reset mean filter counter
}

/**
 * Capture and filter the VOL1 value
 */
void THEREMIN_Task_Capture_VOL1(void)
{
	// Get the input capture value and calculate the period time
	usVolTim1CC = htim1.Instance->CCR2;
	usVolTim1Period = usVolTim1CC - usVolTim1LastCC;
	usVolTim1LastCC = usVolTim1CC;


	// Low pass filter it with a mean filter
	if (usVolTim1Period != 0)
	{
		// Accumulate the period values
		slVolTimPeriodeFiltDiff = 256 * usVolTim1Period - slVolTim1Offset;

		// Count the amount of periods
		slVolTim1PeriodeFilt_cnt ++;
		// Was it a double period?
		if (usVolTim1Period > 4000)
		{
			slVolTim1PeriodeFilt_cnt ++;
			slVolTimPeriodeFiltDiff -= slVolTim1Offset;
		}

		// Use only positive values
		if (slVolTimPeriodeFiltDiff > 0)
		{
			slVolTim1PeriodeFilt += slVolTimPeriodeFiltDiff;
		}
	}
}


/**
 * Capture and filter the VOL2 value
 */
void THEREMIN_Task_Capture_VOL2(void)
{
	// Get the input capture value and calculate the period time
	usVolTim2CC = htim1.Instance->CCR3;
	usVolTim2Period = usVolTim2CC - usVolTim2LastCC;
	usVolTim2LastCC = usVolTim2CC;


	// Low pass filter it with a mean filter
	if (usVolTim2Period != 0)
	{
		// Accumulate the period values
		slVolTimPeriodeFiltDiff = 256 * usVolTim2Period - slVolTim2Offset;

		// Count the amount of periods
		slVolTim2PeriodeFilt_cnt ++;
		// Was it a double period?
		if (usVolTim2Period > 4000)
		{
			slVolTim2PeriodeFilt_cnt ++;
			slVolTimPeriodeFiltDiff -= slVolTim2Offset;
		}

		// Use only positive values
		if (slVolTimPeriodeFiltDiff > 0)
		{
			slVolTim2PeriodeFilt += slVolTimPeriodeFiltDiff;
		}
	}
}

/**
 * Calculate the mean value of VOL1
 */
void THEREMIN_Task_Calculate_VOL1(void)
{
	if (slVolTim1PeriodeFilt != 0)
	{
		slVolTim1MeanPeriode = slVolTim1PeriodeFilt / slVolTim1PeriodeFilt_cnt;
		slVol1 = ((CONFIG_VOL1_NUMERATOR * slVolTim1PeriodeFilt_cnt) /
				(slVolTim1PeriodeFilt + slVolTim1PeriodeFilt_cnt * CONFIG_VOL1_OFFSET_A))
				- CONFIG_VOL1_OFFSET_B;

		// Prepare for next filter interval
		slVolTim1PeriodeFilt_cnt = 0;
		slVolTim1PeriodeFilt = 0;
	}

	// Limit it
	if (slVol1 < 0)
	{
		slVol1 = 0;
	}
	if (slVol1 > 1024)
	{
		slVol1 = 1024;
	}
}

/**
 * Calculate the mean value of VOL2
 */
void THEREMIN_Task_Calculate_VOL2(void)
{
	if (slVolTim2PeriodeFilt != 0)
	{
		slVolTim2MeanPeriode = slVolTim2PeriodeFilt / slVolTim2PeriodeFilt_cnt;
		slVol2 = ((CONFIG_VOL2_NUMERATOR * slVolTim2PeriodeFilt_cnt) /
				(slVolTim2PeriodeFilt + slVolTim2PeriodeFilt_cnt * CONFIG_VOL2_OFFSET_A))
				- CONFIG_VOL2_OFFSET_B;

		// Prepare for next filter interval
		slVolTim2PeriodeFilt_cnt = 0;
		slVolTim2PeriodeFilt = 0;
	}

	// Limit it
	if (slVol2 < 0)
	{
		slVol2 = 0;
	}
	if (slVol2 > 1024)
	{
		slVol2 = 1024;
	}
}

/**
 * Calculate the mean volume and timbre value from VOL1 and VOL2
 */
void THEREMIN_Task_Volume_Timbre(void)
{
	// Volume is the mean value of VOL1 and VOL2
	slVolumeRaw = (slVol1 + slVol2) / 2;
	// Timbre is the difference between VOL1 and VOL2 in the range of 0..128
	slTimbre = (slVol1 - slVol2) + 128;

	// Limit the volume value
	if (slVolumeRaw < 0)
	{
		slVolumeRaw = 0;
	}
	if (slVolumeRaw > 1023)
	{
		slVolumeRaw = 1023;
	}

	// switch sound off, if not tuned.
	if (!iTuned)
	{
		slVolumeRaw = 0;
	}

	// Limit the timbre value
	if (slTimbre < 0)
	{
		slTimbre = 0;
	}
	if (slTimbre > 256)
	{
		slTimbre = 256;
	}
}

/**
 * Apply nonlinearity to the volume
 */
void THEREMIN_Task_Volume_Nonlin(void)
{
	slVolume = slVolumeRaw;
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
		// Start auto-tune by pressing BUTTON_KEY
		if (BSP_PB_GetState(BUTTON_KEY) == GPIO_PIN_SET)
		{

			// Mute the output
			bMute = 1;

			// Beep for "Auto tune start"
			BEEP_Play(NOTE_A6,50,50);
			BEEP_Play(NOTE_B6,50,50);
			BEEP_Play(NOTE_C7,100,50);

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
			slMinVol1Periode = 0x7FFFFFFF;
			slMinVol2Periode = 0x7FFFFFFF;
		}
	}
	else if (bBeepActive == 0)
	{


		// Find lowest pitch period
		if (slPitchPeriodeFilt < slMinPitchPeriode)
		{
			slMinPitchPeriode = slPitchPeriodeFilt;
		}
		// Find lowest volume period
		if (slVolTim1MeanPeriode < slMinVol1Periode)
		{
			slMinVol1Periode = slVolTim1MeanPeriode;
		}
		if (slVolTim2MeanPeriode < slMinVol2Periode)
		{
			slMinVol2Periode = slVolTim2MeanPeriode;
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


			// Beep for "Auto tune ends"
			BEEP_Play(NOTE_A5,100,100);

			DISPLAY_Dark();
			// Use minimum values for offset of pitch and volume
			slPitchOffset = slMinPitchPeriode;
			slVolTim1Offset = slMinVol1Periode;	// + 16384 * 128;
			slVolTim2Offset = slMinVol2Periode;	// + 16384 * 128;
#ifdef XDEBUG
		printf("%d %d\n", usPitchPeriod, usVolTim1Period);
#endif
		//	CONFIG_Write_SLong(EEPROM_ADDR_PITCH_AUTOTUNE_H, slPitchOffset);
		//	CONFIG_Write_SLong(EEPROM_ADDR_VOLTIM1_AUTOTUNE_H, slVolTim1Offset);
		//	CONFIG_Write_SLong(EEPROM_ADDR_VOLTIM2_AUTOTUNE_H, slVolTim2Offset);



			if (USB_STICK_EmptyFileExists("CALVOL.CSV"))
			{
				VOLUME_CalibrationStart();
			}
			else if (USB_STICK_EmptyFileExists("CALPITCH.CSV"))
			{
				PITCH_CalibrationStart();
			}
			else
			{
				// activate output
				bMute = 0;
			}
			fOscSin = 0.0f;
			fOscCos = 1.0f;
			iTuned = 1;
		}
	}

	if (iVolCal_active)
	{
		VOLUME_CalibrationTask();
	}

	if (iPitchCal_active)
	{
		PITCH_CalibrationTask();
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
		THEREMIN_Calc_VolumeTable(VOLSEL_1);
		THEREMIN_Calc_VolumeTable(VOLSEL_2);
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

	if (siAutotune == 0)
	{
		/*
		// Debug values for volume
		printf("%d %d %d %d %d %d %d %d\n",
				(int)usVolTim1Period,
				(int)usVolTim2Period,
				(int)slVolTim1MeanPeriode,
				(int)slVolTim2MeanPeriode,
				(int)slVolFilt,
				(int)slTimbre,
				(int) slVol1,
				(int) slVol2
				);
		*/

		// Debug values for pitch
		/*
		printf("%d %d\n",
				(int)(fPitchFrq*1000.0f),
				(int)(slPitchPeriodeFilt - slPitchOffset)
				);
		*/



		//printf("Stopwatch %d\n", ulStopwatch);
	}
#endif
}




