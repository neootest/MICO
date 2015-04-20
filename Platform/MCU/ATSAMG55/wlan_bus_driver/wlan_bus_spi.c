/*
 * copyright 2013, broadcom corporation
 * all rights reserved.
 *
 * this is UNPUBLISHED PROPRIETARY SOURCE CODE of broadcom corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of broadcom corporation.
 */

#include "MICORTOS.h"

#include "string.h" /* for memcpy */
#include "platform.h"
#include "platform_peripheral.h"
#include "platformLogging.h"
#include "conf_board.h"
#include "MicoPlatform.h"

//Change SPI Driver
#define USE_OWN_SPI_DRV
//#define HARD_CS_NSS0
/******************************************************
 *             constants
 ******************************************************/
#ifdef USE_OWN_SPI_DRV
/* Chip select. */
#define SPI_CHIP_SEL        0
#define SPI_CHIP_PCS spi_get_pcs(SPI_CHIP_SEL)

/* Clock polarity. */
#define SPI_CLK_POLARITY    0

/* Clock phase. */
#define SPI_CLK_PHASE       1

/* Delay before SPCK. */
#define SPI_DLYBS           0x0 //0x40

/* Delay between consecutive transfers. */
#define SPI_DLYBCT          0x0
#endif

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
#ifdef USE_OWN_SPI_DRV
Pdc *spi_m_pdc;
//Pdc *spi_s_pdc;
#else 
struct spi_master_vec_module    spi_master;
struct spi_master_vec_bufdesc   tx_dscr[2];
struct spi_master_vec_bufdesc   rx_dscr[2];
#endif
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
//#ifndef MICO_DISABLE_MCU_POWERSAVE
//    wake_up_interrupt_notify( );
//#endif /* ifndef MICO_DISABLE_MCU_POWERSAVE */
    wiced_platform_notify_irq( );
}

//void dma_irq( void )
void wwd_irq( void )
{
    mico_rtos_set_semaphore( &spi_transfer_finished_semaphore );
}

OSStatus host_platform_bus_init( void )
{
#ifndef USE_OWN_SPI_DRV 
	struct spi_master_vec_config spi; 
#else
	pdc_packet_t pdc_spi_packet;
#endif
    OSStatus result;

    platform_mcu_powersave_disable();

	spi_disable_interrupt(SPI_MASTER_BASE, 0xffffffff);
    //Disable_global_interrupt();//TBD!

    result = mico_rtos_init_semaphore( &spi_transfer_finished_semaphore, 1 );
    if ( result != kNoErr )
    {
        return result;
    }
    
    MicoGpioInitialize( (mico_gpio_t)MICO_GPIO_9, INPUT_PULL_UP );
    //ioport_port_mask_t ul_mask = ioport_pin_to_mask(CREATE_IOPORT_PIN(PORTA,24));
    //pio_set_input(PIOA,ul_mask, PIO_PULLUP|PIO_DEBOUNCE);
    MicoGpioEnableIRQ( (mico_gpio_t)MICO_GPIO_9, IRQ_TRIGGER_RISING_EDGE, spi_irq_handler, NULL );
#ifndef HARD_CS_NSS0	
    MicoGpioInitialize( MICO_GPIO_15, OUTPUT_PUSH_PULL);//spi ss/cs 
	MicoGpioOutputHigh( MICO_GPIO_15 );//MICO_GPIO_15 TBD!
#else
	ioport_set_pin_peripheral_mode(SPI_NPCS0_GPIO, SPI_NPCS0_FLAGS);//TBD!
#endif
    /* set PORTB 01 to high to put WLAN module into g_SPI mode */
    MicoGpioInitialize( (mico_gpio_t)WL_GPIO0, OUTPUT_PUSH_PULL );
    MicoGpioOutputHigh( (mico_gpio_t)WL_GPIO0 );
#ifdef USE_OWN_SPI_DRV 
#if (SAMG55)
	/* Enable the peripheral and set SPI mode. */
	flexcom_enable(BOARD_FLEXCOM_SPI);
	flexcom_set_opmode(BOARD_FLEXCOM_SPI, FLEXCOM_SPI);
#else
	/* Configure an SPI peripheral. */
	pmc_enable_periph_clk(SPI_ID);
#endif
    //Init pdc, and clear RX TX.
    spi_m_pdc = spi_get_pdc_base(SPI_MASTER_BASE);

	pdc_spi_packet.ul_addr = NULL;
	pdc_spi_packet.ul_size = 3;
	pdc_tx_init(spi_m_pdc, &pdc_spi_packet, NULL);
	pdc_rx_init(spi_m_pdc, &pdc_spi_packet, NULL);

	spi_disable(SPI_MASTER_BASE);
	spi_reset(SPI_MASTER_BASE);
	spi_set_lastxfer(SPI_MASTER_BASE);
	spi_set_master_mode(SPI_MASTER_BASE);
	spi_disable_mode_fault_detect(SPI_MASTER_BASE);
#ifdef HARD_CS_NSS0
	//spi_enable_peripheral_select_decode(SPI_MASTER_BASE);
	//spi_set_peripheral_chip_select_value(SPI_MASTER_BASE, SPI_CHIP_SEL);
	spi_set_peripheral_chip_select_value(SPI_MASTER_BASE, SPI_CHIP_PCS); //use soft nss comment here
#endif
	spi_set_clock_polarity(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_POLARITY);
	spi_set_clock_phase(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_PHASE);
	spi_set_bits_per_transfer(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CSR_BITS_8_BIT);
	spi_set_baudrate_div(SPI_MASTER_BASE, SPI_CHIP_SEL, (sysclk_get_cpu_hz() / SPI_BAUD_RATE));
	spi_set_transfer_delay(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_DLYBS, SPI_DLYBCT);

    /* Must be lower priority than the value of configMAX_SYSCALL_INTERRUPT_PRIORITY */
    /* otherwise FreeRTOS will not be able to mask the interrupt */
    /* keep in mind that ARMCM3 interrupt priority logic is inverted, the highest value */
    /* is the lowest priority */
	/* Configure SPI interrupts . */
    spi_enable_interrupt(SPI_MASTER_BASE, SPI_IER_RXBUFF);
    //spi_enable_interrupt(SPI_MASTER_BASE, SPI_IER_NSSR | SPI_IER_RXBUFF);
	
	NVIC_DisableIRQ(SPI_IRQn);
    //irq_register_handler(SPI_IRQn, 3);
	NVIC_ClearPendingIRQ(SPI_IRQn);
	NVIC_SetPriority(SPI_IRQn, 3);
	NVIC_EnableIRQ(SPI_IRQn);

    spi_enable(SPI_MASTER_BASE);
#else
    spi.baudrate = SPI_BAUD_RATE;
	if (STATUS_OK != spi_master_vec_init(&spi_master, SPI_MASTER_BASE, &spi)) {
		return -1;
	}

	spi_master_vec_enable(&spi_master);
#endif
    //if (!Is_global_interrupt_enabled())
    //    Enable_global_interrupt();
    platform_mcu_powersave_enable();

    return kNoErr;
}

OSStatus host_platform_bus_deinit( void )
{

    platform_mcu_powersave_disable();
#ifdef USE_OWN_SPI_DRV
    pdc_disable_transfer(spi_m_pdc, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);
	spi_disable(SPI_MASTER_BASE);
#else
    spi_master_vec_disable(&spi_master);
#endif
    platform_mcu_powersave_enable();

    return kNoErr;
}

OSStatus host_platform_spi_transfer( bus_transfer_direction_t dir, uint8_t* buffer, uint16_t buffer_length )
{
    OSStatus result;
    uint8_t *junk;

    platform_mcu_powersave_disable();
#ifdef USE_OWN_SPI_DRV
	pdc_packet_t pdc_spi_packet;

	pdc_spi_packet.ul_addr = (uint32_t)buffer;
	pdc_spi_packet.ul_size = buffer_length;
	pdc_tx_init(spi_m_pdc, &pdc_spi_packet, NULL);

    if ( dir == BUS_READ ) {
	    pdc_spi_packet.ul_addr = (uint32_t)buffer;
	    pdc_spi_packet.ul_size = buffer_length;
    } 
	if ( dir == BUS_WRITE ) {
        junk = malloc(buffer_length);
	    pdc_spi_packet.ul_addr = (uint32_t)junk;
	    pdc_spi_packet.ul_size = buffer_length;
    }
	pdc_rx_init(spi_m_pdc, &pdc_spi_packet, NULL);
#if 0
	if ( dir == BUS_WRITE ) {
		spi_enable_interrupt(SPI_MASTER_BASE, SPI_IER_ENDTX);
	    NVIC_EnableIRQ(SPI_IRQn);
	}
	else {
		spi_enable_interrupt(SPI_MASTER_BASE, SPI_IER_ENDRX);
	    NVIC_EnableIRQ(SPI_IRQn);
	}
#endif	
    //platform_log("dir = %d, len = %d",dir, buffer_length);//TBD
#ifndef HARD_CS_NSS0	
	MicoGpioOutputLow( MICO_GPIO_15 );
#endif
	/* Enable the RX and TX PDC transfer requests */
    pdc_enable_transfer(spi_m_pdc, PERIPH_PTCR_TXTEN | PERIPH_PTCR_RXTEN);//pdc buffer address increase automatic.
    //platform_log("pdc status = 0x%x",pdc_read_status(spi_m_pdc));

#ifndef NO_MICO_RTOS
    result = mico_rtos_get_semaphore( &spi_transfer_finished_semaphore, 100 );
#else
    /* Waiting transfer done*/
	while((spi_read_status(SPI_MASTER_BASE) & SPI_SR_RXBUFF) == 0) {
        __asm("wfi");
    }
#endif
	if ( dir == BUS_WRITE ) {
        if (junk) 
            free(junk);
    }
	/* Disable the RX and TX PDC transfer requests */
    pdc_disable_transfer(spi_m_pdc, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);

#ifndef HARD_CS_NSS0	
	MicoGpioOutputHigh( MICO_GPIO_15 );
#endif
#if 1
    //clear PDC Perph Status flags
	pdc_spi_packet.ul_addr = NULL;
	pdc_spi_packet.ul_size = 3;
	pdc_tx_init(spi_m_pdc, &pdc_spi_packet, NULL);
	pdc_rx_init(spi_m_pdc, &pdc_spi_packet, NULL);
#endif
#else //spi_master_vec :

    tx_dscr[0].data   = buffer;
    tx_dscr[0].length = buffer_length;
    tx_dscr[1].data   = NULL;
    tx_dscr[1].length = 0;
    
    //if ( dir == BUS_READ ) {
        rx_dscr[0].data   = buffer;
        rx_dscr[0].length = buffer_length;
    //} else {
    //    rx_dscr[0].data   = &junk;
    //    rx_dscr[0].length = 0;
    //}
    rx_dscr[1].data   = NULL;
    rx_dscr[1].length = 0;
#ifndef HARD_CS_NSS0	
    mico_gpio_output_low( MICO_GPIO_15 );
#endif 
    if (spi_master_vec_transceive_buffer_wait(&spi_master, tx_dscr, rx_dscr) != STATUS_OK) {
        platform_log("STATUS = -1");
        return kGeneralErr;
    }
#ifndef HARD_CS_NSS0	   
    mico_gpio_output_high( MICO_GPIO_15 );
#endif
#endif /* USE_OWN_SPI_DR */
    platform_mcu_powersave_enable();
#ifdef USE_OWN_SPI_DRV
    return result;
#else
    return 0;
#endif
}
#ifdef USE_OWN_SPI_DRV
void SPI_Handler(void)
{
    //uint32_t status;
	pdc_packet_t pdc_spi_packet;

	//status = spi_read_status(SPI_MASTER_BASE);
	//spi_disable_interrupt(SPI_MASTER_BASE, 0xffffffff);
	//pdc_disable_transfer(spi_m_pdc, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);
    /* clear interrupt */
	pdc_spi_packet.ul_addr = NULL;
	pdc_spi_packet.ul_size = 3;
    pdc_rx_init(spi_m_pdc, &pdc_spi_packet, NULL);
    //platform_log("status= %x",status); //don't enabel printf in IRQ Handler
    wwd_irq();//pdc int
	
    //if(status & SPI_SR_NSSR) {
	//	if ( status & SPI_SR_RXBUFF ) {
		//	spi_slave_transfer(gs_uc_spi_s_tbuffer, COMM_BUFFER_SIZE,
		//			gs_uc_spi_s_rbuffer, COMM_BUFFER_SIZE);
	//	}
	//}
}
#endif

