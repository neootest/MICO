/**
******************************************************************************
* @file    stm32f2xx_platform.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide functions called by MICO to drive stm32f2xx 
*          platform: - e.g. power save, reboot, platform initialize
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


#include "stm32f4xx_platform.h"
#include "platform.h"
#include "platform_common_config.h"
#include "MicoPlatform.h"
#include "PlatformLogging.h"
#include "rtc.h"
#include <string.h> // For memcmp
#include "crt0.h"
#include "MICODefaults.h"
#include "MicoRTOS.h"

#ifdef __GNUC__
#include "../../GCC/stdio_newlib.h"
#endif /* ifdef __GNUC__ */

#ifdef __GNUC__
#define WEAK __attribute__ ((weak))
#elif defined ( __IAR_SYSTEMS_ICC__ )
#define WEAK __weak
#endif /* ifdef __GNUC__ */

/******************************************************
*                      Macros
******************************************************/
#ifndef BOOTLOADER_MAGIC_NUMBER
#define BOOTLOADER_MAGIC_NUMBER 0x4d435242
#endif

#define NUMBER_OF_LSE_TICKS_PER_MILLISECOND(scale_factor) ( 32768 / 1000 / scale_factor )
#define CONVERT_FROM_TICKS_TO_MS(n,s) ( n / NUMBER_OF_LSE_TICKS_PER_MILLISECOND(s) )

/******************************************************
*                    Constants
******************************************************/

#ifndef STDIO_BUFFER_SIZE
#define STDIO_BUFFER_SIZE   64
#endif

#define RTC_INTERRUPT_EXTI_LINE EXTI_Line22

#define CK_SPRE_CLOCK_SOURCE_SELECTED 0xFFFF

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

void MCU_CLOCKS_NEEDED    ( void );
void MCU_CLOCKS_NOT_NEEDED( void );

void wake_up_interrupt_notify( void );

extern OSStatus host_platform_init( void );

/******************************************************
*               Variables Definitions
******************************************************/
/* mico_cpu_clock_hz is used by MICO RTOS */
const uint32_t  mico_cpu_clock_hz = MCU_CLOCK_HZ;

static char stm32_platform_inited = 0;

#ifndef MICO_DISABLE_STDIO
static const mico_uart_config_t stdio_uart_config =
{
  .baud_rate    = 115200,
  .data_width   = DATA_WIDTH_8BIT,
  .parity       = NO_PARITY,
  .stop_bits    = STOP_BITS_1,
  .flow_control = FLOW_CONTROL_DISABLED,
};

static volatile ring_buffer_t stdio_rx_buffer;
static volatile uint8_t             stdio_rx_data[STDIO_BUFFER_SIZE];
mico_mutex_t        stdio_rx_mutex;
mico_mutex_t        stdio_tx_mutex;
#endif /* #ifndef MICO_DISABLE_STDIO */

#ifndef MICO_DISABLE_MCU_POWERSAVE
static bool wake_up_interrupt_triggered  = false;
static unsigned long rtc_timeout_start_time           = 0;
#endif /* #ifndef MICO_DISABLE_MCU_POWERSAVE */

/******************************************************
*               Function Definitions
******************************************************/
#if defined ( __ICCARM__ )

static inline void __jump_to( uint32_t addr )
{
  __asm( "MOV R1, #0x00000001" );
  __asm( "ORR R0, R0, R1" );  /* Last bit of jump address indicates whether destination is Thumb or ARM code */
  __asm( "BLX R0" );
}

#elif defined ( __GNUC__ )

__attribute__( ( always_inline ) ) static __INLINE void __jump_to( uint32_t addr )
{
  addr |= 0x00000001;  /* Last bit of jump address indicates whether destination is Thumb or ARM code */
  __ASM volatile ("BX %0" : : "r" (addr) );
}

#endif

/*Boot to mico application form APPLICATION_START_ADDRESS defined in platform_common_config.h */
void startApplication(void)
{
  uint32_t text_addr = APPLICATION_START_ADDRESS;
  
  if(((*(volatile uint32_t*)text_addr) & 0x2FFE0000 ) != 0x20000000)
    text_addr += 0x200;
  /* Test if user code is programmed starting from address "ApplicationAddress" */
  if (((*(volatile uint32_t*)text_addr) & 0x2FFE0000 ) == 0x20000000)
  { 
    uint32_t* stack_ptr;
    uint32_t* start_ptr;
    
    __asm( "MOV LR,        #0xFFFFFFFF" );
    __asm( "MOV R1,        #0x01000000" );
    __asm( "MSR APSR_nzcvq,     R1" );
    __asm( "MOV R1,        #0x00000000" );
    __asm( "MSR PRIMASK,   R1" );
    __asm( "MSR FAULTMASK, R1" );
    __asm( "MSR BASEPRI,   R1" );
    __asm( "MSR CONTROL,   R1" );
  
   SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; 
    
    stack_ptr = (uint32_t*) text_addr;  /* Initial stack pointer is first 4 bytes of vector table */
    start_ptr = ( stack_ptr + 1 );  /* Reset vector is second 4 bytes of vector table */
    
    __set_MSP( *stack_ptr );
    __jump_to( *start_ptr );
  }  
}

/* STM32F2 common clock initialisation function
* This brings up enough clocks to allow the processor to run quickly while initialising memory.
* Other platform specific clock init can be done in init_platform() or init_architecture()
*/
WEAK void init_clocks( void )
{
  //RCC_DeInit( ); /* if not commented then the LSE PA8 output will be disabled and never comes up again */
  
  /* Configure Clocks */
  
  RCC_HSEConfig( HSE_SOURCE );
  RCC_WaitForHSEStartUp( );
  
  RCC_HCLKConfig( AHB_CLOCK_DIVIDER );
  RCC_PCLK2Config( APB2_CLOCK_DIVIDER );
  RCC_PCLK1Config( APB1_CLOCK_DIVIDER );
  
  /* Enable the PLL */
  FLASH_SetLatency( INT_FLASH_WAIT_STATE );
  FLASH_PrefetchBufferCmd( ENABLE );
  
  /* Use the clock configuration utility from ST to calculate these values
  * http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/utility/stsw-stm32090.zip
  */
  RCC_PLLConfig( PLL_SOURCE, PLL_M_CONSTANT, PLL_N_CONSTANT, PLL_P_CONSTANT, PPL_Q_CONSTANT ); /* NOTE: The CPU Clock Frequency is independently defined in <WICED-SDK>/Wiced/Platform/<platform>/<platform>.mk */
  RCC_PLLCmd( ENABLE );
  
  while ( RCC_GetFlagStatus( RCC_FLAG_PLLRDY ) == RESET )
  {
  }
  RCC_SYSCLKConfig( SYSTEM_CLOCK_SOURCE );
  
  while ( RCC_GetSYSCLKSource( ) != 0x08 )
  {
  }
  
  /* Configure HCLK clock as SysTick clock source. */
  SysTick_CLKSourceConfig( SYSTICK_CLOCK_SOURCE );
}

WEAK void init_memory( void )
{
  
}

void init_architecture( void )
{
  uint8_t i;
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  
   /*STM32 wakeup by watchdog in standby mode, re-enter standby mode in this situation*/
  if ( (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) && RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET){
     RCC_ClearFlag();
     MicoSystemStandBy(MICO_WAIT_FOREVER);
   }
  PWR_ClearFlag(PWR_FLAG_SB);
  
  if ( stm32_platform_inited == 1 )
    return;
  
  /* Initialise the interrupt priorities to a priority lower than 0 so that the BASEPRI register can mask them */
  for ( i = 0; i < 81; i++ )
  {
    NVIC ->IP[i] = 0xff;
  }
  
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );
  
#ifndef MICO_DISABLE_STDIO
#ifndef NO_MICO_RTOS
  mico_rtos_init_mutex( &stdio_tx_mutex );
  mico_rtos_unlock_mutex ( &stdio_tx_mutex );
  mico_rtos_init_mutex( &stdio_rx_mutex );
  mico_rtos_unlock_mutex ( &stdio_rx_mutex );
#endif
  ring_buffer_init  ( (ring_buffer_t*)&stdio_rx_buffer, (uint8_t*)stdio_rx_data, STDIO_BUFFER_SIZE );
  MicoStdioUartInitialize( &stdio_uart_config, (ring_buffer_t*)&stdio_rx_buffer );
#endif

#ifdef NO_MICO_RTOS
  check_string(SysTick_Config(mico_cpu_clock_hz / 1000)==0, "Systemtick initialize failed!");
#endif
  
  /* Ensure 802.11 device is in reset. */
  host_platform_init( );
  
  MicoRtcInitialize();
  
  /* Disable MCU powersave at start-up. Application must explicitly enable MCU powersave if desired */
  MCU_CLOCKS_NEEDED();
  
  stm32_platform_inited = 1;  
}

/******************************************************
*            Interrupt Service Routines
******************************************************/



#ifndef MICO_DISABLE_MCU_POWERSAVE
static int stm32f2_clock_needed_counter = 0;
#endif

void MCU_CLOCKS_NEEDED( void )
{
#ifndef MICO_DISABLE_MCU_POWERSAVE
  DISABLE_INTERRUPTS;
  if ( stm32f2_clock_needed_counter <= 0 )
  {
    SCB->SCR &= (~((unsigned long)SCB_SCR_SLEEPDEEP_Msk));
    stm32f2_clock_needed_counter = 0;
  }
  stm32f2_clock_needed_counter++;
  ENABLE_INTERRUPTS;
#else
  return;
#endif
}

void MCU_CLOCKS_NOT_NEEDED( void )
{
#ifndef MICO_DISABLE_MCU_POWERSAVE
  DISABLE_INTERRUPTS;
  stm32f2_clock_needed_counter--;
  if ( stm32f2_clock_needed_counter <= 0 )
  {
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    stm32f2_clock_needed_counter = 0;
  }
  ENABLE_INTERRUPTS;
#else
  return;
#endif
}

#ifndef MICO_DISABLE_MCU_POWERSAVE

#define WUT_COUNTER_MAX  0xFFFF

static OSStatus select_wut_prescaler_calculate_wakeup_time( unsigned long* wakeup_time, unsigned long delay_ms, unsigned long* scale_factor )
{
  unsigned long temp;
  bool scale_factor_is_found = false;
  int i                              = 0;
  
  static unsigned long int available_wut_prescalers[] =
  {
    RTC_WakeUpClock_RTCCLK_Div2,
    RTC_WakeUpClock_RTCCLK_Div4,
    RTC_WakeUpClock_RTCCLK_Div8,
    RTC_WakeUpClock_RTCCLK_Div16
  };
  static unsigned long scale_factor_values[] = { 2, 4, 8, 16 };
  
  if ( delay_ms == 0xFFFFFFFF )
  {
    /* wake up in a 100ms, since currently there may be no tasks to run, but after a few milliseconds */
    /* some of them can get unblocked( for example a task is blocked on mutex with unspecified ) */
    *scale_factor = 2;
    RTC_WakeUpClockConfig( RTC_WakeUpClock_RTCCLK_Div2 );
    *wakeup_time = NUMBER_OF_LSE_TICKS_PER_MILLISECOND( scale_factor_values[0] ) * 100;
  }
  else
  {
    for ( i = 0; i < 4; i++ )
    {
      temp = NUMBER_OF_LSE_TICKS_PER_MILLISECOND( scale_factor_values[i] ) * delay_ms;
      if ( temp < WUT_COUNTER_MAX )
      {
        scale_factor_is_found = true;
        *wakeup_time = temp;
        *scale_factor = scale_factor_values[i];
        break;
      }
    }
    if ( scale_factor_is_found )
    {
      /* set new prescaler for wakeup timer */
      RTC_WakeUpClockConfig( available_wut_prescalers[i] );
    }
    else
    {
      /* scale factor can not be picked up for delays more that 32 seconds when RTCLK is selected as a clock source for the wakeup timer
      * for delays more than 32 seconds change from RTCCLK to 1Hz ck_spre clock source( used to update calendar registers ) */
      RTC_WakeUpClockConfig( RTC_WakeUpClock_CK_SPRE_16bits );
      
      /* with 1Hz ck_spre clock source the resolution changes to seconds  */
      *wakeup_time = ( delay_ms / 1000 ) + 1;
      *scale_factor = CK_SPRE_CLOCK_SOURCE_SELECTED;
      
      return kGeneralErr;
    }
  }
  
  return kNoErr;
}

void wake_up_interrupt_notify( void )
{
  wake_up_interrupt_triggered = true;
}


static unsigned long stop_mode_power_down_hook( unsigned long delay_ms )
{
  unsigned long retval;
  unsigned long wut_ticks_passed;
  unsigned long scale_factor = 0;
  UNUSED_PARAMETER(delay_ms);
  UNUSED_PARAMETER(rtc_timeout_start_time);
  UNUSED_PARAMETER(scale_factor);
  
  if ( ( ( SCB->SCR & (unsigned long)SCB_SCR_SLEEPDEEP_Msk) != 0) && delay_ms < 5 ){
    SCB->SCR &= (~((unsigned long)SCB_SCR_SLEEPDEEP_Msk));
    __asm("wfi");
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    /* Note: We return 0 ticks passed because system tick is still going when wfi instruction gets executed */
    ENABLE_INTERRUPTS;
    return 0;
  }
  
  if ( ( ( SCB->SCR & (unsigned long)SCB_SCR_SLEEPDEEP_Msk) ) != 0 )
  {
    /* pick up the appropriate prescaler for a requested delay */
    select_wut_prescaler_calculate_wakeup_time(&rtc_timeout_start_time, delay_ms, &scale_factor );
    DISABLE_INTERRUPTS;
    
    SysTick->CTRL &= (~(SysTick_CTRL_TICKINT_Msk|SysTick_CTRL_ENABLE_Msk)); /* systick IRQ off */
    RTC_ITConfig(RTC_IT_WUT, ENABLE);
    
    EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
    PWR_ClearFlag(PWR_FLAG_WU);
    RTC_ClearFlag(RTC_FLAG_WUTF);
    
    RTC_SetWakeUpCounter( rtc_timeout_start_time );
    RTC_WakeUpCmd( ENABLE );
    rtc_sleep_entry();
    
    DBGMCU->CR |= 0x03; /* Enable debug in stop mode */
    
    /* This code will be running with BASEPRI register value set to 0, the main intention behind that is that */
    /* all interrupts must be allowed to wake the CPU from the power-down mode */
    /* the PRIMASK is set to 1( see DISABLE_INTERRUPTS), thus we disable all interrupts before entering the power-down mode */
    /* This may sound contradictory, however according to the ARM CM3 documentation power-management unit */
    /* takes into account only the contents of the BASEPRI register and it is an external from the CPU core unit */
    /* PRIMASK register value doesn't affect its operation. */
    /* So, if the interrupt has been triggered just before the wfi instruction */
    /* it remains pending and wfi instruction will be treated as a nop  */
    __asm("wfi");
    
    /* After CPU exits powerdown mode, the processer will not execute the interrupt handler(PRIMASK is set to 1) */
    /* Disable rtc for now */
    RTC_WakeUpCmd( DISABLE );
    RTC_ITConfig(RTC_IT_WUT, DISABLE);
    
    /* Initialise the clocks again */
    init_clocks( );
    
    /* Enable CPU ticks */
    SysTick->CTRL |= (SysTick_CTRL_TICKINT_Msk|SysTick_CTRL_ENABLE_Msk);
    
    /* Get the time of how long the sleep lasted */
    wut_ticks_passed = rtc_timeout_start_time - RTC_GetWakeUpCounter();
    UNUSED_VARIABLE(wut_ticks_passed);
    rtc_sleep_exit( delay_ms, &retval );
    /* as soon as interrupts are enabled, we will go and execute the interrupt handler */
    /* which triggered a wake up event */
    ENABLE_INTERRUPTS;
    wake_up_interrupt_triggered = false;
    return retval;
  }
  else
  {
    UNUSED_PARAMETER(wut_ticks_passed);
    ENABLE_INTERRUPTS;
    __asm("wfi");
    
    /* Note: We return 0 ticks passed because system tick is still going when wfi instruction gets executed */
    return 0;
  }
}

#else /* MICO_DISABLE_MCU_POWERSAVE */
static unsigned long idle_power_down_hook( unsigned long delay_ms  )
{
  UNUSED_PARAMETER( delay_ms );
  ENABLE_INTERRUPTS;
  __asm("wfi");
  return 0;
}

#endif /* MICO_DISABLE_MCU_POWERSAVE */


unsigned long platform_power_down_hook( unsigned long delay_ms )
{
#ifndef MICO_DISABLE_MCU_POWERSAVE
  return stop_mode_power_down_hook( delay_ms );
#else
  return idle_power_down_hook( delay_ms );
#endif
}

void RTC_WKUP_irq( void )
{
  EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
}

void platform_idle_hook( void )
{
  __asm("wfi");
}

void MicoSystemReboot(void)
{ 
  NVIC_SystemReset();
}

void MicoSystemStandBy(uint32_t secondsToWakeup)
{ 
  mico_rtc_time_t time;
  uint32_t currentSecond;
  RTC_AlarmTypeDef  RTC_AlarmStructure;

  PWR_WakeUpPinCmd(ENABLE);

  if(secondsToWakeup == MICO_WAIT_FOREVER)
    PWR_EnterSTANDBYMode();

  platform_log("Wake up in %d seconds", secondsToWakeup);
 
  MicoRtcGetTime(&time);
  currentSecond = time.hr*3600 + time.min*60 + time.sec;
  currentSecond += secondsToWakeup;
  RTC_AlarmStructure.RTC_AlarmTime.RTC_H12     = RTC_HourFormat_24;
  RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours   = currentSecond/3600%24;
  RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes = currentSecond/60%60;
  RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds = currentSecond%60;
  RTC_AlarmStructure.RTC_AlarmDateWeekDay = 0x31;
  RTC_AlarmStructure.RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_Date;
  RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay ;

  RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
  /* Disable the Alarm A */
  RTC_ITConfig(RTC_IT_ALRA, DISABLE);

  /* Clear RTC Alarm Flag */ 
  RTC_ClearFlag(RTC_FLAG_ALRAF);

  RTC_SetAlarm(RTC_Format_BIN, RTC_Alarm_A, &RTC_AlarmStructure);

  /* Enable RTC Alarm A Interrupt: this Interrupt will wake-up the system from
     STANDBY mode (RTC Alarm IT not enabled in NVIC) */
  RTC_ITConfig(RTC_IT_ALRA, ENABLE);

  /* Enable the Alarm A */
  RTC_AlarmCmd(RTC_Alarm_A, ENABLE);

  PWR_EnterSTANDBYMode();
}

void MicoMcuPowerSaveConfig( int enable )
{
  if (enable == 1)
    MCU_CLOCKS_NOT_NEEDED();
  else
    MCU_CLOCKS_NEEDED();
}



#ifdef NO_MICO_RTOS
static volatile uint32_t no_os_tick = 0;

void SysTick_Handler(void)
{
  no_os_tick ++;
  IWDG_ReloadCounter();
}

uint32_t mico_get_time_no_os(void)
{
  return no_os_tick;
}

void mico_thread_msleep_no_os(uint32_t milliseconds)
{
  int tick_delay_start = mico_get_time_no_os();
  while(mico_get_time_no_os() < tick_delay_start+milliseconds);  
}
#endif

