/**
 *  Project     timbremin
 *  @file		volume.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		controls the volume antenna
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
#include "beep.h"
#include "theremin.h"
#include "audio_out.h"
#include "volume.h"
#include "usb_stick.h"
#include "config.h"

/* global variables  ------------------------------------------------------- */
VOLUME_VolCalibrationType aCalibrationEntries[20+1]; // Array containing calibration values for each cm

/* local variables  ------------------------------------------------------- */
int iVolCal_delay;	// Delay timer between each cm steps
int iVolVal_step;		// cm step
int iVolCal_active = 0;  // Flag if calibration is active
e_automute_autoprehear eAutomuteAutoprehear = AUTOMUTE_MUTE;

int32_t iVolNumerator = 1;
int32_t iVolLinFactor = 1;
int32_t iVolOffset = 0;

int iLast_VOLUME_LOWER = -1;
int iLast_VOLUME_RANGE = -1;
int iLast_VOLUME_LINEAR = -1;

/* local function ------------------------------------------------------- */
static void VOLUME_AutoMute_AutoPrehear(void);

/**
 * @brief Initialize the module
 *
 */
void VOLUME_Init(void)
{

}


/**
 * @brief Calculate the constants for volume linearization
 *
 */
static void VOLUME_CalcLinearizationConstants(void)
{
	// Has one value changed? If no, do not calculate the values
	if (		iLast_VOLUME_LOWER  != aConfigWorkingSet[CFG_E_VOLUME_LOWER].iVal
			||	iLast_VOLUME_RANGE  != aConfigWorkingSet[CFG_E_VOLUME_RANGE].iVal
			||	iLast_VOLUME_LINEAR != aConfigWorkingSet[CFG_E_VOLUME_LINEAR].iVal
		)
	{
		// The +1 prevents from division by 0 later in theremin.c where these variables are used
		iVolLinFactor =  8*aConfigWorkingSet[CFG_E_VOLUME_LINEAR].iVal + 1;

		// The +1 prevents from division by 0
		iVolNumerator = 1024.0f * (200000.0f/(float)(aConfigWorkingSet[CFG_E_VOLUME_RANGE].iVal+1)
				+ iVolLinFactor);

		iVolOffset = iVolNumerator / (2000000 / aConfigWorkingSet[CFG_E_VOLUME_LOWER].iVal + iVolLinFactor);


		iLast_VOLUME_LOWER = aConfigWorkingSet[CFG_E_VOLUME_LOWER].iVal;
		iLast_VOLUME_RANGE = aConfigWorkingSet[CFG_E_VOLUME_RANGE].iVal;
		iLast_VOLUME_LINEAR = aConfigWorkingSet[CFG_E_VOLUME_LINEAR].iVal;
	}

}

/**
 * @brief Auto-mute and auto-prehear functionality
 *
 */
static void VOLUME_AutoMute_AutoPrehear(void)
{
	if (aConfigWorkingSet[CFG_E_AUTOMUTE].iVal)
	{
		if (eAutomuteAutoprehear == AUTOMUTE_MUTE)
		{
			// Activate it at zero volume and very low pitch frequency
			// 0.005f / 2*PI * 48kHz = 38Hz
			if (fPitchFrq > 0.005f  && slVolumeRaw == 0)
			{
				eAutomuteAutoprehear = AUTOMUTE_READY;
			}
		}
		else if (eAutomuteAutoprehear == AUTOMUTE_READY)
		{
			if (fPitchFrq > 0.49f && slVolumeRaw == 0 && aConfigWorkingSet[CFG_E_AUTOPREHEAR].iVal)
			{
				eAutomuteAutoprehear = AUTOPREHEAR;
			}
			if (fPitchFrq > 0.005f && slVolumeRaw > 0)
			{
				eAutomuteAutoprehear = AUTOMUTE_LOUD;
			}
		}
		else if (eAutomuteAutoprehear == AUTOMUTE_LOUD)
		{
			if (fPitchFrq < 0.005f && slVolumeRaw == 0)
			{
				eAutomuteAutoprehear = AUTOMUTE_MUTE;
			}
		}
		else if (eAutomuteAutoprehear == AUTOPREHEAR)
		{
			if (fPitchFrq > 0.005f && slVolumeRaw > 100)
			{
				eAutomuteAutoprehear = AUTOPREHEAR_LOUD;
			}
		}
		else if (eAutomuteAutoprehear == AUTOPREHEAR_LOUD)
		{
			if (fPitchFrq > 0.005f && slVolumeRaw == 0)
			{
				eAutomuteAutoprehear = AUTOMUTE_LOUD;
			}
		}
	}
	else
	{
		eAutomuteAutoprehear = AUTOMUTE_LOUD;
	}
}

/**
 * @brief 1ms Task
 *
 */
void VOLUME_1msTask(void)
{
	// Process auto-mute and auto-prehear functionality
	VOLUME_AutoMute_AutoPrehear();

	if (iVolCal_active)
	{
		VOLUME_CalibrationTask();
	}

	VOLUME_CalcLinearizationConstants();

}


/**
 * @brief Starts a new calibration sequence
 *
 */
void VOLUME_CalibrationStart(void)
{
	iVolVal_step = 20;
	iVolCal_delay = 8000; // begin with 8 seconds
	iVolCal_active = 1;
}

/**
 * @brief Calibrate the volume antenna
 * 		  Call it every 1ms
 *
 */
void VOLUME_CalibrationTask(void)
{
	iVolCal_delay--;

	if ((iVolCal_delay == 7000) && (iVolVal_step == 20))
	{
		// Beep for "start calibration"
		BEEP_Play(NOTE_D7,50,50);
		BEEP_Play(NOTE_D7,50,50);
	}

	if (iVolCal_delay <= 0)
	{
		iVolCal_delay = 4000; // time between 2 steps: 4sec
		if (iVolVal_step >= 0)
		{
			aCalibrationEntries[iVolVal_step].cm = iVolVal_step;
			aCalibrationEntries[iVolVal_step].value = aOsc[VOLUME].slMeanPeriode;
		}
		iVolVal_step--;

		if (iVolVal_step >= 0)
		{
			// Beep for "next calibration step"
			BEEP_Play(NOTE_D5,100,100);
			BEEP_Play(NOTE_C5,100,100);
		}

		// Calibration finished
		if (iVolVal_step == -1)
		{
			// Beep for "calibration ends"
			BEEP_Play(NOTE_A5,500,100);
			iVolCal_delay = 1000; // Wait until "beep" is played
		}

		// Save result and end calibration
		if (iVolVal_step == -2)
		{
			USB_STICK_WriteVolCalFile("CALVOL.CSV", aCalibrationEntries);
			iVolCal_active = 0;
			// reactivate output
			bMute = 0;
		}

	}
}

