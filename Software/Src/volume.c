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

/* global variables  ------------------------------------------------------- */
VOLUME_VolCalibrationType aCalibrationEntries[20+1]; // Array containing calibration values for each cm

/* local variables  ------------------------------------------------------- */
int iVolCal_delay;	// Delay timer between each cm steps
int iVolVal_step;		// cm step
int iVolCal_active = 0;  // Flag if calibration is active

/**
 * @brief Initialize the module
 *
 */
void VOLUME_Init(void)
{

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
			aCalibrationEntries[iVolVal_step].vol1 = aOsc[VOL1].slMeanPeriode;
			aCalibrationEntries[iVolVal_step].vol2 = aOsc[VOL2].slMeanPeriode;
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

