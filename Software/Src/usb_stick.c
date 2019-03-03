/**
 *  Project     timbremin
 *  @file		usb_stick.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Load WAV table from file
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

#include <stdio.h>
#include "fatfs.h"
#include "usb_host.h"
#include "theremin.h"
#include "config.h"
#include "usb_stick.h"

/* Variable used by FatFs */
int bMounted = 0;
#define LINELENGTH 256
char sLine[LINELENGTH];
uint32_t ulBytesRead;

/* Function prototypes for local functions ------------------------------------ */

/* Local functions ------------------------------------------------------------ */


/**
 * @brief Reads the Configuration file
 */
void USB_STICK_ReadConfigFile(void)
{


	// File on stick?
	if (f_open(&USBHFile, "CONFIG.TXT", FA_READ) == FR_OK)
	{
		// Reset the configuration to default
		CONFIG_FillWithDefault();

		// Read the *.c file line by line
		while (f_gets(sLine, LINELENGTH, &USBHFile) != 0)
		{
			CONFIG_DecodeLine(sLine);
		}
		// Reload the current set
		CONFIG_Update_Set();
		// Close the file
        f_close(&USBHFile);
	}
}


/**
 * @brief Reads the WAV file
 *
 * @param filename: Filename to read
 */
int USB_STICK_EmptyFileExists(char* filename)
{
	FILINFO fno;
	if (bMounted)
	{
	    /* try to open file to read */
	    if (f_stat( filename, &fno) == FR_OK){

	        return (fno.fsize < 10);
	    }
	}
	return 0;
}


/**
 * @brief writes the volume calibration values to a file
 */
void USB_STICK_WriteVolCalFile(char* filename, VOLUME_VolCalibrationType aCalibrationEntries[])
{
	if (!bMounted)
		return;


	if (bMounted)
	{
		// Open the Wave file to be played
		if (f_open(&USBHFile, filename, FA_WRITE) == FR_OK)
		{
			f_printf(&USBHFile,"Result volume antenna calibration:\n");
			f_printf(&USBHFile,"VOLUME;cm\n");
			for (int i=0; i<= 20; i++)
			{
				f_printf(&USBHFile,"%d;%d\n",
						aCalibrationEntries[i].value,
						aCalibrationEntries[i].cm);
			}
	        f_close(&USBHFile);
		}
	}
}


/**
 * @brief writes the volume calibration values to a file
 */
void USB_STICK_WritePitchCalFile(char* filename, PITCH_PitchCalibrationType aCalibrationEntries[])
{
	if (!bMounted)
		return;


	if (bMounted)
	{
		// Open the Wave file to be played
		if (f_open(&USBHFile, filename, FA_WRITE) == FR_OK)
		{
			f_printf(&USBHFile,"Result pitch antenna calibration:\n");
			f_printf(&USBHFile,"PITCH;cm\n");
			for (int i=0; i<= 20; i++)
			{
				f_printf(&USBHFile,"%d;%d\n",
						aCalibrationEntries[i].pitch,
						aCalibrationEntries[i].cm);
			}
	        f_close(&USBHFile);
		}
	}
}


/**
 * @brief Callback function: An USB stick was connected
 */
void USB_STICK_Connected(void)
{
	// Mount the USB stick and read the wave files
	if (f_mount(&USBHFatFS, (TCHAR const*) USBHPath, 0) == FR_OK)
	{
		bMounted = 1;
		USB_STICK_ReadConfigFile();
	}
}

/**
 * @brief Callback function: An USB stick was disconnected
 */
void USB_STICK_Disconnected(void)
{
	bMounted = 0;
}
