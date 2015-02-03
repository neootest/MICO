/**
******************************************************************************
* @file    MicoDriverAdc.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provides flash operation functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2014 MXCHIP Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/ 

/* Includes ------------------------------------------------------------------*/
#include "PlatformLogging.h"
#include "MicoPlatform.h"
#include "platform.h"
#include "Platform_common_config.h"
#include "stdio.h"
#include "AP80xx.h"

/* Private constants --------------------------------------------------------*/
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbyte */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbyte */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbyte */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbyte */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbyte */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbyte */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbyte */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbyte */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbyte */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbyte */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbyte */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbyte */

/* End of the Flash address */
#define FLASH_START_ADDRESS     (uint32_t)0x08000000  
#define FLASH_END_ADDRESS       (uint32_t)0x080FFFFF
#define FLASH_SIZE              (FLASH_END_ADDRESS -  FLASH_START_ADDRESS + 1)
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#ifdef USE_MICO_SPI_FLASH
static sflash_handle_t sflash_handle = {0x0, 0x0, SFLASH_WRITE_NOT_ALLOWED};
#endif
/* Private function prototypes -----------------------------------------------*/
static uint32_t _GetSector( uint32_t Address );
static OSStatus internalFlashInitialize( void );
static OSStatus internalFlashErase(uint32_t StartAddress, uint32_t EndAddress);
static OSStatus internalFlashWrite(volatile uint32_t* FlashAddress, uint32_t* Data ,uint32_t DataLength);
static OSStatus internalFlashByteWrite( volatile uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength );
static OSStatus internalFlashFinalize( void );
#ifdef USE_MICO_SPI_FLASH
static OSStatus spiFlashErase(uint32_t StartAddress, uint32_t EndAddress);
#endif


const char* flash_name[] =
{ 
#ifdef USE_MICO_SPI_FLASH
  [MICO_SPI_FLASH] = "SPI", 
#endif
  [MICO_INTERNAL_FLASH] = "Internal",
};

OSStatus MicoFlashInitialize( mico_flash_t flash )
{ 
  platform_log_trace();
  if(flash == MICO_INTERNAL_FLASH){
    return internalFlashInitialize();    
  }
#ifdef USE_MICO_SPI_FLASH
  else if(flash == MICO_SPI_FLASH){
    if(sflash_handle.device_id)
      return kNoErr;
    else
      return init_sflash( &sflash_handle, 0, SFLASH_WRITE_ALLOWED );
  }
#endif
  else
    return kUnsupportedErr;
}

OSStatus MicoFlashErase( mico_flash_t flash, uint32_t StartAddress, uint32_t EndAddress )
{
  platform_log_trace();
  return kUnsupportedErr;
}

OSStatus MicoFlashWrite(mico_flash_t flash, volatile uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength)
{
  return kUnsupportedErr;
}

OSStatus MicoFlashRead(mico_flash_t flash, volatile uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength)
{
  return kUnsupportedErr;
}

OSStatus MicoFlashFinalize( mico_flash_t flash )
{
  return kUnsupportedErr;
}

OSStatus internalFlashInitialize( void )
{ 
  platform_log_trace();

  return kNoErr;    
}

OSStatus internalFlashErase(uint32_t StartAddress, uint32_t EndAddress)
{
  platform_log_trace();
  return kNoErr;
}

#ifdef USE_MICO_SPI_FLASH
OSStatus spiFlashErase(uint32_t StartAddress, uint32_t EndAddress)
{
  platform_log_trace();
  return kNoErr;
}
#endif


OSStatus internalFlashWrite(volatile uint32_t* FlashAddress, uint32_t* Data ,uint32_t DataLength)
{
  platform_log_trace();
  return kNoErr;
}

OSStatus internalFlashFinalize( void )
{
  return kNoErr;
}


OSStatus internalFlashByteWrite(__IO uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength)
{
  platform_log_trace();
  return kNoErr;
}

/**
* @brief  Returns the write protection status of user flash area.
* @param  None
* @retval 0: No write protected sectors inside the user flash area
*         1: Some sectors inside the user flash area are write protected
*/
uint16_t _PlatformFlashGetWriteProtectionStatus(void)
{
  platform_log_trace();
  return kNoErr;
}

/**
* @brief  Disables the write protection of user flash area.
* @param  None
* @retval 1: Write Protection successfully disabled
*         2: Error: Flash write unprotection failed
*/
uint32_t _PlatformFlashDisableWriteProtection(void)
{
  platform_log_trace();
  return kNoErr;
}

/**
* @brief  Gets the sector of a given address
* @param  Address: Flash address
* @retval The sector of a given address
*/
static uint32_t _GetSector(uint32_t Address)
{
    platform_log_trace();
  return kNoErr;
}
