/**
******************************************************************************
* @file    mico_driver_rtc.c 
* @author  william xu
* @version V1.0.0
* @date    05-may-2014
* @brief   this file provide RTC driver functions.
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

#include "mico_rtos.h"
#include "mico_platform.h"
#include "mico_defaults.h"

#include "platform.h"
#include "rtc.h"

/******************************************************
*                      macros
******************************************************/

#define MICO_VERIFY_TIME(time, valid) \
if( (time->sec > 60) || ( time->min > 60 ) || (time->hr > 24) || ( time->date > 31 ) || ( time->month > 12 )) \
  { \
    valid= false; \
  } \
else \
  { \
    valid= true; \
  }

/******************************************************
*                    constants
******************************************************/

#ifndef MICO_DISABLE_MCU_POWERSAVE
#define rtc_wakeup_init        mico_rtc_initialize            
#else
#ifdef MICO_ENABLE_MCU_RTC
#define platform_rtc_init      mico_rtc_initialize            
#else /* #ifdef MICO_ENABLE_MCU_RTC */
#define platform_rtc_noinit     mico_rtc_initialize
#endif /* #ifdef MICO_ENABLE_MCU_RTC */
#endif /* ifndef MICO_DISABLE_MCU_POWERSAVE */

//#define USE_RTC_BKP 0x00BB32F2 // use RTC BKP to initilize system time.

//#define RTC_INTERRUPT_EXTI_LINE EXTI_Line22




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

mico_rtc_time_t mico_default_time =
{
  /* set it to 12:20:30 08/04/2013 monday */
  .sec   = 30,
  .min   = 20,
  .hr    = 12,
  .weekday  = 1,
  .date  = 8,
  .month = 4,
  .year  = 13
};

/******************************************************
*               function declarations
******************************************************/


/******************************************************
*               function definitions
******************************************************/

void platform_rtc_noinit(void)
{
}

#if defined(MICO_DISABLE_MCU_POWERSAVE) && defined(MICO_ENABLE_MCU_RTC)
/*  */
void platform_rtc_init(void)
{

    rtc_set_hour_mode(RTC, 0); //0 : 24-hour, 1 : 12-hour

#ifdef USE_RTC_BKP

#else
  mico_rtc_set_time(&mico_default_time);
#endif
}
#endif


#ifndef MICO_DISABLE_MCU_POWERSAVE
void rtc_wakeup_init(void)
{

    rtc_set_hour_mode(RTC, 0); //0 : 24-hour, 1 : 12-hour

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);
	rtc_enable_interrupt(RTC, RTC_IER_TIMEN | RTC_IER_ALREN);
    
#ifdef USE_RTC_BKP

#else
  mico_rtc_set_time(&mico_default_time);
#endif
}
#endif /* #ifndef MICO_DISABLE_MCU_POWERSAVE */

/**
* this function will return the value of time read from the on board CPU real time clock. time value must be given in the format of
* the structure wiced_rtc_time_t
*
* @return    WICED_SUCCESS : on success.
* @return    WICED_ERROR   : if an error occurred with any step
*/
OSStatus mico_rtc_get_time(mico_rtc_time_t* time)
{
#ifdef MICO_ENABLE_MCU_RTC
    uint32_t hour, minute, second, year, month, day, week;
    if( time == 0 )
    {
        return kParamErr;
    }
    rtc_get_time(RTC, &hour, &minute, &second); //BCD format.
    time->sec     = second;
    time->min     = minute;
    time->hr      = hour;
    rtc_get_date(RTC, &year, &month, &day, &week);
    time->date    = day;
    time->weekday = week;
    time->month   = month;
    time->year    = year;
    
    return kNoErr;
#else
    UNUSED_PARAMETER(time);
    return kUnsupportedErr;
#endif

}

/**
* this function will set MCU RTC time to a new value. time value must be given in the format of
* the structure wiced_rtc_time_t
*
* @return    WICED_SUCCESS : on success.
* @return    WICED_ERROR   : if an error occurred with any step
*/
OSStatus mico_rtc_set_time(mico_rtc_time_t* time)
{
#ifdef MICO_ENABLE_MCU_RTC
    uint32_t hour, minute, second, year, month, day, week;
    bool    valid = false;
    
    mico_mcu_power_save_config(false);
  
    MICO_VERIFY_TIME(time, valid);
    if( valid == false )
    {
        return kParamErr;
    }
    second  = time->sec;
    minute  = time->min;
    hour    = time->hr;
    rtc_set_time(RTC, hour, minute, second);
    day     = time->date;
    week    = time->weekday;
    month   = time->month;
    year    = time->year;
    rtc_set_date(RTC, year, month, week, day);

    mico_mcu_power_save_config(true);

    return kNoErr;
#else
    UNUSED_PARAMETER(time);
    return kUnsupportedErr;
#endif
}


