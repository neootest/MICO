/**
******************************************************************************
* @file    gpio_irq.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provides GPIO external interrupt operation functions.
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

#include "gpio_irq.h"
#include "string.h"

/******************************************************
*                      Macros
******************************************************/

/******************************************************
*                    Constants
******************************************************/

#define NUMBER_OF_GPIO_IRQ_LINES (16)

/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/

/******************************************************
*                    Structures
******************************************************/

typedef struct
{
  gpio_port_t*       owner_port;
  gpio_irq_handler_t handler;
  void*              arg;
} gpio_irq_data_t;

/******************************************************
*               Function Declarations
******************************************************/

static uint8_t convert_port_to_port_number( gpio_port_t* port );

/******************************************************
*               Variables Definitions
******************************************************/

static volatile gpio_irq_data_t gpio_irq_data[NUMBER_OF_GPIO_IRQ_LINES];
static uint8_t gpio_irq_management_initted = 0;

/******************************************************
*               Function Definitions
******************************************************/

OSStatus gpio_irq_enable( gpio_port_t* gpio_port, gpio_pin_number_t gpio_pin_number, gpio_irq_trigger_t trigger, gpio_irq_handler_t handler, void* arg )
{
  uint32_t interrupt_line = (uint32_t) ( 1 << gpio_pin_number );
  
  if ( gpio_irq_management_initted == 0 )
  {
    memset( (void*)gpio_irq_data, 0, sizeof( gpio_irq_data ) );
    
    /* Switch on SYSCFG peripheral clock to allow writing into SYSCFG registers */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_SYSCFG, ENABLE );
    
    gpio_irq_management_initted = 1;
  }
  
  if ( ( EXTI->IMR & interrupt_line ) == 0 )
  {
    NVIC_InitTypeDef nvic_init_structure;
    EXTI_InitTypeDef exti_init_structure;
    
    if (gpio_pin_number == 10) {
      /* This is STM32 Bug, please check at:
      * https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/Flat.aspx?RootFolder=%2Fpublic%2FSTe2ecommunities%2Fmcu%2FLists%2Fcortex_mx_stm32%2FGPIO%20Interrupt%20Issue&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=581#{B7C01461-8CEC-4711-B331-64F4AECF4786}
      * ERRATA sheet in section 2.1.8.
      Configuration of PH10 and PI10 as external interrupts is erroneous
      
      Description
      
      PH10 or PI10 is selected as the source for the EXTI10 external interrupt by setting bits
      EXTI10[3:0] of SYSCFG_EXTICR3 register to 0x0111 or 0x1000, respectively. However,
      this erroneous operation enables PH2 and PI2 as external interrupt inputs.
      
      As a result, it is not possible to use PH10/PI10 as interrupt sources if PH2/PI2 are not
      selected as the interrupt source, as well. This means that bits EXTI10[3:0] of
      
      SYSCFG_EXTICR3 register and bits EXTI2[3:0] of SYSCFG_EXTICR1 should be
      programmed to the same value:
      ¡ñ 0x0111 to select PH10/PH2
      ¡ñ 0x1000 to select PI10/PI2
      
      */
      if (gpio_port == GPIOH)
        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOH, EXTI_PinSource2); 
      else if (gpio_port == GPIOI)
        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOI, EXTI_PinSource2);
    }
    SYSCFG_EXTILineConfig( convert_port_to_port_number( gpio_port ), gpio_pin_number );
    
    if ( trigger == IRQ_TRIGGER_BOTH_EDGES )
    {
      exti_init_structure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    }
    else if ( trigger == IRQ_TRIGGER_FALLING_EDGE )
    {
      exti_init_structure.EXTI_Trigger = EXTI_Trigger_Falling;
    }
    else if ( trigger == IRQ_TRIGGER_RISING_EDGE )
    {
      exti_init_structure.EXTI_Trigger = EXTI_Trigger_Rising;
    }
    
    exti_init_structure.EXTI_Line    = interrupt_line;
    exti_init_structure.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti_init_structure.EXTI_LineCmd = ENABLE;
    EXTI_Init( &exti_init_structure );
    
    if ( gpio_pin_number <= 4 )
    {
      nvic_init_structure.NVIC_IRQChannel = (uint8_t) ( EXTI0_IRQn + gpio_pin_number );
    }
    else if ( gpio_pin_number <= 9 )
    {
      nvic_init_structure.NVIC_IRQChannel = EXTI9_5_IRQn;
    }
    else if ( gpio_pin_number <= 15 )
    {
      nvic_init_structure.NVIC_IRQChannel = EXTI15_10_IRQn;
    }
    
    /* Must be lower priority than the value of configMAX_SYSCALL_INTERRUPT_PRIORITY otherwise FreeRTOS will not be able to mask the interrupt */
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0xE;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x0C;
    nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init( &nvic_init_structure );
    
    gpio_irq_data[gpio_pin_number].owner_port = gpio_port;
    gpio_irq_data[gpio_pin_number].handler    = handler;
    gpio_irq_data[gpio_pin_number].arg        = arg;
    return kNoErr;
  }
  
  return kGeneralErr;
}

OSStatus gpio_irq_disable( gpio_port_t* gpio_port, gpio_pin_number_t gpio_pin_number )
{
  uint32_t interrupt_line     = (uint32_t) ( 1 << gpio_pin_number );
  uint8_t interrupt_line_used = 0;
  
  if ( ( EXTI->IMR & interrupt_line ) && gpio_irq_data[gpio_pin_number].owner_port == gpio_port )
  {
    NVIC_InitTypeDef nvic_init_structure;
    EXTI_InitTypeDef exti_init_structure;
    
    /* Disable EXTI interrupt line */
    exti_init_structure.EXTI_Line    = (uint32_t) ( 1 << gpio_pin_number );
    exti_init_structure.EXTI_LineCmd = DISABLE;
    exti_init_structure.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti_init_structure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_Init( &exti_init_structure );
    exti_init_structure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_Init( &exti_init_structure );
    
    /* Disable NVIC interrupt */
    if ( gpio_pin_number <= 4 )
    {
      nvic_init_structure.NVIC_IRQChannel = (uint8_t) ( EXTI0_IRQn + gpio_pin_number );
      interrupt_line_used = 0;
    }
    else if ( gpio_pin_number <= 9 )
    {
      nvic_init_structure.NVIC_IRQChannel = EXTI9_5_IRQn;
      interrupt_line_used = ( ( EXTI->IMR & 0x3e0U ) != 0 );
    }
    else if ( gpio_pin_number <= 15 )
    {
      nvic_init_structure.NVIC_IRQChannel = EXTI15_10_IRQn;
      interrupt_line_used = ( ( EXTI->IMR & 0xfc00U ) != 0 );
    }
    
    if ( !interrupt_line_used )
    {
      nvic_init_structure.NVIC_IRQChannelCmd                = DISABLE;
      nvic_init_structure.NVIC_IRQChannelPreemptionPriority = 0;
      nvic_init_structure.NVIC_IRQChannelSubPriority        = 0;
      NVIC_Init( &nvic_init_structure );
    }
    
    gpio_irq_data[gpio_pin_number].owner_port = 0;
    gpio_irq_data[gpio_pin_number].handler    = 0;
    gpio_irq_data[gpio_pin_number].arg        = 0;
    return kNoErr;
  }
  
  return kGeneralErr;
}

void gpio_irq( void )
{
  uint32_t active_interrupt_vector = (uint32_t) ( ( SCB->ICSR & 0x3fU ) - 16 );
  uint32_t gpio_number;
  uint32_t interrupt_line;
  
  switch ( active_interrupt_vector )
  {
  case EXTI0_IRQn:
    interrupt_line = EXTI_Line0;
    gpio_number = 0;
    break;
  case EXTI1_IRQn:
    interrupt_line = EXTI_Line1;
    gpio_number = 1;
    break;
  case EXTI2_IRQn:
    interrupt_line = EXTI_Line2;
    gpio_number = 2;
    break;
  case EXTI3_IRQn:
    interrupt_line = EXTI_Line3;
    gpio_number = 3;
    break;
  case EXTI4_IRQn:
    interrupt_line = EXTI_Line4;
    gpio_number = 4;
    break;
  case EXTI9_5_IRQn:
    interrupt_line = EXTI_Line5;
    for ( gpio_number = 5; gpio_number < 10 && ( EXTI->PR & interrupt_line ) == 0; gpio_number++ )
    {
      interrupt_line <<= 1;
    }
    break;
  case EXTI15_10_IRQn:
    interrupt_line = EXTI_Line10;
    for ( gpio_number = 10; gpio_number < 16 && ( EXTI->PR & interrupt_line ) == 0; gpio_number++ )
    {
      interrupt_line <<= 1;
    }
    break;
  default:
    return;
  }
  
  /* Clear interrupt flag */
  EXTI->PR = interrupt_line;
  
  /* Call the respective GPIO interrupt handler/callback */
  if ( gpio_irq_data[gpio_number].handler != NULL )
  {
    void * arg = gpio_irq_data[gpio_number].arg; /* Avoids undefined order of access to volatiles */
    gpio_irq_data[gpio_number].handler( arg );
  }
}

static uint8_t convert_port_to_port_number( gpio_port_t* port )
{
  switch ( (int) port )
  {
  case GPIOA_BASE:
    return EXTI_PortSourceGPIOA;
  case GPIOB_BASE:
    return EXTI_PortSourceGPIOB;
  case GPIOC_BASE:
    return EXTI_PortSourceGPIOC;
  case GPIOD_BASE:
    return EXTI_PortSourceGPIOD;
  case GPIOE_BASE:
    return EXTI_PortSourceGPIOE;
  case GPIOF_BASE:
    return EXTI_PortSourceGPIOF;
  case GPIOG_BASE:
    return EXTI_PortSourceGPIOG;
  case GPIOH_BASE:
    return EXTI_PortSourceGPIOH;
  case GPIOI_BASE:
    return EXTI_PortSourceGPIOI;
  default:
    return 0;
  }
}

/*EXTI ISR*/
void EXTI0_IRQHandler(void)
{
  gpio_irq();//SDIO OOB interrupt
}

void EXTI1_IRQHandler(void)
{
  gpio_irq();
}

void EXTI2_IRQHandler(void)
{
  gpio_irq();
}

void EXTI3_IRQHandler(void)
{
  gpio_irq();//User defined external interrupt, EMW3162 button 1: PA3
}

void EXTI4_IRQHandler(void)
{
  gpio_irq();
}

void EXTI9_5_IRQHandler(void)
{
  gpio_irq(); //User defined external interrupt, EMW3161 button 1: PH9
}

void EXTI15_10_IRQHandler(void)
{
  gpio_irq();
}
