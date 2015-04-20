/*
 * copyright 2013, broadcom corporation
 * all rights reserved.
 *
 * this is UNPUBLISHED PROPRIETARY SOURCE CODE of broadcom corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of broadcom corporation.
 */

#include "mico_rtos.h"

#include "string.h" /* for memcpy */
#include "gpio_irq.h"
#include "platform.h"
#include "mico_common_driver.h"
#include "platform_logging.h"

/******************************************************
 *             constants
 ******************************************************/

#define DMA_TIMEOUT_LOOPS      (10000000)
#define SPI_BAUD_RATE           24000000
/**
 * transfer direction for the mico platform bus interface
 */
typedef enum
{
    /* if updating this enum, the bus_direction_mapping variable will also need to be updated */
    BUS_READ,
    BUS_WRITE
} bus_transfer_direction_t;

/******************************************************
 *             structures
 ******************************************************/

/******************************************************
 *             variables
 ******************************************************/

static mico_semaphore_t spi_transfer_finished_semaphore;

struct spi_master_vec_module    spi_master;
struct spi_master_vec_bufdesc   tx_dscr[2];
struct spi_master_vec_bufdesc   rx_dscr[2];

/******************************************************
 *             function declarations
 ******************************************************/

/* powersave functionality */
extern void MCU_CLOCKS_NEEDED( void );
extern void MCU_CLOCKS_NOT_NEEDED( void );

extern void wiced_platform_notify_irq( void );

/******************************************************
 *             function definitions
 ******************************************************/

static void spi_irq_handler( void* arg )
{
    UNUSED_PARAMETER(arg);
#ifndef MICO_DISABLE_MCU_POWERSAVE
    wake_up_interrupt_notify( );
#endif /* ifndef MICO_DISABLE_MCU_POWERSAVE */
    wiced_platform_notify_irq( );
}

//void dma_irq( void )
void wwd_irq( void )
{
    /* clear interrupt */
    mico_rtos_set_semaphore( &spi_transfer_finished_semaphore );
}

OSStatus host_platform_bus_init( void )
{
	struct spi_master_vec_config spi; 
    OSStatus result;
    
    MCU_CLOCKS_NEEDED();
    
    result = mico_rtos_init_semaphore( &spi_transfer_finished_semaphore, 1 );
    if ( result != kNoErr )
    {
        return result;
    }
    
    mico_gpio_initialize( (mico_gpio_t)MICO_GPIO_9, INPUT_PULL_UP );
    mico_gpio_enable_IRQ( (mico_gpio_t)MICO_GPIO_9, IRQ_TRIGGER_RISING_EDGE, spi_irq_handler, NULL );
    mico_gpio_initialize( MICO_GPIO_15, OUTPUT_PUSH_PULL);//spi ss/cs 
	mico_gpio_output_high( MICO_GPIO_15 );
    
    spi.baudrate = SPI_BAUD_RATE;
  
    /* set PORTB 01 to high to put WLAN module into g_SPI mode */
    mico_gpio_initialize( (mico_gpio_t)WL_GPIO0, OUTPUT_PUSH_PULL );
    mico_gpio_output_high( (mico_gpio_t)WL_GPIO0 );
	
	if (STATUS_OK != spi_master_vec_init(&spi_master, SPI, &spi)) {
		return -1;
	}

	spi_master_vec_enable(&spi_master);
   
    MCU_CLOCKS_NOT_NEEDED();

    return kNoErr;
}

OSStatus host_platform_bus_deinit( void )
{

    MCU_CLOCKS_NEEDED();

    spi_master_vec_disable(&spi_master);

    MCU_CLOCKS_NOT_NEEDED();

    return kNoErr;
}

OSStatus host_platform_spi_transfer( bus_transfer_direction_t dir, uint8_t* buffer, uint16_t buffer_length )
{
    OSStatus result;
    uint8_t *junk;//Jer TBD by the length == 0

    MCU_CLOCKS_NEEDED();

    tx_dscr[0].data   = buffer;
    tx_dscr[0].length = buffer_length;
    tx_dscr[1].data   = NULL;
    tx_dscr[1].length = 0;
    
//    ioport_set_pin_level(SPI_NPCS0_GPIO, IOPORT_PIN_LEVEL_LOW);//cs high(enable)
    if ( dir == BUS_READ ) {
        rx_dscr[0].data   = buffer;
        rx_dscr[0].length = buffer_length;
        rx_dscr[1].data   = NULL;
        rx_dscr[1].length = 0;
	    if (spi_master_vec_transceive_buffer_wait(&spi_master, tx_dscr, rx_dscr) != STATUS_OK) {
            return kGeneralErr;
        }
    } else {
        rx_dscr[0].data   = junk;
        rx_dscr[0].length = 0;
        rx_dscr[1].data   = NULL;
        rx_dscr[1].length = 0;
	    if (spi_master_vec_transceive_buffer_wait(&spi_master, tx_dscr, rx_dscr) != STATUS_OK) {
            return kGeneralErr;
        }
    }
#ifndef NO_MICO_RTOS
    result = mico_rtos_get_semaphore( &spi_transfer_finished_semaphore, 100 );
#else 
    loop_count = 0;
	while(spi_read_status(spi_hw) & SPI_SR_RXBUFF) == 0 && ( loop_count < (uint32_t) DMA_TIMEOUT_LOOPS ) ) {
        loop_count++;
    }
    
    if ( loop_count >= (uint32_t) DMA_TIMEOUT_LOOPS )
    {
        MCU_CLOCKS_NOT_NEEDED( );
        return kTimeoutErr;
    }
#endif
 //   ioport_set_pin_level(SPI_NPCS0_GPIO, IOPORT_PIN_LEVEL_HIGH);//cs high(disable)

    MCU_CLOCKS_NOT_NEEDED();

    return result;
}
