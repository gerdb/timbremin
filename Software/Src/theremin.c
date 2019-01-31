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

uint16_t usDistortionTable[2048+1];
uint16_t usImpedanceTable[2048+1];

// Pitch
uint16_t usPitchCC;			// value of capture compare register
uint16_t usPitchLastCC; 	// last value (last task)
uint16_t usPitchPeriod;		// period of oscillator
int32_t slPitchOffset; 		// offset value (result of auto-tune)
int32_t slPitchPeriodeFilt;	// low pass filtered period
int32_t slPitch;			// pitch value
float fPitch;			// pitch value
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

int32_t	slOutCapacitor = 0;

int32_t slVolumeRaw;
int32_t slVolume;
int32_t slTimbre;
float fTimbre = 0.0f;

float fOscSin = 0.0f;
float fOscCos = 1.0f;
float fOscCorr = 1.0f;
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

float fVollAddSynth_2 = 0.0f;
float fVollAddSynth_3 = 0.0f;
float fVollAddSynth_4 = 0.0f;
float fVollAddSynth_5 = 0.0f;
float fVollAddSynth_l2 = 0.0f;
float fVollAddSynth_l3 = 0.0f;
float fVollAddSynth_l4 = 0.0f;
float fVollAddSynth_l5 = 0.0f;

/*
// State variable filters for formants
float fSVFf_1;
float fSVFq_1;
float fSVFf_2;
float fSVFq_2;
float fSVFf_3;
float fSVFq_3;
float fSVFf_4;
float fSVFq_4;
float fSVFf_5;
float fSVFq_5;
float fSVFf_6;
float fSVFq_6;
float fSVFZ1_1 = 0.0f;
float fSVFZ2_1 = 0.0f;
float fSVFZ1_2 = 0.0f;
float fSVFZ2_2 = 0.0f;
float fSVFZ1_3 = 0.0f;
float fSVFZ2_3 = 0.0f;
float fSVFZ1_4 = 0.0f;
float fSVFZ2_4 = 0.0f;
float fSVFZ1_5 = 0.0f;
float fSVFZ2_5 = 0.0f;
float fSVFZ1_6 = 0.0f;
float fSVFZ2_6 = 0.0f;
float fSVF_out_1 = 0.0f;
float fSVF_out_2 = 0.0f;
float fSVF_out_3 = 0.0f;
float fSVF_out_4 = 0.0f;
float fSVF_out_5 = 0.0f;
float fSVF_out_6 = 0.0f;
*/
float fVolScale = 1.0f;
float fVolShift = 0.0f;

int32_t slThereminOut = 0;

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
int32_t test3;
int task = 0;

/*
float fBQ_out_1 = 0.0f;
float fBQZ1_1 = 0.0f;
float fBQZ2_1 = 0.0f;
float fBQ_out_2 = 0.0f;
float fBQZ1_2 = 0.0f;
float fBQZ2_2 = 0.0f;
float fBQ_out_3 = 0.0f;
float fBQZ1_3 = 0.0f;
float fBQZ2_3 = 0.0f;
float fBQ_out_4 = 0.0f;
float fBQZ1_4 = 0.0f;
float fBQZ2_4 = 0.0f;
float fBQ_out_5 = 0.0f;
float fBQZ1_5 = 0.0f;
float fBQZ2_5 = 0.0f;
*/







// Biquad coefficients for 48kHz sampling frequency, f0= 10kHz and 10th order LP
// see http://www.earlevel.com/main/2016/09/29/cascading-filters/
// and http://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/
// Q-Values: 0.50623256, 0.56116312, 0.70710678, 1.1013446, 3.1962266
/*
//Q: 0.50623256
#define BQ_A0_1 0.1896540888062538f
#define BQ_A1_1 0.3793081776125076f
#define BQ_A2_1 0.1896540888062538f
#define BQ_B1_1 -0.26490745527271625f
#define BQ_B2_1 0.023523810497731508f
//Q: 0.56116312
#define BQ_A0_2 0.19917299441439854f
#define BQ_A1_2 0.39834598882879707f
#define BQ_A2_2 0.19917299441439854f
#define BQ_B1_2 -0.27820339356493434f
#define BQ_B2_2 0.07489537122252837f
//Q: 0.70710678
#define BQ_A0_3 0.22019470012300807f
#define BQ_A1_3 0.44038940024601614f
#define BQ_A2_3 0.22019470012300807f
#define BQ_B1_3 -0.3075663595827598f
#define BQ_B2_3 0.18834516007479202f
//Q: 1.1013446
#define BQ_A0_4 0.2576190655938374f
#define BQ_A1_4 0.5152381311876748f
#define BQ_A2_4 0.2576190655938374f
#define BQ_B1_4 -0.35984044175243773f
#define BQ_B2_4 0.39031670412778746f
//Q: 3.1962266
#define BQ_A0_5 0.32194349801676825f
#define BQ_A1_5 0.6438869960335365f
#define BQ_A2_5 0.32194349801676825f
#define BQ_B1_5 -0.4496883422763653f
#define BQ_B2_5 0.7374623343434382f
*/
/*
//Q: 0.50623256
#define BQ_A0_1 0.06452600200988579f
#define BQ_A1_1 0.12905200401977157f
#define BQ_A2_1 0.06452600200988579f
#define BQ_B1_1 -0.9909072675518413f
#define BQ_B2_1 0.24901127559138433f
//Q: 0.56116312
#define BQ_A0_2 0.06698822155330401f
#define BQ_A1_2 0.13397644310660803f
#define BQ_A2_2 0.06698822155330401f
#define BQ_B1_2 -1.0287188654175745f
#define BQ_B2_2 0.2966717516307906f
//Q: 0.70710678
#define BQ_A0_3 0.07223087528927949f
#define BQ_A1_3 0.14446175057855898f
#define BQ_A2_3 0.07223087528927949f
#define BQ_B1_3 -1.109228792058311f
#define BQ_B2_3 0.39815229321542905f
//Q: 1.1013446
#define BQ_A0_4 0.0809508018494106f
#define BQ_A1_4 0.1619016036988212f
#define BQ_A2_4 0.0809508018494106f
#define BQ_B1_4 -1.2431381980622418f
#define BQ_B2_4 0.5669414054598841f
//Q: 3.1962266
#define BQ_A0_5 0.09433928047651055f
#define BQ_A1_5 0.1886785609530211f
#define BQ_A2_5 0.09433928047651055f
#define BQ_B1_5 -1.4487412163776068f
#define BQ_B2_5 0.8260983382836492f
*/
int taskTableCnt = 0;
void (*taskTable[96]) () = {0};

extern TIM_HandleTypeDef htim1;	// Handle of timer for input capture

uint32_t ulStopwatch = 0;
#define STOPWATCH_START() DWT->CYCCNT = 0;
#define STOPWATCH_STOP() ulStopwatch = DWT->CYCCNT;

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


	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	// Read auto-tune values from virtual EEPRom
	slPitchOffset = CONFIG_Read_SLong(EEPROM_ADDR_PITCH_AUTOTUNE_H);
	slVolTim1Offset = CONFIG_Read_SLong(EEPROM_ADDR_VOLTIM1_AUTOTUNE_H);
	slVolTim2Offset = CONFIG_Read_SLong(EEPROM_ADDR_VOLTIM2_AUTOTUNE_H);

	// Get the VolumeShift value from the flash configuration
	fVolShift = ((float) (CONFIG.VolumeShift)) * 0.1f + 11.5f;


	// Calculate the LUT for volume and pitch
	THEREMIN_Calc_PitchTable();
	THEREMIN_Calc_DistortionTable();
	THEREMIN_Calc_ImpedanceTable();

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
/*
void THEREMIN_InitStateVariableFilters(void)
{
	fSVFf_1 = 2.0f * sinf(M_PI_f * 360.0f / 48000.0f);
	fSVFq_1 = 49.0f/ 360.0f;
	fSVFf_2 = 2.0f * sinf(M_PI_f * 1700.0f / 48000.0f);
	fSVFq_2 = 38.0f / 1700.0f;;
	fSVFf_3 = 2.0f * sinf(M_PI_f * 2313.0f / 48000.0f);
	fSVFq_3 = 59.0f / 2313.0f;
	fSVFf_4 = 2.0f * sinf(M_PI_f * 2827.0f / 48000.0f);
	fSVFq_4 = 74.0f/ 2827.0f;
	fSVFf_5 = 2.0f * sinf(M_PI_f * 3751.0f / 48000.0f);
	fSVFq_5 = 135.0f / 3751.0f;

	fSVFf_6 = 2.0f * sinf(M_PI_f * 2000.0f / 48000.0f);
	fSVFq_6 = 1.0f;
}
*/

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
 * @brief Recalculates the distortion LUT
 *
 */
/*
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
		//ssNonTimbreVol = 256 * (fmax50-fmin50);

		//fscale = 1.0f/(fmax-fmin);
		for (int32_t i = 0; i <= 2048; i++)
		{
			// Calculate the nonlinear function
			f = 0.5f*(fscale * (expf((float)(i-1024)*0.000976562f * fNonLin)-1.0f )+1.0f );
			// Fill the distortion LUT
			usDistortionTable[i] = f*65535.0f;
		}
	}
	else
	{
		//ssNonTimbreVol = 128;
		for (int32_t i = 0; i < 2048; i++)
		{
			// Fill the distortion LUT
			usDistortionTable[i] = i*32;
		}
		usDistortionTable[2048] = 65535;
}
}
*/

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
	usPitchCC = htim1.Instance->CCR1;
	usPitchPeriod = usPitchCC - usPitchLastCC;
	usPitchLastCC = usPitchCC;


	// cycles: 29
	// Low pass filter period values
	// factor *1024 is necessary, because of the /1024 integer division
	// factor *1024 is necessary, because we want to over sample the input signal
	if (usPitchPeriod != 0)
	{
		//                                   11bit                10bit  10bit
		slPitchPeriodeFilt += ((int16_t) (usPitchPeriod - 2048) * 1024 * 1024
				- slPitchPeriodeFilt) / 256;
	}

	// cycles:21
	fPitch = ((float) (slPitchPeriodeFilt - slPitchOffset));//*0.00000001f;


	if (bBeepActive)
	{
		BEEP_Task();
		return;
	}




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
	/*
	fPitchFrq1 = fPitchFrq * 0.5f;

	fPitchFrq3 = fPitchFrq * 3.0f;
	fPitchFrq4 = fPitchFrq * 4.0f;
	fPitchFrq5 = fPitchFrq * 5.0f;
	//fPitchFrq = 0.1f;
*/
	fPitchFrq2 = fPitchFrq * 2.0f;

	fOscSin += fPitchFrq * fOscCos;
	fOscCos -= fPitchFrq * fOscSin;
	fOscSin += fPitchFrq * fOscCos;
	fOscCos -= fPitchFrq * fOscSin;
	fOscCorr = 1.0f +((1.0f - (fOscSin * fOscSin + fOscCos * fOscCos))*0.01f);
	fOscCos *= fOscCorr;
	fOscSin *= fOscCorr;


	fOscSin2 += fPitchFrq * fOscCos2;
	fOscCos2 -= fPitchFrq * fOscSin2;
	fOscSin2 += fPitchFrq * fOscCos2;
	fOscCos2 -= fPitchFrq * fOscSin2;
	fOscSin2 += fPitchFrq * fOscCos2;
	fOscCos2 -= fPitchFrq * fOscSin2;
	fOscSin2 += fPitchFrq * fOscCos2;
	fOscCos2 -= fPitchFrq * fOscSin2;
	fOscCorr2 = 1.0f +((1.0f - (fOscSin2 * fOscSin2 + fOscCos2 * fOscCos2))*0.01f);
	fOscCos2 *= fOscCorr2;
	fOscSin2 *= fOscCorr2;

	fOscSin3 += fPitchFrq * fOscCos3;
	fOscCos3 -= fPitchFrq * fOscSin3;
	fOscSin3 += fPitchFrq * fOscCos3;
	fOscCos3 -= fPitchFrq * fOscSin3;
	fOscSin3 += fPitchFrq * fOscCos3;
	fOscCos3 -= fPitchFrq * fOscSin3;
	fOscSin3 += fPitchFrq * fOscCos3;
	fOscCos3 -= fPitchFrq * fOscSin3;
	fOscSin3 += fPitchFrq * fOscCos3;
	fOscCos3 -= fPitchFrq * fOscSin3;
	fOscSin3 += fPitchFrq * fOscCos3;
	fOscCos3 -= fPitchFrq * fOscSin3;
	fOscCorr3 = 1.0f +((1.0f - (fOscSin3 * fOscSin3 + fOscCos3 * fOscCos3))*0.01f);
	fOscCos3 *= fOscCorr3;
	fOscSin3 *= fOscCorr3;


	fOscSin4 += fPitchFrq * fOscCos4;
	fOscCos4 -= fPitchFrq * fOscSin4;
	fOscSin4 += fPitchFrq * fOscCos4;
	fOscCos4 -= fPitchFrq * fOscSin4;
	fOscSin4 += fPitchFrq * fOscCos4;
	fOscCos4 -= fPitchFrq * fOscSin4;
	fOscSin4 += fPitchFrq * fOscCos4;
	fOscCos4 -= fPitchFrq * fOscSin4;
	fOscSin4 += fPitchFrq * fOscCos4;
	fOscCos4 -= fPitchFrq * fOscSin4;
	fOscSin4 += fPitchFrq * fOscCos4;
	fOscCos4 -= fPitchFrq * fOscSin4;
	fOscSin4 += fPitchFrq * fOscCos4;
	fOscCos4 -= fPitchFrq * fOscSin4;
	fOscSin4 += fPitchFrq * fOscCos4;
	fOscCos4 -= fPitchFrq * fOscSin4;
	fOscCorr4 = 1.0f +((1.0f - (fOscSin4 * fOscSin4 + fOscCos4 * fOscCos4))*0.01f);
	fOscCos4 *= fOscCorr4;
	fOscSin4 *= fOscCorr4;

	fOscSin5 += fPitchFrq * fOscCos5;
	fOscCos5 -= fPitchFrq * fOscSin5;
	fOscSin5 += fPitchFrq * fOscCos5;
	fOscCos5 -= fPitchFrq * fOscSin5;
	fOscSin5 += fPitchFrq * fOscCos5;
	fOscCos5 -= fPitchFrq * fOscSin5;
	fOscSin5 += fPitchFrq * fOscCos5;
	fOscCos5 -= fPitchFrq * fOscSin5;
	fOscSin5 += fPitchFrq * fOscCos5;
	fOscCos5 -= fPitchFrq * fOscSin5;
	fOscSin5 += fPitchFrq * fOscCos5;
	fOscCos5 -= fPitchFrq * fOscSin5;
	fOscSin5 += fPitchFrq * fOscCos5;
	fOscCos5 -= fPitchFrq * fOscSin5;
	fOscSin5 += fPitchFrq * fOscCos5;
	fOscCos5 -= fPitchFrq * fOscSin5;
	fOscSin5 += fPitchFrq * fOscCos5;
	fOscCos5 -= fPitchFrq * fOscSin5;
	fOscSin5 += fPitchFrq * fOscCos5;
	fOscCos5 -= fPitchFrq * fOscSin5;
	fOscCorr5 = 1.0f +((1.0f - (fOscSin5 * fOscSin5 + fOscCos5 * fOscCos5))*0.01f);
	fOscCos5 *= fOscCorr5;
	fOscSin5 *= fOscCorr5;

	/*
	if (fOscSin > 1.0f)
	{
		fOscSin = 1.0f;
	}
	if (fOscSin < -1.0f)
	{
		fOscSin = -1.0f;
	}*/
/*
	fOscSin5 = (fOscSin + 0.5f) *4.0f;
	if (fOscSin5 > 0.6f)
	{
		fOscSin5 = 0.6f;
	}
	if (fOscSin5 < -0.6f)
	{
		fOscSin5 = -0.6f;
	}
*/
	//ssResult = (fOscSin  * 30.0f /*+ fOscSin2 * 10.0f - fOscSin3 * 5.0f*/) * (float)slVolFilt;
/*
	slResult = ((fOscSin + 0.8f) * 8.0* 30.0f) * (float)slVolFilt;
	if (slResult>20000)
	{
		ssResult = 20000;
	}
	else if (slResult<-20000)
	{
		ssResult = -20000;
	}
	else
	{
		ssResult = slResult;

	}
*/
	/*
	ssResult = (fOscSin * 30000.0f);
	if (ssResult > (slVolFilt * 32) )
	{
		ssResult = (slVolFilt * 32) ;
	}
	if (ssResult < ( - slVolFilt * 32) )
	{
		ssResult = ( - slVolFilt * 32) ;
	}
*/
	f = (float)slVolFilt;
	f = f * f * 0.000976562f;
	fVollAddSynth_l2 = fVollAddSynth_2 * f;
	fVollAddSynth_l3 = fVollAddSynth_3 * f;
	fVollAddSynth_l4 = fVollAddSynth_4 * f;
	fVollAddSynth_l5 = fVollAddSynth_5 * f;
	/*
	fVollAddSynth_l2 = fVollAddSynth_2 * (f-300.0f);
	fVollAddSynth_l3 = fVollAddSynth_3 * (f-300.0f);
	fVollAddSynth_l4 = fVollAddSynth_4 * (f-300.0f);
	fVollAddSynth_l5 = fVollAddSynth_5 * (f-300.0f);
	if (fVollAddSynth_l2<0.0f ) fVollAddSynth_l2 = 0.0f;
	if (fVollAddSynth_l3<0.0f ) fVollAddSynth_l3 = 0.0f;
	if (fVollAddSynth_l4<0.0f ) fVollAddSynth_l4 = 0.0f;
	if (fVollAddSynth_l5<0.0f ) fVollAddSynth_l5 = 0.0f;*/

/*
	if (ssResult < 0)
	{
		ssResult *= -1;
	}*/

	ssResult =
			 - fOscSin2 * fVollAddSynth_l2 * 12.0f +
			fOscSin3 * fVollAddSynth_l3 * 20.0f +
			fOscSin4 * fVollAddSynth_l4 * 20.0f +
			fOscSin5 * fVollAddSynth_l5 * 20.0f +

			fOscSin * 20.0f * (float)slVolFilt;

	/*if (ssResult < 0)
	{
		ssResult *= -1;
	}*/

/*
	fFiltLP1 += (fResult1 - fFiltLP1) * 0.1f; // fPitchFrq1 * 2.0f;
	fFiltLP2 += (fFiltLP1 - fFiltLP2) * 0.1f; // fPitchFrq1 * 2.0f;
	ssResult = fFiltLP2;
*/
	//ssResult += (fOscSin2 * 15.0f - fOscSin3 * 15.0f) * (float)slVolFilt;
	//ssResult2 = 0;

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
	p1 = usDistortionTable[tabix];
	p2 = usDistortionTable[tabix + 1];
	usDistorted = (p1 + (((p2 - p1) * tabsub) / 32));

	//i1 = (slTimbre * (usDistorted-32768) + (256-slTimbre) * ssResult ) / 256 ;
	//i1 -=( ssResult * 3 )/ 4;

	//i1 = (slTimbre * ssResult2 + 256 * ssResult ) / 256 ;

	p1 = usImpedanceTable[tabix];
	p2 = usImpedanceTable[tabix + 1];
	usImpedance = (p1 + (((p2 - p1) * tabsub) / 32));
	slOutCapacitor += ((ssResult-slOutCapacitor) * usImpedance) / 65536;

	//fFilterIn = slOutCapacitor;
//	fFilterIn = (slTimbre * (usDistorted-32768) + (256-slTimbre) * ssResult ) / 256 ;
	slThereminOut = (slTimbre * slOutCapacitor + (256-slTimbre) * ssResult ) / 256 ;


	//result = (float)iWavOut;







	//STOPWATCH_STOP();

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
	float f;
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

	f = ((float)aConfigWorkingSet[CFG_E_ADDSYNTH_2].iVal * 0.001f);
	fVollAddSynth_2 = f*f;
	f = ((float)aConfigWorkingSet[CFG_E_ADDSYNTH_3].iVal * 0.001f);
	fVollAddSynth_3 = f*f;
	f = ((float)aConfigWorkingSet[CFG_E_ADDSYNTH_4].iVal * 0.001f);
	fVollAddSynth_4 = f*f;
	f = ((float)aConfigWorkingSet[CFG_E_ADDSYNTH_5].iVal * 0.001f);
	fVollAddSynth_5 = f*f;

					;
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
	else// if (bBeepActive == 0)
	{
		// Wait 200ms to start with the detection
		// until both volume channels are stabilized
		if (siAutotune < (1000-200))
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
			slPitchOffset = slMinPitchPeriode;
			slVolTim1Offset = slMinVol1Periode;	// + 16384 * 128;
			slVolTim2Offset = slMinVol2Periode;	// + 16384 * 128;

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
		printf("%d %d %d %d\n",
				(int)(fPitchFrq*1000.0f),
				(int)usPitchPeriod,
				(int)slPitchPeriodeFilt , (int)slPitchOffset
				);
*/
		/*
		printf("%d %d %d %d %d\n",
				(int)usVolTim1Period,
				(int)usVolTim2Period,
				(int)slVolTim1MeanPeriode,
				(int)slVolTim2MeanPeriode,
				(int)usPitchPeriod
				);
		*/
		//printf("Stopwatch %d\n", ulStopwatch);
	}
}




