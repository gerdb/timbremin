/**
 *  Project     timbremin
 *  @file		effect.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for effect.c
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

#ifndef EFFECT_H_
#define EFFECT_H_


/* Global variables  ------------------------------------------------------- */
extern RNG_HandleTypeDef hrng;


/* Function prototypes ----------------------------------------------------- */
void EFFECT_Init(void);
void EFFECT_Task(void);

#endif /* EFFECT_H_ */
