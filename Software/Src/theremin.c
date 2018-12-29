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


// Linearization tables for pitch an volume
float fPitchLinTable[2048+1];

uint16_t usNonLinTable[2048+1];

// Pitch
uint16_t usPitchCC;			// value of capture compare register
uint16_t usPitchLastCC; 	// last value (last task)
uint16_t usPitchPeriod;		// period of oscillator
int32_t slPitchOffset; 		// offset value (result of auto-tune)
int32_t slPitchPeriodeFilt;	// low pass filtered period
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
float fTimbre = 0.0f;

int16_t ssNonTimbreVol = 0;
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
int test3;
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
	taskTable[2] = THEREMIN_Task_Volume;
	taskTable[6] = THEREMIN_Task_Timbre;
	taskTable[8] = THEREMIN_Task_Volume_Nonlin;
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


	// Calculate the LUT for volume and pitch
	THEREMIN_Calc_PitchTable();
	THEREMIN_Calc_DistortionTable();

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
	float f,fmin50,fmax50,fmax, fscale;
	float fNonLin = ((float)aConfigWorkingSet[CFG_E_DISTORTION].iVal * 0.001f)*6.0f;

	if (aConfigWorkingSet[CFG_E_DISTORTION].iVal != 0)
	{
		fmax = expf(fNonLin);
		fscale = 1.0f/(fmax - 1.0f );

		// take the Vpp value at 50% volume and use it to scale the
		// sine value
		fmin50 = 0.5f*(fscale * (expf(-0.5f * fNonLin)-1.0f )+1.0f );
		fmax50 = 0.5f*(fscale * (expf(+0.5f * fNonLin)-1.0f )+1.0f );
		ssNonTimbreVol = 256 * (fmax50-fmin50);

		//fscale = 1.0f/(fmax-fmin);
		for (int32_t i = 0; i <= 2048; i++)
		{
			// Calculate the nonlinear function
			f = 0.5f*(fscale * (expf((float)(i-1024)*0.000976562f * fNonLin)-1.0f )+1.0f );
			// Fill the distortion LUT
			usNonLinTable[i] = f*65535.0f;
		}
	}
	else
	{
		ssNonTimbreVol = 128;
		for (int32_t i = 0; i < 2048; i++)
		{
			// Fill the distortion LUT
			usNonLinTable[i] = i*32;
		}
		usNonLinTable[2048] = 65535;
	}
}

/**
 * @brief 96kHz DAC task called in interrupt
 *
 * At 168MHz the absolute maximum cycle count would be 168MHz/96kHz = 1750
 */
inline void THEREMIN_96kHzDACTask_A(void)
{
	int32_t tabix;
	float p1f, p2f, tabsubf;
	int p1, p2, tabsub;
	floatint_ut u;
	//float fresult = 0.0f;
	int16_t ssResult;
	uint16_t usResult;

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
	fPitch = ((float) (slPitchPeriodeFilt - slPitchOffset));//*0.00000001f;



	if (fPitch >= 1.0f)
	{
		// cycles:
		// fast pow approximation by LUT with interpolation
		// float bias: 127, so 127 << 23bit mantissa is: 0x3F800000
		// We use (5 bit of) the exponent and 6 bit of the mantissa
		u.f = fPitch;
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

	//fPitchFrq = 0.1f;

	fOscSin += fPitchFrq * fOscCos;
	fOscCos -= fPitchFrq * fOscSin;
	fOscSin += fPitchFrq * fOscCos;
	fOscCos -= fPitchFrq * fOscSin;
	fOscCorr = 1.0f +((1.0f - (fOscSin * fOscSin + fOscCos * fOscCos))*0.01f);
	fOscCos *= fOscCorr;
	fOscSin *= fOscCorr;

	/*
	if (fOscSin > 1.0f)
	{
		fOscSin = 1.0f;
	}
	if (fOscSin < -1.0f)
	{
		fOscSin = -1.0f;
	}*/
	ssResult = fOscSin * 30.0f * (float)slVolFilt;
	//result = fabs(fOscSin) * (float)slVolFilt;
/*
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
*/

	tabix = (ssResult+32768) / 32;
	tabsub = ssResult & 0x001F;
	p1 = usNonLinTable[tabix];
	p2 = usNonLinTable[tabix + 1];
	usResult = (p1 + (((p2 - p1) * tabsub) / 32));

	ssDACValueR = (slTimbre * 128 * (usResult-32768) + (256-slTimbre) * ssResult  * ssNonTimbreVol) / (256*128) ;

	//result = (float)iWavOut;

	// Low pass filter the output to avoid aliasing noise.
	slVolFiltL += slVolume - slVolFilt;
	slVolFilt = slVolFiltL / 1024;


	// Limit the output to 16bit
	if (bMute)
	{
		ssDACValueR = 0;
	}


	// Output also to the speaker
	ssDACValueL = ssDACValueR;
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
	STOPWATCH_START();

	if (slVolTim1PeriodeFilt > 0)
	{
		slVolTim1MeanPeriode = slVolTim1PeriodeFilt / slVolTim1PeriodeFilt_cnt;
		slVol1 = ((aConfigWorkingSet[CFG_E_VOL1_NUMERATOR].iVal * slVolTim1PeriodeFilt_cnt) /
				(slVolTim1PeriodeFilt + slVolTim1PeriodeFilt_cnt * aConfigWorkingSet[CFG_E_VOL1_OFFSET_A].iVal))
				- aConfigWorkingSet[CFG_E_VOL1_OFFSET_B].iVal;

		// Prepare for next filter interval
		slVolTim1PeriodeFilt_cnt = 0;
		slVolTim1PeriodeFilt = 0;
	}
	STOPWATCH_STOP();

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
	if (slVolTim2PeriodeFilt > 0)
	{
		slVolTim2MeanPeriode = slVolTim2PeriodeFilt / slVolTim2PeriodeFilt_cnt;
		slVol2 = ((aConfigWorkingSet[CFG_E_VOL2_NUMERATOR].iVal * slVolTim2PeriodeFilt_cnt) /
				(slVolTim2PeriodeFilt + slVolTim2PeriodeFilt_cnt * aConfigWorkingSet[CFG_E_VOL2_OFFSET_A].iVal))
				- aConfigWorkingSet[CFG_E_VOL2_OFFSET_B].iVal;

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
 * Calculate the mean volume from VOL1 and VOL2
 */
void THEREMIN_Task_Volume(void)
{

	if ((slVolTim1MeanPeriode + slVolTim2MeanPeriode) > 0)
	{
		slVolumeRaw = ((aConfigWorkingSet[CFG_E_VOL12_NUMERATOR].iVal) /
				(slVolTim1MeanPeriode + slVolTim2MeanPeriode + aConfigWorkingSet[CFG_E_VOL12_OFFSET_A].iVal))
				- aConfigWorkingSet[CFG_E_VOL12_OFFSET_B].iVal;
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
	if (aConfigWorkingSet[CFG_E_LOUDER_DOWN].iVal == 1)
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
 * Calculate the timbre value from VOL1 and VOL2
 */
void THEREMIN_Task_Timbre(void)
{
	// Timbre is the difference between VOL1 and VOL2 in the range of 0..128
	slTimbre = (slVol1 - slVol2) + 128;

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
	fTimbre = (float)slTimbre * 0.00390625f;
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
	static int startup_cnt = 1;
	int bReqCalcPitchTable = 0;

	if (startup_cnt < 1000000)
	{
		startup_cnt++;
	}



	if (siAutotune == 0)
	{
		// Start auto-tune by pressing BUTTON_KEY
		if (BSP_PB_GetState(BUTTON_KEY) == GPIO_PIN_SET ||
				(startup_cnt == aConfigWorkingSet[CFG_E_STARTUP_AUTOTUNE].iVal)
			)
		{
			// Disable further auto-tune after startup
			startup_cnt = 1000000;
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
/*
	// volume scale pot
	if (POTS_HasChanged(POT_VOL_SCALE))
	{
		// from 2^0.25 ... 2^4.0
		// 2^((POT-2048)/1024)
		fVolScale = powf(2,
				((float) (POTS_GetScaledValue(POT_VOL_SCALE) - 2048))
						* 0.000976562f );// 1/1024
		THEREMIN_Calc_VolumeTable(VOLSEL_1);
		THEREMIN_Calc_VolumeTable(VOLSEL_2);
	}

	// waveform pot
	if (POTS_HasChanged(POT_WAVEFORM))
	{
		eWaveform = POTS_GetScaledValue(POT_WAVEFORM);
		THEREMIN_Calc_WavTable();
	}
*/
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




