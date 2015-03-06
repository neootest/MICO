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
#include "spi_flash.h"

//#define DEBUG_FLASH

#ifdef DEBUG_FLASH
#define APP_DBG printf
#endif
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

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

static bool FlashUnlock(void);
static bool FlashLock(SPI_FLASH_LOCK_RANGE lock_range);

const char* flash_name[] =
{ 
#ifdef USE_MICO_SPI_FLASH
  [MICO_SPI_FLASH] = "SPI", 
#endif
  [MICO_INTERNAL_FLASH] = "Internal",
};


#ifdef DEBUG_FLASH
SPI_FLASH_INFO  FlashInfo;

void GetFlashGD(int32_t protect)
{    
	uint8_t  str[20];
	int32_t FlashCapacity = 0;
		
	switch(FlashInfo.Did)
	{
		case 0x1340:	
			strcpy((char *)str,"GD25Q40(GD25Q40B)");
			FlashCapacity = 0x00080000;
			break;

		case 0x1240:
        	strcpy((char *)str,"GD25Q20(GD25Q20B)");
			FlashCapacity = 0x00040000;
			break;       

		case 0x1540:
			strcpy((char *)str,"GD25Q16(GD25Q16B)");
			FlashCapacity = 0x00200000;			
			break; 

		case 0x1640:
        	strcpy((char *)str,"GD25Q32(GD25Q32B)");
			FlashCapacity = 0x00400000;        
			break;

		case 0x1740:
        	strcpy((char *)str,"GD25Q64B");
			FlashCapacity = 0x00800000;          
			break;

		case 0x1440:
        	strcpy((char *)str,"GD25Q80(GD25Q80B)");
			FlashCapacity = 0x00100000;         
			break;

		case 0x1840:
            strcpy((char *)str,"GD25Q128B");
            FlashCapacity = 0x01000000;         
            break;

		default:
			break;
	}
        
    if(FlashCapacity > 0)
    {
        APP_DBG("Module:                ");
        APP_DBG("%s\r\n",str);
        APP_DBG("Capacity:                     ");
        APP_DBG("0x%08X\r\n", FlashCapacity);
    }  
    else
    {
        APP_DBG("Found failed\r\n");
    }
}

void GetFlashWinBound(int32_t protect)
{	
    uint8_t  str[20];
	int32_t FlashCapacity = 0;
    
	switch(FlashInfo.Did)
    {
        case 0x1440:
            strcpy((char *)str,"W25Q80BV");
            FlashCapacity = 0x00100000;             
            break;
        
        case 0x1760:
            strcpy((char *)str,"W25Q64DW");
            FlashCapacity = 0x00800000;             
            break;
        				
        case 0x1740:
            strcpy((char *)str,"W25Q64CV");
            FlashCapacity = 0x00800000; 
            break;
        
        default:
            break;
    }
    
    if(FlashCapacity > 0)
    {
        APP_DBG("Module:                ");
        APP_DBG("%s\r\n",str);
        APP_DBG("Capacity:                     ");
        APP_DBG("0x%08X\r\n", FlashCapacity);
    }  
    else
    {
        APP_DBG("Found failed\r\n");
    }
}

void GetFlashPct(void)
{	
    uint8_t  str[20];
	int32_t FlashCapacity = 0;
    
	switch(FlashInfo.Did)
    {
        case 0x0126:
            strcpy((char *)str,"PCT26VF016");
            FlashCapacity = 0x00200000;        
			break;

        case 0x0226:       
            strcpy((char *)str,"PCT26VF032");
            FlashCapacity = 0x00400000;
            break;

        default:            
			break;
    }
      
    if(FlashCapacity > 0)
    {
        APP_DBG("Module:                ");
        APP_DBG("%s\r\n",str);
        APP_DBG("Capacity:                     ");
        APP_DBG("0x%08X\r\n", FlashCapacity);
    }  
    else
    {
        APP_DBG("Found failed\r\n");
    }
}

void GetFlashEon(int32_t protect)
{
    uint8_t  str[20];
	int32_t FlashCapacity = 0;
    
	switch(FlashInfo.Did)
    {
        case 0x1430:
            strcpy((char *)str,"EN25Q80A");
            FlashCapacity = 0x00100000; 
            break;

        case 0x1530:
            strcpy((char *)str,"EN25Q16A");
            FlashCapacity = 0x00200000; 
            break;

        case 0x1830:
            strcpy((char *)str,"EN25Q128");
            FlashCapacity = 0x01000000; 
            break;

        case 0x1630:
            strcpy((char *)str,"EN25Q32A");
            FlashCapacity = 0x00400000; 
            break;

        case 0x1330:
            strcpy((char *)str,"EN25Q40");
            FlashCapacity = 0x00080000; 
            break;

        case 0x1730:
            strcpy((char *)str,"EN25Q64");
            FlashCapacity = 0x00800000; 
            break;

        case 0x1570:
            strcpy((char *)str,"EN25QH16");
            FlashCapacity = 0x00200000; 
            break;

        case 0x1670: 
            strcpy((char *)str,"EN25QH32");
            FlashCapacity = 0x00400000; 
            break;

        default:
            break;
    }
    
    if(FlashCapacity > 0)
    {
        APP_DBG("Module:                ");
        APP_DBG("%s\r\n",str);
        APP_DBG("Capacity:                     ");
        APP_DBG("0x%08X\r\n", FlashCapacity);
    }  
    else
    {
        APP_DBG("Found failed\r\n");
    }
}

void GetFlashBg(int32_t protect)
{	
    uint8_t  str[20];
	int32_t FlashCapacity = 0;
    
    switch(FlashInfo.Did)
    {
        case 0x1540:
            strcpy((char *)str,"BG25Q16A");
            FlashCapacity = 0x00200000; 
            break;

        case 0x1340:
            strcpy((char *)str,"BG25Q40A");
            FlashCapacity = 0x00080000; 
            break;
        
        case 0x1440:
            strcpy((char *)str,"BG25Q80A");
            FlashCapacity = 0x00100000; 
            break;
        
        default:
			break;         
    }
    
    if(FlashCapacity > 0)
    {
        APP_DBG("Module:                ");
        APP_DBG("%s\r\n",str);
        APP_DBG("Capacity:                     ");
        APP_DBG("0x%08X\r\n", FlashCapacity);
    }  
    else
    {
        APP_DBG("Found failed\r\n");
    }
}

void GetFlashEsmt(int32_t protect)
{
    uint8_t  str[20];
	int32_t FlashCapacity = 0;
       
	switch(FlashInfo.Did)
    {
        case 0x1440:
            strcpy((char *)str,"F25L08QA");
            FlashCapacity = 0x00100000; 
            break;

        case 0x1540:
            strcpy((char *)str,"F25L16QA");
            FlashCapacity = 0x00200000; 
            break;

        case 0x1641:
            strcpy((char *)str,"F25L32QA");
            FlashCapacity = 0x00400000; 
            break;

        case 0x1741:
            strcpy((char *)str,"F25L64QA");
            FlashCapacity = 0x00800000; 
            break;
              
        default:
            break;
    }    
    
    if(FlashCapacity > 0)
    {
        APP_DBG("Module:                ");
        APP_DBG("%s\r\n",str);
        APP_DBG("Capacity:                     ");
        APP_DBG("0x%08X\r\n", FlashCapacity);
    }  
    else
    {
        APP_DBG("Found failed\r\n");
    }
}

void GetDidInfo(int32_t protect)
{
	APP_DBG("%-30s","Did:");
	APP_DBG("0x%08X\r\n",FlashInfo.Did);
	APP_DBG("%-30s","Lock Area(BP4~BP0:Bit4~Bit0):");
	APP_DBG("0x%08X\r\n",protect);
}

void GetFlashInfo(void)
{
	int32_t protect = 0;
   
	APP_DBG("\r\n\r\n****************************************************************\n");
        APP_DBG("%-30s\r\n","Flash information");
	
	if(FlashInfo.Mid != FLASH_PCT)
	{
		protect = SpiFlashIOCtl(3,0);
		protect = (protect >> 2) & 0x1F;
	}
	
    switch(FlashInfo.Mid)
    {
        case FLASH_GD:
            APP_DBG("Manufacture:                         GD\r\n");
			GetDidInfo(protect);
            GetFlashGD(protect);
            break;
        
        case FLASH_WINBOUND:
            APP_DBG("Manufacture:                         WINBOUND\r\n");
			GetDidInfo(protect);
            GetFlashWinBound(protect);
            break;
        
        case FLASH_PCT:
            APP_DBG("Manufacture:                         PCT\r\n");
            GetFlashPct();
            break;
        
        case FLASH_EON:            
            APP_DBG("Manufacture:                         EN\r\n");
			GetDidInfo(protect);
            GetFlashEon(protect);
            break;
        
        case FLASH_BG:
            APP_DBG("Manufacture:                         BG\r\n");
			GetDidInfo(protect);
            GetFlashBg(protect);
            break;
        
        case FLASH_ESMT:
            APP_DBG("Manufacture:                         ESMT\r\n");
			GetDidInfo(protect);
            GetFlashEsmt(protect);
            break;
        
        default:            
            APP_DBG("Manufacture:                         not found\r\n");
            break;
    }
	APP_DBG("\r\n");
	APP_DBG("****************************************************************\n");
}

#endif /* DEBUG_FLASH */

OSStatus MicoFlashInitialize( mico_flash_t flash )
{ 
  platform_log_trace();
  OSStatus err = kNoErr;
  require_action( flash == MICO_SPI_FLASH, exit, err = kUnsupportedErr);

  SpiFlashInfoInit();
#ifdef  DEBUG_FLASH 
  SpiFlashGetInfo(&FlashInfo);
  GetFlashInfo();
#endif
    
  require_action(FlashUnlock(), exit, err = kUnknownErr);
exit:
  return err;
}

OSStatus MicoFlashErase( mico_flash_t flash, uint32_t StartAddress, uint32_t EndAddress )
{
  platform_log_trace();
  OSStatus err = kNoErr;
  require_action( flash == MICO_SPI_FLASH, exit, err = kUnsupportedErr);

  err = SpiFlashErase( StartAddress, EndAddress - StartAddress +1 );

  require_noerr_string(err, exit, "Flash erase error!");

exit:
  return err;
}

OSStatus MicoFlashWrite(mico_flash_t flash, volatile uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength)
{
  platform_log_trace();
  OSStatus err = kNoErr;
  require_action( flash == MICO_SPI_FLASH, exit, err = kUnsupportedErr);

  err = SpiFlashWrite(*FlashAddress, Data, DataLength);
  require_noerr_string(err, exit, "Flash write error!");
  *FlashAddress += DataLength;
exit:
  return err;
}

OSStatus MicoFlashRead(mico_flash_t flash, volatile uint32_t* FlashAddress, uint8_t* Data ,uint32_t DataLength)
{
  platform_log_trace();
  OSStatus err = kNoErr;
  require_action( flash == MICO_SPI_FLASH, exit, err = kUnsupportedErr);
  require_quiet(DataLength > 0, exit);

  err = SpiFlashRead(*FlashAddress, Data, DataLength);
  require_noerr_string(err, exit, "Flash read error!");
  *FlashAddress += DataLength;
exit:
  if ( err != 0 ) {
    platform_log("Err read addr: 0x%x,:%d",  *FlashAddress, DataLength);
    while(1);
  }
  return err;
}


OSStatus MicoFlashFinalize( mico_flash_t flash )
{
  platform_log_trace();
  OSStatus err = kNoErr;
  require_action( flash == MICO_SPI_FLASH, exit, err = kUnsupportedErr);
    
  require_action(FlashLock(FLASH_LOCK_RANGE_ALL), exit, err = kUnknownErr);
exit:
  return err;
}


OSStatus internalFlashInitialize( void )
{ 
  // platform_log_trace();
  // SpiFlashGetInfo(&FlashInfo);
  // GetFlashInfo();
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

bool FlashUnlock(void)
{
  char cmd[3] = "\x35\xBA\x69";
  
  if(SpiFlashIOCtl(IOCTL_FLASH_UNPROTECT, cmd, sizeof(cmd)) != FLASH_NONE_ERR)
  {
    return false;
  }

  return true;
}


bool FlashLock(SPI_FLASH_LOCK_RANGE lock_range)
{
  if(SpiFlashIOCtl(IOCTL_FLASH_PROTECT, lock_range) != FLASH_NONE_ERR)
  {
    return false;
  }

  return true;
}

