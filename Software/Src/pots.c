/**
 *  Project     timbremin
 *  @file		pots.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Process the analog potentiometer values
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

#include "stm32f4xx_hal.h"
#include "pots.h"
#include <stdlib.h>

extern ADC_HandleTypeDef hadc1;

uint16_t usADCResult[ADC_CHANNELS];
int iPotTask = 0;
POTS_PotTypeDef strPots[ADC_CHANNELS];

/**
 * @brief initialize the module
 */
void POTS_Init(void)
{
	__HAL_ADC_ENABLE(&hadc1);
	/* Set ADC state                                                          */
	/* - Clear state bitfield related to regular group conversion results     */
	/* - Set state bitfield related to regular group operation                */
	ADC_STATE_CLR_SET(hadc1.State,
			HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR,
			HAL_ADC_STATE_REG_BUSY);
	/* Reset ADC all error code fields */
	ADC_CLEAR_ERRORCODE(&hadc1);

	/* Clear regular group conversion flag and overrun flag */
	/* (To ensure of no unknown state from potential previous ADC operations) */
	__HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_EOC | ADC_FLAG_OVR);

	/* Enable ADC overrun interrupt */
	__HAL_ADC_ENABLE_IT(&hadc1, ADC_IT_OVR);

	/* Enable ADC DMA mode */
	hadc1.Instance->CR2 |= ADC_CR2_DMA;

	/* Start the DMA channel */
	HAL_DMA_Start(hadc1.DMA_Handle, (uint32_t) &hadc1.Instance->DR,
			(uint32_t)((uint32_t*)usADCResult), ADC_CHANNELS);

    __HAL_UNLOCK(hadc1.DMA_Handle);
	hadc1.DMA_Handle->State = HAL_DMA_STATE_READY;

	// Initialize the pots structure
	for (int i=0; i< (ADC_CHANNELS); i++)
	{
		strPots[i].usRawVal = 0;
		strPots[i].usStabilized = 0;
		strPots[i].iStabilizeCnt = 0;
		strPots[i].bChanged = 1;
		strPots[i].iScaledValue = 0;
		strPots[i].iScaledValueOld = 0;
		strPots[i].eConfigEntry = CFG_E_NONE;
	}

	// Start with -1, so in the next task, the first value will be 0
	iPotTask = -1;

	CONFIG_Assign_All_Pots();
}

static void POTS_Scale(int i)
{
	// Scale the pot value
	int iVal = (strPots[i].usStabilized * 1100) / 4096 - 50;
	if (iVal<0)
	{
		iVal = 0;
	}
	if (iVal>1000)
	{
		iVal = 1000;
	}

	strPots[i].iScaledValue = iVal;
}

/**
 * @brief 1ms Task
 */
void POTS_1msTask(void)
{
	// Generate a task counter
	iPotTask ++;


	switch (iPotTask)
	{
	case 0:
		// Start ADC conversion for 9 AD channels
		hadc1.Instance->CR2 |= (uint32_t) ADC_CR2_SWSTART;
		break;

	case 1:

		for (int i=0;i<ADC_CHANNELS ; i++)
		{
			// Use the ADC value
			strPots[i].usRawVal = usADCResult[i];

			// Change in pot value detected?
			if (abs((int)(strPots[i].usRawVal) - (int)(strPots[i].usStabilized)) > POT_STAB_THERESHOLD )
			{
				strPots[i].iStabilizeCnt = POT_STAB_TIME;
			}

			// Update the stabilized value a certain time after change detection
			if (strPots[i].iStabilizeCnt != 0)
			{
				strPots[i].usStabilized = strPots[i].usRawVal;
			}


			POTS_Scale(i);

			// Has the scaled value changed?
			if ((strPots[i].iScaledValue != strPots[i].iScaledValueOld) && strPots[i].iStabilizeCnt != 0)
			{
				strPots[i].bChanged = 1;
			}

			// Update the stabilized value a certain time after change detection
			if (strPots[i].iStabilizeCnt != 0)
			{
				strPots[i].iStabilizeCnt --;
			}

			strPots[i].iScaledValueOld = strPots[i].iScaledValue;


		}
		break;

	case 2:

		for (int i=0;i<ADC_CHANNELS ; i++)
		{
			if (strPots[i].bChanged && strPots[i].eConfigEntry != CFG_E_NONE)
			{
				// Update the configuration value with the potentiometer value
				aConfigWorkingSet[strPots[i].eConfigEntry].iVal = strPots[i].iScaledValue;
				strPots[i].bChanged = 0;
			}
		}
		// Next iPotTask will be 0
		iPotTask = -1;
		break;
	}
}

/**
 * @brief Assigns a potentiometer to a configuration entry
 */
void POTS_Assign(int pot, CONFIG_eConfigEntry configurationEntry)
{
	if (pot >= 0 && pot <ADC_CHANNELS)
	{
		strPots[pot].eConfigEntry = configurationEntry;
		POTS_Scale(pot);
		strPots[pot].bChanged = 1;
	}
}
