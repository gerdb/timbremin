/**
 *  Project     timbremin
 *  @file		printf.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for printf.c
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
#ifndef __PRINTF_H
#define __PRINTF_H


/* Includes ------------------------------------------------------------------*/


#include "../Drivers/BSP/STM32F4-Discovery/stm32f4_discovery.h"
#include "stm32f4xx_hal.h"
#include "console.h"

/* Function prototypes -------------------------------------------------------*/
int my_printf(const char *format, ...);
int sprintf(char *out, const char *format, ...);

#endif /* __PRINTF_H */
