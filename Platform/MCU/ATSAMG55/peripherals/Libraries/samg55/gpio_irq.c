/**
******************************************************************************
* @file    gpio_irq.c 
* @author  william xu
* @version V1.0.0
* @date    05-may-2014
* @brief   this file provides GPIO external interrupt operation functions.
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

#include "gpio_irq.h"
#include "string.h"

/******************************************************
*                      macros
******************************************************/

/******************************************************
*                    constants
******************************************************/

#define MAX_INTERRUPT_SOURCES   (7)  //can be increase ,7 is the smallest

/******************************************************
*                   enumerations
******************************************************/

/******************************************************
*                 type definitions
******************************************************/

/******************************************************
*                    structures
******************************************************/

typedef struct
{
  gpio_port_t        owner_port;
  uint32_t           id;
  uint32_t           mask;
  gpio_irq_handler_t handler;
  void              *arg;
} gpio_irq_data_t;

/******************************************************
*               function declarations
******************************************************/

//static uint8_t convert_port_to_port_number( gpio_port_t* port );

/******************************************************
*               variables definitions
******************************************************/

static volatile gpio_irq_data_t gpio_irq_data[MAX_INTERRUPT_SOURCES];
static uint32_t gs_ul_nb_sources = 0;
static uint8_t gpio_irq_management_initted = 0;

/******************************************************
*               function definitions
******************************************************/

OSStatus gpio_irq_enable( gpio_port_t gpio_port, gpio_pin_number_t gpio_pin_number, gpio_irq_trigger_t trigger, gpio_irq_handler_t handler, void* arg )
{
    //gpio_irq_data_t *pSource; 
    uint32_t ul_attr;
    if (gs_ul_nb_sources >= MAX_INTERRUPT_SOURCES)
		return 1;
    //Pio *p_pio = (Pio *)ioport_port_to_base(gpio_port);
    Pio *p_pio = arch_ioport_port_to_base(gpio_port);
    //ioport_pin_t pin = CREATE_IOPORT_PIN(gpio_port, gpio_pin_number);
    ioport_port_mask_t ul_mask = ioport_pin_to_mask(CREATE_IOPORT_PIN(gpio_port, gpio_pin_number));

    if ( gpio_irq_management_initted == 0 )
    {
        memset( (void*)gpio_irq_data, 0, sizeof( gpio_irq_data ) );
        gpio_irq_management_initted = 1;
    }

    
    if (trigger == IRQ_TRIGGER_RISING_EDGE ) {
        ul_attr = PIO_IT_RISE_EDGE;     
    } else if (trigger == IRQ_TRIGGER_FALLING_EDGE ) {
        ul_attr = PIO_IT_FALL_EDGE;     
    } else if (trigger == IRQ_TRIGGER_BOTH_EDGES ) {
        ul_attr = PIO_IT_EDGE;     
    }
	//pSource = &(gpio_irq_data[gs_ul_nb_sources]);
    gpio_irq_data[gs_ul_nb_sources].owner_port = gpio_port;
	if ( gpio_port == PORTA) {
        gpio_irq_data[gs_ul_nb_sources].id     = ID_PIOA;
	   // pmc_enable_periph_clk(ID_PIOA);
    } else if (gpio_port == PORTB) {
        gpio_irq_data[gs_ul_nb_sources].id     = ID_PIOB;
	   // pmc_enable_periph_clk(ID_PIOB);
    }
    gpio_irq_data[gs_ul_nb_sources].mask       = ul_mask;
    gpio_irq_data[gs_ul_nb_sources].handler    = handler;
    gpio_irq_data[gs_ul_nb_sources].arg        = arg;
    gs_ul_nb_sources++;
	/* Configure interrupt mode */
	pio_configure_interrupt(p_pio, ul_mask, ul_attr);
	
	if ( gpio_port == PORTA){
        //NVIC_EnableIRQ( PIOA_IRQn );
	    //NVIC_SetPriority(PIOA_IRQn, 0xE); 
        //pio_handler_set_priority(PIOA, PIOA_IRQn, IRQ_PRIORITY_PIO);
        irq_register_handler(PIOA_IRQn, 0xE);
    } else if (gpio_port == PORTB) {
        //NVIC_EnableIRQ( PIOB_IRQn);
	    //NVIC_SetPriority(PIOB_IRQn, 0xE); 
        //pio_handler_set_priority(PIOB, PIOB_IRQn, IRQ_PRIORITY_PIO);
        irq_register_handler(PIOB_IRQn, 0xE);
    }
    pio_enable_interrupt(p_pio, ul_mask);
    
    return kGeneralErr;
}

OSStatus gpio_irq_disable( gpio_port_t gpio_port, gpio_pin_number_t gpio_pin_number )
{
    Pio *p_pio = arch_ioport_port_to_base(gpio_port);
    ioport_port_mask_t ul_mask = ioport_pin_to_mask(CREATE_IOPORT_PIN(gpio_port, gpio_pin_number));

    pio_disable_interrupt(p_pio, ul_mask);
  
  return kGeneralErr;
}

void gpio_irq(Pio *p_pio, uint32_t ul_id)
{
	uint32_t status;
	uint32_t i;

	/* Read PIO controller status */
	status = pio_get_interrupt_status(p_pio);
	status &= pio_get_interrupt_mask(p_pio);

	/* Check pending events */
	if (status != 0) {
		/* Find triggering source */
		i = 0;
		while (status != 0) {
			/* Source is configured on the same controller */
			if (gpio_irq_data[i].id == ul_id) {
				/* Source has PIOs whose statuses have changed */
				if ((status & gpio_irq_data[i].mask) != 0) {
                    void * arg = gpio_irq_data[i].arg; /* avoids undefined order of access to volatiles */
					gpio_irq_data[i].handler(arg);
					status &= ~(gpio_irq_data[i].mask);
				}
			}
			i++;
			if (i >= MAX_INTERRUPT_SOURCES) {
				break;
			}
		}
	}
  
}

#if 1
void PIOA_Handler(void)
{
	gpio_irq(PIOA, ID_PIOA);
}

#ifdef ID_PIOB
/**
 * \brief Parallel IO Controller B interrupt handler
 * Redefined PIOB interrupt handler for NVIC interrupt table.
 */
void PIOB_Handler(void)
{
    gpio_irq(PIOB, ID_PIOB);
}
#endif

void pio_handler_set_priority(Pio *p_pio, IRQn_Type ul_irqn, uint32_t ul_priority)
{
	uint32_t bitmask = 0;

	bitmask = pio_get_interrupt_mask(p_pio);
	pio_disable_interrupt(p_pio, 0xFFFFFFFF);
	pio_get_interrupt_status(p_pio);
	NVIC_DisableIRQ(ul_irqn);
	NVIC_ClearPendingIRQ(ul_irqn);
	NVIC_SetPriority(ul_irqn, ul_priority);
	NVIC_EnableIRQ(ul_irqn);
	pio_enable_interrupt(p_pio, bitmask);
}
#endif

