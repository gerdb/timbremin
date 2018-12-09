/**
 *  Project     timbremin
 *  @file		display.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for display.c
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

#ifndef DISPLAY_H_
#define DISPLAY_H_

/* types ----------------------------------------------------- */
typedef enum
{
	NOTE_NONE,
	NOTE_C,
	NOTE_CIS,
	NOTE_D,
	NOTE_DIS,
	NOTE_E,
	NOTE_F,
	NOTE_FIS,
	NOTE_G,
	NOTE_GIS,
	NOTE_A,
	NOTE_AIS,
	NOTE_H
}e_note;


/* Function prototypes ----------------------------------------------------- */
void DISPLAY_AutotuneDisplay(void);
void DISPLAY_PitchDisplay(float fAudioFrequency);
void DISPLAY_Dark(void);

#endif /* DISPLAY_H_ */
