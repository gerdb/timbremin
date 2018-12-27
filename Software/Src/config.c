/**
 *  Project     timbremin
 *  @file		config.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Project configuration
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
#include "config.h"
#include "eeprom.h"
#include "pots.h"
#include <string.h>


/* global variables  ------------------------------------------------------- */
/* local variables  ------------------------------------------------------- */
volatile const CONFIG_TypeDef __attribute__((section (".myConfigSection"))) CONFIG =
{ (VERSION_MAJOR << 16 | VERSION_MINOR << 8 | VERSION_BUILD), // Version
		0,	// VolumeShift
		};

uint16_t VirtAddVarTab[NB_OF_VAR] = {
		EEPROM_ADDR_PITCH_AUTOTUNE_H,
		EEPROM_ADDR_PITCH_AUTOTUNE_L,
		EEPROM_ADDR_VOLTIM1_AUTOTUNE_H,
		EEPROM_ADDR_VOLTIM1_AUTOTUNE_L,
		EEPROM_ADDR_VOLTIM2_AUTOTUNE_H,
		EEPROM_ADDR_VOLTIM2_AUTOTUNE_L
};

int iCurrentSet = 0;

int32_t aConfigValues[SETS][CFG_E_ENTRIES];
CONFIG_sConfigEntry aConfigWorkingSet[CFG_E_ENTRIES];
CONFIG_eConfigEntry aPotsAssignedToConfigEntry[AMOUNT_POTS];

/**
 * @brief initialize the module
 */
void CONFIG_Init(void)
{

	/* Unlock the Flash Program Erase controller */
	HAL_FLASH_Unlock();

	/* EEPROM Init */
	if (EE_Init() != EE_OK)
	{
		;
	}

	// Default values for all entries
	for (int i=0;i<CFG_E_ENTRIES; i++)
	{
		aConfigWorkingSet[i].bHasChanged = 1;
		aConfigWorkingSet[i].iMaxVal = 1000;
		aConfigWorkingSet[i].iVal = 0;
		aConfigWorkingSet[i].iFactor = 1;
		aConfigWorkingSet[i].bIsGlobal = 0;

	}

	aConfigWorkingSet[CFG_E_VOL1_NUMERATOR].bIsGlobal = 1;
	aConfigWorkingSet[CFG_E_VOL2_NUMERATOR].bIsGlobal = 1;
	aConfigWorkingSet[CFG_E_VOL12_NUMERATOR].bIsGlobal = 1;
	aConfigWorkingSet[CFG_E_VOL1_OFFSET_A].bIsGlobal = 1;
	aConfigWorkingSet[CFG_E_VOL2_OFFSET_A].bIsGlobal = 1;
	aConfigWorkingSet[CFG_E_VOL12_OFFSET_A].bIsGlobal = 1;
	aConfigWorkingSet[CFG_E_VOL1_OFFSET_B].bIsGlobal = 1;
	aConfigWorkingSet[CFG_E_VOL2_OFFSET_B].bIsGlobal = 1;
	aConfigWorkingSet[CFG_E_VOL12_OFFSET_B].bIsGlobal = 1;

	aConfigWorkingSet[CFG_E_VOL1_NUMERATOR].iFactor = 10000;
	aConfigWorkingSet[CFG_E_VOL2_NUMERATOR].iFactor = 10000;
	aConfigWorkingSet[CFG_E_VOL12_NUMERATOR].iFactor = 10000;
	aConfigWorkingSet[CFG_E_VOL1_OFFSET_A].iFactor = 10;
	aConfigWorkingSet[CFG_E_VOL2_OFFSET_A].iFactor = 10;
	aConfigWorkingSet[CFG_E_VOL12_OFFSET_A].iFactor = 10;

	CONFIG_FillWithDefault();

	//aPotsAssignedToConfigEntry[0] = CFG_E_SEL_SET;
	//aPotsAssignedToConfigEntry[1] = CFG_E_VOLUME_OUT;

	CONFIG_Select_Set(0);
}

/**
 * @brief fill the configuration with default values
 */
void CONFIG_FillWithDefault(void)
{
	// Default values for all entries
	for (int i=0;i<AMOUNT_POTS; i++)
	{
		aPotsAssignedToConfigEntry[i] = CFG_E_NONE;
	}

	// Fill all sets with default values
	for (int i=0;i<SETS;i++)
	{
		aConfigValues[i][CFG_E_VOL1_NUMERATOR] = 77;
		aConfigValues[i][CFG_E_VOL1_OFFSET_A] = 50;
		aConfigValues[i][CFG_E_VOL1_OFFSET_B] = 23;
		aConfigValues[i][CFG_E_VOL2_NUMERATOR] = 116;
		aConfigValues[i][CFG_E_VOL2_OFFSET_A] = 72;
		aConfigValues[i][CFG_E_VOL2_OFFSET_B] = 29;
		aConfigValues[i][CFG_E_VOL12_NUMERATOR] = 192;
		aConfigValues[i][CFG_E_VOL12_OFFSET_A] = 120;
		aConfigValues[i][CFG_E_VOL12_OFFSET_B] = 26;
		aConfigValues[i][CFG_E_VOLUME_OUT] = 500;
		aConfigValues[i][CFG_E_PITCH_SHIFT] = 500;
		aConfigValues[i][CFG_E_PITCH_SCALE] = 500;
		aConfigValues[i][CFG_E_VOLUME_SHIFT] = 500;
		aConfigValues[i][CFG_E_VOLUME_SCALE] = 500;
	}
}
/**
 * @brief Writes a 32 bit value to a virtual EEProm address
 *
 * @param addr: the virtual eeprom address
 * @value the 32bit value to write
 */
void CONFIG_Write_SLong(int addr, int32_t value)
{
	if ((EE_WriteVariable((uint16_t)addr, (uint16_t)( (uint32_t)value >> 16 ) ) ) != HAL_OK)
	{
		return;
	}

	if ((EE_WriteVariable((uint16_t)(addr+1), (uint16_t)value )) != HAL_OK)
	{
		return;
	}
}

/**
 * @brief Reloads the current set (if some parameters have changed
 *
 */
void CONFIG_Update_Set(void)
{
	CONFIG_Select_Set(iCurrentSet);
}

/**
 * @brief Selects an new configuration set and copies the data into the working variable
 *
 * @param set index of the new set
 */
void CONFIG_Select_Set(int set)
{
	iCurrentSet = set;
	// Default values for all entries
	for (int ii=1;ii<CFG_E_ENTRIES; ii++)
	{
		int32_t iSourceVal;
		if (aConfigWorkingSet[ii].bIsGlobal)
		{
			iSourceVal = aConfigValues[0][ii];
		}
		else
		{
			iSourceVal = aConfigValues[set][ii];
		}

		if (aConfigWorkingSet[ii].iVal != iSourceVal)
		{
			aConfigWorkingSet[ii].iVal = iSourceVal*aConfigWorkingSet[ii].iFactor;
			aConfigWorkingSet[ii].bHasChanged = 1;
		}
	}
}


/**
 * @brief Assigns all pots to the configuration structures
 *
 */
void CONFIG_Assign_All_Pots(void)
{
	// Default values for all entries
	for (int i=0;i<AMOUNT_POTS; i++)
	{
		POTS_Assign(i, aPotsAssignedToConfigEntry[i]);
	}
}

/**
 * @brief Reads a 32 bit value from a virtual EEProm address
 *
 * @param addr: the virtual eeprom address
 * @return the 32bit value to write
 */
int32_t CONFIG_Read_SLong(int addr)
{
	uint16_t VarDataTmp = 0;
	int32_t value = 0;

	if (EE_ReadVariable((uint16_t)addr, &VarDataTmp) != HAL_OK)
	{
		return 0;
	}
	value = VarDataTmp;
	value <<= 16;

	if (EE_ReadVariable((uint16_t)(addr+1), &VarDataTmp) != HAL_OK)
	{
		return 0;
	}
	value |= VarDataTmp;
	return value;
}

/**
 * @brief Gets the enum value from its name
 *
 * @param name: name of the parameter
 * @return The enum
 */
CONFIG_eConfigEntry CONFIG_NameToEnum(char* name)
{
	if (strcmp(name, "SEL_SET") == 0) return CFG_E_SEL_SET;
	if (strcmp(name, "VOL1_NUMERATOR") == 0) return CFG_E_VOL1_NUMERATOR;
	if (strcmp(name, "VOL1_OFFSET_A") == 0) return CFG_E_VOL1_OFFSET_A;
	if (strcmp(name, "VOL1_OFFSET_B") == 0) return CFG_E_VOL1_OFFSET_B;
	if (strcmp(name, "VOL2_NUMERATOR") == 0) return CFG_E_VOL2_NUMERATOR;
	if (strcmp(name, "VOL2_OFFSET_A") == 0) return CFG_E_VOL2_OFFSET_A;
	if (strcmp(name, "VOL2_OFFSET_B") == 0) return CFG_E_VOL2_OFFSET_B;
	if (strcmp(name, "VOL12_NUMERATOR") == 0) return CFG_E_VOL12_NUMERATOR;
	if (strcmp(name, "VOL12_OFFSET_A") == 0) return CFG_E_VOL12_OFFSET_A;
	if (strcmp(name, "VOL12_OFFSET_B") == 0) return CFG_E_VOL12_OFFSET_B;
	if (strcmp(name, "VOLUME_OUT") == 0) return CFG_E_VOLUME_OUT;
	if (strcmp(name, "PITCH_SHIFT") == 0) return CFG_E_PITCH_SHIFT;
	if (strcmp(name, "PITCH_SCALE") == 0) return CFG_E_PITCH_SCALE;
	if (strcmp(name, "VOLUME_SHIFT") == 0) return CFG_E_VOLUME_SHIFT;
	if (strcmp(name, "VOLUME_SCALE") == 0) return CFG_E_VOLUME_SCALE;
	return CFG_E_NONE;
}

/**
 * @brief Assigns a potentiometer to a parameter
 *
 * @param index: index of the potentiometer
 * @param cfgname: name of the parameter
 */
void CONFIG_ConfigurePot(int index, char* cfgname)
{
	CONFIG_eConfigEntry eConfigEntry;
	eConfigEntry = CONFIG_NameToEnum(cfgname);

	// Is all valid?
	if (index >= 0 && index <ADC_CHANNELS && eConfigEntry != CFG_E_NONE)
	{
		aPotsAssignedToConfigEntry[index] = eConfigEntry;
		POTS_Assign(index, eConfigEntry);
	}
}

/**
 * @brief Sets a parameter by its name and index
 *
 * @param cfgname: name of the parameter
 * @param index: index of the set
 * @param val: value of the parameter
 */
void CONFIG_ConfigureParameter(char* cfgname, int index, int val)
{
	index--; //we count internally from 0..7 instead of 1..8

	CONFIG_eConfigEntry eConfigEntry;
	eConfigEntry = CONFIG_NameToEnum(cfgname);

	// Is all valid?
	if (eConfigEntry != CFG_E_NONE)
	{

		if (aConfigWorkingSet[eConfigEntry].bIsGlobal)
		{
			index = 0;
		}
		if ((index >= 0 && index < SETS) )
		{
			aConfigValues[index][eConfigEntry] = val;
		}
	}


}
