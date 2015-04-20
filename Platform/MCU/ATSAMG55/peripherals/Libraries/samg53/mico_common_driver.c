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
const uint32_t mico_cpu_clock_hz = 48000000;

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
static bool wake_up_interrupt_triggered = false;
#endif

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
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	pio_configure_pin_group(CONF_UART_PIO, CONF_PINS_UART,
			CONF_PINS_UART_FLAGS);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}

WEAK void init_clocks( void ){
    sysclk_init();

}

WEAK void init_memory( void ){
}

void init_architecture( void) {
    sysclk_init();
    board_init(); //Jer will rm.
    configure_console();
#ifndef MICO_DISABLE_STDIO
#ifndef NO_MICO_RTOS
    mico_rtos_init_mutex( &stdio_tx_mutex );
    mico_rtos_unlock_mutex ( &stdio_tx_mutex );
    mico_rtos_init_mutex( &stdio_rx_mutex );
    mico_rtos_unlock_mutex ( &stdio_rx_mutex );
#endif
    //ring_buffer_init  ( (ring_buffer_t*)&stdio_rx_buffer, (uint8_t*)stdio_rx_data, STDIO_BUFFER_SIZE );
    //mico_stdio_uart_initialize( &stdio_uart_config, (ring_buffer_t*)&stdio_rx_buffer );
#endif

    platform_log("%d\r\n",SystemCoreClock);
    SysTick_Config(SystemCoreClock / 1000);
    
    MCU_CLOCKS_NEEDED();
}


void start_application( void ){
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

void wake_up_interrupt_notify( void )
{
    wake_up_interrupt_triggered = true;
}

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


