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
#include "project.h"
#include "audio_out.h"
#include "theremin.h"
#include "display.h"
#include "pots.h"
#include "config.h"
#include "usb_stick.h"
#include "beep.h"
#include "volume.h"
#include "console.h"
#include <math.h>
#include <stdlib.h>


// Linearization tables for pitch an volume
float fPitchLinTable[2048+1];

uint16_t usDistortionTable[2048+1];
uint16_t usImpedanceTable[2048+1];

// Pitch
float fPitchFrq;		// pitch frequency scaled to 96kHz task
float fPitchFrq1;		// pitch frequency scaled to 96kHz task
float fPitchFrq2;		// pitch frequency scaled to 96kHz task
float fPitchFrq3;		// pitch frequency scaled to 96kHz task
float fPitchFrq4;		// pitch frequency scaled to 96kHz task
float fPitchFrq5;		// pitch frequency scaled to 96kHz task
float fFiltLP1 = 0.0f;
float fFiltLP2 = 0.0f;
float fPitchScale = 1.0f;
float fPitchShift = 1.0f;
float fFilterIn = 0.0f;

float fFrq1;
float fFrq2;

// Volume
float fVol = 0.0;			// volume value
int32_t slVolTimPeriodeFiltDiff;	// low pass filtered period
int32_t slVolFiltL;				// volume value, filtered (internal filter value)
int32_t slVolFilt;				// volume value, filtered
int32_t slVolumeRaw;
int32_t slVolume;
int32_t slTimbre;
float fTimbre = 0.0f;
int32_t	slOutCapacitor = 0;

// Audio oscillators
float fOscSin = 0.0f;
float fOscCos = 1.0f;
float fOscCorr = 1.0f;
float fFrqCorr = 0.0f;
float fFrq = 0.0f;
float fOscOut = 0.0f;
float fOscOutMin = 0.0f;
float fOscOutMax = 0.0f;
float fOscOutOffset = 0.0f;
float fOscOutScale = 0.0f;
float fOscOutOffsetFilt = 0.0f;
float fOscOutScaleFilt = 0.0f;

float fOscMix = 0.0f;
float fOscPhase = 0.0f;
float fOscSaw = 0.0f;
float fBlep = 0.0f;

float fOscHp1 = 0.0f;
float fOscLP1 = 0.0f;
float fOscLP2 = 0.0f;
float fOscLP3 = 0.0f;
float fOscLP4 = 0.0f;
float fOscFilt5 = 0.0f;

float fSVF1z1 = 0.0f;
float fSVF1z2 = 0.0f;
float fSVF1LP = 0.0f;
float fSVF1HP = 0.0f;
float fSVF1BR = 0.0f;

float fSVF2z1 = 0.0f;
float fSVF2z2 = 0.0f;
float fSVF2LP = 0.0f;
float fSVF2HP = 0.0f;
float fSVF2BR = 0.0f;

float fSVF3z1 = 0.0f;
float fSVF3z2 = 0.0f;
float fSVF3LP = 0.0f;
float fSVF3HP = 0.0f;
float fSVF3BR = 0.0f;

float fSVF4z1 = 0.0f;
float fSVF4z2 = 0.0f;
float fSVF4LP = 0.0f;
float fSVF4HP = 0.0f;
float fSVF4BR = 0.0f;


float fSVF5z1 = 0.0f;
float fSVF5z2 = 0.0f;
float fSVF5LP = 0.0f;
float fSVF5HP = 0.0f;
float fSVF5BR = 0.0f;

float fSVF6z1 = 0.0f;
float fSVF6z2 = 0.0f;
float fSVF6LP = 0.0f;
float fSVF6HP = 0.0f;
float fSVF6BR = 0.0f;

int iOscSign = 0;
int iOscSignLast = 0;


float fOscSin2 = 0.0f;
float fOscCos2 = 1.0f;
float fOscCorr2 = 1.0f;
float fOscSin3 = 0.0f;
float fOscCos3 = 1.0f;
float fOscCorr3 = 1.0f;
float fOscSin4 = 0.0f;
float fOscCos4 = 1.0f;
float fOscCorr4 = 1.0f;
float fOscSin5 = 0.0f;
float fOscCos5 = 1.0f;
float fOscCorr5 = 1.0f;

float fAddSynth_1 = 0.0f;
float fAddSynth_2 = 0.0f;
float fAddSynth_3 = 0.0f;
float fAddSynth_4 = 0.0f;
float fAddSynth_5 = 0.0f;
float fRichness = 0.0f;

float fVolScale = 1.0f;
float fVolShift = 0.0f;

int32_t slThereminOut = 0;

uint32_t ulWaveTableIndex = 0;

float fWavStepFilt = 0.0f;

// Auto-tune
int siAutotune = 0;			// Auto-tune down counter
uint32_t ulLedCircleSpeed;	// LED indicator speed
uint32_t ulLedCirclePos;	// LED indicator position
int iTuned = 0;

// Calibration
e_calibration eCalibration = CALIB_OFF;
int bCalib1stFound = 0;
int iCalibCnt = 0;
uint16_t usCalibFirst;
uint16_t usCalibSecond;
uint16_t usCalibDiff;
s_osc aOsc[3];

int iWavMask = 0x0FFF;
int iWavLength = 4096;
int bUseNonLinTab = 0;


int task = 0;


int taskTableCnt = 0;
void (*taskTable[96]) () = {0};

extern TIM_HandleTypeDef htim1;	// Handle of timer for input capture

uint32_t ulStopwatch = 0;

/* local functions  ------------------------------------------------------- */
static void THEREMIN_Calibrate(int osc);

/**
 * @brief Initialize the module
 *
 */
void THEREMIN_Init(void)
{
	int i;

	// Fill the task table
	taskTable[0] = THEREMIN_Task_Select_Volume;
	taskTable[2] = THEREMIN_Task_Volume;
	taskTable[6] = THEREMIN_Task_Timbre;
	taskTable[8] = THEREMIN_Task_Volume_Nonlin;
	taskTable[16] = THEREMIN_Task_Activate_Volume;
	for (i = 18; i< 46; i+=2)
	{
		taskTable[i] = THEREMIN_Task_Capture_Volume;
	}
	taskTable[46] = THEREMIN_Task_Calculate_Volume;

	taskTable[48] = THEREMIN_Task_Select_Timbre;
	taskTable[64] = THEREMIN_Task_Activate_Timbre;
	for (i = 66; i< 94; i+=2)
	{
		taskTable[i] = THEREMIN_Task_Capture_Timbre;
	}
	taskTable[94] = THEREMIN_Task_Calculate_Timbre;


	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	// Read auto-tune values from virtual EEPRom
	aOsc[PITCH].slOffset = CONFIG_Read_SLong(EEPROM_ADDR_PITCH_AUTOTUNE_H);
	aOsc[VOLUME].slOffset = CONFIG_Read_SLong(EEPROM_ADDR_VOLTIM1_AUTOTUNE_H);
	aOsc[TIMBRE].slOffset = CONFIG_Read_SLong(EEPROM_ADDR_VOLTIM2_AUTOTUNE_H);

	// Get the VolumeShift value from the flash configuration
	fVolShift = ((float) (CONFIG.VolumeShift)) * 0.1f + 11.5f;


	// Calculate the LUT for volume and pitch
	THEREMIN_Calc_PitchTable();
	THEREMIN_Calc_DistortionTable();
	THEREMIN_Calc_ImpedanceTable();

	HAL_GPIO_WritePin(SEL_VOLUME_OSC_GPIO_Port, SEL_VOLUME_OSC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SEL_TIMBRE_OSC_GPIO_Port, SEL_TIMBRE_OSC_Pin, GPIO_PIN_RESET);


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
				* 0.05f;

		// Limit the output values
		if (f > 0.5f)
		{
			f = 0.5f;
		}
		if (f < 0.0f)
		{
			f = 0.0f;
		}

		// Fill the pitch table
		fPitchLinTable[i] = f;
	}
}

/**
 * @brief Recalculates the distortion LUT
 *
 */

void THEREMIN_Calc_DistortionTable(void)
{
	float f,f1;
	float fDistortion = ((float)aConfigWorkingSet[CFG_E_DISTORTION].iVal * 0.001f)*10.0f;

	if (aConfigWorkingSet[CFG_E_DISTORTION].iVal != 0)
	{
		for (int32_t i = 0; i < 1024; i++)
		{
			// Calculate the nonlinear function
			f1 = ((float)(1024-i)*0.000976562f);
			f=(1.0f-f1+((expf(-f1*fDistortion)-1.0f)/fDistortion+f1))*0.5f;
			// Fill the distortion LUT
			usDistortionTable[i] = f*65535.0f;
		}

		for (int32_t i = 1024; i < 2048; i++)
		{
			// Fill the distortion LUT
			usDistortionTable[i] = i*32;
		}
	}
	else
	{
		for (int32_t i = 0; i < 2048; i++)
		{
			// Fill the distortion LUT with non distortion value
			usDistortionTable[i] = i*32;
		}
	}
	usDistortionTable[2048] = 65535;
}

/**
 * @brief Recalculates the impedance LUT
 *
 */
void THEREMIN_Calc_ImpedanceTable(void)
{
	float f,f1;
	float fImpedance = ((float)aConfigWorkingSet[CFG_E_IMPEDANCE].iVal * 0.001f)*4.0f;

	if (aConfigWorkingSet[CFG_E_IMPEDANCE].iVal != 0)
	{
		for (int32_t i = 0; i < 2048; i++)
		{
			// Calculate the nonlinear function
			f1 = ((float)(i)*0.000488281f);
			f=powf(f1,fImpedance);
			// Fill the distortion LUT
			usImpedanceTable[i] = f*65535.0f;
		}
	}
	else
	{
		for (int32_t i = 0; i < 2048; i++)
		{
			// Fill the impedance LUT with R=0Ohm value
			usImpedanceTable[i] = 65535;
		}
	}
	usImpedanceTable[2048] = 65535;
}

float THEREMIN_PolyBLEP(float t, float dt)
{
	if (t < 0.0f)
	{
		t += 1.0f;
	}

    // t-t^2/2 +1/2
    // 0 < t <= 1
    // discontinuities between 0 & 1
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }

    // t^2/2 +t +1/2
    // -1 <= t <= 0
    // discontinuities between -1 & 0
    else if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }

    // no discontinuities
    // 0 otherwise
    else return 0.0f;
}


/**
 * @brief 96kHz DAC task called in interrupt
 *
 * At 168MHz the absolute maximum cycle count would be 168MHz/96kHz = 1750
 */
inline void THEREMIN_96kHzDACTask_A(void)
{
	int32_t tabix;
	float p1f, p2f, tabsubf,f;
	int p1, p2, tabsub;
	floatint_ut u;
	int16_t ssResult;
	uint16_t usDistorted, usImpedance;

	//STOPWATCH_START();

	// Low pass filter the output to avoid aliasing noise.
	slVolFiltL += slVolume - slVolFilt;
	slVolFilt = slVolFiltL / 1024;


	// Get the input capture value and calculate the period time
	aOsc[PITCH].usCC = htim1.Instance->CCR1;
	aOsc[PITCH].usPeriodRaw = aOsc[PITCH].usPeriod = aOsc[PITCH].usCC - aOsc[PITCH].usLastCC;
	aOsc[PITCH].usLastCC = aOsc[PITCH].usCC;

	// Find the typical frequency of the pitch oscillator
	if (eCalibration == CALIB_PITCH)
	{
		if (!bCalib1stFound)
		{
			bCalib1stFound = 1;
			usCalibFirst = aOsc[PITCH].usPeriod;
		}
		else
		{
			if (	(aOsc[PITCH].usPeriod > (usCalibFirst + usCalibFirst / 4))
				||	(aOsc[PITCH].usPeriod < (usCalibFirst - usCalibFirst / 4)))
			{
				bCalib1stFound = 0;
				if (aOsc[PITCH].usPeriod > usCalibFirst)
				{
					usCalibSecond = aOsc[PITCH].usPeriod;
				}
				else
				{
					usCalibSecond = usCalibFirst;
					usCalibFirst = aOsc[PITCH].usPeriod;
				}
				eCalibration = CALIB_PITCH_FINISHED;
			}

			// Timeout after 1sec
			iCalibCnt ++;
			if (iCalibCnt > 48000)
			{
				iCalibCnt = 0;
				bCalib1stFound = 0;
				usCalibFirst = 0;
				usCalibSecond = 0;
				eCalibration = CALIB_PITCH_FINISHED;
			}
		}
	}


	// cycles:
	// Low pass filter period values
	// factor *1024 is necessary, because of the /1024 integer division
	// factor *1024 is necessary, because we want to over sample the input signal
	if (aOsc[PITCH].usPeriod != 0)
	{
		if (aOsc[PITCH].usPeriod > aOsc[PITCH].usCalibThreshold2)
		{
			aOsc[PITCH].usPeriod *= aOsc[PITCH].iCalibFact3;
		}
		else if (aOsc[PITCH].usPeriod > aOsc[PITCH].usCalibThreshold1)
		{
			aOsc[PITCH].usPeriod *= aOsc[PITCH].iCalibFact2;
		}
		else
		{
			aOsc[PITCH].usPeriod *= aOsc[PITCH].iCalibFact1;
		}
		aOsc[PITCH].usPeriodRawN = aOsc[PITCH].usPeriod;
		aOsc[PITCH].usPeriod-= aOsc[PITCH].usPeriodOffset;
		//                                   11bit      10bit  10bit
		aOsc[PITCH].slPeriodeFilt += ((int16_t)aOsc[PITCH].usPeriod * 1024 * 1024
				- aOsc[PITCH].slPeriodeFilt) / 256;

	}

	// cycles:21
	aOsc[PITCH].fValue = ((float) (aOsc[PITCH].slPeriodeFilt - aOsc[PITCH].slOffset)) * aOsc[PITCH].fCalibfScale;

	if (bBeepActive)
	{
		BEEP_Task();
		return;
	}



	STOPWATCH_START();
	STOPWATCH_STOP();

	if (aOsc[PITCH].fValue >= 1.0f)
	{
		// cycles:
		// fast pow approximation by LUT with interpolation
		// float bias: 127, so 127 << 23bit mantissa is: 0x3F800000
		// We use (5 bit of) the exponent and 6 bit of the mantissa
		u.f = aOsc[PITCH].fValue;
		tabix = ((u.ui - 0x3F800000) >> 17);
//		tabsub = (u.ui & 0x0001FFFF) >> 2;
		tabsubf = (u.ui & 0x0001FFFF) *  0.000007629f; // =2^-18
		p1f = fPitchLinTable[tabix];
		p2f = fPitchLinTable[tabix + 1];
		fPitchFrq = (p1f + (((p2f - p1f) * tabsubf)));
	}
	else
	{
		fPitchFrq = 0.0f;
	}



	// ************* Sine oscillator *************
	//
	// Sine oscillator, phase in synch with sawtooth
	// TIME: 56
	fFrq = (fPitchFrq + fFrqCorr) * 0.5f;
	fOscSin += fFrq * fOscCos;
	fOscCos -= fFrq * fOscSin;
	fOscSin += fFrq * fOscCos;
	fOscCos -= fFrq * fOscSin;
	fOscCorr = 1.0f +((1.0f - (fOscSin * fOscSin + fOscCos * fOscCos))*0.01f);
	fOscCos *= fOscCorr;
	fOscSin *= fOscCorr;

	fOscSin2 = 2.0f * fOscSin * fOscCos;
	fOscSin3 = 2.0f * fOscSin2 * fOscCos - fOscSin;
	fOscSin4 = 2.0f * fOscSin3 * fOscCos - fOscSin2;

	STOPWATCH_START();

	fBlep = THEREMIN_PolyBLEP(fOscPhase  * 0.159154943f, fPitchFrq * 0.159154943f);
	// ************* PolyBLEP Sawtooth oscillator *************
	//
	// See http://metafunction.co.uk/all-about-digital-oscillators-part-2-blits-bleps/
	// See http://www.martin-finke.de/blog/articles/audio-plugins-018-polyblep-oscillator/
	fOscSaw = fOscPhase * -0.318309886f + 1.0f;
	fOscSaw += fBlep;
    STOPWATCH_STOP();


	// Phase increment for rectangle and sawtooth oscillator
	fOscPhase += fPitchFrq;
    if (fOscPhase >= 6.283185307f)
    {
    	fOscPhase -= 6.283185307f;
    	fFrqCorr = -(fOscSin - fOscPhase) * 0.159154943f * 0.1f * fPitchFrq;

        fOscOutOffset = (fOscOutMax + fOscOutMin) / 2;
        if (fOscOutMax > fOscOutMin + 0.1f)
        {
            fOscOutScale = 30.0f / (fOscOutMax - fOscOutMin);
        }
        fOscOutMin = 0.0f;
        fOscOutMax = 0.0f;
    }


    fOscMix = fAddSynth_1 * fOscSin
    		+ fAddSynth_2 * fOscSin2
    		+ fAddSynth_3 * fOscSin3
    		+ fAddSynth_4 * fOscSin4
    		+ fAddSynth_5 * fOscSin5
			+ fRichness * fOscSaw ;


    fFrq1 = ((1.0f * 10.0f) + 0.5f) * fPitchFrq; //0.2f;//fPitchFrq;
    fFrq2 = ((fAddSynth_4 * 4.0f)) * fPitchFrq;
	if (fFrq1 > 1.0f)
	{
		fFrq1 = 1.0f;
	}
    fSVF3LP = fSVF3z2 + 0.3f * fSVF3z1 ;
    fSVF3z2 = fSVF3LP;
    fSVF3HP = fOscOut - 1.0f * fSVF3z1 - fSVF3LP;
    fSVF3z1 = fSVF3z1 + 0.3f * fSVF3HP;



    fOscHp1 += (fOscMix - fOscHp1) * 0.00390625f; // 30Hz HighPass
    fOscOut = fOscMix-fOscHp1;

    if (fOscOut < fOscOutMin)
    {
    	fOscOutMin = fOscOut;
    }
    if (fOscOut > fOscOutMax)
    {
    	fOscOutMax = fOscOut;
    }

    fOscOutOffsetFilt += (fOscOutOffset - fOscOutOffsetFilt) * 0.001f;
    fOscOutScaleFilt += (fOscOutScale - fOscOutScaleFilt) * 0.001f;



	slThereminOut = (fOscOut - fOscOutOffsetFilt) * fOscOutScaleFilt  * (float)slVolFilt;
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
 * Switch on volume oscillator
 */
void THEREMIN_Task_Select_Volume(void)
{
	// Select oscillator of VOL1
	HAL_GPIO_WritePin(SEL_VOLUME_OSC_GPIO_Port, SEL_VOLUME_OSC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SEL_TIMBRE_OSC_GPIO_Port, SEL_TIMBRE_OSC_Pin, GPIO_PIN_RESET);
}

/**
 * Switch on timbre oscillator
 */
void THEREMIN_Task_Select_Timbre(void)
{
	// Select oscillator of VOL2
	HAL_GPIO_WritePin(SEL_VOLUME_OSC_GPIO_Port, SEL_VOLUME_OSC_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SEL_TIMBRE_OSC_GPIO_Port, SEL_TIMBRE_OSC_Pin, GPIO_PIN_SET);
}

/**
 * Oscillator is stable. Use now the frequency signal
 */
void THEREMIN_Task_Activate_Volume(void)
{
	aOsc[VOLUME].usLastCC = htim1.Instance->CCR2; // Read capture compare to be prepared
	aOsc[VOLUME].slPeriodeFilt = 0;	// Reset mean filter counter
	aOsc[VOLUME].slPeriodeFilt_cnt = 0;	// Reset mean filter counter
}

/**
 * Oscillator is stable. Use now the frequency signal
 */
void THEREMIN_Task_Activate_Timbre(void)
{
	aOsc[TIMBRE].usLastCC = htim1.Instance->CCR3; // Read capture compare to be prepared
	aOsc[TIMBRE].slPeriodeFilt = 0;	// Reset mean filter counter
	aOsc[TIMBRE].slPeriodeFilt_cnt = 0;	// Reset mean filter counter
}

/**
 * Capture and filter the VOL1 value
 */
void THEREMIN_Task_Capture_Volume(void)
{
	// Get the input capture value and calculate the period time
	aOsc[VOLUME].usCC = htim1.Instance->CCR2;
	aOsc[VOLUME].usPeriodRaw = aOsc[VOLUME].usPeriod = aOsc[VOLUME].usCC - aOsc[VOLUME].usLastCC;
	aOsc[VOLUME].usLastCC = aOsc[VOLUME].usCC;

	// Find the typical frequency of the vol1 oscillator
	if (eCalibration == CALIB_VOLUME)
	{
		if (!bCalib1stFound)
		{
			bCalib1stFound = 1;
			usCalibFirst = aOsc[VOLUME].usPeriod;
		}
		else
		{
			if (	(aOsc[VOLUME].usPeriod > (usCalibFirst + usCalibFirst / 4))
				||	(aOsc[VOLUME].usPeriod < (usCalibFirst - usCalibFirst / 4)))
			{
				bCalib1stFound = 0;
				if (aOsc[VOLUME].usPeriod > usCalibFirst)
				{
					usCalibSecond = aOsc[VOLUME].usPeriod;
				}
				else
				{
					usCalibSecond = usCalibFirst;
					usCalibFirst = aOsc[VOLUME].usPeriod;
				}
				eCalibration = CALIB_VOLUME_FINISHED;
			}

			// Timeout after 1sec
			iCalibCnt ++;
			if (iCalibCnt > 48000)
			{
				iCalibCnt = 0;
				bCalib1stFound = 0;
				usCalibFirst = 0;
				usCalibSecond = 0;
				eCalibration = CALIB_VOLUME_FINISHED;
			}
		}
	}


	// Low pass filter it with a mean filter
	if (aOsc[VOLUME].usPeriod != 0)
	{



		//TODO: Thresholds
		// Was it a .... period?
		if (aOsc[VOLUME].usPeriod > 8081)
		{
			aOsc[VOLUME].touched = 1;
		} else
		// Was it a triple period?
		if (aOsc[VOLUME].usPeriod > 5772)
		{
			// Count the amount of periods
			aOsc[VOLUME].slPeriodeFilt_cnt +=3;
			// Accumulate the period values
			slVolTimPeriodeFiltDiff = 256 * aOsc[VOLUME].usPeriod - 3*aOsc[VOLUME].slOffset;
		}
		// Was it a double period?
		else if (aOsc[VOLUME].usPeriod > 3463)
		{
			// Count the amount of periods
			aOsc[VOLUME].slPeriodeFilt_cnt +=2;
			// Accumulate the period values
			slVolTimPeriodeFiltDiff = 256 * aOsc[VOLUME].usPeriod - 2*aOsc[VOLUME].slOffset;
		}
		else
		{
			// Count the amount of periods
			aOsc[VOLUME].slPeriodeFilt_cnt +=1;
			// Accumulate the period values
			slVolTimPeriodeFiltDiff = 256 * aOsc[VOLUME].usPeriod - aOsc[VOLUME].slOffset;
		}

		// Use only positive values
		if (slVolTimPeriodeFiltDiff > 0)
		{
			aOsc[VOLUME].slPeriodeFilt += slVolTimPeriodeFiltDiff;
		}
	}
}


/**
 * Capture and filter the timbre value
 */
void THEREMIN_Task_Capture_Timbre(void)
{
	// Get the input capture value and calculate the period time
	aOsc[TIMBRE].usCC = htim1.Instance->CCR3;
	aOsc[TIMBRE].usPeriod = aOsc[TIMBRE].usCC - aOsc[TIMBRE].usLastCC;
	aOsc[TIMBRE].usLastCC = aOsc[TIMBRE].usCC;

	// Find the typical frequency of the vol1 oscillator
	if (eCalibration == CALIB_TIMBRE)
	{
		if (!bCalib1stFound)
		{
			bCalib1stFound = 1;
			usCalibFirst = aOsc[TIMBRE].usPeriod;
		}
		else
		{
			if (	(aOsc[TIMBRE].usPeriod > (usCalibFirst + usCalibFirst / 4))
				||	(aOsc[TIMBRE].usPeriod < (usCalibFirst - usCalibFirst / 4)))
			{
				bCalib1stFound = 0;
				if (aOsc[TIMBRE].usPeriod > usCalibFirst)
				{
					usCalibSecond = aOsc[TIMBRE].usPeriod;
				}
				else
				{
					usCalibSecond = usCalibFirst;
					usCalibFirst = aOsc[TIMBRE].usPeriod;
				}
				eCalibration = CALIB_TIMBRE_FINISHED;
			}

			// Timeout after 1sec
			iCalibCnt ++;
			if (iCalibCnt > 48000)
			{
				iCalibCnt = 0;
				bCalib1stFound = 0;
				usCalibFirst = 0;
				usCalibSecond = 0;
				eCalibration = CALIB_TIMBRE_FINISHED;
			}
		}
	}

	// Low pass filter it with a mean filter
	if (aOsc[TIMBRE].usPeriod != 0)
	{
		// Accumulate the period values
		slVolTimPeriodeFiltDiff = 256 * aOsc[TIMBRE].usPeriod - aOsc[TIMBRE].slOffset;

		// Count the amount of periods
		aOsc[TIMBRE].slPeriodeFilt_cnt ++;
		// Was it a triple period?
		if (aOsc[TIMBRE].usPeriod > 6000)
		{
			aOsc[TIMBRE].slPeriodeFilt_cnt +=2;
			slVolTimPeriodeFiltDiff -= 2*aOsc[TIMBRE].slOffset;
		}
		// Was it a double period?
		else if (aOsc[TIMBRE].usPeriod > 4000)
		{
			aOsc[TIMBRE].slPeriodeFilt_cnt ++;
			slVolTimPeriodeFiltDiff -= aOsc[TIMBRE].slOffset;
		}

		// Use only positive values
		if (slVolTimPeriodeFiltDiff > 0)
		{
			aOsc[TIMBRE].slPeriodeFilt += slVolTimPeriodeFiltDiff;
		}
	}
}

/**
 * Calculate the mean value of Volume
 */
void THEREMIN_Task_Calculate_Volume(void)
{

	if (aOsc[VOLUME].slPeriodeFilt > 0)
	{
		aOsc[VOLUME].slMeanPeriode = aOsc[VOLUME].slPeriodeFilt / aOsc[VOLUME].slPeriodeFilt_cnt;
//		aOsc[VOLUME].slValue = ((aConfigWorkingSet[CFG_E_VOL1_NUMERATOR].iVal * aOsc[VOLUME].slPeriodeFilt_cnt) /
//				(aOsc[VOLUME].slPeriodeFilt + aOsc[VOLUME].slPeriodeFilt_cnt * aConfigWorkingSet[CFG_E_VOL1_OFFSET_A].iVal))
//				- aConfigWorkingSet[CFG_E_VOL1_OFFSET_B].iVal;

		// Prepare for next filter interval
		aOsc[VOLUME].slPeriodeFilt_cnt = 0;
		aOsc[VOLUME].slPeriodeFilt = 0;
	}

//	// Limit it
//	if (aOsc[VOLUME].slValue < 0)
//	{
//		aOsc[VOLUME].slValue = 0;
//	}
//	if (aOsc[VOLUME].slValue > 1024)
//	{
//		aOsc[VOLUME].slValue = 1024;
//	}
}

/**
 * Calculate the mean value of timbre
 */
void THEREMIN_Task_Calculate_Timbre(void)
{
	if (aOsc[TIMBRE].slPeriodeFilt > 0)
	{
		aOsc[TIMBRE].slMeanPeriode = aOsc[TIMBRE].slPeriodeFilt / aOsc[TIMBRE].slPeriodeFilt_cnt;
//		aOsc[TIMBRE].slValue = ((aConfigWorkingSet[CFG_E_VOL2_NUMERATOR].iVal * aOsc[TIMBRE].slPeriodeFilt_cnt) /
//				(aOsc[TIMBRE].slPeriodeFilt + aOsc[TIMBRE].slPeriodeFilt_cnt * aConfigWorkingSet[CFG_E_VOL2_OFFSET_A].iVal))
//				- aConfigWorkingSet[CFG_E_VOL2_OFFSET_B].iVal;

		// Prepare for next filter interval
		aOsc[TIMBRE].slPeriodeFilt_cnt = 0;
		aOsc[TIMBRE].slPeriodeFilt = 0;
	}

//	// Limit it
//	if (aOsc[TIMBRE].slValue < 0)
//	{
//		aOsc[TIMBRE].slValue = 0;
//	}
//	if (aOsc[TIMBRE].slValue > 1024)
//	{
//		aOsc[TIMBRE].slValue = 1024;
//	}
}
/**
 * Calculate the mean volume from VOL1 and VOL2
 */
void THEREMIN_Task_Volume(void)
{
	if (aOsc[VOLUME].touched)
	{
		slVolumeRaw = 0;
		aOsc[VOLUME].touched = 0;
	}
	else
	{
		// Linearization
		slVolumeRaw = iVolNumerator / (aOsc[VOLUME].slMeanPeriode + iVolLinFactor) - iVolOffset;
	}

	// Limit the volume value
	if (slVolumeRaw < 0)
	{
		slVolumeRaw = 0;
	}
	if (slVolumeRaw > 1023)
	{
		slVolumeRaw = 1023;
	}

	// change the direction
	if (aConfigWorkingSet[CFG_E_LOUDER_DOWN].iVal != 0)
	{
		slVolumeRaw = 1023 - slVolumeRaw;
	}

	// switch sound off, if not tuned.
	if (!iTuned)
	{
		slVolumeRaw = 0;
	}
}

/**
 * Calculate the timbre value
 */
void THEREMIN_Task_Timbre(void)
{
	float f;
	slTimbre = ((aOsc[TIMBRE].slMeanPeriode - 1000 )/ 16);

	// Limit the timbre value
	if (slTimbre < 0)
	{
		slTimbre = 0;
	}
	if (slTimbre > 256)
	{
		slTimbre = 256;
	}

	// Scale it from 0..256 to 0.0f .. 1.0f
	fTimbre = 1.0f;//(float)slTimbre * 0.00390625f;

	fRichness = ((float)aConfigWorkingSet[CFG_E_RICHNESS].iVal * 0.001f);

	fAddSynth_1 = ((float)aConfigWorkingSet[CFG_E_ADDSYNTH_1].iVal * 0.001f) - fRichness * (0.636619772f / 1.0f);
	fAddSynth_2 = ((float)aConfigWorkingSet[CFG_E_ADDSYNTH_2].iVal * 0.001f) - fRichness * (0.636619772f / 2.0f);
	fAddSynth_3 = ((float)aConfigWorkingSet[CFG_E_ADDSYNTH_3].iVal * 0.001f) - fRichness * (0.636619772f / 3.0f);
	/*fAddSynth_4 = ((float)aConfigWorkingSet[CFG_E_ADDSYNTH_4].iVal * 0.001f);
	fAddSynth_5 = ((float)aConfigWorkingSet[CFG_E_ADDSYNTH_5].iVal * 0.001f);*/

	fAddSynth_4 = 0 -fRichness * (0.636619772f / 4.0f);
	fAddSynth_5 = 0 -fRichness * (0.636619772f / 5.0f);

	/*
	fAddSynth_1 = 1.0f;
	fAddSynth_2 = 0.0f;
	fAddSynth_3 = 0.0f;
	fAddSynth_4 = 0.0f;
	fAddSynth_5 = 0.0f;
	*/

}

/**
 * Apply nonlinearity to the volume
 */
void THEREMIN_Task_Volume_Nonlin(void)
{

	if (eAutomuteAutoprehear == AUTOMUTE_MUTE )
	{
		slVolume = 0;
	}
	else if (eAutomuteAutoprehear == AUTOPREHEAR)
	{
		slVolume = 100;
	}
	else
	{
		slVolume = slVolumeRaw;
	}
}

/**
 * Calibrate one oscillator
 */
static void THEREMIN_Calibrate(int osc)
{
	usCalibDiff = usCalibSecond - usCalibFirst;

	aOsc[osc].iCalibN = (usCalibSecond + usCalibDiff/2)  / usCalibDiff;

	switch (aOsc[osc].iCalibN)
	{
	// n= 0 or 1 (rarely 2)
	// Oscillator: f = 100kHz..384kHz
	// Period = 0 or 3500..13440
	// Results in 3500 .. 13440
	case 1:
		aOsc[osc].iCalibFact1 = 0;
		aOsc[osc].iCalibFact2 = 2;
		aOsc[osc].iCalibFact3 = 1;
		aOsc[osc].ulCalibScale = 2;
		// Calculate an offset to reduce the result to 11 bit
		aOsc[osc].usPeriodOffset = usCalibSecond * aOsc[osc].iCalibFact2;
		// Calculate the (first) threshold
		aOsc[osc].usCalibThreshold1 = usCalibFirst + usCalibDiff / 2;
		break;
	// n= 1 or 2 (rarely 3)
	// Oscillator: f = 384kHz..768kHz
	// Period = 1750..3500 or 3500..7000
	// Results in 3500 .. 7000
	case 2:
		aOsc[osc].iCalibFact1 = 6;
		aOsc[osc].iCalibFact2 = 3;
		aOsc[osc].iCalibFact3 = 2;
		aOsc[osc].ulCalibScale = 6;
		// Calculate an offset to reduce the result to 11 bit
		aOsc[osc].usPeriodOffset = usCalibSecond * aOsc[osc].iCalibFact2;
		// Calculate the (first) threshold
		aOsc[osc].usCalibThreshold1 = usCalibFirst + usCalibDiff / 2;
		break;
	// n= 2 or 3 (rarely 1)
	// Oscillator: f = 768kHz..1152kHz
	// Period = 2333..3500 or 3500..10500
	// Results in 7000 .. 21000
	case 3:
		aOsc[osc].iCalibFact1 = 6;
		aOsc[osc].iCalibFact2 = 3;
		aOsc[osc].iCalibFact3 = 2;
		aOsc[osc].ulCalibScale = 6;
		// Calculate an offset to reduce the result to 11 bit
		aOsc[osc].usPeriodOffset = usCalibSecond * aOsc[osc].iCalibFact3;
		// Calculate the (first) threshold
		aOsc[osc].usCalibThreshold1 = usCalibFirst - usCalibDiff / 2;
		break;
	default:
		aOsc[osc].iCalibFact1 = 0;
		aOsc[osc].iCalibFact2 = 0;
		aOsc[osc].iCalibFact3 = 0;
		aOsc[osc].ulCalibScale = 0;
		aOsc[osc].usCalibThreshold1 = 0;
	}

	aOsc[osc].usCalibThreshold2 = aOsc[osc].usCalibThreshold1 + usCalibDiff;



	// Scale the pitch frequency after filter to have
	// an oscillator frequency independent span
	if (aOsc[osc].usPeriodOffset > 0)
	{
		aOsc[osc].fCalibfScale = 3500.0f/ (float)aOsc[osc].usPeriodOffset;
		// Set the offset to 93.75%
		aOsc[osc].usPeriodOffset -= aOsc[osc].usPeriodOffset / 16;
	}
	else
	{
		aOsc[osc].fCalibfScale = 0.0f;
		aOsc[osc].usPeriodOffset = 0;
	}

}




/**
 * @brief 1ms task
 *
 */
void THEREMIN_1msTask(void)
{
	static int startup_cnt = 1;
	int bReqCalcPitchTable = 0;

	int iAutoTuneStart = aConfigWorkingSet[CFG_E_STARTUP_AUTOTUNE].iVal;
	if (iAutoTuneStart>0)
	{
		// Delay 2s so the auto-tune on start-up will be started
		// not earlier than after the power-on beep
		iAutoTuneStart+=2000;
	}

	if (startup_cnt < 1000000)
	{
		startup_cnt++;
	}



	if (siAutotune == 0)
	{
		// Start auto-tune by pressing BUTTON_KEY
		if (BSP_PB_GetState(BUTTON_KEY) == GPIO_PIN_SET ||
				(startup_cnt == iAutoTuneStart)
			)
		{
			// Disable further auto-tune after startup
			startup_cnt = 1000000;
			// Mute the output
			bMute = 1;

			// Beep for "Auto tune start"
			//BEEP_Play(NOTE_A6,50,50);
			//BEEP_Play(NOTE_B6,50,50);
			//BEEP_Play(NOTE_C7,100,50);

			DISPLAY_Dark();

			// 1.0sec auto-tune
			siAutotune = 1000;

			// Start with calibration
			eCalibration = CALIB_PITCH;

			// Reset LED indicator and pitch and volume values
			ulLedCircleSpeed = siAutotune;
			ulLedCirclePos = 0;
			aOsc[PITCH].slOffset = 0;
			aOsc[PITCH].slPeriodeFilt = 0x7FFFFFFF;
			aOsc[PITCH].slMinPeriode = 0x7FFFFFFF;
			aOsc[VOLUME].slOffset = 0;
			aOsc[TIMBRE].slOffset = 0;
			aOsc[VOLUME].slMinPeriode = 0x7FFFFFFF;
			aOsc[TIMBRE].slMinPeriode = 0x7FFFFFFF;
			aOsc[VOLUME].slMeanPeriode = 0x7FFFFFFF;
			aOsc[TIMBRE].slMeanPeriode = 0x7FFFFFFF;
		}
	}
	else if (eCalibration != CALIB_OFF)
	{
		if (eCalibration == CALIB_PITCH_FINISHED)
		{
			THEREMIN_Calibrate(PITCH);
			eCalibration = CALIB_VOLUME;
		}
		else if (eCalibration == CALIB_VOLUME_FINISHED)
		{
			THEREMIN_Calibrate(VOLUME);
			eCalibration = CALIB_TIMBRE;
		}
		else if (eCalibration == CALIB_TIMBRE_FINISHED)
		{
			THEREMIN_Calibrate(TIMBRE);
			eCalibration = CALIB_OFF;
		}
	}
	else // if (bBeepActive == 0)
	{
		// Wait 200ms to start with the detection
		// until both volume channels are stabilized
		if (siAutotune < (1000-200))
		{
			// Find lowest pitch period
			if (aOsc[PITCH].slPeriodeFilt < aOsc[PITCH].slMinPeriode)
			{
				aOsc[PITCH].slMinPeriode = aOsc[PITCH].slPeriodeFilt;
			}
			// Find lowest volume period
			if (aOsc[VOLUME].slMeanPeriode < aOsc[VOLUME].slMinPeriode)
			{
				aOsc[VOLUME].slMinPeriode = aOsc[VOLUME].slMeanPeriode;
			}
			if (aOsc[TIMBRE].slMeanPeriode < aOsc[TIMBRE].slMinPeriode)
			{
				aOsc[TIMBRE].slMinPeriode = aOsc[TIMBRE].slMeanPeriode;
			}
		}
		siAutotune--;

		BEEP_AutoTuneSound(siAutotune);

		// LED indicator
		ulLedCircleSpeed = siAutotune;
		ulLedCirclePos += ulLedCircleSpeed;
		BSP_LED_Off(((ulLedCirclePos / 32768) + 4 - 1) & 0x03);
		BSP_LED_On(((ulLedCirclePos / 32768) + 4) & 0x03);

		// Auto-tune is finished
		if (siAutotune == 0)
		{


			// Beep for "Auto tune ends"
			//BEEP_Play(NOTE_A5,100,100);

			DISPLAY_Dark();
			// Use minimum values for offset of pitch and volume
			aOsc[PITCH].slOffset = aOsc[PITCH].slMinPeriode;
			aOsc[VOLUME].slOffset = aOsc[VOLUME].slMinPeriode;	// + 16384 * 128;
			aOsc[TIMBRE].slOffset = aOsc[TIMBRE].slMinPeriode;	// + 16384 * 128;

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
			eAutomuteAutoprehear = AUTOMUTE_MUTE;
		}
	}



	// pitch scale configuration
	if (aConfigWorkingSet[CFG_E_PITCH_SCALE].bHasChanged)
	{
		// from 2^-2.0 ... 2^2.0
		// from 0.25 .. 4
		// 2^((iCFG_PitchScale-500)/250)
		fPitchScale = powf(2,
				((float) (aConfigWorkingSet[CFG_E_PITCH_SCALE].iVal - 500))
						* 0.004f /* 1/250 */);

		// Request the calculation of a new pitch table
		bReqCalcPitchTable = 1;
		aConfigWorkingSet[CFG_E_PITCH_SCALE].bHasChanged = 0;
	}

	// pitch shift configuration
	if (aConfigWorkingSet[CFG_E_PITCH_SHIFT].bHasChanged)
	{
		// from 2^-4.0 ... 2^4.0
		// from 0.0625 .. 16
		// 2^((iCFG_PitchShift-500)/125)
		fPitchShift = powf(2,
				((float) (aConfigWorkingSet[CFG_E_PITCH_SHIFT].iVal - 500))
						* 0.008f /* 1/125 */);

		// Request the calculation of a new pitch table
		bReqCalcPitchTable = 1;
		aConfigWorkingSet[CFG_E_PITCH_SHIFT].bHasChanged = 0;
	}

	// Is it necessary to recalculate the pitch table?
	if (bReqCalcPitchTable)
	{
		THEREMIN_Calc_PitchTable();
	}

	if (siAutotune == 0)
	{
		// fPitchFrq * 48kHz / 2PI)
		DISPLAY_PitchDisplay(fWavStepFilt * 7639.4372f);
	}
	else
	{
		DISPLAY_AutotuneDisplay();
	}
}

