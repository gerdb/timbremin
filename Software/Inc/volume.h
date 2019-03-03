/**
 *  Project     timbremin
 *  @file		volume.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for volume.c
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

#ifndef VOLUME_H_
#define VOLUME_H_

/* Types  ------------------------------------------------------- */
typedef struct
{
	int32_t value;
	int cm;
}VOLUME_VolCalibrationType;


// for auto-mute and auto-prehear
typedef enum
{
	AUTOMUTE_MUTE,
	AUTOMUTE_READY,
	AUTOMUTE_LOUD,
	AUTOPREHEAR,
	AUTOPREHEAR_LOUD
}e_automute_autoprehear;


/* Global variables  ------------------------------------------------------- */
extern e_automute_autoprehear eAutomuteAutoprehear;
/* Function prototypes ----------------------------------------------------- */
void VOLUME_Init(void);
void VOLUME_1msTask(void);
void VOLUME_CalibrationStart(void);
void VOLUME_CalibrationTask(void);
void VOLUME_AutoMute_AutoPrehear(void);

#endif /* VOLUME_H_ */
