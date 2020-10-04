/**
  ******************************************************************************
  * @file    STM32G474E-EVAL/src/flash_if.c
  * @author  MCD Application Team
  * @version v2.0.0
  * @date    16-May-2019
  * @brief   This file provides all the memory related operation functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2019 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/** @addtogroup STM32G474E-EVAL
  * @{
  */

/** @addtogroup Flash_IF
  * @brief Internal flash memory manipulating functions.
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "flash_if.h"
#include "main.h"
#include "common.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Public functions ---------------------------------------------------------*/

/**
  * @brief  This function does an erase of all user flash area
  * @param  bank_active: start of user flash area
  * @retval FLASHIF_OK : user flash area successfully erased
  *         FLASHIF_ERASEKO : error occurred
  */
FLASHIF_StatusTypeDef FLASH_If_Erase(uint32_t bank_active)
{
  uint32_t bank_to_erase, error = 0U;
  FLASH_EraseInitTypeDef pEraseInit;
  HAL_StatusTypeDef status;
  FLASHIF_StatusTypeDef fstatus;

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  pEraseInit.Banks = bank_active;
  pEraseInit.NbPages = 127U;
  pEraseInit.Page = 0U;
  pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE;

  status = HAL_FLASHEx_Erase(&pEraseInit, &error);

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  if (status != HAL_OK)
  {
    /* Error occurred while page erase */
    fstatus = FLASHIF_ERASEKO;
  }
  else
  {
    fstatus = FLASHIF_OK;
  }

  return fstatus;
}

/**
  * @brief  This function does an CRC check of an application loaded in a memory bank.
  * @param  start: start of user flash area
  * @retval FLASHIF_OK: user flash area successfully erased
  *         other: error occurred
  */
FLASHIF_StatusTypeDef FLASH_If_Check(uint32_t start)
{
  FLASHIF_StatusTypeDef status = FLASHIF_OK;

  /* checking if the data could be code (first word is stack location) */
  if ((*(uint32_t *)start >> 24U) != 0x20U)
  {
    status = FLASHIF_EMPTY;
  }

  return status;
}

/**
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  destination: start address for target location
  * @param  p_source: pointer on buffer with data to write
  * @param  length: length of data buffer (unit is 32-bit word)
  * @retval FLASHIF_StatusTypeDef
  *         FLASHIF_OK: Data successfully written to Flash memory
  *         FLASHIF_WRITING_ERROR: Error occurred while writing data in Flash memory
  *         FLASHIF_WRITINGCTRL_ERROR: Written Data in flash memory is different from expected one
  */
FLASHIF_StatusTypeDef FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
  FLASHIF_StatusTypeDef status = FLASHIF_OK;
  uint32_t i = 0U;

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* DataLength must be a multiple of 64 bit */
  for (i = 0U; (i < length / 2U) && (destination <= (USER_FLASH_END_ADDRESS - 8U)); i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, destination, *((uint64_t *)(p_source + 2U * i))) == HAL_OK)
    {
      /* Check the written value */
      if (*(uint64_t *)destination != *(uint64_t *)(p_source + 2U * i))
      {
        /* Flash content doesn't match SRAM content */
        status = FLASHIF_WRITINGCTRL_ERROR;
      }
      /* Increment FLASH destination address */
      destination += 8U;
    }
    else
    {
      /* Error occurred while writing data in Flash memory */
      status = FLASHIF_WRITING_ERROR;
    }

    if (status != FLASHIF_OK)
    {
      break;
    }
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return status;
}

/**
  * @brief  Configure the write protection status of user flash area.
  * @retval uint32_t FLASHIF_OK if change is applied.
  */
FLASHIF_StatusTypeDef FLASH_If_WriteProtectionClear( void )
{
  FLASH_OBProgramInitTypeDef OptionsBytesStruct1;
  HAL_StatusTypeDef retr;

  /* Unlock the Flash to enable the flash control register access *************/
  retr = HAL_FLASH_Unlock();

  /* Unlock the Options Bytes *************************************************/
  retr |= HAL_FLASH_OB_Unlock();

  OptionsBytesStruct1.RDPLevel = OB_RDP_LEVEL_0;
  OptionsBytesStruct1.OptionType = OPTIONBYTE_WRP;
  OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK2_AREAA;
  OptionsBytesStruct1.WRPEndOffset = 0x00U;
  OptionsBytesStruct1.WRPStartOffset = FLASH_PAGE_NB - 1U;
  retr |= HAL_FLASHEx_OBProgram(&OptionsBytesStruct1);

  OptionsBytesStruct1.WRPArea = OB_WRPAREA_BANK2_AREAB;
  retr |= HAL_FLASHEx_OBProgram(&OptionsBytesStruct1);

  return (retr == HAL_OK ? FLASHIF_OK : FLASHIF_PROTECTION_ERRROR);
}

/**
  * @brief  Modify the BFB2 status of user flash area.
  * @param  none
  * @retval HAL_StatusTypeDef HAL_OK if change is applied.
  */
HAL_StatusTypeDef FLASH_If_BankSwitch(void)
{
  FLASH_OBProgramInitTypeDef ob_config;
  HAL_StatusTypeDef result;

  HAL_FLASH_Lock();

  /* Clear OPTVERR bit set on virgin samples */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

  /* Get the current configuration */
  HAL_FLASHEx_OBGetConfig( &ob_config );

  ob_config.OptionType = OPTIONBYTE_USER;
  ob_config.USERType = OB_USER_BFB2;
  if ((ob_config.USERConfig) & (OB_BFB2_ENABLE)) /* BANK1 active for boot */
  {
    ob_config.USERConfig = OB_BFB2_DISABLE;
  }
  else
  {
    ob_config.USERConfig = OB_BFB2_ENABLE;
  }

  /* Initiating the modifications */
  result = HAL_FLASH_Unlock();

  /* program if unlock is successful */
  if ( result == HAL_OK )
  {
    result = HAL_FLASH_OB_Unlock();

    /* program if unlock is successful*/
    if ((READ_BIT(FLASH->CR, FLASH_CR_OPTLOCK) == RESET))
    {
      result = HAL_FLASHEx_OBProgram(&ob_config);
    }
    if (result == HAL_OK)
    {
      HAL_FLASH_OB_Launch();
    }
  }
  return result;
}

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
