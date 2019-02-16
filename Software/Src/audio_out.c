/**
 *  Project     timbremin
 *  @file		audio_out.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Outputs the digital audio data to DAC
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



#include "../Drivers/BSP/Components/cs43l22/cs43l22.h"
#include "../Drivers/BSP/STM32F4-Discovery/stm32f4_discovery.h"
#include "../Drivers/BSP/Components/Common/audio.h"
#include "stm32f4xx_hal.h"
#include "audio_out.h"
#include "theremin.h"
#include "pots.h"
#include "effect.h"
#include <math.h>


/* global variables  ------------------------------------------------------- */
int bMute = 0;
int16_t ssDACValueR;		// output for audio DAC RIGHT (ear phone)
int16_t ssDACValueL;		// output for audio DAC LEFT (speaker)

/* local variables  ------------------------------------------------------- */
__IO uint32_t I2S_DR;
static AUDIO_DrvTypeDef           *pAudioDrv;
extern I2S_HandleTypeDef hi2s3;

static uint8_t Volume = 60;	// Initial Volume level (from 0 (Mute) to 100 (Max))

void AUDIO_OUT_Init(void)
{
	  uint8_t ret = AUDIO_OK;

	  if(ret == AUDIO_OK)
	  {
	    /* Retrieve audio codec identifier */
	    if(((cs43l22_drv.ReadID(AUDIO_I2C_ADDRESS)) & CS43L22_ID_MASK) == CS43L22_ID)
	    {
	      /* Initialize the audio driver structure */
	      pAudioDrv = &cs43l22_drv;
	    }
	    else
	    {
	      ret = AUDIO_ERROR;
	    }
	  }

	  if(ret == AUDIO_OK)
	  {
	    pAudioDrv->Init(AUDIO_I2C_ADDRESS, OUTPUT_DEVICE_HEADPHONE, Volume, 48000);
	  }

	  if(ret == AUDIO_OK)
	  {
		  /* Call the audio Codec Play function */
		  if(pAudioDrv->Play(AUDIO_I2C_ADDRESS, 0, 0) != 0)
		  {
			  ret = AUDIO_ERROR;
		  }
		  else
		  {
		    /* Update the Media layer and enable it for play */
		    //HAL_I2S_Transmit_DMA(&hAudioOutI2s, pBuffer, DMA_MAX(Size/AUDIODATA_SIZE));

		    /* Return AUDIO_OK when all operations are correctly done */
		    ret = AUDIO_OK;
		  }
	  }

	  if(ret == AUDIO_OK)
	  {
		    /* Enable TXE and ERR interrupt */
		    __HAL_I2S_ENABLE_IT(&hi2s3, (I2S_IT_TXE | I2S_IT_ERR));

		    /* Check if the I2S is already enabled */
		    if((hi2s3.Instance->I2SCFGR &SPI_I2SCFGR_I2SE) != SPI_I2SCFGR_I2SE)
		    {
		      /* Enable I2S peripheral */
		      __HAL_I2S_ENABLE(&hi2s3);
		    }

		    hi2s3.Instance->DR = 0;
	  }
}

/*
 * @brief 1ms Task to set the volume of the DAC headphone amplifier
 *
 */
void AUDIO_OUT_1msTask(void)
{
	if (aConfigWorkingSet[CFG_E_VOLUME_OUT].bHasChanged) {
		float vol =  ((float)aConfigWorkingSet[CFG_E_VOLUME_OUT].iVal)*0.001f;
		vol = powf(vol, 0.2f);
		pAudioDrv->SetVolume(AUDIO_I2C_ADDRESS,vol * 90);
	}
}

/*
 * This IRQ Handler is called every 96kHz, to send a new sample
 *  first left then right audio channel to the DAC over the I2S bus.
 *  The Audio frequency is 48kHz
 */
void AUDIO_OUT_I2S_IRQHandler(void)
{
	static int toggle = 0;

	if (toggle)
	{

		// Fill the audio DAC with the LEFT value = speaker
		hi2s3.Instance->DR = ssDACValueL;
		toggle = 0;
		THEREMIN_96kHzDACTask_A();

	}
	else
	{

		// Fill the audio DAC with the RIGHT value = ear phone
		hi2s3.Instance->DR = ssDACValueR;
		toggle = 1;
		EFFECT_48kHzTask();

	}
	// Get the new value for the next task
	THEREMIN_96kHzDACTask_Common();
}


