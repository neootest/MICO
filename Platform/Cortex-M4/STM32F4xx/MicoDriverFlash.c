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
#include "stm32f4xx.h"
#include "stdio.h"
#ifdef USE_MICO_SPI_FLASH
#include "spi_flash.h"
#endif

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
  if(flash == MICO_INTERNAL_FLASH){
    if(StartAddress<INTERNAL_FLASH_START_ADDRESS || EndAddress > INTERNAL_FLASH_END_ADDRESS)
      return kParamErr;
    return internalFlashErase(StartAddress, EndAddress);    
  }
#ifdef USE_MICO_SPI_FLASH
  else if(flash == MICO_SPI_FLASH){
    if(StartAddress>=EndAddress || EndAddress > SPI_FLASH_END_ADDRESS)
      return kParamErr;
    return spiFlashErase(StartAddress, EndAddress); 
  }
#endif
  else
    return kUnsupportedErr;
}

OSStatus MicoFlashWrite(mico_flash_t flash, volatile uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength)
{
  if(flash == MICO_INTERNAL_FLASH){
    if( *FlashAddress<INTERNAL_FLASH_START_ADDRESS || *FlashAddress + DataLength > INTERNAL_FLASH_END_ADDRESS + 1)
      return kParamErr;
    return internalFlashWrite(FlashAddress, (uint32_t *)Data, DataLength);    
  }
#ifdef USE_MICO_SPI_FLASH
  else if(flash == MICO_SPI_FLASH){
    if( *FlashAddress + DataLength > SPI_FLASH_END_ADDRESS + 1)
      return kParamErr;
    int returnVal = sflash_write( &sflash_handle, *FlashAddress, Data, DataLength );
    *FlashAddress += DataLength;
    return returnVal;
  }
#endif
  else
    return kUnsupportedErr;
}

OSStatus MicoFlashRead(mico_flash_t flash, volatile uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength)
{
  
  if(flash == MICO_INTERNAL_FLASH){
    if( *FlashAddress<INTERNAL_FLASH_START_ADDRESS || *FlashAddress + DataLength > INTERNAL_FLASH_END_ADDRESS + 1)
      return kParamErr;
    memcpy(Data, (void *)(*FlashAddress), DataLength);
    *FlashAddress += DataLength;
    return kNoErr;
  }
#ifdef USE_MICO_SPI_FLASH
  else if(flash == MICO_SPI_FLASH){
    if( *FlashAddress + DataLength > SPI_FLASH_END_ADDRESS + 1)
      return kParamErr;
    int returnVal = sflash_read( &sflash_handle, *FlashAddress, Data, DataLength );
    *FlashAddress += DataLength;
    return returnVal;
  }
#endif
  else
    return kUnsupportedErr;
}

OSStatus MicoFlashFinalize( mico_flash_t flash )
{
  if(flash == MICO_INTERNAL_FLASH){
    return internalFlashFinalize();    
  }
#ifdef USE_MICO_SPI_FLASH
  else if(flash == MICO_SPI_FLASH){
    sflash_handle.device_id = 0x0;
    return kNoErr;
  }
#endif
  else
    return kUnsupportedErr;
}

OSStatus internalFlashInitialize( void )
{ 
  platform_log_trace();
  FLASH_Unlock(); 
  /* Clear pending flags (if any) */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
  return kNoErr;    
}

OSStatus internalFlashErase(uint32_t StartAddress, uint32_t EndAddress)
{
  platform_log_trace();
  OSStatus err = kNoErr;
  uint32_t StartSector, EndSector, i = 0;
  
  /* Get the sector where start the user flash area */
  StartSector = _GetSector(StartAddress);
  EndSector = _GetSector(EndAddress);
  
  for(i = StartSector; i <= EndSector; i += 8)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
    be done by word */
    require_action(FLASH_EraseSector(i, VoltageRange_3) == FLASH_COMPLETE, exit, err = kWriteErr); 
  }
  
exit:
  return err;
}

#ifdef USE_MICO_SPI_FLASH
OSStatus spiFlashErase(uint32_t StartAddress, uint32_t EndAddress)
{
  platform_log_trace();
  OSStatus err = kNoErr;
  uint32_t StartSector, EndSector, i = 0;
  
  /* Get the sector where start the user flash area */
  StartSector = StartAddress>>12;
  EndSector = EndAddress>>12;
  
  for(i = StartSector; i <= EndSector; i += 1)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
    be done by word */
    require_action(sflash_sector_erase(&sflash_handle, i<<12) == kNoErr, exit, err = kWriteErr); 
  }
  
exit:
  return err;
}
#endif


OSStatus internalFlashWrite(volatile uint32_t* FlashAddress, uint32_t* Data ,uint32_t DataLength)
{
  platform_log_trace();
  OSStatus err = kNoErr;
  uint32_t i = 0;
  uint32_t dataInRam;
  u8 startNumber;
  uint32_t DataLength32 = DataLength;
  
  /*First bytes that are not 32bit align*/
  if(*FlashAddress%4){
    startNumber = 4-(*FlashAddress)%4;
    err = internalFlashByteWrite(FlashAddress, (uint8_t *)Data, startNumber);
    require_noerr(err, exit);
    DataLength32 = DataLength - startNumber;
    Data = (uint32_t *)((u32)Data + startNumber);
  }
  
  /*Program flash by words*/
  for (i = 0; (i < DataLength32/4) && (*FlashAddress <= (FLASH_END_ADDRESS-3)); i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
    be done by word */ 
    dataInRam = *(Data+i);
    require_action(FLASH_ProgramWord(*FlashAddress, dataInRam) == FLASH_COMPLETE, exit, err = kWriteErr); 
    require_action(*(uint32_t*)*FlashAddress == dataInRam, exit, err = kChecksumErr); 
    /* Increment FLASH destination address */
    *FlashAddress += 4;
  }
  
  /*Last bytes that cannot be write by a 32 bit word*/
  err = internalFlashByteWrite(FlashAddress, (uint8_t *)Data + i*4, DataLength32-i*4);
  require_noerr(err, exit);
  
exit:
  return err;
}

OSStatus internalFlashFinalize( void )
{
  FLASH_Lock();
  return kNoErr;
}


OSStatus internalFlashByteWrite(__IO uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength)
{
  uint32_t i = 0;
  uint32_t dataInRam;
  OSStatus err = kNoErr;
  
  for (i = 0; (i < DataLength) && (*FlashAddress <= (FLASH_END_ADDRESS)); i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
    be done by word */ 
    dataInRam = *(uint8_t*)(Data+i);
    
    require_action(FLASH_ProgramByte(*FlashAddress, dataInRam) == FLASH_COMPLETE, exit, err = kWriteErr); 
    require_action(*(uint8_t*)*FlashAddress == dataInRam, exit, err = kChecksumErr); 
    *FlashAddress +=1;
  }
  
exit:
  return err;
}

/**
* @brief  Returns the write protection status of user flash area.
* @param  None
* @retval 0: No write protected sectors inside the user flash area
*         1: Some sectors inside the user flash area are write protected
*/
uint16_t _PlatformFlashGetWriteProtectionStatus(void)
{
  uint32_t UserStartSector = FLASH_Sector_1;
  
  /* Get the sector where start the user flash area */
  UserStartSector = _GetSector(APPLICATION_START_ADDRESS);
  
  /* Check if there are write protected sectors inside the user flash area */
  if ((FLASH_OB_GetWRP() >> (UserStartSector/8)) == (0xFFF >> (UserStartSector/8)))
  { /* No write protected sectors inside the user flash area */
    return 1;
  }
  else
  { /* Some sectors inside the user flash area are write protected */
    return 0;
  }
}

/**
* @brief  Disables the write protection of user flash area.
* @param  None
* @retval 1: Write Protection successfully disabled
*         2: Error: Flash write unprotection failed
*/
uint32_t _PlatformFlashDisableWriteProtection(void)
{
  __IO uint32_t UserStartSector = FLASH_Sector_1, UserWrpSectors = OB_WRP_Sector_1;
  
  /* Get the sector where start the user flash area */
  UserStartSector = _GetSector(APPLICATION_START_ADDRESS);
  
  /* Mark all sectors inside the user flash area as non protected */  
  UserWrpSectors = 0xFFF-((1 << (UserStartSector/8))-1);
  
  /* Unlock the Option Bytes */
  FLASH_OB_Unlock();
  
  /* Disable the write protection for all sectors inside the user flash area */
  FLASH_OB_WRPConfig(UserWrpSectors, DISABLE);
  
  /* Start the Option Bytes programming process. */  
  if (FLASH_OB_Launch() != FLASH_COMPLETE)
  {
    /* Error: Flash write unprotection failed */
    return (2);
  }
  
  /* Write Protection successfully disabled */
  return (1);
}

/**
* @brief  Gets the sector of a given address
* @param  Address: Flash address
* @retval The sector of a given address
*/
static uint32_t _GetSector(uint32_t Address)
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_Sector_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_Sector_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_Sector_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_Sector_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_Sector_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_Sector_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_Sector_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_Sector_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_Sector_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_Sector_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_Sector_10;  
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = FLASH_Sector_11;  
  }
  return sector;
}
