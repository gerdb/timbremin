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
#include "usb_stick.h"

/* Variable used by FatFs */
int bMounted = 0;
int bWavLoaded = 0;
#define LINELENGTH 256
char sLine[LINELENGTH];
uint32_t ulBytesRead;

/* Function prototypes for local functions ------------------------------------ */
static int USB_STICK_ParseNumber(int* number, const char* line);

/* Local functions ------------------------------------------------------------ */

/**
 * @brief Parse one line of the OpenTheremin *.c file
 * @param number: parsed line as number
 * @param line: The line as string
 * @return 1: if it was a line with number
 */
static int USB_STICK_ParseNumber(int* number, const char* line)
{
	char sTemp[9];
	int i = 0;

	// Has it a valid length?
	if (strlen(line) > 0 && strlen(line) < 8)
	{
		// Use only numbers and the sign
		while ((line[i] == '-' || (line[i] >= '0' && line[i] <= '9')) && i < 8)
		{
			// Copy only valid characters
			sTemp[i] = line[i];
			i++;
		}

		// End of string
		sTemp[i] = '\0';

		// Was there a number?
		if (i > 0)
		{
			// Pase the string
			*number = atoi(sTemp);
			return 1;
		}
	}
	return 0;
}

/**
 * @brief Read an OpenTheremin *.c wave file
 * @param filename: Filename to read
 */
void USB_STICK_ReadCFile(char* filename)
{
	int value;
	int cnt = 0;
	int32_t l;

	// File on stick?
	if (f_open(&USBHFile, filename, FA_READ) == FR_OK)
	{
		// Read the *.c file line by line
		while (f_gets(sLine, LINELENGTH, &USBHFile) != 0)
		{
			// Parse a line of the *.c file and get the number
			if (USB_STICK_ParseNumber(&value, sLine))
			{
				// Scale it from 12bit (OpenTheremin) to 16bit and limit it
				l = value * 16;

				if (l > 32767)
					l = 32767;
				if (l < -32768)
					l = -32768;

				ssWaveTable[cnt] = l;
				cnt++;
			}
		}

		// At least one number was valid?
		if (cnt >= 1)
		{
			// Fill the rest of the table by repeating the content
			for (int i = cnt; i < 4096; i++)
			{
				ssWaveTable[i] = ssWaveTable[i - cnt];
			}
		}
	}
}

/**
 * @brief Read an Microsoft *.wav audio file
 * @param filename: Filename to read
 */
void USB_STICK_ReadWAVFile(char* filename)
{
	WAVE_FormatTypeDef waveformat;
	unsigned int ulBytesread = 0;
	uint32_t ulWaveDataLength = 0;
	int16_t ssWavSample, ssLastWavSample;
	int iWaveCnt = 0;
	int iWaveSamplePos, iLastWaveSamplePos;

	// Open the Wave file to be played
	if (f_open(&USBHFile, filename, FA_READ) == FR_OK)
	{
		// Read sizeof(WaveFormat) from the selected file
		f_read(&USBHFile, &waveformat, sizeof(waveformat), &ulBytesread);

		// Set WaveDataLenght to the Speech Wave length
		ulWaveDataLength = waveformat.SubChunk2Size / 2;

		iWaveCnt = 0;
		ssLastWavSample = 0;
		iLastWaveSamplePos = 0;
		// Read all samples
		for (int i = 0; i < ulWaveDataLength && i < 1024; i++)
		{
			// Read one sample
			f_read(&USBHFile, &ssWavSample, 2, &ulBytesread);
			iWaveSamplePos = (i * 1024) / ulWaveDataLength;
			for (; iWaveCnt < iWaveSamplePos; iWaveCnt++)
			{
				//Interpolate to scale e.g. 160 sample from a WAV file to 1024 samples in the wave table
				ssWaveTable[iWaveCnt] = (ssWavSample - ssLastWavSample)
						* (iWaveCnt - iLastWaveSamplePos)
						/ (iWaveSamplePos - iLastWaveSamplePos)
						+ ssLastWavSample;
			}
			ssLastWavSample = ssWavSample;
			iLastWaveSamplePos = iWaveSamplePos;
		}

		// Fill the rest of the table by repeating the content
		for (int i = 1024; i < 4096; i++)
		{
			ssWaveTable[i] = ssWaveTable[i - 1024];
		}

	}
}

/**
 * @brief Reads the WAV file
 */
void USB_STICK_ReadFiles(void)
{
	if (bMounted && (eWaveform == USBSTICK) && !bWavLoaded)
	{
		USB_STICK_ReadWAVFile("WAV1.WAV");
		USB_STICK_ReadCFile("WAV1.C");
		bWavLoaded = 1;
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
		USB_STICK_ReadFiles();
	}
}

/**
 * @brief Callback function: An USB stick was disconnected
 */
void USB_STICK_Disconnected(void)
{
	bMounted = 0;
	bWavLoaded = 0;
}
