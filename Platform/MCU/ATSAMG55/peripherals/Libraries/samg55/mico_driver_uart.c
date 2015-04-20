/**
******************************************************************************
* @file    mico_driver_uart.c 
* @author  william xu
* @version V1.0.0
* @date    05-may-2014
* @brief   this file provide UART driver functions.
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


#include "mico_rtos.h"
#include "mico_platform.h"

#include "platform.h"

#include "gpio_irq.h"

/******************************************************
*                    constants
******************************************************/
#define USART1_IRQHandler   FLEXCOM7_Handler
#define USART2_IRQHandler   FLEXCOM0_Handler

#define USE_USART_PDC
#define ENABLE_TX_SEMA  1 	

#ifdef BOOTLOADER 
#define UART_NO_OS NO_MICO_RTOS
#else
//#define UART_NO_OS
#endif
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
    uint32_t            rx_size;
    ring_buffer_t*      rx_buffer;
#ifndef UART_NO_OS
    mico_semaphore_t    rx_complete;
    mico_semaphore_t    tx_complete;
#else
    volatile bool       rx_complete;
    volatile bool       tx_complete;
#endif
    mico_semaphore_t    sem_wakeup;
    OSStatus            tx_dma_result;
    OSStatus            rx_dma_result;
    bool                initialized;
} uart_interface_t;

/******************************************************
*               variables definitions
******************************************************/

static uart_interface_t uart_interfaces[NUMBER_OF_UART_INTERFACES];
#ifdef USE_USART_PDC
static pdc_packet_t pdc_uart_rx_packet;
#endif
#ifndef UART_NO_OS
static mico_uart_t current_uart;
#endif

/******************************************************
*               function declarations
******************************************************/

static OSStatus internal_uart_init ( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer );
static OSStatus platform_uart_receive_bytes( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout );




/* interrupt service functions - called from interrupt vector table */
#ifndef UART_NO_OS
static void thread_wakeup(void *arg);
static void rx_pin_wakeup_handler(void *arg);
#endif

void usart2_rx_dma_irq( void );
void usart1_rx_dma_irq( void );

/******************************************************
*               function definitions
******************************************************/

OSStatus mico_uart_initialize( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
    // #ifndef MICO_DISABLE_STDIO
    //     if (uart == STDIO_UART)
    //     {
    //         return kGeneralErr;
    //     }
    // #endif
    
    return internal_uart_init(uart, config, optional_rx_buffer);
}


OSStatus mico_stdio_uart_initialize( const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
    return internal_uart_init(STDIO_UART, config, optional_rx_buffer);
}


OSStatus internal_uart_init( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
    sam_usart_opt_t usart_settings;
    bool hardware_shaking = false;
    int ret;
#ifdef USE_USART_PDC
	pdc_packet_t pdc_uart_packet;
#endif

    mico_mcu_power_save_config(false);

    if (uart_interfaces[uart].initialized) {
        return kNoErr; //kGeneralErr; kNoErr just to test Only.
    }
	usart_disable_interrupt(uart_mapping[uart].usart, 0xffffffff);
   // Disable_global_interrupt();//TBD!

#ifndef UART_NO_OS
    ret = mico_rtos_init_semaphore(&uart_interfaces[uart].tx_complete, 1);
    if (ret != 0) return kGeneralErr;
    ret = mico_rtos_init_semaphore(&uart_interfaces[uart].rx_complete, 1);
    if (ret != 0) return kGeneralErr;
#else
    uart_interfaces[uart].tx_complete = false;
    uart_interfaces[uart].rx_complete = false;
#endif
    
    // peripheral_clock rx, tx
    // i/o mode rx,tx
	ioport_set_port_peripheral_mode(uart_mapping[uart].gpio_bank, 
            uart_mapping[uart].pin_tx | uart_mapping[uart].pin_rx, uart_mapping[uart].mux_mode);
#if 0
#ifndef UART_NO_OS
    if(config->flags & UART_WAKEUP_ENABLE){
        current_uart = uart;
        mico_rtos_init_semaphore( &uart_interfaces[uart].sem_wakeup, 1 );
        mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART_WAKEUP", thread_wakeup, 0x100, &current_uart);
    }
#endif
#endif
    // peripheral_clock cts, rts
    // i/o mode cts, rts
    if ( uart_mapping[uart].pin_cts && (config->flow_control == FLOW_CONTROL_CTS || config->flow_control == FLOW_CONTROL_CTS_RTS) ) {
        hardware_shaking = true;
	    ioport_set_port_peripheral_mode(uart_mapping[uart].gpio_bank,
                uart_mapping[uart].pin_cts, uart_mapping[uart].mux_mode);
    }
    
    if ( uart_mapping[uart].pin_rts && (config->flow_control == FLOW_CONTROL_RTS || config->flow_control == FLOW_CONTROL_CTS_RTS) ) {
        hardware_shaking = true;
	    ioport_set_port_peripheral_mode(uart_mapping[uart].gpio_bank,
                uart_mapping[uart].pin_rts, uart_mapping[uart].mux_mode);
    }
    // peripheral_clock 
#if (SAMG55)
    flexcom_enable(uart_mapping[uart].flexcom_base);
    flexcom_set_opmode(uart_mapping[uart].flexcom_base, FLEXCOM_USART);
#else
	sysclk_enable_peripheral_clock(uart_mapping[uart].id_peripheral_clock);
#endif
    // usart init
	usart_settings.baudrate = config->baud_rate;
    switch ( config->parity ) {
        case NO_PARITY:
	        usart_settings.parity_type = US_MR_PAR_NO;
            break;
        case EVEN_PARITY:
	        usart_settings.parity_type = US_MR_PAR_EVEN;
            break;
        case ODD_PARITY:
	        usart_settings.parity_type = US_MR_PAR_ODD;
            break;
        default:
            return kGeneralErr;
    }
    switch ( config->data_width) {
        case DATA_WIDTH_5BIT: 
                              usart_settings.char_length = US_MR_CHRL_5_BIT;
                              break;
        case DATA_WIDTH_6BIT: 
                              usart_settings.char_length = US_MR_CHRL_6_BIT;
                              break;
        case DATA_WIDTH_7BIT: 
                              usart_settings.char_length = US_MR_CHRL_7_BIT;
                              break;
        case DATA_WIDTH_8BIT: 
                              usart_settings.char_length = US_MR_CHRL_8_BIT;
                              break;
        case DATA_WIDTH_9BIT: 
                              usart_settings.char_length = US_MR_MODE9;
                              break;
        default:
            return kGeneralErr;
    }
	usart_settings.stop_bits = ( config->stop_bits == STOP_BITS_1 ) ? US_MR_NBSTOP_1_BIT : US_MR_NBSTOP_2_BIT;
	usart_settings.channel_mode= US_MR_CHMODE_NORMAL;
		
    if (!hardware_shaking) {
        usart_init_rs232(uart_mapping[uart].usart, &usart_settings, sysclk_get_peripheral_hz());
    } else {
        usart_init_hw_handshaking(uart_mapping[uart].usart, &usart_settings, sysclk_get_peripheral_hz());
    }
	
    //usart_disable_interrupt(uart_mapping[uart].usart, 0xffffffff);
	pdc_disable_transfer(uart_mapping[uart].dma_base, PERIPH_PTCR_TXTDIS | PERIPH_PTCR_RXTDIS);
#ifdef USE_USART_PDC
    pdc_uart_packet.ul_addr = NULL;
    pdc_uart_packet.ul_size = 1;
    pdc_tx_init(uart_mapping[uart].dma_base, &pdc_uart_packet, &pdc_uart_packet); //clear TX & RX
    pdc_rx_init(uart_mapping[uart].dma_base, &pdc_uart_packet, &pdc_uart_packet);

    usart_enable_interrupt(uart_mapping[uart].usart,  US_IER_TXBUFE | US_IER_RXBUFF); 
	/* Enable the RX and TX PDC transfer requests */
	pdc_enable_transfer(uart_mapping[uart].dma_base, PERIPH_PTCR_TXTEN | PERIPH_PTCR_RXTEN);
#endif    
    // nvic init
	NVIC_DisableIRQ(uart_mapping[uart].usart_irq);
    //irq_register_handler(uart_mapping[uart].usart_irq, 0x0C);
	NVIC_ClearPendingIRQ(uart_mapping[uart].usart_irq);
	NVIC_SetPriority(uart_mapping[uart].usart_irq, 0x06);
	NVIC_EnableIRQ(uart_mapping[uart].usart_irq);
    
    /* Enable the receiver and transmitter. */
    usart_enable_tx(uart_mapping[uart].usart);
    usart_enable_rx(uart_mapping[uart].usart);
    
    // dma init
#if 1
    /* setup ring buffer */
    if (optional_rx_buffer != NULL)
    {
        /* note that the ring_buffer should've been initialised first */
        uart_interfaces[uart].rx_buffer = optional_rx_buffer;
        uart_interfaces[uart].rx_size   = 0;
        platform_uart_receive_bytes( uart, optional_rx_buffer->buffer, optional_rx_buffer->size, 0 );
    }
    else
    { //init rx dma nvic 
#ifdef USE_USART_PDC
        pdc_packet_t pdc_uart_packet;
	    
        pdc_uart_packet.ul_addr = (uint32_t) optional_rx_buffer->buffer;
	    pdc_uart_packet.ul_size = optional_rx_buffer->size;
	    pdc_rx_init(uart_mapping[uart].dma_base, &pdc_uart_packet, NULL);
#endif
    }
#endif
    uart_interfaces[uart].initialized = true;
    //Enable_global_interrupt();
    //if (!Is_global_interrupt_enabled())
    //    Enable_global_interrupt();
    mico_mcu_power_save_config(true);
    
    return kNoErr;
}

OSStatus mico_uart_finalize( mico_uart_t uart )
{
    
    mico_mcu_power_save_config(false);
 
    usart_disable_tx(uart_mapping[uart].usart);
    usart_disable_rx(uart_mapping[uart].usart);

	NVIC_SetPriority(uart_mapping[uart].usart_irq, 13);
    NVIC_DisableIRQ(uart_mapping[uart].usart_irq);
#ifndef UART_NO_OS
    mico_rtos_deinit_semaphore(&uart_interfaces[uart].rx_complete);
    mico_rtos_deinit_semaphore(&uart_interfaces[uart].tx_complete);
#endif
    
    mico_mcu_power_save_config(true);
    
    return kNoErr;
}

OSStatus mico_uart_send( mico_uart_t uart, const void* data, uint32_t size )
{
    OSStatus result = kNoErr;
#ifdef USE_USART_PDC
	pdc_packet_t pdc_uart_packet;
#endif
    /* reset DMA transmission result. the result is assigned in interrupt handler */
    uart_interfaces[uart].tx_dma_result = kGeneralErr;
    
    mico_mcu_power_save_config(false); 
    //if (!Is_global_interrupt_enabled())
    //    Enable_global_interrupt();
    //usart_enable_interrupt(uart_mapping[uart].usart,  US_IER_TXBUFE); 
	//NVIC_EnableIRQ(uart_mapping[uart].usart_irq);
    //tx 
#ifdef USE_USART_PDC
    pdc_uart_packet.ul_addr = (uint32_t) data;
    pdc_uart_packet.ul_size = size;
    pdc_tx_init(uart_mapping[uart].dma_base, &pdc_uart_packet, NULL);
    
	/* Enable the RX and TX PDC transfer requests */
	pdc_enable_transfer(uart_mapping[uart].dma_base, PERIPH_PTCR_TXTEN );
#else
    uint8_t *c = (uint8_t *)data;
    //usart_enable_interrupt(uart_mapping[uart].usart, ( US_IER_TXBUFE)); 
    while (usart_write(uart_mapping[uart].usart, *c)!=0);
	//return 1;
#endif
#if ENABLE_TX_SEMA
#ifndef UART_NO_OS
    result = mico_rtos_get_semaphore( &uart_interfaces[ uart ].tx_complete, MICO_NEVER_TIMEOUT );
#else 
    while(uart_interfaces[ uart ].tx_complete == false);
    uart_interfaces[ uart ].tx_complete = false;
#endif
#endif
    while (!usart_is_tx_buf_empty(uart_mapping[uart].usart)) {
    }

	pdc_disable_transfer(uart_mapping[uart].dma_base, PERIPH_PTCR_TXTDIS);

    uart_interfaces[uart].tx_dma_result = result;
    mico_mcu_power_save_config(true);
    if (result != kNoErr) {
        return kGeneralErr;
    }
    
    return uart_interfaces[uart].tx_dma_result;
}

OSStatus mico_uart_recv( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
#if 1
        if (size > 0) {
            usart_set_rx_timeout(uart_mapping[uart].usart, 160); //timeout reffer to baudrate.
            usart_start_rx_timeout(uart_mapping[uart].usart); //TBD!
            usart_enable_interrupt(uart_mapping[uart].usart, US_IER_TIMEOUT | US_IER_RXBUFF);/*| US_IER_RXRDY*/ 
        }
#endif
    //return platform_uart_receive_bytes( uart, data, size, timeout );
    if (uart_interfaces[uart].rx_buffer != NULL)
    {
        while (size != 0)
        {
            uint32_t transfer_size = MIN(uart_interfaces[uart].rx_buffer->size / 2, size);
            
            /* check if ring buffer already contains the required amount of data. */
            if ( transfer_size > ring_buffer_used_space( uart_interfaces[uart].rx_buffer ) )
            {
                /* set rx_size and wait in rx_complete semaphore until data reaches rx_size or timeout occurs */
                uart_interfaces[uart].rx_size = transfer_size;
                
#ifndef UART_NO_OS
                if ( mico_rtos_get_semaphore( &uart_interfaces[uart].rx_complete, timeout) != kNoErr )
                {
                    uart_interfaces[uart].rx_size = 0;
                    return kTimeoutErr;
                }
#else
                uart_interfaces[uart].rx_complete = false;
                int delay_start = mico_get_time_no_os();
                while(uart_interfaces[uart].rx_complete == false){
                    if(mico_get_time_no_os() >= delay_start + timeout && timeout != MICO_NEVER_TIMEOUT){
                        uart_interfaces[uart].rx_size = 0;
                        return kTimeoutErr;
                    }
                }
#endif
                
                /* reset rx_size to prevent semaphore being set while nothing waits for the data */
                uart_interfaces[uart].rx_size = 0;
            }
            
            size -= transfer_size;
            
            // grab data from the buffer
            do
            {
                uint8_t* available_data;
                uint32_t bytes_available;
                
                ring_buffer_get_data( uart_interfaces[uart].rx_buffer, &available_data, &bytes_available );
                bytes_available = MIN( bytes_available, transfer_size );
                memcpy( data, available_data, bytes_available );
                transfer_size -= bytes_available;
                data = ( (uint8_t*) data + bytes_available );
                ring_buffer_consume( uart_interfaces[uart].rx_buffer, bytes_available );
            } while ( transfer_size != 0 );
        }
        
        if ( size != 0 )
        {
            return kGeneralErr;
        }
        else
        {
            return kNoErr;
        }
    }
    else
    {
        return platform_uart_receive_bytes( uart, data, size, timeout );
    }
}

static OSStatus platform_uart_receive_bytes( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
    //usart_set_rx_timeout(uart_mapping[uart].usart, timeout);
    
    //usart_enable_interrupt(uart_mapping[uart].usart, US_IER_RXBUFF | US_IER_TIMEOUT);
	//NVIC_EnableIRQ(uart_mapping[uart].usart_irq);
    //msleep(10);
    if ( uart_interfaces[uart].rx_buffer != NULL )
    {
        //usart_enable_interrupt(uart_mapping[uart].usart, US_IER_RXRDY);
    }
    else
    {
    }
    /* reset DMA transmission result. the result is assigned in interrupt handler */
    uart_interfaces[uart].rx_dma_result = kGeneralErr;
#ifdef USE_USART_PDC
    pdc_uart_rx_packet.ul_addr = (uint32_t) data;
    pdc_uart_rx_packet.ul_size = size;
    pdc_rx_init(uart_mapping[uart].dma_base, &pdc_uart_rx_packet, NULL);
    
    /* Enable the RX and TX PDC transfer requests */
    //pdc_enable_transfer(uart_mapping[uart].dma_base, PERIPH_PTCR_RXTEN );
#else
    //usart_enable_interrupt(uart_mapping[uart].usart,  US_IER_ENDRX);
    while (usart_read(uart_mapping[uart].usart, data));
#endif
    
    if ( timeout > 0 )
    {
#ifndef UART_NO_OS
        mico_rtos_get_semaphore( &uart_interfaces[uart].rx_complete, timeout );
#else
        uart_interfaces[uart].rx_complete = false;
        int delay_start = mico_get_time_no_os();
        while(uart_interfaces[uart].rx_complete == false){
            if(mico_get_time_no_os() >= delay_start + timeout && timeout != MICO_NEVER_TIMEOUT){
                break;
            }
        }    
#endif
        return uart_interfaces[uart].rx_dma_result;
    }
    
    
    return kNoErr;
}

uint32_t mico_uart_get_length_in_buffer( mico_uart_t uart )
{
    //g_pdc_count = pdc_read_rx_counter(uart_mapping[uart].dma_base);
    //return (uart_interfaces[uart].rx_buffer->size/2 - g_pdc_count);
    return ring_buffer_used_space( uart_interfaces[uart].rx_buffer );
}
#if 0
#ifndef UART_NO_OS
static void thread_wakeup(void *arg)
{
    mico_uart_t uart = *(mico_uart_t *)arg;
    
    while(1){
        if(mico_rtos_get_semaphore(&uart_interfaces[ uart ].sem_wakeup, 1000) != kNoErr){
        mico_mcu_power_save_config(true);
        }
    }
}
#endif

/******************************************************
*            interrupt service routines
******************************************************/
#ifndef UART_NO_OS
void rx_pin_wakeup_handler(void *arg)
{
    (void)arg;
    mico_uart_t uart = *(mico_uart_t *)arg;
    mico_mcu_power_save_config(false);
    mico_rtos_set_semaphore(&uart_interfaces[uart].sem_wakeup);
}
#endif

#endif

static void irq_handler( mico_uart_t uart )
{
    uint32_t status = 0;
    uint32_t g_pdc_count = 0;
    Usart *p_usart = uart_mapping[uart].usart;
#ifdef USE_USART_PDC
	pdc_packet_t pdc_uart_packet;
#endif
    // clear all inte
    status = usart_get_status(p_usart);
	//usart_disable_interrupt(p_usart, 0xffffffff);
	//pdc_disable_transfer(uart_mapping[uart].dma_base, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);
	/* Get USART status and check if CMP is set */
	if (status &  US_CSR_TXBUFE) { //pdc int
#ifdef USE_USART_PDC
        //clear CSR by tx_init
        pdc_uart_packet.ul_addr = NULL;
        pdc_uart_packet.ul_size = 6;
        pdc_tx_init(uart_mapping[uart].dma_base, &pdc_uart_packet, &pdc_uart_packet);
#endif
		/* Disable USART IRQ */
		//usart_disable_interrupt(p_usart, US_IDR_TXBUFE);
#if ENABLE_TX_SEMA
        #ifndef UART_NO_OS
        /* set semaphore regardless of result to prevent waiting thread from locking up */
        mico_rtos_set_semaphore( &uart_interfaces[uart].tx_complete);
        #else
        uart_interfaces[uart].tx_complete = true;
        #endif
#endif
	}
    //if (status & (US_CSR_RXRDY)) { //RXRDY for one character is ready.
        //usart_disable_interrupt(p_usart, US_IDR_RXRDY);
        //usart_restart_rx_timeout(uart_mapping[uart].usart);
    //}
    // update tail 
    if (status & (US_CSR_TIMEOUT | US_CSR_RXBUFF)) {
    //if (status & (US_CSR_RXRDY| US_CSR_RXBUFF)) {
        usart_disable_interrupt(p_usart, US_IDR_RXBUFF | US_IDR_TIMEOUT);

        if (status & US_CSR_RXBUFF) {
            pdc_rx_init(uart_mapping[uart].dma_base, &pdc_uart_rx_packet, NULL);
            g_pdc_count = 0;
        } else {
            g_pdc_count = pdc_read_rx_counter(uart_mapping[uart].dma_base);
        }
        uart_interfaces[ uart ].rx_buffer->tail = uart_interfaces[ uart ].rx_buffer->size - (g_pdc_count);
        

        // notify thread if sufficient data are available
        if ( ( uart_interfaces[ uart ].rx_size > 0 ) &&
                ( ring_buffer_used_space( uart_interfaces[ uart ].rx_buffer ) >= uart_interfaces[uart].rx_size ) )
        {
#ifndef UART_NO_OS
            mico_rtos_set_semaphore( &uart_interfaces[ uart ].rx_complete );
#else
            uart_interfaces[ uart ].rx_complete = true;
#endif
            uart_interfaces[ uart ].rx_size = 0;
        }
#if 0 
#ifndef UART_NO_OS
        if(uart_interfaces[ uart ].sem_wakeup)
            mico_rtos_set_semaphore(&uart_interfaces[ uart ].sem_wakeup);
#endif
#endif
        //g_pdc_count = 0;
    }
}

void USART1_IRQHandler( void )
{
    irq_handler(MICO_UART_1);
}

void USART2_IRQHandler( void )
{
    irq_handler(MICO_UART_2);
}


