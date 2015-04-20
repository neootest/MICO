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
//#include "platform_config.h"
#include "stdio.h"
#ifdef USE_MICO_SPI_FLASH
#include "spi_flash.h"
#endif
#include "flash_efc.h"

/* Private constants --------------------------------------------------------*/
#define ADDR_FLASH_SECTOR_00     ((uint32_t)0x00400000) /* base @ of sector 0, 8 kbyte */
#define ADDR_FLASH_SECTOR_01     ((uint32_t)0x00402000) /* base @ of sector 1, 8 kbyte */
#define ADDR_FLASH_SECTOR_02     ((uint32_t)0x00404000) /* base @ of sector 2, 112 kbyte */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x00420000) /* base @ of sector 1, 128 kbyte */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x00440000) /* base @ of sector 2, 128 kbyte */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x00480000) /* base @ of sector 3, 128 kbyte */

/* End of the Flash address */
#define FLASH_START_ADDRESS     IFLASH_ADDR //internal flash
#define FLASH_END_ADDRESS       (IFLASH_ADDR + IFLASH_SIZE - 1)
#define FLASH_SIZE              IFLASH_SIZE


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

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
          
/* Private variables ---------------------------------------------------------*/
#ifdef USE_MICO_SPI_FLASH
static sflash_handle_t sflash_handle = {0x0, 0x0, SFLASH_WRITE_NOT_ALLOWED};
#endif
/* Private function prototypes -----------------------------------------------*/
static uint32_t _GetSector( uint32_t Address );
static OSStatus _GetAddress(uint32_t sector, uint32_t *startAddress, uint32_t *endAddress);
static OSStatus internalFlashInitialize( void );
static OSStatus internalFlashErase(uint32_t StartAddress, uint32_t EndAddress);
static OSStatus internalFlashWrite(volatile uint32_t* FlashAddress, uint32_t* Data ,uint32_t DataLength);
static OSStatus internalFlashByteWrite( volatile uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength );
static OSStatus internalFlashFinalize( void );
#ifdef USE_MICO_SPI_FLASH
static OSStatus spiFlashErase(uint32_t StartAddress, uint32_t EndAddress);
#endif

OSStatus platform_flash_init( platform_flash_driver_t *driver, const platform_flash_t *peripheral )
{
  OSStatus err = kNoErr;

  require_action_quiet( driver != NULL && peripheral != NULL, exit, err = kParamErr);
  require_action_quiet( driver->initialized == false, exit, err = kNoErr);

  driver->peripheral = (platform_flash_t *)peripheral;

  if( driver->peripheral->flash_type == FLASH_TYPE_INTERNAL ){
    err = internalFlashInitialize();
    require_noerr(err, exit);
  }
#ifdef USE_MICO_SPI_FLASH
  else if( driver->peripheral->flash_type == FLASH_TYPE_SPI ){
    err = init_sflash( &sflash_handle, 0, SFLASH_WRITE_ALLOWED );
    require_noerr(err, exit);
  }
#endif
  else{
    err = kTypeErr;
    goto exit;
  }
#ifndef NO_MICO_RTOS 
  err = mico_rtos_init_mutex( &driver->flash_mutex );
#endif
  require_noerr(err, exit);
  driver->initialized = true;

exit:
  return err;
}

OSStatus platform_flash_erase( platform_flash_driver_t *driver, uint32_t StartAddress, uint32_t EndAddress  )
{
  OSStatus err = kNoErr;

  require_action_quiet( driver != NULL, exit, err = kParamErr);
  require_action_quiet( driver->initialized != false, exit, err = kNotInitializedErr);
  require_action( StartAddress >= driver->peripheral->flash_start_addr 
               && EndAddress   <= driver->peripheral->flash_start_addr + driver->peripheral->flash_length - 1, exit, err = kParamErr);
#ifndef NO_MICO_RTOS 
  mico_rtos_lock_mutex( &driver->flash_mutex );
#endif
  if( driver->peripheral->flash_type == FLASH_TYPE_INTERNAL ){
    err = internalFlashErase(StartAddress, EndAddress);    
    require_noerr(err, exit_with_mutex);
  }
#ifdef USE_MICO_SPI_FLASH
  else if( driver->peripheral->flash_type == FLASH_TYPE_SPI ){
    err = spiFlashErase(StartAddress, EndAddress);
    require_noerr(err, exit_with_mutex);
  }
#endif
  else{
    err = kTypeErr;
    goto exit_with_mutex;
  }

exit_with_mutex: 
#ifndef NO_MICO_RTOS 
  mico_rtos_unlock_mutex( &driver->flash_mutex );
#endif
exit:
  return err;
}

OSStatus platform_flash_write( platform_flash_driver_t *driver, volatile uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength  )
{
  OSStatus err = kNoErr;

  require_action_quiet( driver != NULL, exit, err = kParamErr);
  require_action_quiet( driver->initialized != false, exit, err = kNotInitializedErr);
  require_action( *FlashAddress >= driver->peripheral->flash_start_addr 
               && *FlashAddress + DataLength <= driver->peripheral->flash_start_addr + driver->peripheral->flash_length, exit, err = kParamErr);
#ifndef NO_MICO_RTOS 
  mico_rtos_lock_mutex( &driver->flash_mutex );
#endif
  if( driver->peripheral->flash_type == FLASH_TYPE_INTERNAL ){
    err = internalFlashWrite(FlashAddress, (uint32_t *)Data, DataLength); 
    require_noerr(err, exit_with_mutex);
  }
#ifdef USE_MICO_SPI_FLASH
  else if( driver->peripheral->flash_type == FLASH_TYPE_SPI ){
    err = sflash_write( &sflash_handle, *FlashAddress, Data, DataLength );
    require_noerr(err, exit_with_mutex);
    *FlashAddress += DataLength;
  }
#endif
  else{
    err = kTypeErr;
    goto exit_with_mutex;
  }

exit_with_mutex: 
#ifndef NO_MICO_RTOS 
  mico_rtos_unlock_mutex( &driver->flash_mutex );
#endif

exit:
  return err;
}

OSStatus platform_flash_read( platform_flash_driver_t *driver, volatile uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength  )
{
  OSStatus err = kNoErr;

  require_action_quiet( driver != NULL, exit, err = kParamErr);
  require_action_quiet( driver->initialized != false, exit, err = kNotInitializedErr);
  require_action( (*FlashAddress >= driver->peripheral->flash_start_addr) 
               && (*FlashAddress + DataLength) <= (driver->peripheral->flash_start_addr + driver->peripheral->flash_length), exit, err = kParamErr);
#ifndef NO_MICO_RTOS 
  mico_rtos_lock_mutex( &driver->flash_mutex );
#endif
  if( driver->peripheral->flash_type == FLASH_TYPE_INTERNAL ){
    memcpy(Data, (void *)(*FlashAddress), DataLength);
    *FlashAddress += DataLength;
  }
#ifdef USE_MICO_SPI_FLASH
  else if( driver->peripheral->flash_type == FLASH_TYPE_SPI ){
    err = sflash_read( &sflash_handle, *FlashAddress, Data, DataLength );
    require_noerr(err, exit_with_mutex);
    *FlashAddress += DataLength;
  }
#endif
  else{
    err = kTypeErr;
    goto exit_with_mutex;
  }

exit_with_mutex: 
#ifndef NO_MICO_RTOS 
  mico_rtos_unlock_mutex( &driver->flash_mutex );
#endif
exit:
  return err;
}

OSStatus platform_flash_deinit( platform_flash_driver_t *driver)
{
  OSStatus err = kNoErr;

  require_action_quiet( driver != NULL, exit, err = kParamErr);

  driver->initialized = false;
#ifndef NO_MICO_RTOS 
  mico_rtos_deinit_mutex( &driver->flash_mutex );
#endif

  if( driver->peripheral->flash_type == FLASH_TYPE_INTERNAL){
    err = internalFlashFinalize();   
    require_noerr(err, exit); 
  }
#ifdef USE_MICO_SPI_FLASH
  else if( driver->peripheral->flash_type == FLASH_TYPE_SPI ){
    sflash_handle.device_id = 0x0;
  }
#endif
  else
    return kUnsupportedErr;

  
exit:
  
  return err;
}

OSStatus internalFlashInitialize( void )
{ 
  platform_log_trace();
  uint32_t ul_rc;
  
  ul_rc = flash_init(FLASH_ACCESS_MODE_128, 6);//TBD! 3-4 wait in 128bit, 1 in 64bit
  if (ul_rc != FLASH_RC_OK) {
    platform_log("flash err:%d",ul_rc);
    return kGeneralErr;
    }
  
  return kNoErr; 
}

OSStatus internalFlashErase(uint32_t start_address, uint32_t end_address)
{
  platform_log_trace();
  OSStatus err = kNoErr;
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
  return err;
}



OSStatus internalFlashWrite(volatile uint32_t* flash_address, uint32_t* data ,uint32_t data_length)
{
  platform_log_trace();
  OSStatus err = kNoErr;
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
exit:
  return err;
}

OSStatus internalFlashFinalize( void )
{
  return kNoErr;
}
