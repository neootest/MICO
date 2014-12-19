/*
 * @brief Common SystemInit function for LPC412x chips
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

 #include "board.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Set up and initialize hardware prior to call to main */
void SystemInit(void)
{
  LPC_SYSCTL->SYSAHBCLKCTRL[0] |= 0x00000018; // Magicoe
#if defined(__FPU_PRESENT) && __FPU_PRESENT == 1
	fpuInit();
#endif

#if defined(NO_BOARD_LIB)
	/* Chip specific SystemInit */
	Chip_SystemInit();
#else
	/* Enable RAM 2 clock */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SRAM2);
	/* Board specific SystemInit */
	Board_SystemInit();
#endif
      LPC_SYSCTL->SYSAHBCLKCTRL[0] |= 0x00000018; // Magicoe
      
      Chip_SYSCTL_PowerUp(PDRUNCFG_PD_IRC_OSC_EN|PDRUNCFG_PD_IRC_EN);
      /* Configure PIN0.21 as CLKOUT with pull-up, monitor the MAINCLK on scope */
      Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 21, IOCON_MODE_PULLUP | IOCON_FUNC1 | IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);
      Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_RTC, 1);
      Chip_Clock_EnableRTCOsc();
      Chip_RTC_Init(LPC_RTC);
      Chip_RTC_Enable1KHZ(LPC_RTC);
      Chip_RTC_Enable(LPC_RTC);
}

volatile uint32_t timer0_cnt = 0;
void CTIMER0_IRQHandler(void)
{
  if (Chip_TIMER_MatchPending(LPC_CTIMER0, 1)) {
    Chip_TIMER_ClearMatch(LPC_CTIMER0, 1);
//  LPC_CTIMER0->IR = TIMER_IR_CLR(1);
//    Board_LED_Toggle(2);
    timer0_cnt++;
  }
}

extern void STATUS_GPIO_Init(void);
extern void BOOT_GPIO_Init(void);
extern void STANDBY_GPIO_Init(void);
extern void EASYLINK_GPIO_Init(void);

void set(void)
{
  SystemCoreClockUpdate();
  //Board_Init();
  LPC_SYSCTL->SYSAHBCLKCTRL[0] |= 0x18; // Magicoe
//  Chip_TIMER_Init(LPC_CTIMER0);
//  Chip_TIMER_PrescaleSet(LPC_CTIMER0, 100);
//  Chip_TIMER_Reset(LPC_CTIMER0);
//  Chip_TIMER_MatchEnableInt(LPC_CTIMER0, 1);
//  Chip_TIMER_SetMatch(LPC_CTIMER0, 1, SystemCoreClock/96);//(SystemCoreClock / 1000));
//  printf("SystemCoreClock %d\r\n", SystemCoreClock);
//  Chip_TIMER_ResetOnMatchEnable(LPC_CTIMER0, 1);
//  Chip_TIMER_Enable(LPC_CTIMER0);
  Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 29, (IOCON_FUNC0 | IOCON_GPIO_MODE | IOCON_MODE_PULLUP | IOCON_DIGITAL_EN));
  Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 29);
  Chip_GPIO_SetPinState(LPC_GPIO, 0, 29, 0);
  
  Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 30, (IOCON_FUNC0 | IOCON_GPIO_MODE | IOCON_MODE_PULLUP | IOCON_DIGITAL_EN));
  Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 30);
  Chip_GPIO_SetPinState(LPC_GPIO, 0, 30, 1);
  
  Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 31, (IOCON_FUNC0 | IOCON_GPIO_MODE | IOCON_MODE_PULLUP | IOCON_DIGITAL_EN));
  Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 31);
  Chip_GPIO_SetPinState(LPC_GPIO, 0, 31, 1);
  
  //host_platform_bus_init();
  
  STATUS_GPIO_Init();
  BOOT_GPIO_Init();
  STANDBY_GPIO_Init();
  EASYLINK_GPIO_Init();
//  NVIC_ClearPendingIRQ(CTIMER0_IRQn);
//  NVIC_EnableIRQ(CTIMER0_IRQn);
}