/**
 *  Project     timbremin
 *  @file		beep.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for beep.c
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


#ifndef BEEP_H_
#define BEEP_H_

#include "notes.h"

/* Defines  ------------------------------------------------------- */
#define ATTAC_VALUE 16384
#define ATTAC_SPEED 512
#define SUSTAIN_VALUE 16384
#define DECAY_SPEED 32
#define RELEASE_SPEED 16

/* Types  ------------------------------------------------------- */
typedef struct
{
	float frequency;
	int duration;
	int pause;
}BEEP_BeepType;

// The different waveforms
typedef enum
{
	ATTAC,
	DECAY,
	SUSTAIN,
	RELEASE,
	PAUSE
}e_ADSR;

/* Global variables  ------------------------------------------------------- */
extern int bBeepActive;


/* Function prototypes ----------------------------------------------------- */
void BEEP_Init(void);
void BEEP_Play(float frequency, int duration, int pause);
void BEEP_AutoTuneSound(int autotuneCounter);
void BEEP_Task(void);

#endif /* BEEP_H_ */
