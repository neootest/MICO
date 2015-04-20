/**
******************************************************************************
* @file    mico_driver_adc.c 
* @author  william xu
* @version V1.0.0
* @date    05-may-2014
* @brief   this file provides flash operation functions.
******************************************************************************
*
*  the MIT license
*  copyright (c) 2014 MXCHIP inc.
*
*  permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "software"), to deal
*  in the software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the software, and to permit persons to whom the software is furnished
*  to do so, subject to the following conditions:
*
*  the above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/ 

/* includes ------------------------------------------------------------------*/
#include "platform_logging.h"
#include "mico_platform.h"
#include "stdio.h"
#ifdef USE_MICO_SPI_FLASH
#include "spi_flash.h"
#endif

/* private constants --------------------------------------------------------*/
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* base @ of sector 0, 16 kbyte */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* base @ of sector 1, 16 kbyte */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* base @ of sector 2, 16 kbyte */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* base @ of sector 3, 16 kbyte */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* base @ of sector 4, 64 kbyte */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* base @ of sector 5, 128 kbyte */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* base @ of sector 6, 128 kbyte */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* base @ of sector 7, 128 kbyte */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* base @ of sector 8, 128 kbyte */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* base @ of sector 9, 128 kbyte */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* base @ of sector 10, 128 kbyte */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* base @ of sector 11, 128 kbyte */

/* end of the flash address */
#define FLASH_START_ADDRESS     (uint32_t)0x08000000  
#define FLASH_END_ADDRESS       (uint32_t)0x080FFFFF
#define FLASH_SIZE              (FLASH_END_ADDRESS -  FLASH_START_ADDRESS + 1)
/* private typedef -----------------------------------------------------------*/
/* private define ------------------------------------------------------------*/
/* private macro -------------------------------------------------------------*/
/* private variables ---------------------------------------------------------*/
#ifdef USE_MICO_SPI_FLASH
static sflash_handle_t sflash_handle = {0x0, 0x0, SFLASH_WRITE_NOT_ALLOWED};
#endif
/* private function prototypes -----------------------------------------------*/
static uint32_t get_sector( uint32_t address );
static OSStatus internal_flash_initialize( void );
static OSStatus internal_flash_erase(uint32_t start_address, uint32_t end_address);
static OSStatus internal_flash_write(volatile uint32_t* flash_address, uint32_t* data ,uint32_t data_length);
static OSStatus internal_flash_byte_write( volatile uint32_t* flash_address, uint8_t* data ,uint32_t data_length );
static OSStatus internal_flash_finalize( void );
#ifdef USE_MICO_SPI_FLASH
static OSStatus spi_flash_erase(uint32_t start_address, uint32_t end_address);
#endif


const char* flash_name[] =
{ 
#ifdef USE_MICO_SPI_FLASH
  [MICO_SPI_FLASH] = "SPI", 
#endif
  [MICO_INTERNAL_FLASH] = "internal",
};

OSStatus mico_flash_initialize( mico_flash_t flash )
{ 
  platform_log_trace();
  if(flash == MICO_INTERNAL_FLASH){
  }
#ifdef USE_MICO_SPI_FLASH
  else if(flash == MICO_SPI_FLASH){
  }
#endif
  else
    return kUnsupportedErr;
}

OSStatus mico_flash_erase( mico_flash_t flash, uint32_t start_address, uint32_t end_address )
{ 
  platform_log_trace();
  if(flash == MICO_INTERNAL_FLASH){
    if(start_address<INTERNAL_FLASH_START_ADDRESS || end_address > INTERNAL_FLASH_END_ADDRESS)
      return kParamErr;
    return internal_flash_erase(start_address, end_address);    
  }
#ifdef USE_MICO_SPI_FLASH
  else if(flash == MICO_SPI_FLASH){
    if(start_address>=end_address || end_address > SPI_FLASH_END_ADDRESS)
      return kParamErr;
    return spi_flash_erase(start_address, end_address); 
  }
#endif
  else
    return kUnsupportedErr;
}

OSStatus mico_flash_write(mico_flash_t flash, volatile uint32_t* flash_address, uint8_t* data ,uint32_t data_length)
{
  if(flash == MICO_INTERNAL_FLASH){
    if( *flash_address<INTERNAL_FLASH_START_ADDRESS || *flash_address + data_length > INTERNAL_FLASH_END_ADDRESS + 1)
      return kParamErr;
    return internal_flash_write(flash_address, (uint32_t *)data, data_length);    
  }
#ifdef USE_MICO_SPI_FLASH
  else if(flash == MICO_SPI_FLASH){
    if( *flash_address + data_length > SPI_FLASH_END_ADDRESS + 1)
      return kParamErr;
    return return_val;
  }
#endif
  else
    return kUnsupportedErr;
}

OSStatus mico_flash_read(mico_flash_t flash, volatile uint32_t* flash_address, uint8_t* data ,uint32_t data_length)
{
  
  if(flash == MICO_INTERNAL_FLASH){
    if( *flash_address<INTERNAL_FLASH_START_ADDRESS || *flash_address + data_length > INTERNAL_FLASH_END_ADDRESS + 1)
      return kParamErr;
    memcpy(data, (void *)(*flash_address), data_length);
    *flash_address += data_length;
    return kNoErr;
  }
#ifdef USE_MICO_SPI_FLASH
  else if(flash == MICO_SPI_FLASH){
    if( *flash_address + data_length > SPI_FLASH_END_ADDRESS + 1)
      return kParamErr;
    return return_val;
  }
#endif
  else
    return kUnsupportedErr;
}

OSStatus mico_flash_finalize( mico_flash_t flash )
{
  if(flash == MICO_INTERNAL_FLASH){
    return internal_flash_finalize();    
  }
#ifdef USE_MICO_SPI_FLASH
  else if(flash == MICO_SPI_FLASH){
    return kNoErr;
  }
#endif
  else
    return kUnsupportedErr;
}

OSStatus internal_flash_initialize( void )
{ 
  platform_log_trace();
  return kNoErr;    
}

OSStatus internal_flash_erase(uint32_t start_address, uint32_t end_address)
{
  platform_log_trace();
  OSStatus err = kNoErr;
  uint32_t start_sector, end_sector, i = 0;
  
  /* get the sector where start the user flash area */
  start_sector = get_sector(start_address);
  end_sector = get_sector(end_address);
  
  for(i = start_sector; i <= end_sector; i += 8)
  {
    /* device voltage range supposed to be [2.7V to 3.6V], the operation will
    be done by word */
  }
  
exit:
  return err;
}

#ifdef USE_MICO_SPI_FLASH
OSStatus spi_flash_erase(uint32_t start_address, uint32_t end_address)
{
  platform_log_trace();
  OSStatus err = kNoErr;
  uint32_t start_sector, end_sector, i = 0;
  
  /* get the sector where start the user flash area */
  start_sector = start_address>>12;
  end_sector = end_address>>12;
  
  for(i = start_sector; i <= end_sector; i += 1)
  {
    /* device voltage range supposed to be [2.7V to 3.6V], the operation will
    be done by word */
  }
  
exit:
  return err;
}
#endif


OSStatus internal_flash_write(volatile uint32_t* flash_address, uint32_t* data ,uint32_t data_length)
{
  platform_log_trace();
  OSStatus err = kNoErr;
  uint32_t i = 0;
  uint32_t data_in_ram;
  uint8_t start_number;
  uint32_t data_length32 = data_length;
  
  /*first bytes that are not 32bit align*/
  if(*flash_address%4){
    start_number = 4-(*flash_address)%4;
    err = internal_flash_byte_write(flash_address, (uint8_t *)data, start_number);
    require_noerr(err, exit);
    data_length32 = data_length - start_number;
    data = (uint32_t *)((uint32_t)data + start_number);
  }
  
  /*program flash by words*/
  for (i = 0; (i < data_length32/4) && (*flash_address <= (FLASH_END_ADDRESS-3)); i++)
  {
    /* device voltage range supposed to be [2.7V to 3.6V], the operation will
    be done by word */ 
    data_in_ram = *(data+i);
    //require_action(FLASH_ProgramWord(*flash_address, data_in_ram) == FLASH_COMPLETE, exit, err = kWriteErr); 
    require_action(*(uint32_t*)*flash_address == data_in_ram, exit, err = kChecksumErr); 
    /* increment FLASH destination address */
    *flash_address += 4;
  }
  
  /*last bytes that cannot be write by a 32 bit word*/
  err = internal_flash_byte_write(flash_address, (uint8_t *)data + i*4, data_length32-i*4);
  require_noerr(err, exit);
  
exit:
  return err;
}

OSStatus internal_flash_finalize( void )
{
  return kNoErr;
}


OSStatus internal_flash_byte_write(__IO uint32_t* flash_address, uint8_t* data ,uint32_t data_length)
{
  uint32_t i = 0;
  uint32_t data_in_ram;
  OSStatus err = kNoErr;
  
  for (i = 0; (i < data_length) && (*flash_address <= (FLASH_END_ADDRESS)); i++)
  {
    /* device voltage range supposed to be [2.7V to 3.6V], the operation will
    be done by word */ 
    data_in_ram = *(uint8_t*)(data+i);
    
    //require_action(FLASH_ProgramByte(*flash_address, data_in_ram) == FLASH_COMPLETE, exit, err = kWriteErr); 
    require_action(*(uint8_t*)*flash_address == data_in_ram, exit, err = kChecksumErr); 
    *flash_address +=1;
  }
  
exit:
  return err;
}

/**
* @brief  returns the write protection status of user flash area.
* @param  none
* @retval 0: no write protected sectors inside the user flash area
*         1: some sectors inside the user flash area are write protected
*/
uint16_t platform_flash_get_write_protection_status(void)
{
  uint32_t user_start_sector ;//= FLASH_Sector_1;
  
  /* get the sector where start the user flash area */
  user_start_sector = get_sector(APPLICATION_START_ADDRESS);
  
  /* check if there are write protected sectors inside the user flash area */
 // if ((FLASH_OB_GetWRP() >> (user_start_sector/8)) == (0xFFF >> (user_start_sector/8)))
 // { /* no write protected sectors inside the user flash area */
 //   return 1;
 // }
 // else
 // { /* some sectors inside the user flash area are write protected */
    return 0;
 // }
}

/**
* @brief  disables the write protection of user flash area.
* @param  none
* @retval 1: write protection successfully disabled
*         2: error: flash write unprotection failed
*/
uint32_t platform_flash_disable_write_protection(void)
{
 // __IO uint32_t user_start_sector = FLASH_Sector_1, user_wrp_sectors = OB_WRP_Sector_1;
  
  /* get the sector where start the user flash area */
 // user_start_sector = get_sector(APPLICATION_START_ADDRESS);
  
  /* mark all sectors inside the user flash area as non protected */  
//  user_wrp_sectors = 0xFFF-((1 << (user_start_sector/8))-1);
  
  /* Unlock the Option Bytes */
  
  /* disable the write protection for all sectors inside the user flash area */
  
  /* start the option bytes programming process. */  
  
  /* write protection successfully disabled */
  return (1);
}

/**
* @brief  gets the sector of a given address
* @param  address: flash address
* @retval the sector of a given address
*/
static uint32_t get_sector(uint32_t address)
{
  uint32_t sector = 0;
  
  if((address < ADDR_FLASH_SECTOR_1) && (address >= ADDR_FLASH_SECTOR_0))
  {
  //  sector = FLASH_Sector_0;  
  }
  else if((address < ADDR_FLASH_SECTOR_2) && (address >= ADDR_FLASH_SECTOR_1))
  {
   // sector = FLASH_Sector_1;  
  }
  else if((address < ADDR_FLASH_SECTOR_3) && (address >= ADDR_FLASH_SECTOR_2))
  {
    //sector = FLASH_Sector_2;  
  }
  else if((address < ADDR_FLASH_SECTOR_4) && (address >= ADDR_FLASH_SECTOR_3))
  {
   // sector = FLASH_Sector_3;  
  }
  else if((address < ADDR_FLASH_SECTOR_5) && (address >= ADDR_FLASH_SECTOR_4))
  {
   // sector = FLASH_Sector_4;  
  }
  else if((address < ADDR_FLASH_SECTOR_6) && (address >= ADDR_FLASH_SECTOR_5))
  {
   // sector = FLASH_Sector_5;  
  }
  else if((address < ADDR_FLASH_SECTOR_7) && (address >= ADDR_FLASH_SECTOR_6))
  {
   // sector = FLASH_Sector_6;  
  }
  else if((address < ADDR_FLASH_SECTOR_8) && (address >= ADDR_FLASH_SECTOR_7))
  {
   // sector = FLASH_Sector_7;  
  }
  else if((address < ADDR_FLASH_SECTOR_9) && (address >= ADDR_FLASH_SECTOR_8))
  {
   // sector = FLASH_Sector_8;  
  }
  else if((address < ADDR_FLASH_SECTOR_10) && (address >= ADDR_FLASH_SECTOR_9))
  {
  //  sector = FLASH_Sector_9;  
  }
  else if((address < ADDR_FLASH_SECTOR_11) && (address >= ADDR_FLASH_SECTOR_10))
  {
   // sector = FLASH_Sector_10;  
  }
  else/*(address < FLASH_END_ADDR) && (address >= ADDR_FLASH_SECTOR_11))*/
  {
   // sector = FLASH_Sector_11;  
  }
  return sector;
}
