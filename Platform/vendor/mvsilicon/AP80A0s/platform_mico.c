/***
 *@file     platform_mico.c
 *@author   Jerry Yu
 *@Version  0.0.1
 *@Date     Jan-22nd, 2015
 *@brief    Implements interfaces of platform for Mico.
*  The MIT License
*  Copyright © 2015 MXCHIP Inc.
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
 * */

#include "MicoPlatform.h"
#include "PlatformInternal.h"
#include "crt0.h"
#include "platform_mico.h"



#ifndef MICO_DISABLE_STDIO

#ifndef STDIO_BUFFER_SIZE
#define STDIO_BUFFER_SIZE   64
#endif
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

//SPI_FLASH_INFO flash_info = {0};

void Osc32kExtCapCalibrate(void){
    unsigned short Val;
    char        cmd[3] = "\x35\xBA\x69";
    SPI_FLASH_INFO flash_info = {0};

    SpiFlashGetInfo(&flash_info);
    
    if(SpiFlashIOCtl(IOCTL_FLASH_UNPROTECT, cmd, sizeof(cmd)) != FLASH_NONE_ERR){
        return;
    }
    if(SpiFlashRead(0xA0, (unsigned char*)&Val, 2) != FLASH_NONE_ERR){
        return;
    }
    ClkOsc32kNoNeedExtCapacitanceSet((char)Val, (char)(Val >> 8));
}

/*Boot to mico application form APPLICATION_START_ADDRESS */
void startApplication( void ){

}

/* STM32F2 common clock initialisation function
* This brings up enough clocks to allow the processor to run quickly while initialising memory.
* Other platform specific clock init can be done in init_platform() or init_architecture()
* This a WEAK function.
*/
WEAK void init_clocks( void ){
}

/* WEAK function */
WEAK void init_memory( void ){

}

void init_architecture( void) {

    ClkPorRcToDpll(0); // 0: 32768Hz, from Rc48MHz to Dpll sysclk 96Mhz 
    CacheInit(); // TBD! maybe mv to above.
    ClkModuleEn(MICO_MODULE_CLK_SWITCH);
    ClkModuleGateEn(MICO_MODULE_CLK_GATE_SWITCH);

    WaitMs(200);
   // WdgDis();//TBD!
    
    SysGetWakeUpFlag();

    SysPowerKeyInit(POWERKEY_MODE_SLIDE_SWITCH, 500);//refer to power monitor.

    //Flash
    SpiFlashInfoInit();

 //   ClkPorRcToDpll(0); // 0: 32768Hz, from Rc48MHz to Dpll sysclk 96Mhz 

    //GD flash , True mean enable hpm.
    SpiFlashClkSet(FLASHCLK_SYSCLK_SEL,TRUE);

    //SarAdcLdoinVolInit();
    //LcdCtrlRegInit();
   //Osc32kExtCapCalibrate();//32KHz external oscillator calibration

#ifndef MICO_DISABLE_STDIO
//    OsSetDebugFlag(1);
#ifndef NO_MICO_RTOS
   mico_rtos_init_mutex( &stdio_tx_mutex );
   mico_rtos_unlock_mutex ( &stdio_tx_mutex );
   mico_rtos_init_mutex( &stdio_rx_mutex );
   mico_rtos_unlock_mutex ( &stdio_rx_mutex );
#endif
 //  ring_buffer_init  ( (ring_buffer_t*)&stdio_rx_buffer, (uint8_t*)stdio_rx_data, STDIO_BUFFER_SIZE );
   MicoStdioUartInitialize( &stdio_uart_config, (ring_buffer_t*)&stdio_rx_buffer );
#else  
//   OsSetDebugFlag(0);
#endif
    printf("uart initialized.\n");
   Osc32kExtCapCalibrate();//32KHz external oscillator calibration
#ifdef FUNC_BREAKPOINT_EN
   BP_LoadInfo();// TBD!
#endif 
 //  CacheInit(); // TBD! maybe mv to above.
 //   WdgFeed();
    printf("System Clock : %d \n",ClkGetCurrentSysClkFreq());
    printf("Flash Clock : %d \n",ClkFshcClkSelGet());
    printf("out init_architecture.\n");
}

void MCU_CLOCKS_NEEDED( void )
{
  
  // 有外设操作的低功耗模式
  return;

}

void MCU_CLOCKS_NOT_NEEDED( void )
{
// 进入低功耗
  return;

}


void wake_up_interrupt_notify( void )
{
//  wake_up_interrupt_triggered = true;
}


// Power Down??
unsigned long platform_power_down_hook( unsigned long delay_ms )
{
  return 0; // return 真实的睡眠时间
}

void platform_idle_hook( void )
{
}

void MicoSystemReboot(void)
{ 
  NVIC_SystemReset();
}

void MicoSystemStandBy(uint32_t timeOut)
{ 
	
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

void FLASH_Unlock(void){

}

void FLASH_Lock(void){
}

FLASH_Status FLASH_ProgramByte(uint32_t Address, uint8_t Data){
    return FLASH_COMPLETE;
}

void FLASH_ClearFlag(uint32_t FLASH_FLAG){

}

