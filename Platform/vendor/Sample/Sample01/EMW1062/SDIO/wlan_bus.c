/**
******************************************************************************
* @file    wlan_bus.h 
* @author  William Xu
* @version V1.0.0
* @date    16-Sep-2014
* @brief   This file provides bus communication functions with Wi-Fi RF chip.
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

#include "MicoRtos.h"
//#include "misc.h"
#include "string.h" /* For memcpy */
#include "gpio_irq.h"
#include "platform_common_config.h"
#include "PlatformLogging.h"

/* Powersave functionality */
extern void MCU_CLOCKS_NEEDED( void );
extern void MCU_CLOCKS_NOT_NEEDED( void );

/******************************************************
 *             Constants
 ******************************************************/

#define COMMAND_FINISHED_CMD52_TIMEOUT_LOOPS (100000)
#define COMMAND_FINISHED_CMD53_TIMEOUT_LOOPS (100000)
#define SDIO_TX_RX_COMPLETE_TIMEOUT_LOOPS    (100000)
#define SDIO_DMA_TIMEOUT_LOOPS               (1000000)
#define MAX_TIMEOUTS                         (30)

#define SDIO_ERROR_MASK   ( SDIO_STA_CCRCFAIL | SDIO_STA_DCRCFAIL | SDIO_STA_CTIMEOUT | SDIO_STA_DTIMEOUT | SDIO_STA_TXUNDERR | SDIO_STA_RXOVERR | SDIO_STA_STBITERR )

/**
 * Transfer direction for the mico platform bus interface
 */
typedef enum
{
    /* If updating this enum, the bus_direction_mapping variable will also need to be updated */
    BUS_READ,
    BUS_WRITE
} bus_transfer_direction_t;


static const uint32_t bus_direction_mapping[] =
{
    [BUS_READ]  = 0x00000002,
    [BUS_WRITE] = 0x00000000,
};

#define SDIO_IRQ_CHANNEL       ((u8)0x31)
#define DMA2_3_IRQ_CHANNEL     ((u8)DMA2_Stream3_IRQn)

#define WL_GPIO_INTR_PIN_NUM   EXTI_PinSource0
#define WL_GPIO_INTR_PORT_SRC  EXTI_PortSourceGPIOB
#define WL_GPIO_INTR_ILINE     EXTI_Line0
#define WL_GPIO_INTR_CHAN      0x06

#define BUS_LEVEL_MAX_RETRIES   5

#define SDIO_ENUMERATION_TIMEOUT_MS    (500)


/******************************************************
 *                   Enumerations
 ******************************************************/

/*
 * SDIO specific constants
 */
typedef enum
{
    SDIO_CMD_0  =  0,
    SDIO_CMD_3  =  3,
    SDIO_CMD_5  =  5,
    SDIO_CMD_7  =  7,
    SDIO_CMD_52 = 52,
    SDIO_CMD_53 = 53,
    __MAX_VAL   = 64
} sdio_command_t;

typedef enum
{
    SDIO_BLOCK_MODE = ( 0 << 2 ), /* These are STM32 implementation specific */
    SDIO_BYTE_MODE  = ( 1 << 2 )  /* These are STM32 implementation specific */
} sdio_transfer_mode_t;

typedef enum
{
    SDIO_1B_BLOCK    =  1,
    SDIO_2B_BLOCK    =  2,
    SDIO_4B_BLOCK    =  4,
    SDIO_8B_BLOCK    =  8,
    SDIO_16B_BLOCK   =  16,
    SDIO_32B_BLOCK   =  32,
    SDIO_64B_BLOCK   =  64,
    SDIO_128B_BLOCK  =  128,
    SDIO_256B_BLOCK  =  256,
    SDIO_512B_BLOCK  =  512,
    SDIO_1024B_BLOCK = 1024,
    SDIO_2048B_BLOCK = 2048
} sdio_block_size_t;

typedef enum
{
    RESPONSE_NEEDED,
    NO_RESPONSE
} sdio_response_needed_t;

/******************************************************
 *             Structures
 ******************************************************/

typedef struct
{
    /*@shared@*/ /*@null@*/ uint8_t* data;
    uint16_t length;
} sdio_dma_segment_t;

/******************************************************
 *             Variables
 ******************************************************/

static uint8_t   temp_dma_buffer[2*1024];
static uint8_t*  user_data;
static uint32_t  user_data_size;
static uint8_t*  dma_data_source;
static uint32_t  dma_transfer_size;

static mico_semaphore_t sdio_transfer_finished_semaphore;

static bool             sdio_transfer_failed;
static bus_transfer_direction_t current_transfer_direction;
static uint32_t                 current_command;

/******************************************************
 *             Function declarations
 ******************************************************/

static uint32_t          sdio_get_blocksize_dctrl   ( sdio_block_size_t block_size );
static sdio_block_size_t find_optimal_block_size    ( uint32_t data_size );
static void              sdio_prepare_data_transfer ( bus_transfer_direction_t direction, sdio_block_size_t block_size, /*@unique@*/ uint8_t* data, uint16_t data_size ) /*@modifies dma_data_source, user_data, user_data_size, dma_transfer_size@*/;

void dma_irq ( void );
OSStatus host_platform_sdio_transfer( bus_transfer_direction_t direction, sdio_command_t command, sdio_transfer_mode_t mode, sdio_block_size_t block_size, uint32_t argument, /*@null@*/ uint32_t* data, uint16_t data_size, sdio_response_needed_t response_expected, /*@out@*/ /*@null@*/ uint32_t* response );
extern void wiced_platform_notify_irq( void );

/******************************************************
 *             Function definitions
 ******************************************************/

static void sdio_oob_irq_handler( void* arg )
{
    UNUSED_PARAMETER(arg);
    wiced_platform_notify_irq( );
}

void sdio_irq( void )
{

}

/*@-exportheader@*/ /* Function picked up by linker script */
void dma_irq( void )
{

}

/*@+exportheader@*/

static void sdio_enable_bus_irq( void )
{
}

static void sdio_disable_bus_irq( void )
{
}

OSStatus host_enable_oob_interrupt( void )
{
   /* Set GPIO_B[1:0] to input. One of them will be re-purposed as OOB interrupt */
  MicoGpioInitialize( (mico_gpio_t)WL_GPIO0, OUTPUT_OPEN_DRAIN_NO_PULL );
  MicoGpioInitialize( (mico_gpio_t)WL_GPIO0, OUTPUT_OPEN_DRAIN_NO_PULL );
  MicoGpioEnableIRQ( (mico_gpio_t)WL_GPIO1, IRQ_TRIGGER_RISING_EDGE, sdio_oob_irq_handler, 0 );
  
  return kNoErr;
}

uint8_t host_platform_get_oob_interrupt_pin( void )
{
    if (1) //TBD!
    {
        /* WLAN GPIO1 */
        return 1;
    }
    else
    {
        /* WLAN GPIO0 */
        return 0;
    }
}

OSStatus host_platform_bus_init( void )
{
    OSStatus result;

    MCU_CLOCKS_NEEDED();

    result = mico_rtos_init_semaphore( &sdio_transfer_finished_semaphore, 1 );
    if ( result != kNoErr )
    {
        return result;
    }

    MCU_CLOCKS_NOT_NEEDED();

    return kNoErr;
}

OSStatus host_platform_sdio_enumerate( void )
{
    OSStatus       result;
    uint32_t       loop_count;
    uint32_t       data = 0;

    loop_count = 0;
    do
    {
        /* Send CMD0 to set it to idle state */
        host_platform_sdio_transfer( BUS_WRITE, SDIO_CMD_0, SDIO_BYTE_MODE, SDIO_1B_BLOCK, 0, 0, 0, NO_RESPONSE, NULL );

        /* CMD5. */
        host_platform_sdio_transfer( BUS_READ, SDIO_CMD_5, SDIO_BYTE_MODE, SDIO_1B_BLOCK, 0, 0, 0, NO_RESPONSE, NULL );

        /* Send CMD3 to get RCA. */
        result = host_platform_sdio_transfer( BUS_READ, SDIO_CMD_3, SDIO_BYTE_MODE, SDIO_1B_BLOCK, 0, 0, 0, RESPONSE_NEEDED, &data );
        loop_count++;
        if ( loop_count >= (uint32_t) SDIO_ENUMERATION_TIMEOUT_MS )
        {
            return kTimeoutErr;
        }
    } while ( ( result != kNoErr ) && ( mico_thread_msleep( (uint32_t) 1 ), ( 1 == 1 ) ) );
    /* If you're stuck here, check the platform matches your hardware */

    /* Send CMD7 with the returned RCA to select the card */
    host_platform_sdio_transfer( BUS_WRITE, SDIO_CMD_7, SDIO_BYTE_MODE, SDIO_1B_BLOCK, data, 0, 0, RESPONSE_NEEDED, NULL );

    return kNoErr;
}

OSStatus host_platform_bus_deinit( void )
{
    OSStatus   result;

    result = mico_rtos_deinit_semaphore( &sdio_transfer_finished_semaphore );

    MCU_CLOCKS_NEEDED();

    MCU_CLOCKS_NOT_NEEDED();

    return result;
}

OSStatus host_platform_sdio_transfer( bus_transfer_direction_t direction, sdio_command_t command, sdio_transfer_mode_t mode, sdio_block_size_t block_size, uint32_t argument, /*@null@*/ uint32_t* data, uint16_t data_size, sdio_response_needed_t response_expected, /*@out@*/ /*@null@*/ uint32_t* response )
{
    uint32_t loop_count = 0;
    OSStatus result;
    uint16_t attempts = 0;

    check_string(!((command == SDIO_CMD_53) && (data == NULL)), "Bad args" );

    if ( response != NULL )
    {
        *response = 0;
    }

    MCU_CLOCKS_NEEDED();


exit:
    MCU_CLOCKS_NOT_NEEDED();
    return result;
}


static void sdio_prepare_data_transfer( bus_transfer_direction_t direction, sdio_block_size_t block_size, /*@unique@*/ uint8_t* data, uint16_t data_size ) /*@modifies dma_data_source, user_data, user_data_size, dma_transfer_size@*/
{
    /* Setup a single transfer using the temp buffer */
    user_data         = data;
    user_data_size    = data_size;
    dma_transfer_size = (uint32_t) ( ( ( data_size + (uint16_t) block_size - 1 ) / (uint16_t) block_size ) * (uint16_t) block_size );

    if ( direction == BUS_WRITE )
    {
        dma_data_source = data;
    }
    else
    {
        dma_data_source = temp_dma_buffer;
    }

}



void host_platform_enable_high_speed_sdio( void )
{
    //...
    sdio_enable_bus_irq( );
}

static sdio_block_size_t find_optimal_block_size( uint32_t data_size )
{
    if ( data_size > (uint32_t) 256 )
        return SDIO_512B_BLOCK;
    if ( data_size > (uint32_t) 128 )
        return SDIO_256B_BLOCK;
    if ( data_size > (uint32_t) 64 )
        return SDIO_128B_BLOCK;
    if ( data_size > (uint32_t) 32 )
        return SDIO_64B_BLOCK;
    if ( data_size > (uint32_t) 16 )
        return SDIO_32B_BLOCK;
    if ( data_size > (uint32_t) 8 )
        return SDIO_16B_BLOCK;
    if ( data_size > (uint32_t) 4 )
        return SDIO_8B_BLOCK;
    if ( data_size > (uint32_t) 2 )
        return SDIO_4B_BLOCK;

    return SDIO_4B_BLOCK;
}

static uint32_t sdio_get_blocksize_dctrl(sdio_block_size_t block_size)
{
    switch (block_size)
    {
/*        case SDIO_1B_BLOCK:    return SDIO_DataBlockSize_1b;
        case SDIO_2B_BLOCK:    return SDIO_DataBlockSize_2b;
        case SDIO_4B_BLOCK:    return SDIO_DataBlockSize_4b;
        case SDIO_8B_BLOCK:    return SDIO_DataBlockSize_8b;
        case SDIO_16B_BLOCK:   return SDIO_DataBlockSize_16b;
        case SDIO_32B_BLOCK:   return SDIO_DataBlockSize_32b;
        case SDIO_64B_BLOCK:   return SDIO_DataBlockSize_64b;
        case SDIO_128B_BLOCK:  return SDIO_DataBlockSize_128b;
        case SDIO_256B_BLOCK:  return SDIO_DataBlockSize_256b;
        case SDIO_512B_BLOCK:  return SDIO_DataBlockSize_512b;
        case SDIO_1024B_BLOCK: return SDIO_DataBlockSize_1024b;
        case SDIO_2048B_BLOCK: return SDIO_DataBlockSize_2048b;*/
        default: return 0;
    }
}

void SDIO_IRQHandler(void)
{
  /* Process All SDIO Interrupt Sources */
  sdio_irq();
}

void DMA2_Stream3_IRQHandler(void)
{
  dma_irq();
}
