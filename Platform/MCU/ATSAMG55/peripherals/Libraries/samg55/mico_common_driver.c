/**
******************************************************************************
* @file    mico_common_driver.c 
* @author  jerry yu
* @version V1.0.0
* @date    Mar.11st 2015
* @brief   impelments the mico kernel apis. 
******************************************************************************
*
*  the MIT license
*  copyright (c) 2015 MXCHIP inc.
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

#include "mico_common_driver.h"
#include "mico_platform.h"
#include "crt0.h"
#include "platform_logging.h"

/******************************************************
*                    constants
******************************************************/

#ifndef STDIO_BUFFER_SIZE
#define STDIO_BUFFER_SIZE   64
#endif

/******************************************************
*               function declarations
******************************************************/
void MCU_CLOCKS_NEEDED    ( void );
void MCU_CLOCKS_NOT_NEEDED( void );

void wake_up_interrupt_notify( void );

extern OSStatus host_platform_init( void );

/******************************************************
*               variables definitions
******************************************************/
/* mico_cpu_clock_hz is used by MICO RTOS */
const uint32_t mico_cpu_clock_hz = 120000000;

static bool platform_was_initialized = 0;

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
static volatile uint8_t       stdio_rx_data[STDIO_BUFFER_SIZE];
mico_mutex_t        stdio_rx_mutex;
mico_mutex_t        stdio_tx_mutex;
#endif /* #ifndef MICO_DISABLE_STDIO */

#ifndef MICO_DISABLE_MCU_POWERSAVE
static bool wake_up_interrupt_triggered = false;
#endif

#if defined ( __ICCARM__ )
static inline void __jump_to( uint32_t addr )
{
  __asm( "MOV R1, #0x00000001" );
  __asm( "ORR R0, R0, R1" );  /* last bit of jump address indicates whether destination is thumb or ARM code */
  __asm( "BLX R0" );
}


#elif defined ( __GNUC__ )
__attribute__( ( always_inline ) ) static __INLINE void __jump_to( uint32_t addr )
{
  addr |= 0x00000001;  /* last bit of jump address indicates whether destination is thumb or ARM code */
  __ASM volatile ("BX %0" : : "r" (addr) );
}


#elif defined ( __CC_ARM )
static void __asm __jump_to( uint32_t addr )
{
  MOV R1, #0x00000001
  ORR R0, R0, R1  /* last bit of jump address indicates whether destination is thumb or ARM code */
  BLX R0
}
#endif

/*boot to mico application form APPLICATION_START_ADDRESS defined in platform.h */
void start_application(void)
{
  uint32_t text_addr = APPLICATION_START_ADDRESS;
  uint32_t* stack_ptr;
  uint32_t* start_ptr;
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  
  if (((*(volatile uint32_t*)text_addr) & 0x2FFE0000 ) != 0x20000000)
  text_addr += 0x200;
  /* test if user code is programmed starting from address "application_address" */
  if (((*(volatile uint32_t*)text_addr) & 0x2FFE0000 ) == 0x20000000)
  { 
    stack_ptr = (uint32_t*) text_addr;  /* initial stack pointer is first 4 bytes of vector table */
    start_ptr = ( stack_ptr + 1 );  /* reset vector is second 4 bytes of vector table */

    #if defined ( __ICCARM__)
    __ASM( "MOV LR,        #0xFFFFFFFF" );
    __ASM( "MOV R1,        #0x01000000" );
    __ASM( "MSR APSR_nzcvq,     R1" );
    __ASM( "MOV R1,        #0x00000000" );
    __ASM( "MSR PRIMASK,   R1" );
    __ASM( "MSR FAULTMASK, R1" );
    __ASM( "MSR BASEPRI,   R1" );
    __ASM( "MSR CONTROL,   R1" );
    #endif
    
    __set_MSP( *stack_ptr );
    __jump_to( *start_ptr );
  }  
}
#if 1
/**
 *  Configure UART console.Jer. TBD!
 */
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
#ifdef CONF_UART_CHAR_LENGTH
		.charlength = CONF_UART_CHAR_LENGTH,
#endif
		.paritytype = CONF_UART_PARITY,
#ifdef CONF_UART_STOP_BITS
		.stopbits = CONF_UART_STOP_BITS,
#endif
	};

	/* Configure console UART. */
	stdio_serial_init(CONF_UART, &uart_serial_options);
}
#endif
WEAK void init_clocks( void ){
    sysclk_init();

}

WEAK void init_memory( void ){
}

static void set_irqn_priority( void )
{
    int32_t i, pri;
    for (i = NonMaskableInt_IRQn;i< PERIPH_COUNT_IRQn; i++) {
        if ( i == -13 || i==-3 || i == 10 ) continue;
        if ((i < -5) && (i > -10)) continue;
        if ( (i < -3) || i == 2  ) {
           // if ( i ==-5 )
           //     NVIC_SetPriority(i, 3);
           // else
            continue; 
        } else {
            NVIC_SetPriority(i, 15);
        }
    }
}

void init_architecture( void) {
    sysclk_init();
	ioport_init();
    if (platform_was_initialized) {
        return;
    }

    board_init();
    //set_irqn_priority();
#ifndef MICO_DISABLE_STDIO
#ifndef NO_MICO_RTOS
    mico_rtos_init_mutex( &stdio_tx_mutex );
    mico_rtos_unlock_mutex ( &stdio_tx_mutex );
    mico_rtos_init_mutex( &stdio_rx_mutex );
    mico_rtos_unlock_mutex ( &stdio_rx_mutex );
#endif
#if 0
    configure_console();
#else
    ring_buffer_init  ( (ring_buffer_t*)&stdio_rx_buffer, (uint8_t*)stdio_rx_data, STDIO_BUFFER_SIZE );
    mico_stdio_uart_initialize( &stdio_uart_config, (ring_buffer_t*)&stdio_rx_buffer );
#endif
#endif 

#ifdef NO_MICO_RTOS
    check_string(SysTick_Config(mico_cpu_clock_hz / 1000)==0, "systemtick initialize failed!");
#endif
#ifndef BOOTLOADER 
    /* ensure 802.11 device is in reset. */
    host_platform_init( );
#endif
    MCU_CLOCKS_NEEDED();
    platform_was_initialized = 1;
}

void MCU_CLOCKS_NEEDED( void ){
    return;
}

void MCU_CLOCKS_NOT_NEEDED( void ){
    return;
}

void RTC_WKUP_irq( void )
{
  //EXTI_ClearITPendingBit( RTC_INTERRUPT_EXTI_LINE );
}

void platform_idle_hook( void )
{
  __asm("wfi");
}

#ifndef MICO_DISABLE_MCU_POWERSAVE
void wake_up_interrupt_notify( void )
{
    wake_up_interrupt_triggered = true;
}
#endif

static unsigned long stop_mode_power_down_hook( unsigned long delay_ms )
{
  UNUSED_PARAMETER( delay_ms );
  ENABLE_INTERRUPTS;//Jer TBD!
  __asm("wfi");
  return 0;
}

static unsigned long idle_power_down_hook( unsigned long delay_ms  )
{
  UNUSED_PARAMETER( delay_ms );
  ENABLE_INTERRUPTS;
  __asm("wfi");
  return 0;
}

unsigned long platform_power_down_hook( unsigned long delay_ms ){
#ifndef MICO_DISABLE_MCU_POWERSAVE
    return stop_mode_power_down_hook( delay_ms );
#else
   return idle_power_down_hook(delay_ms);
#endif
}

void mico_mcu_power_save_config( int enable ){
}

void mico_system_reboot(void){
    NVIC_SystemReset();
}

void mico_system_stand_by(uint32_t seconds_to_wakeup){

}

#ifdef NO_MICO_RTOS
static volatile uint32_t no_os_tick = 0;

void SysTick_Handler(void)
{
  no_os_tick ++;
  //IWDG_ReloadCounter();
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

void hard_fault_handler_c (unsigned int * hardfault_args)
{
  unsigned int stacked_r0;
  unsigned int stacked_r1;
  unsigned int stacked_r2;
  unsigned int stacked_r3;
  unsigned int stacked_r12;
  unsigned int stacked_lr;
  unsigned int stacked_pc;
  unsigned int stacked_psr;

  stacked_r0 = ((unsigned long) hardfault_args[0]);
  stacked_r1 = ((unsigned long) hardfault_args[1]);
  stacked_r2 = ((unsigned long) hardfault_args[2]);
  stacked_r3 = ((unsigned long) hardfault_args[3]);
 
  stacked_r12 = ((unsigned long) hardfault_args[4]);
  stacked_lr = ((unsigned long) hardfault_args[5]);
  stacked_pc = ((unsigned long) hardfault_args[6]);
  stacked_psr = ((unsigned long) hardfault_args[7]);
    printf("hardfault:\r\n");
    printf("r0  = 0x%x\r\n",stacked_r0);
    printf("r1  = 0x%x\r\n",stacked_r1);
    printf("r2  = 0x%x\r\n",stacked_r2);
    printf("r3  = 0x%x\r\n",stacked_r3);
    printf("r12 = 0x%x\r\n",stacked_r12);
    printf("lr  = 0x%x\r\n",stacked_lr);
    printf("pc  = 0x%x\r\n",stacked_pc);
    printf("psr = 0x%x\r\n",stacked_psr);
    
    while(1) {
    }

}

void HardFault_Handler(void)
{
 __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " b hard_fault_handler_c                                    \n"
    );
    
}


