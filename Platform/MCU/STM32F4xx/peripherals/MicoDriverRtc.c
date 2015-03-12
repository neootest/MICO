/**
******************************************************************************
* @file    MicoDriverRtc.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide RTC driver functions.
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

#include "MICORTOS.h"
#include "MICOPlatform.h"
#include "MicoDefaults.h"

#include "platform.h"
#include "platform_common_config.h"
#include "stm32f4xx_platform.h"
#include "stm32f4xx.h"

/******************************************************
*                      Macros
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
*                    Constants
******************************************************/

#ifndef MICO_DISABLE_MCU_POWERSAVE
#define RTC_Wakeup_init        MicoRtcInitialize            
#else
#ifdef MICO_ENABLE_MCU_RTC
#define platform_rtc_init      MicoRtcInitialize            
#else /* #ifdef MICO_ENABLE_MCU_RTC */
#define platform_rtc_noinit     MicoRtcInitialize
#endif /* #ifdef MICO_ENABLE_MCU_RTC */
#endif /* ifndef MICO_DISABLE_MCU_POWERSAVE */

#define USE_RTC_BKP 0x00BB32F2 // Use RTC BKP to initilize system time.

#define RTC_INTERRUPT_EXTI_LINE EXTI_Line22




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
*               Variables Definitions
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
*               Function Declarations
******************************************************/


/******************************************************
*               Function Definitions
******************************************************/

void platform_rtc_noinit(void)
{
}

#if defined(MICO_DISABLE_MCU_POWERSAVE) && defined(MICO_ENABLE_MCU_RTC)
/*  */
void platform_rtc_init(void)
{
  RTC_InitTypeDef RTC_InitStruct;
  
  RTC_InitStruct.RTC_HourFormat = RTC_HourFormat_24;
  
  /* RTC ticks every second */
  RTC_InitStruct.RTC_AsynchPrediv = 0x7F;
  RTC_InitStruct.RTC_SynchPrediv = 0xFF;
  
  /* Enable the PWR clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  
  /* RTC clock source configuration ------------------------------------------*/
  /* Allow access to BKP Domain */
  PWR_BackupAccessCmd(ENABLE);
  
  RTC_Init( &RTC_InitStruct );
  /* Enable the LSE OSC */
  RCC_LSEConfig(RCC_LSE_ON);
  /* Wait till LSE is ready */
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {
  }
  /* Select the RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
  
  /* Enable the RTC Clock */
  RCC_RTCCLKCmd(ENABLE);
  
  /* RTC configuration -------------------------------------------------------*/
  /* Wait for RTC APB registers synchronisation */
  RTC_WaitForSynchro();
  
#ifdef USE_RTC_BKP
  if (RTC_ReadBackupRegister(RTC_BKP_DR0) != USE_RTC_BKP) {
    /* set it to 12:20:30 08/04/2013 monday */
    MicoRtcSetTime(&mico_default_time);
    RTC_WriteBackupRegister(RTC_BKP_DR0, USE_RTC_BKP);
  }
#else
  /* write default application time inside rtc */
  MicoRtcSetTime(&mico_default_time);
#endif
  
}
#endif


#ifndef MICO_DISABLE_MCU_POWERSAVE
void RTC_Wakeup_init(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  RTC_InitTypeDef RTC_InitStruct;
  
  RTC_InitStruct.RTC_HourFormat = RTC_HourFormat_24;
  
  /* RTC ticks every second */
  RTC_InitStruct.RTC_AsynchPrediv = 0x7F;
  RTC_InitStruct.RTC_SynchPrediv = 0xFF;
  
  RTC_Init( &RTC_InitStruct );
  
  /* Enable the PWR clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  
  /* RTC clock source configuration ------------------------------------------*/
  /* Allow access to BKP Domain */
  PWR_BackupAccessCmd(ENABLE);
#ifdef USE_RTC_BKP
  PWR_BackupRegulatorCmd(ENABLE);
#endif
  
  /* Enable the LSE OSC */
  RCC_LSEConfig(RCC_LSE_ON);
  /* Wait till LSE is ready */
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {
  }
  
  /* Select the RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
  
  /* Enable the RTC Clock */
  RCC_RTCCLKCmd(ENABLE);
  
  /* RTC configuration -------------------------------------------------------*/
  /* Wait for RTC APB registers synchronisation */
  RTC_WaitForSynchro();
  
  RTC_WakeUpCmd( DISABLE );
  EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
  PWR_ClearFlag(PWR_FLAG_WU);
  RTC_ClearFlag(RTC_FLAG_WUTF);
  
  RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div2);
  
  EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
  EXTI_InitStructure.EXTI_Line = RTC_INTERRUPT_EXTI_LINE;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  RTC_ITConfig(RTC_IT_WUT, DISABLE);
  
  /* Prepare Stop-Mode but leave disabled */
  PWR_ClearFlag(PWR_FLAG_WU);
  PWR->CR |= PWR_CR_LPDS;
  PWR->CR &= (unsigned long)(~(PWR_CR_PDDS));
  SCB->SCR |= ((unsigned long)SCB_SCR_SLEEPDEEP_Msk);
  
#ifdef USE_RTC_BKP
  if (RTC_ReadBackupRegister(RTC_BKP_DR0) != USE_RTC_BKP) {
    /* set it to 12:20:30 08/04/2013 monday */
    MicoRtcSetTime(&mico_default_time);
    RTC_WriteBackupRegister(RTC_BKP_DR0, USE_RTC_BKP);
  }
#else
  //#ifdef RTC_ENABLED
  /* application must have wiced_application_default_time structure declared somewhere, otherwise it wont compile */
  /* write default application time inside rtc */
  MicoRtcSetTime(&mico_default_time);
  //#endif /* RTC_ENABLED */
#endif
  
}
#endif /* #ifndef MICO_DISABLE_MCU_POWERSAVE */

/**
* This function will return the value of time read from the on board CPU real time clock. Time value must be given in the format of
* the structure wiced_rtc_time_t
*
* @return    WICED_SUCCESS : on success.
* @return    WICED_ERROR   : if an error occurred with any step
*/
OSStatus MicoRtcGetTime(mico_rtc_time_t* time)
{
#ifdef MICO_ENABLE_MCU_RTC
  RTC_TimeTypeDef rtc_read_time;
  RTC_DateTypeDef rtc_read_date;
  
  if( time == 0 )
  {
    return kParamErr;
  }
  
  /* save current rtc time */
  RTC_GetTime( RTC_Format_BIN, &rtc_read_time );
  RTC_GetDate( RTC_Format_BIN, &rtc_read_date );
  
  /* fill structure */
  time->sec = rtc_read_time.RTC_Seconds;
  time->min = rtc_read_time.RTC_Minutes;
  time->hr = rtc_read_time.RTC_Hours;
  time->weekday = rtc_read_date.RTC_WeekDay;
  time->date = rtc_read_date.RTC_Date;
  time->month= rtc_read_date.RTC_Month;
  time->year = rtc_read_date.RTC_Year;
  
  return kNoErr;
#else /* #ifdef WICED_ENABLE_MCU_RTC */
  UNUSED_PARAMETER(time);
  return kUnsupportedErr;
#endif /* #ifdef WICED_ENABLE_MCU_RTC */
}

/**
* This function will set MCU RTC time to a new value. Time value must be given in the format of
* the structure wiced_rtc_time_t
*
* @return    WICED_SUCCESS : on success.
* @return    WICED_ERROR   : if an error occurred with any step
*/
OSStatus MicoRtcSetTime(mico_rtc_time_t* time)
{
#ifdef MICO_ENABLE_MCU_RTC
  RTC_TimeTypeDef rtc_write_time;
  RTC_DateTypeDef rtc_write_date;
  bool    valid = false;
  
  MicoMcuPowerSaveConfig(false);
  
  MICO_VERIFY_TIME(time, valid);
  if( valid == false )
  {
    return kParamErr;
  }
  rtc_write_time.RTC_Seconds = time->sec;
  rtc_write_time.RTC_Minutes = time->min;
  rtc_write_time.RTC_Hours   = time->hr;
  rtc_write_date.RTC_WeekDay = time->weekday;
  rtc_write_date.RTC_Date    = time->date;
  rtc_write_date.RTC_Month   = time->month;
  rtc_write_date.RTC_Year    = time->year;
  
  
  RTC_SetTime( RTC_Format_BIN, &rtc_write_time );
  RTC_SetDate( RTC_Format_BIN, &rtc_write_date );
  
  MicoMcuPowerSaveConfig(true);
  return kNoErr;
#else /* #ifdef MICO_ENABLE_MCU_RTC */
  UNUSED_PARAMETER(time);
  return kUnsupportedErr;
#endif /* #ifdef MICO_ENABLE_MCU_RTC */
}

