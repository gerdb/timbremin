/**
 *  Project     timbremin
 *  @file		config.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for config.c
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
#ifndef CONFIG_H_
#define CONFIG_H_

/* Constants ------------------------------------------------------------ */
#define VERSION_MAJOR	0
#define VERSION_MINOR	0
#define VERSION_BUILD	1

#define SETS 8

// Virtual EEProm addresses. 32bit values need 2 addresses
#define EEPROM_ADDR_PITCH_AUTOTUNE_H	0
#define EEPROM_ADDR_PITCH_AUTOTUNE_L	1
#define EEPROM_ADDR_VOLTIM1_AUTOTUNE_H	2
#define EEPROM_ADDR_VOLTIM1_AUTOTUNE_L	3
#define EEPROM_ADDR_VOLTIM2_AUTOTUNE_H	4
#define EEPROM_ADDR_VOLTIM2_AUTOTUNE_L	5
// !! Update also NB_OF_VAR in eeprom.h !!


/* Types ---------------------------------------------------------------- */
typedef struct
{
	int32_t iVal;
	int32_t	iFactor;
	int bHasChanged;
	int bIsGlobal;
}CONFIG_sConfigEntry;

typedef enum
{
	CFG_E_SEL_SET,
	CFG_E_VOL1_NUMERATOR,
	CFG_E_VOL1_OFFSET_A,
	CFG_E_VOL1_OFFSET_B,
	CFG_E_VOL2_NUMERATOR,
	CFG_E_VOL2_OFFSET_A,
	CFG_E_VOL2_OFFSET_B,
	CFG_E_VOL12_NUMERATOR,
	CFG_E_VOL12_OFFSET_A,
	CFG_E_VOL12_OFFSET_B,
	CFG_E_VOLUME_OUT,
	CFG_E_PITCH_SHIFT,
	CFG_E_PITCH_SCALE,
	CFG_E_VOLUME_SHIFT,
	CFG_E_VOLUME_SCALE,
	CFG_E_LOUDER_DOWN,
	CFG_E_STARTUP_AUTOTUNE,
	CFG_E_DISTORTION,
	CFG_E_IMPEDANCE,
	CFG_E_ADDSYNTH_2,
	CFG_E_ADDSYNTH_3,
	CFG_E_ADDSYNTH_4,
	CFG_E_ADDSYNTH_5,
	CFG_E_CHORUS_DELAY,
	CFG_E_CHORUS_FEEDBACK,
	CFG_E_CHORUS_MODULATION,
	CFG_E_CHORUS_FREQUENCY,
	CFG_E_CHORUS_INTENSITY,
	CFG_E_REVERB_ROOMSIZE,
	CFG_E_REVERB_DAMPING,
	CFG_E_REVERB_INTENSITY,
	CFG_E_AUTOMUTE,
	CFG_E_AUTOPREHEAR,
	CFG_E_VOLUME_LOWER,
	CFG_E_VOLUME_RANGE,
	CFG_E_VOLUME_LINEAR,

	CFG_E_ENTRIES
}CONFIG_eConfigEntry;

#define CFG_E_NONE CFG_E_ENTRIES

typedef struct
{
  int32_t   Version;
  int32_t   VolumeShift;
  int32_t   cfg02;
  int32_t   cfg03;
  int32_t   cfg04;
  int32_t   cfg05;
  int32_t   cfg06;
  int32_t   cfg07;
  int32_t   cfg08;
  int32_t   cfg09;
  int32_t   cfg10;
  int32_t   cfg11;
  int32_t   cfg12;
  int32_t   cfg13;
  int32_t   cfg14;
  int32_t   cfg15;

  int32_t   cfg16;
  int32_t   cfg17;
  int32_t   cfg18;
  int32_t   cfg19;
  int32_t   cfg20;
  int32_t   cfg21;
  int32_t   cfg22;
  int32_t   cfg23;
  int32_t   cfg24;
  int32_t   cfg25;
  int32_t   cfg26;
  int32_t   cfg27;
  int32_t   cfg28;
  int32_t   cfg29;
  int32_t   cfg30;
  int32_t   cfg31;

  int32_t   cfg32;
  int32_t   cfg33;
  int32_t   cfg34;
  int32_t   cfg35;
  int32_t   cfg36;
  int32_t   cfg37;
  int32_t   cfg38;
  int32_t   cfg39;
  int32_t   cfg40;
  int32_t   cfg41;
  int32_t   cfg42;
  int32_t   cfg43;
  int32_t   cfg44;
  int32_t   cfg45;
  int32_t   cfg46;
  int32_t   cfg47;

  int32_t   cfg48;
  int32_t   cfg49;
  int32_t   cfg50;
  int32_t   cfg51;
  int32_t   cfg52;
  int32_t   cfg53;
  int32_t   cfg54;
  int32_t   cfg55;
  int32_t   cfg56;
  int32_t   cfg57;
  int32_t   cfg58;
  int32_t   cfg59;
  int32_t   cfg60;
  int32_t   cfg61;
  int32_t   cfg62;
  int32_t   cfg63;
}CONFIG_TypeDef;


/* global variables ----------------------------------------------------- */
volatile extern const CONFIG_TypeDef __attribute__((section (".myConfigSection"))) CONFIG;
extern int32_t aConfigValues[SETS][CFG_E_ENTRIES];
extern CONFIG_sConfigEntry aConfigWorkingSet[CFG_E_ENTRIES];

/* Function prototypes ----------------------------------------------------- */
void CONFIG_Init(void);
void CONFIG_FillWithDefault(void);
void CONFIG_Select_Set(int set);
void CONFIG_Assign_All_Pots(void);
char*  CONFIG_ConfigurePot(int index, char* cfgname, int set);
char*  CONFIG_ConfigureParameter(char* cfgname, int index, int val, int set);
char*  CONFIG_GetAllParameters(int index);
void CONFIG_Update_Set(void);
CONFIG_eConfigEntry CONFIG_NameToEnum(char* name);
char* CONFIG_EnumToName(CONFIG_eConfigEntry enumName);
char* CONFIG_DecodeLine(char* sLine);

void CONFIG_Write_SLong(int addr, int32_t value);
int32_t CONFIG_Read_SLong(int addr);

#endif /* CONFIG_H_ */
