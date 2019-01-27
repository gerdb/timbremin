/**
 *  Project     timbremin
 *  @file		beep.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Plays feedback beeps
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
#include "beep.h"
#include "beepwave.h"

/* Global variables  ------------------------------------------------------- */
int bBeepActive = 0;

/* local variables  ------------------------------------------------------- */
int iBeepReadCnt = 0;	// Stack read counter
int iBeepWriteCnt = 0; // Stack write counter
BEEP_BeepType aBeeps[256]; // Stack containing notes to play
BEEP_BeepType* thisBeep; // Pointer to the current note which is been playing
e_ADSR eADSR = ATTAC;	// ADSR state

int iADSRCnt = 0; // AD_R counter
int iTimeCnt = 0; // Sustain counter
uint32_t iBeepTimeCnt = 0; // Counter for beep wave table
int iAutoTuneCnt = 0;

/**
 * @brief Initialize the module
 *
 */
void BEEP_Init(void)
{
	thisBeep = &aBeeps[0];
}

/**
 * @brief Add a note to the list of notes to play
 *
 * @frequency The frequency to play
 * @duration Of the note in ms
 * @pause after the note in ms
 *
 */
void BEEP_Play(float frequency, int duration, int pause)
{
	bBeepActive = 1;
	aBeeps[iBeepWriteCnt].frequency = frequency;
	aBeeps[iBeepWriteCnt].duration = duration * 48;
	aBeeps[iBeepWriteCnt].pause = pause * 48;
	iBeepWriteCnt ++;
	iBeepWriteCnt &= 0xFF;
}

/**
 * @brief plays a sound during auto tune procedure
 *
 * @autotuneCounter counter during auto tune from 999 down to 0
 *
 */
void BEEP_AutoTuneSound(int autotuneCounter)
{
	iAutoTuneCnt = autotuneCounter;
	bBeepActive = iAutoTuneCnt != 0;
}

/**
 * @brief Play it
 *
 *
 */
void BEEP_Task(void)
{
	int beepVol;
	if (!bBeepActive)
		return;

	if (iAutoTuneCnt > 0 )
	{
		//iBeepTimeCnt += thisBeep->frequency*1024.0f / 48000.0f * 1024.0f;
		iBeepTimeCnt += ((1400 - iAutoTuneCnt)*(1400 - iAutoTuneCnt)) / 32;

		beepVol = iAutoTuneCnt & 0x003F;
		if (beepVol > 32)
		{
			beepVol = 0;
		}
		else
		{
			beepVol *=256;
		}
		// Apply the beep only to the ear phone
		ssDACValueR = (int16_t)((usBeepWave[(iBeepTimeCnt/1024) & 0x000003FF] * beepVol) / 16384);

	}
	else
	{
		switch (eADSR)
		{
		case ATTAC:
			iADSRCnt += ATTAC_SPEED;
			if (iADSRCnt >= ATTAC_VALUE )
			{
				iADSRCnt = ATTAC_VALUE;
				eADSR = DECAY;
			}
			break;
		case DECAY:
			iADSRCnt -= DECAY_SPEED;
			if (iADSRCnt <= SUSTAIN_VALUE )
			{
				iADSRCnt = SUSTAIN_VALUE;
				eADSR = SUSTAIN;
				iTimeCnt = 0;
			}
			break;
		case SUSTAIN:
			iTimeCnt ++;
			if (iTimeCnt >= thisBeep->duration )
			{
				eADSR = RELEASE;
			}
			break;
		case RELEASE:
			iADSRCnt -= RELEASE_SPEED;
			if (iADSRCnt <= 0 )
			{
				iADSRCnt = 0;
				iTimeCnt = 0;
				eADSR = PAUSE;
			}
			break;
		case PAUSE:
			iTimeCnt ++;
			if (iTimeCnt >= thisBeep->pause )
			{
				eADSR = ATTAC;

				// Get the next beep note
				iBeepReadCnt ++;
				iBeepReadCnt &= 0xFF;
				thisBeep = &aBeeps[iBeepReadCnt];

				// Are there any more note? or was it the last one?
				if (iBeepReadCnt == iBeepWriteCnt)
				{
					bBeepActive = 0;
				}

			}
			break;
		}

		//iBeepTimeCnt += thisBeep->frequency*1024.0f / 48000.0f * 1024.0f;
		iBeepTimeCnt += (uint32_t)(thisBeep->frequency*21.845333333f
				//Modulate the frequency so the beep sounds a little bit "liquid"
				* ((32767.0f- (float)(iADSRCnt)) * 0.000030518f)
			);

		// Apply the beep only to the ear phone
		ssDACValueR = (int16_t)((usBeepWave[(iBeepTimeCnt/1024) & 0x000003FF] * iADSRCnt) / 16384/2);
	}

}

