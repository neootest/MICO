/**
******************************************************************************
* @file    mico_driver_wdg.c 
* @author  william xu
* @version V1.0.0
* @date    05-may-2014
* @brief   this file provide WDG driver functions.
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


#include "mico_platform.h"
#include "mico_rtos.h"
#include "common.h"
#include "debug.h"
#include "mico_common_driver.h"
#include "platform.h"

/******************************************************
 *                    constants
 ******************************************************/

/******************************************************
 *                   enumerations
 ******************************************************/

/******************************************************
 *                 type definitions
 ******************************************************/

/******************************************************
 *                    structures
 ******************************************************/

/******************************************************
 *               variables definitions
 ******************************************************/
#ifndef MICO_DISABLE_WATCHDOG
static __IO uint32_t lsi_freq = 0;
static __IO uint32_t capture_number = 0, period_value = 0;
static mico_semaphore_t  _measure_lsi_complete_SEM = NULL;
uint16_t tmp_CC4[2] = {0, 0};
#endif

/******************************************************
 *               function declarations
 ******************************************************/
#ifndef MICO_DISABLE_WATCHDOG
static uint32_t get_lsi_frequency(void);
#endif

/******************************************************
 *               function definitions
 ******************************************************/

OSStatus mico_wdg_initialize( uint32_t timeout_ms )
{
#ifndef MICO_DISABLE_WATCHDOG
    OSStatus err = kNoErr;
	uint32_t wdt_mode, timeout_value;

	/* Get timeout value. */
	timeout_value = wdt_get_timeout_value(timeout_ms* 1000,
			BOARD_FREQ_SLCK_XTAL);
    require_action( timeout_value != WDT_INVALID_ARGUMENT, exit, err = kParamErr );

	/* Configure WDT to trigger an interrupt (or reset). */
	wdt_mode = WDT_MR_WDFIEN |  /* Enable WDT fault interrupt. */
			WDT_MR_WDRPROC   |  /* WDT fault resets processor only. */
			WDT_MR_WDDBGHLT  |  /* WDT stops in debug state. */
			WDT_MR_WDIDLEHLT;   /* WDT stops in idle state. */
	/* Initialize WDT with the given parameters. */
	wdt_init(WDT, wdt_mode, timeout_value, timeout_value);
	printf("Enable watchdog with %d microseconds period\n\r",
			(int)wdt_get_us_timeout_period(WDT, BOARD_FREQ_SLCK_XTAL));

	/* Configure and enable WDT interrupt. */
	NVIC_DisableIRQ(WDT_IRQn);
	NVIC_ClearPendingIRQ(WDT_IRQn);
	NVIC_SetPriority(WDT_IRQn, 5);
	NVIC_EnableIRQ(WDT_IRQn);

exit:
    return err;
#else
  UNUSED_PARAMETER( timeout_ms );
  return kUnsupportedErr;
#endif
}

OSStatus mico_wdg_finalize( void )
{
#ifndef MICO_DISABLE_WATCHDOG
    // PLATFORM_TO_DO
    wdt_disable(WDT);
    return kNoErr;
#else
    return kNoErr;
#endif
}

void mico_wdg_reload( void )
{

#ifndef MICO_DISABLE_WATCHDOG
  wdt_restart(WDT);
#else
  return;
#endif
}


#ifndef MICO_DISABLE_WATCHDOG
/**
  * @brief  configures TIM5 to measure the LSI oscillator frequency. 
  * @param  none
  * @retval LSI frequency
  */
uint32_t get_lsi_frequency(void)
{
    return 0;//TBD!
}

/**
  * @brief  this function handles TIM5 global interrupt request.
  * @param  none
  * @retval none
  */
void WDT_Handler(void)
{
	/* Clear status bit to acknowledge interrupt by dummy read. */
	wdt_get_status(WDT);
	/* Restart the WDT counter. */
	wdt_restart(WDT);
}
#endif
