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
#define ASF_SERVICE_FLASH
/* includes ------------------------------------------------------------------*/
#include "platform_logging.h"
#include "mico_platform.h"
#include "stdio.h"
#ifdef USE_MICO_SPI_FLASH
#include "spi_flash.h"
#endif
#ifdef ASF_SERVICE_FLASH
#include "flash_efc.h"
#endif

#ifndef ASF_SERVICE_FLASH
/* private constants --------------------------------------------------------*/
#define ADDR_FLASH_SECTOR_00     ((uint32_t)0x00400000) /* base @ of sector 0, 8 kbyte */
#define ADDR_FLASH_SECTOR_01     ((uint32_t)0x00402000) /* base @ of sector 1, 8 kbyte */
#define ADDR_FLASH_SECTOR_02     ((uint32_t)0x00404000) /* base @ of sector 2, 112 kbyte */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x00420000) /* base @ of sector 1, 128 kbyte */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x00440000) /* base @ of sector 2, 128 kbyte */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x00480000) /* base @ of sector 3, 128 kbyte */
#endif
/* end of the flash address */
#define FLASH_START_ADDRESS     IFLASH_ADDR //internal flash
#define FLASH_END_ADDRESS       (IFLASH_ADDR + IFLASH_SIZE - 1)
#define FLASH_SIZE              IFLASH_SIZE
/* private typedef -----------------------------------------------------------*/

/* critical disable irq */
#define mem_flash_op_enter() do {\
		__DSB(); __ISB();        \
		cpu_irq_disable();       \
		__DMB();                 \
	} while (0)
/* critical enable irq */
#define mem_flash_op_exit() do {  \
		__DMB(); __DSB(); __ISB();\
		cpu_irq_enable();         \
	} while (0)

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
  [MICO_INTERNAL_FLASH] = "Internal",
};

OSStatus mico_flash_initialize( mico_flash_t flash )
{ 
  platform_log_trace();
  if(flash == MICO_INTERNAL_FLASH){
      return internal_flash_initialize();
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
    uint32_t ul_rc;
#ifdef ASF_SERVICE_FLASH
    ul_rc = flash_init(FLASH_ACCESS_MODE_128, 6);//TBD! 3-4 wait in 128bit, 1 in 64bit
    if (ul_rc != FLASH_RC_OK) {
        platform_log("flash err:%d",ul_rc);
        return kGeneralErr;
    }
#else

#endif
  return kNoErr;    
}

OSStatus internal_flash_erase(uint32_t start_address, uint32_t end_address)
{
  platform_log_trace();
  OSStatus err = kNoErr;
#ifdef ASF_SERVICE_FLASH
	uint32_t page_addr = start_address;
	uint32_t page_off  = page_addr % (IFLASH_PAGE_SIZE*16);
	uint32_t rc, erased = 0;
    uint32_t size = end_address - start_address;
	if (page_off) {
		platform_log("flash: erase address must be 16 page aligned");
		page_addr = page_addr - page_off;
		platform_log("flash: erase from %x", (unsigned)page_addr);
	}
	for (erased = 0; erased < size;) {
		rc = flash_erase_page((uint32_t)page_addr, IFLASH_ERASE_PAGES_16);
		erased    += IFLASH_PAGE_SIZE*16;
		page_addr += IFLASH_PAGE_SIZE*16;
		if (rc != FLASH_RC_OK) {
			platform_log("flash: %x erase error", (unsigned)page_addr);
			return kGeneralErr;
		}
	}
#else  
  uint32_t start_sector, end_sector, i = 0;
  /* get the sector where start the user flash area */
  start_sector = get_sector(start_address);
  end_sector = get_sector(end_address);
  
  for(i = start_sector; i <= end_sector; i += 8)
  {
    /* device voltage range supposed to be [2.7V to 3.6V], the operation will
    be done by word */
  }
#endif
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
#ifdef ASF_SERVICE_FLASH
  //page write length IFLASH_PAGE_SIZE
    uint32_t rc ;
    mem_flash_op_enter();
#if SAMG55
    rc = flash_write((*flash_address), data, data_length, false);
#else 
    rc = flash_write((*flash_address), data, data_length, true);
#endif
    mem_flash_op_exit();
    if (rc != FLASH_RC_OK) {
        platform_log("flash err: %d",rc);
        return kGeneralErr;
    }
    *flash_address += data_length;

#else
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
    if (flash_write((uint32_t)flash_address, &data_in_ram, 4, false)) {
        return kGeneralErr;
    }
    //require_action(FLASH_ProgramWord(*flash_address, data_in_ram) == FLASH_COMPLETE, exit, err = kWriteErr); 
    require_action(*(uint32_t*)*flash_address == data_in_ram, exit, err = kChecksumErr); 
    /* increment FLASH destination address */
    *flash_address += 4;
  }
  
  /*last bytes that cannot be write by a 32 bit word*/
  err = internal_flash_byte_write(flash_address, (uint8_t *)data + i*4, data_length32-i*4);
  require_noerr(err, exit);
#endif
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
  uint8_t *data_in_ram;
  OSStatus err = kNoErr;
  
  for (i = 0; (i < data_length) && (*flash_address <= (FLASH_END_ADDRESS)); i++)
  {
    /* device voltage range supposed to be [2.7V to 3.6V], the operation will
    be done by word */ 
    data_in_ram = (uint8_t*)(data+i);
    
    if (flash_write((uint32_t)flash_address, data_in_ram, 1, false)) {
        return kGeneralErr;
    }
    //require_action(FLASH_ProgramByte(*flash_address, data_in_ram) == FLASH_COMPLETE, exit, err = kWriteErr); 
    require_action(*(uint8_t*)*flash_address == *data_in_ram, exit, err = kChecksumErr); 
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
 //   return 0;
 // }
    return 1;
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
  return sector;
}
