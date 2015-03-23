/**
******************************************************************************
* @file    platform.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provides all MICO Peripherals mapping table and platform
*          specific funcgtions.
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

#include "stdio.h"
#include "string.h"

#include "MICOPlatform.h"
#include "platform.h"
#include "MicoDriverMapping.h"
#include "platform_common_config.h"
#include "PlatformLogging.h"
#include "sd_card.h"
#include "nvm.h"


/******************************************************
*                      Macros
******************************************************/

/******************************************************
*                    Constants
******************************************************/
//keep consist with miscfg.h definition
#define UPGRADE_NVM_ADDR        (176)//boot upgrade information at NVRAM address
#define UPGRADE_ERRNO_NOERR   (-1) //just initialization after boot up
#define UPGRADE_ERRNO_ENOENT  (-2) //no such file open by number
#define UPGRADE_ERRNO_EIO   (-5) //read/write error
#define UPGRADE_ERRNO_E2BIG   (-7) //too big than flash capacity
#define UPGRADE_ERRNO_EBADF   (-9) //no need to upgrade
#define UPGRADE_ERRNO_EFAULT  (-14) //address fault
#define UPGRADE_ERRNO_EBUSY   (-16) //flash lock fail
#define UPGRADE_ERRNO_ENODEV  (-19) //no upgrade device found
#define UPGRADE_ERRNO_ENODATA (-61) //flash is empty
#define UPGRADE_ERRNO_EPROTOTYPE (-91) //bad head format("MVO\x12")
#define UPGRADE_ERRNO_ELIBBAD (-80) //CRC error
#define UPGRADE_ERRNO_USBDEV  (-81) //no upgrade USB device
#define UPGRADE_ERRNO_SDDEV   (-82) //no upgrade SD device
#define UPGRADE_ERRNO_USBFILE (-83) //no upgrade file found in USB device
#define UPGRADE_ERRNO_SDFILE  (-84) //no upgrade file found in SD device
#define UPGRADE_ERRNO_NOBALL  (-85) //no upgrade ball in USB & SD device
#define UPGRADE_ERRNO_CODEMGC (-86) //wrong code magic number
#define UPGRADE_ERRNO_CODBUFDAT (-87) //code successful but data fail,because of constant data offset setting

#define UPGRADE_SUCC_MAGIC    (0x57F9B3C8) //just a successful magic
#define UPGRADE_REQT_MAGIC    (0x9ab4d18e) //just a request magic
#define UPGRADE_ERRNO_LASTCLR (0x581f9831) //just a clear magic

#define UDISK_PORT_NUM		        2		// USB PORT

/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/

/******************************************************
*                    Structures
******************************************************/

/******************************************************
*               Function Declarations
******************************************************/
extern WEAK void PlatformEasyLinkButtonClickedCallback(void);
extern WEAK void PlatformStandbyButtonClickedCallback(void);
extern WEAK void PlatformEasyLinkButtonLongPressedCallback(void);
extern WEAK void bootloader_start(void);

/******************************************************
*               Variables Definitions
******************************************************/

/* This table maps STM32 pins to GPIO definitions on the schematic
* A full pin definition is provided in <WICED-SDK>/include/platforms/BCM943362WCD4/platform.h
*/

static uint32_t _default_start_time = 0;
static mico_timer_t _button_EL_timer;

const platform_pin_mapping_t gpio_mapping[] =
{
//  /* Common GPIOs for internal use */
//  [MICO_GPIO_WLAN_POWERSAVE_CLOCK]    = {WL_32K_OUT_BANK, WL_32K_OUT_PIN, WL_32K_OUT_BANK_CLK},
//  [WL_GPIO0]                          = {GPIOB, 12,  RCC_AHB1Periph_GPIOB},
  [WL_GPIO1]                            = {GPIOA,  10},
//  [WL_REG]                            = {GPIOC,  1,  RCC_AHB1Periph_GPIOC},
  [WL_REG]                            = {GPIOB,  20},
  [MICO_SYS_LED]                        = {GPIOA,  3 }, 
  [MICO_RF_LED]                         = {GPIOA,  4 }, 
//  [BOOT_SEL]                          = {GPIOB,  1,  RCC_AHB1Periph_GPIOB}, 
//  [MFG_SEL]                           = {GPIOB,  9,  RCC_AHB1Periph_GPIOB}, 
  [EasyLink_BUTTON]                     = {GPIOA,  5}, 
  [STDIO_UART_RX]                       = {GPIOB,  6},
  [STDIO_UART_TX]                       = {GPIOB,  7},
  [SDIO_INT]                            = {GPIOA,  24},
  [USB_DETECT]                          = {GPIOA,  22},

//  /* GPIOs for external use */
  [APP_UART_RX]                         = {GPIOB, 29},
  [APP_UART_TX]                         = {GPIOB, 28},  
//  [MICO_GPIO_4]  = {GPIOC,  7,  RCC_AHB1Periph_GPIOC},
//  [MICO_GPIO_5]  = {GPIOA,  4,  RCC_AHB1Periph_GPIOA},
//  [MICO_GPIO_6]  = {GPIOA,  4,  RCC_AHB1Periph_GPIOA},
//  [MICO_GPIO_7]  = {GPIOB,  3,  RCC_AHB1Periph_GPIOB},
//  [MICO_GPIO_8]  = {GPIOB , 4,  RCC_AHB1Periph_GPIOB},
//  [MICO_GPIO_9]  = {GPIOB,  5,  RCC_AHB1Periph_GPIOB},
//  [MICO_GPIO_10] = {GPIOB,  8,  RCC_AHB1Periph_GPIOB},
//  [MICO_GPIO_12] = {GPIOC,  2,  RCC_AHB1Periph_GPIOC},
//  [MICO_GPIO_13] = {GPIOB, 14,  RCC_AHB1Periph_GPIOB},
//  [MICO_GPIO_14] = {GPIOC,  6,  RCC_AHB1Periph_GPIOC},
//  [MICO_GPIO_18] = {GPIOA, 15,  RCC_AHB1Periph_GPIOA},
//  [MICO_GPIO_19] = {GPIOB, 11,  RCC_AHB1Periph_GPIOB},
//  [MICO_GPIO_20] = {GPIOA, 12,  RCC_AHB1Periph_GPIOA},
//  [MICO_GPIO_21] = {GPIOA, 11,  RCC_AHB1Periph_GPIOA},
//  [MICO_GPIO_22] = {GPIOA,  9,  RCC_AHB1Periph_GPIOA},
//  [MICO_GPIO_23] = {GPIOA, 10,  RCC_AHB1Periph_GPIOA},
//  [MICO_GPIO_29] = {GPIOA,  0,  RCC_AHB1Periph_GPIOA},  
};

/*
* Possible compile time inputs:
* - Set which ADC peripheral to use for each ADC. All on one ADC allows sequential conversion on all inputs. All on separate ADCs allows concurrent conversion.
*/
/* TODO : These need fixing */
const platform_adc_mapping_t adc_mapping[] =
{
  [MICO_ADC_1] = {1},
  [MICO_ADC_2] = {1},
  [MICO_ADC_3] = {1},
};


/* PWM mappings */
const platform_pwm_mapping_t pwm_mappings[] =
{
#if ( MICO_WLAN_POWERSAVE_CLOCK_SOURCE == MICO_WLAN_POWERSAVE_CLOCK_IS_PWM )
  /* Extended PWM for internal use */
  [MICO_PWM_WLAN_POWERSAVE_CLOCK] = {TIM1, 4, RCC_APB2Periph_TIM1, GPIO_AF_TIM1, (platform_pin_mapping_t*)&gpio_mapping[MICO_GPIO_WLAN_POWERSAVE_CLOCK] }, /* or TIM2/Ch2                       */
#endif
  
  [MICO_PWM_1]  = {1},    /* or TIM10/Ch1                       */
  [MICO_PWM_2]  = {1}, /* or TIM1/Ch2N                       */
  [MICO_PWM_3]  = {1},    
  /* TODO: fill in the other options here ... */
};

const platform_spi_mapping_t spi_mapping[] =
{
  [MICO_SPI_1]  =
  {
    1
  }
};

const platform_uart_mapping_t uart_mapping[] =
{
[MICO_UART_1] =
  {
    .uart                            = FUART,
    .pin_tx                          = &gpio_mapping[STDIO_UART_TX],
    .pin_rx                          = &gpio_mapping[STDIO_UART_RX],
    .pin_cts                         = NULL,
    .pin_rts                         = NULL,
  },
  [MICO_UART_2] =
  {
    .uart                            = BUART,
    .pin_tx                          = &gpio_mapping[APP_UART_TX],
    .pin_rx                          = &gpio_mapping[APP_UART_RX],
    .pin_cts                         = NULL,
    .pin_rts                         = NULL,
  },
};

const platform_i2c_mapping_t i2c_mapping[] =
{
  [MICO_I2C_1] =
  {
    1,
  },
};

/******************************************************
*               Function Definitions
******************************************************/

static void _button_EL_irq_handler( void* arg )
{
  (void)(arg);
  int interval = -1;

  mico_start_timer(&_button_EL_timer);
  
  if ( MicoGpioInputGet( (mico_gpio_t)EasyLink_BUTTON ) == 0 ) {
    _default_start_time = mico_get_time()+1;
    mico_start_timer(&_button_EL_timer);
    MicoGpioEnableIRQ( (mico_gpio_t)EasyLink_BUTTON, IRQ_TRIGGER_RISING_EDGE, _button_EL_irq_handler, NULL );
  } else {
    interval = mico_get_time() + 1 - _default_start_time;
    if ( (_default_start_time != 0) && interval > 50 && interval < RestoreDefault_TimeOut){
      /* EasyLink button clicked once */
      PlatformEasyLinkButtonClickedCallback();
      //platform_log("PlatformEasyLinkButtonClickedCallback!");
      MicoGpioOutputLow( (mico_gpio_t)MICO_RF_LED );
      MicoGpioEnableIRQ( (mico_gpio_t)EasyLink_BUTTON, IRQ_TRIGGER_FALLING_EDGE, _button_EL_irq_handler, NULL );
   }
   mico_stop_timer(&_button_EL_timer);
   _default_start_time = 0;
  }
}

//static void _button_STANDBY_irq_handler( void* arg )
//{
//  (void)(arg);
//  PlatformStandbyButtonClickedCallback();
//}

static void _button_EL_Timeout_handler( void* arg )
{
  (void)(arg);
  _default_start_time = 0;
  MicoGpioEnableIRQ( (mico_gpio_t)EasyLink_BUTTON, IRQ_TRIGGER_FALLING_EDGE, _button_EL_irq_handler, NULL );
  if( MicoGpioInputGet( (mico_gpio_t)EasyLink_BUTTON ) == 0){
    //platform_log("PlatformEasyLinkButtonLongPressedCallback!");
    PlatformEasyLinkButtonLongPressedCallback();
  }
  mico_stop_timer(&_button_EL_timer);
}

bool watchdog_check_last_reset( void )
{
//  if ( RCC->CSR & RCC_CSR_WDGRSTF )
//  {
//    /* Clear the flag and return */
//    RCC->CSR |= RCC_CSR_RMVF;
//    return true;
//  }
//  
  return false;
}

OSStatus mico_platform_init( void )
{
#ifdef DEBUG
  #if defined(__CC_ARM)
    platform_log("Build by Keil");
  #elif defined (__IAR_SYSTEMS_ICC__)
    platform_log("Build by IAR");
  #endif
#endif
 platform_log( "Mico platform initialised" );
 if ( true == watchdog_check_last_reset() )
 {
   platform_log( "WARNING: Watchdog reset occured previously. Please see watchdog.c for debugging instructions." );
 }
  
  return kNoErr;
}

void init_platform( void )
{
  MicoGpioInitialize( (mico_gpio_t)MICO_SYS_LED, OUTPUT_PUSH_PULL );
  MicoSysLed(false);
  MicoGpioInitialize( (mico_gpio_t)MICO_RF_LED, OUTPUT_PUSH_PULL );
  MicoRfLed(false);
  
  //  Initialise EasyLink buttons
  //MicoGpioInitialize( (mico_gpio_t)EasyLink_BUTTON, INPUT_PULL_UP );
  //mico_init_timer(&_button_EL_timer, RestoreDefault_TimeOut, _button_EL_Timeout_handler, NULL);
  //MicoGpioEnableIRQ( (mico_gpio_t)EasyLink_BUTTON, IRQ_TRIGGER_FALLING_EDGE, _button_EL_irq_handler, NULL );
//  
//  //  Initialise Standby/wakeup switcher
//  MicoGpioInitialize( Standby_SEL, INPUT_PULL_UP );
//  MicoGpioEnableIRQ( Standby_SEL , IRQ_TRIGGER_FALLING_EDGE, _button_STANDBY_irq_handler, NULL);

}

#ifdef BOOTLOADER
#include "host_stor.h"
#include "fat_file.h" 
#include "host_hcd.h"
#include "dir.h"

static bool HardwareInit(DEV_ID DevId);
static FOLDER	 RootFolder;
static void FileBrowse(FS_CONTEXT* FsContext);
static bool UpgradeFileFound = false;


void init_platform_bootloader( void )
{
  uint32_t BootNvmInfo;
  OSStatus err;
  
  /* Check USB-HOST is inserted */
  err = MicoGpioInitialize( (mico_gpio_t)USB_DETECT, INPUT_PULL_DOWN );
  require_noerr(err, exit);
  mico_thread_msleep_no_os(2);
  
  require_string( MicoGpioInputGet( (mico_gpio_t)USB_DETECT ) == true, exit, "USB device is not inserted" );

  platform_log("USB device inserted");
  if( HardwareInit(DEV_ID_USB) ){
    FolderOpenByNum(&RootFolder, NULL, 1);
    FileBrowse(RootFolder.FsContext);
  }

  /* Check last firmware update is success or not. */
  NvmRead(UPGRADE_NVM_ADDR, (uint8_t*)&BootNvmInfo, 4);

  if(false == UpgradeFileFound)
  {
    if(BootNvmInfo == UPGRADE_SUCC_MAGIC)
    {
      /*
       * boot up check for the last time
       */
      platform_log("[UPGRADE]:upgrade successful completely");
    }
    else if(BootNvmInfo == (uint32_t)UPGRADE_ERRNO_NOERR)
    {
      platform_log("[UPGRADE]:no upgrade, boot normallly");
    }
    else if(BootNvmInfo == (uint32_t)UPGRADE_ERRNO_CODBUFDAT)
    {
      platform_log("[UPGRADE]:upgrade successful partly, data fail");
    }
    else
    {
      platform_log("[UPGRADE]:upgrade error, errno = %d", (int32_t)BootNvmInfo);
    }
  }
  else
  {
    if(BootNvmInfo == (uint32_t)UPGRADE_ERRNO_NOERR)
    {
      platform_log("[UPGRADE]:found upgrade ball, prepare to boot upgrade");
      BootNvmInfo = UPGRADE_REQT_MAGIC;
      NvmWrite(UPGRADE_NVM_ADDR, (uint8_t*)&BootNvmInfo, 4);
            //if you want PORRESET to reset GPIO only,uncomment it
            //GpioPorSysReset(GPIO_RSTSRC_PORREST);
      NVIC_SystemReset();
      while(1);;;
    }
    else if(BootNvmInfo == UPGRADE_SUCC_MAGIC)
    {
      BootNvmInfo = (uint32_t)UPGRADE_ERRNO_NOERR;
      NvmWrite(UPGRADE_NVM_ADDR, (uint8_t*)&BootNvmInfo, 4);
      platform_log("[UPGRADE]:found upgrade ball file for the last time, re-plugin/out, if you want to upgrade again");
    }
    else
    {
      platform_log("[UPGRADE]:upgrade error, errno = %d", (int32_t)BootNvmInfo);
      if( BootNvmInfo == -9 ) {
        platform_log("[UPGRADE]:Same file, no need to update");
        goto exit;
      }
      BootNvmInfo = (uint32_t)UPGRADE_ERRNO_NOERR;
      NvmWrite(UPGRADE_NVM_ADDR, (uint8_t*)&BootNvmInfo, 4);
      BootNvmInfo = UPGRADE_REQT_MAGIC;
      NvmWrite(UPGRADE_NVM_ADDR, (uint8_t*)&BootNvmInfo, 4);
            //if you want PORRESET to reset GPIO only,uncomment it
            //GpioPorSysReset(GPIO_RSTSRC_PORREST);
      NVIC_SystemReset();
    }
  }
exit:
  return;
}

static bool IsCardLink(void)
{
  return false;
}
static bool IsUDiskLink(void)
{
	return UsbHost2IsLink();
}

bool CheckAllDiskLinkFlag(void)
{
    return TRUE;
}

static bool HardwareInit(DEV_ID DevId)
{
	switch(DevId)
	{
		case DEV_ID_SD:
			if(!IsCardLink())
			{
				return FALSE;
			}
			FSDeInit(DEV_ID_SD);
			if(SdCardInit())	
			{
				return FALSE;
			}
			if(!FSInit(DEV_ID_SD))
			{
				return FALSE;
			}
			return TRUE;
		case DEV_ID_USB:
			Usb2SetDetectMode(1, 0);
			UsbSetCurrentPort(UDISK_PORT_NUM);
			if(!IsUDiskLink())
			{
				return FALSE;
			}
			FSDeInit(DEV_ID_SD);
			FSDeInit(DEV_ID_USB);
			if(!HostStorInit())
			{
				return FALSE;
			}
			if(!FSInit(DEV_ID_USB))
			{
				return FALSE;
			}
			return TRUE;
		default:
			break;
	}
	return FALSE;
}
static void FileBrowse(FS_CONTEXT* FsContext)
{	
	uint8_t	EntryType;
	DirSetStartEntry(FsContext, FsContext->gFsInfo.RootStart, 0, TRUE);
	FsContext->gFolderDirStart = FsContext->gFsInfo.RootStart;
	while(1)
	{
		EntryType = DirGetNextEntry(FsContext);
		switch(EntryType)
		{
			case ENTRY_FILE:
//				platform_log("%-.11s  %d年%d月%d日 %d:%d:%d  %d 字节",
//					FsContext->gCurrentEntry->FileName,
//					1980+(FsContext->gCurrentEntry->CreateDate >> 9),
//					(FsContext->gCurrentEntry->CreateDate >> 5) & 0xF,
//					(FsContext->gCurrentEntry->CreateDate) & 0x1F,
//					FsContext->gCurrentEntry->CreateTime >> 11,
//					(FsContext->gCurrentEntry->CreateTime >> 5) & 0x3F,
//					((FsContext->gCurrentEntry->CreateTime << 1) & 0x3F) + (FsContext->gCurrentEntry->CrtTimeTenth / 100),
//					FsContext->gCurrentEntry->Size);
      	if((FsContext->gCurrentEntry->ExtName[0] == 'M') && 
           (FsContext->gCurrentEntry->ExtName[1] == 'V') && 
           (FsContext->gCurrentEntry->ExtName[2] == 'A')){
            UpgradeFileFound = true;
            return;
          }
				break;
			case ENTRY_FOLDER:
				break;
			case ENTRY_END:
        return;
			default:
				break;
		}
	}
}

#endif



void host_platform_reset_wifi( bool reset_asserted )
{
  if ( reset_asserted == true )
  {
    MicoGpioOutputLow( (mico_gpio_t)WL_RESET );  
  }
  else
  {
    MicoGpioOutputHigh( (mico_gpio_t)WL_RESET ); 
  }
}

void host_platform_power_wifi( bool power_enabled )
{
  if ( power_enabled == true )
  {
    MicoGpioOutputLow( (mico_gpio_t)WL_REG );  
  }
  else
  {
    MicoGpioOutputHigh( (mico_gpio_t)WL_REG ); 
  }
}

void MicoSysLed(bool onoff)
{
    if (onoff) {
        MicoGpioOutputLow( (mico_gpio_t)MICO_SYS_LED );
    } else {
        MicoGpioOutputHigh( (mico_gpio_t)MICO_SYS_LED );
    }
}

void MicoRfLed(bool onoff)
{
    if (onoff) {
        MicoGpioOutputLow( (mico_gpio_t)MICO_RF_LED );
    } else {
        MicoGpioOutputHigh( (mico_gpio_t)MICO_RF_LED );
    }
}

bool MicoShouldEnterMFGMode(void)
{
//  if(MicoGpioInputGet((mico_gpio_t)BOOT_SEL)==false && MicoGpioInputGet((mico_gpio_t)MFG_SEL)==false)
//    return true;
//  else
    return false;
}

bool MicoShouldEnterBootloader(void)
{
//  if(MicoGpioInputGet((mico_gpio_t)BOOT_SEL)==false && MicoGpioInputGet((mico_gpio_t)MFG_SEL)==true)
//    return true;
//  else
    return true;
}


