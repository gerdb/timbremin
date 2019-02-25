/**
 *  Project     timbremin
 *  @file		theremin.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for theremin.c
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

#ifndef THEREMIN_H_
#define THEREMIN_H_


/* Defines  ------------------------------------------------------------------ */
#define STOPWATCH_START() DWT->CYCCNT = 0;
#define STOPWATCH_STOP() ulStopwatch = DWT->CYCCNT;

#define PITCH	0
#define VOL1	1
#define VOL2	2

/* Types  ------------------------------------------------------------------ */
// Union to convert between float and int
typedef union
{
	float f;
	int i;
	uint32_t ui;
} floatint_ut;

// The different waveforms
typedef enum
{
	CALIB_OFF,
	CALIB_PITCH,
	CALIB_PITCH_FINISHED,
	CALIB_VOL1,
	CALIB_VOL1_FINISHED,
	CALIB_VOL2,
	CALIB_VOL2_FINISHED
}e_calibration;

// The different waveforms
typedef enum
{
	SINE = 0,
	CAT = 1,
	COSPULSE = 2,
	HARMON = 3,
	COMPRESSED = 4,
	GLOTTAL = 5,
	THEREMIN = 6,
	USBSTICK = 7
}e_waveform;

// The different volume antennas
typedef enum
{
	VOLSEL_NONE = 0,
	VOLSEL_1 = 1,
	VOLSEL_2 = 2
}e_vol_sel;

// Structure with oscillator calibration data
typedef struct
{
	uint16_t usCC;			// value of capture compare register
	uint16_t usLastCC; 	// last value (last task)
	uint16_t usPeriod;		// period of oscillator
	uint16_t usPeriodOffset;	// offset to reduce filter range to 32bit
	int32_t slOffset; 		// offset value (result of auto-tune)
	int32_t slPeriodeFilt;	// low pass filtered period
	int32_t slValue;			//  value
	float fValue;			//  value
	uint16_t usCalibThreshold1;
	uint16_t usCalibThreshold2;
	int iCalibN;
	int iCalibFact1;
	int iCalibFact2;
	int iCalibFact3;
	uint32_t ulCalibScale;
	float fCalibfScale;
	uint16_t usPeriodRaw;
	uint16_t usPeriodRawN;
	int slPeriodeFilt_cnt;	// low pass filtered period
	int32_t slMinPeriode;	// minimum value during auto-tune
	int32_t slMeanPeriode;	// low pass filtered period
}s_osc;

#define TESTPORT_ON()  GPIOD->BSRR = 0x00002000
#define TESTPORT_OFF() GPIOD->BSRR = 0x20000000

#define M_PI_f 3.14159265358979323846f

/* Global variables  ------------------------------------------------------- */
extern int16_t ssWaveTable[4 * 1024];
extern e_waveform eWaveform;
extern int32_t slThereminOut;	// Output sound from theremin into reverb

// global variables for debug output
extern s_osc aOsc[3];
extern uint32_t ulStopwatch;		// Stopwatch

/* Function prototypes ----------------------------------------------------- */
void THEREMIN_Init(void);
void THEREMIN_96kHzDACTask_A(void);
void THEREMIN_96kHzDACTask_B(void);
void THEREMIN_96kHzDACTask_Common(void);

void THEREMIN_Task_Select_VOL1(void);
void THEREMIN_Task_Select_VOL2(void);
void THEREMIN_Task_Activate_VOL1(void);
void THEREMIN_Task_Activate_VOL2(void);
void THEREMIN_Task_Capture_VOL1(void);
void THEREMIN_Task_Capture_VOL2(void);
void THEREMIN_Task_Calculate_VOL1(void);
void THEREMIN_Task_Calculate_VOL2(void);
void THEREMIN_Task_Volume(void);
void THEREMIN_Task_Timbre(void);
void THEREMIN_Task_Volume_Nonlin(void);

void THEREMIN_Calc_PitchTable(void);
void THEREMIN_Calc_DistortionTable(void);
void THEREMIN_Calc_ImpedanceTable(void);
void THEREMIN_1msTask(void);




#endif /* THEREMIN_H_ */
