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
#include "console.h"
#include "eeprom.h"
#include "pots.h"
#include <string.h>
#include <stdlib.h>


/* global variables  ------------------------------------------------------- */

/* local functions  ------------------------------------------------------- */
static void CONFIG_UseParameter(int ii);

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
char sResult[100];

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
	aConfigWorkingSet[CFG_E_STARTUP_AUTOTUNE].bIsGlobal = 1;


	aConfigWorkingSet[CFG_E_VOL1_NUMERATOR].iFactor = 10000;
	aConfigWorkingSet[CFG_E_VOL2_NUMERATOR].iFactor = 10000;
	aConfigWorkingSet[CFG_E_VOL12_NUMERATOR].iFactor = 10000;
	aConfigWorkingSet[CFG_E_VOL1_OFFSET_A].iFactor = 10;
	aConfigWorkingSet[CFG_E_VOL2_OFFSET_A].iFactor = 10;
	aConfigWorkingSet[CFG_E_VOL12_OFFSET_A].iFactor = 10;
	aConfigWorkingSet[CFG_E_STARTUP_AUTOTUNE].iFactor = 100;

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
		aConfigValues[i][CFG_E_LOUDER_DOWN] = 1;
		aConfigValues[i][CFG_E_STARTUP_AUTOTUNE] = 1;
		aConfigValues[i][CFG_E_DISTORTION] = 0;
		aConfigValues[i][CFG_E_IMPEDANCE] = 1000;
		aConfigValues[i][CFG_E_ADDSYNTH_2] = 800;
		aConfigValues[i][CFG_E_ADDSYNTH_3] = 200;
		aConfigValues[i][CFG_E_ADDSYNTH_4] = 500;
		aConfigValues[i][CFG_E_ADDSYNTH_5] = 300;
		aConfigValues[i][CFG_E_CHORUS_DELAY] = 400;
		aConfigValues[i][CFG_E_CHORUS_FEEDBACK] = 0;
		aConfigValues[i][CFG_E_CHORUS_MODULATION] = 0;
		aConfigValues[i][CFG_E_CHORUS_FREQUENCY] = 300;
		aConfigValues[i][CFG_E_CHORUS_INTENSITY] = 0;
		aConfigValues[i][CFG_E_REVERB_ROOMSIZE] = 500;
		aConfigValues[i][CFG_E_REVERB_DAMPING] = 500;
		aConfigValues[i][CFG_E_REVERB_INTENSITY] = 0;
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
 * @brief copies the parameter to the working set
 *
 * @param ii index of the parameter
 */
static void CONFIG_UseParameter(int ii)
{
	int32_t iSourceVal;
	if (aConfigWorkingSet[ii].bIsGlobal)
	{
		iSourceVal = aConfigValues[0][ii];
	}
	else
	{
		iSourceVal = aConfigValues[iCurrentSet][ii];
	}

	if (aConfigWorkingSet[ii].iVal != iSourceVal)
	{
		aConfigWorkingSet[ii].iVal = iSourceVal*aConfigWorkingSet[ii].iFactor;
		aConfigWorkingSet[ii].bHasChanged = 1;
	}
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
		CONFIG_UseParameter(ii);
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
	if (strcmp(name, "LOUDER_DOWN") == 0) return CFG_E_LOUDER_DOWN;
	if (strcmp(name, "STARTUP_AUTOTUNE") == 0) return CFG_E_STARTUP_AUTOTUNE;
	if (strcmp(name, "DISTORTION") == 0) return CFG_E_DISTORTION;
	if (strcmp(name, "IMPEDANCE") == 0) return CFG_E_IMPEDANCE;
	if (strcmp(name, "ADDSYNTH_2") == 0) return CFG_E_ADDSYNTH_2;
	if (strcmp(name, "ADDSYNTH_3") == 0) return CFG_E_ADDSYNTH_3;
	if (strcmp(name, "ADDSYNTH_4") == 0) return CFG_E_ADDSYNTH_4;
	if (strcmp(name, "ADDSYNTH_5") == 0) return CFG_E_ADDSYNTH_5;
	if (strcmp(name, "CHORUS_DELAY") == 0) return CFG_E_CHORUS_DELAY;
	if (strcmp(name, "CHORUS_FEEDBACK") == 0) return CFG_E_CHORUS_FEEDBACK;
	if (strcmp(name, "CHORUS_MODULATION") == 0) return CFG_E_CHORUS_MODULATION;
	if (strcmp(name, "CHORUS_FREQUENCY") == 0) return CFG_E_CHORUS_FREQUENCY;
	if (strcmp(name, "CHORUS_INTENSITY") == 0) return CFG_E_CHORUS_INTENSITY;
	if (strcmp(name, "REVERB_ROOMSIZE") == 0) return CFG_E_REVERB_ROOMSIZE;
	if (strcmp(name, "REVERB_DAMPING") == 0) return CFG_E_REVERB_DAMPING;
	if (strcmp(name, "REVERB_INTENSITY") == 0) return CFG_E_REVERB_INTENSITY;

	return CFG_E_NONE;
}

/**
 * @brief Gets the name from its enum value
 *
 * @param enumName The enum
 * @return name of the parameter
 */
char* CONFIG_EnumToName(CONFIG_eConfigEntry enumName)
{
	if (enumName == CFG_E_SEL_SET) return "SEL_SET";
	if (enumName == CFG_E_VOL1_NUMERATOR) return "VOL1_NUMERATOR";
	if (enumName == CFG_E_VOL1_OFFSET_A) return "VOL1_OFFSET_A";
	if (enumName == CFG_E_VOL1_OFFSET_B) return "VOL1_OFFSET_B";
	if (enumName == CFG_E_VOL2_NUMERATOR) return "VOL2_NUMERATOR";
	if (enumName == CFG_E_VOL2_OFFSET_A) return "VOL2_OFFSET_A";
	if (enumName == CFG_E_VOL2_OFFSET_B) return "VOL2_OFFSET_B";
	if (enumName == CFG_E_VOL12_NUMERATOR) return "VOL12_NUMERATOR";
	if (enumName == CFG_E_VOL12_OFFSET_A) return "VOL12_OFFSET_A";
	if (enumName == CFG_E_VOL12_OFFSET_B) return "VOL12_OFFSET_B";
	if (enumName == CFG_E_VOLUME_OUT) return "VOLUME_OUT";
	if (enumName == CFG_E_PITCH_SHIFT) return "PITCH_SHIFT";
	if (enumName == CFG_E_PITCH_SCALE) return "PITCH_SCALE";
	if (enumName == CFG_E_VOLUME_SHIFT) return "VOLUME_SHIFT";
	if (enumName == CFG_E_VOLUME_SCALE) return "VOLUME_SCALE";
	if (enumName == CFG_E_LOUDER_DOWN) return "LOUDER_DOWN";
	if (enumName == CFG_E_STARTUP_AUTOTUNE) return "STARTUP_AUTOTUNE";
	if (enumName == CFG_E_DISTORTION) return "DISTORTION";
	if (enumName == CFG_E_IMPEDANCE) return "IMPEDANCE";
	if (enumName == CFG_E_ADDSYNTH_2) return "ADDSYNTH_2";
	if (enumName == CFG_E_ADDSYNTH_3) return "ADDSYNTH_3";
	if (enumName == CFG_E_ADDSYNTH_4) return "ADDSYNTH_4";
	if (enumName == CFG_E_ADDSYNTH_5) return "ADDSYNTH_5";
	if (enumName == CFG_E_CHORUS_DELAY) return "CHORUS_DELAY";
	if (enumName == CFG_E_CHORUS_FEEDBACK) return "CHORUS_FEEDBACK";
	if (enumName == CFG_E_CHORUS_MODULATION) return "CHORUS_MODULATION";
	if (enumName == CFG_E_CHORUS_FREQUENCY) return "CHORUS_FREQUENCY";
	if (enumName == CFG_E_CHORUS_INTENSITY) return "CHORUS_INTENSITY";
	if (enumName == CFG_E_REVERB_ROOMSIZE) return "REVERB_ROOMSIZE";
	if (enumName == CFG_E_REVERB_DAMPING) return "REVERB_DAMPING";
	if (enumName == CFG_E_REVERB_INTENSITY) return "REVERB_INTENSITY";

	return "unknown parameter";
}

/**
 * @brief Assigns a potentiometer to a parameter
 *
 * @param index: index of the potentiometer
 * @param cfgname: name of the parameter
 */
char* CONFIG_ConfigurePot(int index, char* cfgname, int set)
{
	index--; //we count internally from 0..8 instead of 1..9

	CONFIG_eConfigEntry eConfigEntry;
	eConfigEntry = CONFIG_NameToEnum(cfgname);

	// Is all valid?
	if (index >= 0 && index <ADC_CHANNELS)
	{
		if (set)
		{
			if (eConfigEntry != CFG_E_NONE)
			{
				aPotsAssignedToConfigEntry[index] = eConfigEntry;
				POTS_Assign(index, eConfigEntry);
				return "OK";
			}
			else
			{
				return "Unknown entry";
			}
		}
		else
		{
			return CONFIG_EnumToName(aPotsAssignedToConfigEntry[index]);
		}
	}
	return "Index out of range";
}




/**
 * @brief Sets a parameter by its name and index
 *
 * @param cfgname: name of the parameter
 * @param index: index of the set
 * @param val: value of the parameter
 * @param set: 1: set, 0:get
 */
char* CONFIG_ConfigureParameter(char* cfgname, int index, int val, int set)
{
	int startIndex = 0;
	int endIndex = 0;
	index--; //we count internally from 0..7 instead of 1..8

	CONFIG_eConfigEntry eConfigEntry;
	eConfigEntry = CONFIG_NameToEnum(cfgname);

	// Is all valid?
	if (eConfigEntry != CFG_E_NONE)
	{

		if (aConfigWorkingSet[eConfigEntry].bIsGlobal
				|| (index <0 && !set))
		{
			index = 0;
		}
		if (index < SETS )
		{
			// Set value of one SET
			if (index >= 0)
			{
				startIndex = index;
				endIndex = index;
			}
			// Or all, if there is no index
			else
			{
				startIndex = 0;
				endIndex = SETS-1;
			}

			if (set)
			{
				for (int i=startIndex; i<=endIndex; i++)
				{
					aConfigValues[i][eConfigEntry] = val;
				}
				CONFIG_UseParameter(eConfigEntry);
				return "OK";
			}
			else
			{
				sprintf(sResult, "%d", (int)aConfigValues[startIndex][eConfigEntry]);
				return sResult;
			}
		}
		return "Index out of range";
	}
	return "Unknown Parameter";

}

/**
 * @brief Decodes a configuration line
 *
 * @param sLine: the line
 */
char* CONFIG_DecodeLine(char* sLine)
{
	int eqfound, qmfound;
	char c;
	int i,ii;
	int eol;
	char sPartLeft[100]="";
	char sPartRight[100]="";
	int index = 0;


	// Count the "=" signs
	eqfound = 0;
	qmfound = 0;
	i = 0;
	ii= 0;
	eol = 0;
	index = -1;

	do
	{
		c=sLine[i];
		// End of line?
		if (c== '\0' || c== '\''|| c== '/' || c== '\r' || c== '\n' || ii >=80 )
		{
			eol = 1;
		}
		else
		{
			if(c== '=')
			{
				eqfound++;
				ii= 0;
			}
			if(c== '?')
			{
				qmfound++;
				ii= 0;
			}
			else
			{
				// Value with index in brackets
				if (c== '(')
				{
					if (sLine[i+2] == ')')
					{
						index = sLine[i+1]-'0';
						i+=2;
					}
					else
					{
						eol = 1;
						// mark it as invalid
						eqfound = 0;
						qmfound = 0;
					}
				}
				else
				{
					// Ignore spaced
					if (c!= ' ')
					{
						// On the left or right side of the "="?
						if (eqfound == 0 && qmfound == 0)
						{
							sPartLeft[ii]=c;
							sPartLeft[ii+1]='\0';
							ii++;
						}
						if ((eqfound == 1 || qmfound == 1) && c!= '=')
						{
							sPartRight[ii]=c;
							sPartRight[ii+1]='\0';
							ii++;
						}
					}
				}
			}
		}
		i++;
	} while (!eol);
	if (eqfound || qmfound)
	{
		// Assign the potentiometer
		if (strcmp(sPartLeft, "POT") == 0)
		{
			if (eqfound)
			{
				return CONFIG_ConfigurePot(index, sPartRight, 1);
			}
			else if (qmfound)
			{
				return CONFIG_ConfigurePot(index, "" ,0 );
			}
			else
			{
				return "Error";
			}
		}
		// select a debug mode
		else if (strcmp(sPartLeft, "DEBUG") == 0)
		{
			if (eqfound)
			{
				eDebugMode = atoi(sPartRight);
				return "OK";
			}
			else if (qmfound)
			{
				sprintf(sResult, "%d", eDebugMode);
				return sResult;
			}
			else
			{
				return "Error";
			}
		}
		// Set the parameter
		else
		{
			if (eqfound)
			{
				return CONFIG_ConfigureParameter(sPartLeft, index, atoi(sPartRight), 1);
			}
			else if (qmfound)
			{
				return CONFIG_ConfigureParameter(sPartLeft, index, 0, 0);
			}
			else
			{
				return "Error";
			}

		}
	}



	return "Error";
}

