/**
 * \file
 *
 * \brief SERCOM SPI master with vectored I/O driver implementation
 *
 * Copyright (C) 2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

//#include "asf.h"
//#include "mico_common_driver.h"
#include "spi_master_vec.h"
#include "conf_board.h"
//#include "dbg_print.h"

/**
 * \ingroup asfdoc_samd20_sercom_spi_master_vec_group
 *
 * @{
 */

/** @} */

/* SPI PDC register base. */
Pdc *g_p_spi_pdc = 0;

struct spi_master_vec_module *g_module = 0;
#ifndef USE_OWN_SPI_DRV
/* Chip select. */
#define SPI_CHIP_SEL     0 // why 0 happen hard fault?
#define SPI_CHIP_PCS spi_get_pcs(SPI_CHIP_SEL)

/* Clock polarity. */
#define SPI_CLK_POLARITY 0

/* Clock phase. */
#define SPI_CLK_PHASE    1 

/* Delay before SPCK. */
#define SPI_DLYBS 0x00//0x40

/* Delay between consecutive transfers. */
#define SPI_DLYBCT 0x00//0x10

/**
 * \brief Initialize hardware and driver instance
 *
 * This function configures the clock system for the specified SERCOM module,
 * sets up the related pins and their MUX, initializes the SERCOM in SPI master
 * mode, and prepares the driver instance for operation.
 *
 * \pre \ref system_init() must have been called prior to this function.
 *
 * The SERCOM SPI module is left disabled after initialization, and must be
 * enabled with \ref spi_master_vec_enable() before a transfer can be done.
 *
 * \param[out] module Driver instance to initialize.
 * \param[in,out] sercom SERCOM module to initialize and associate driver
 * instance with.
 * \param[in] config Driver configuration to use.
 *
 * \return Status of initialization.
 * \retval STATUS_OK if initialization succeeded.
 * \retval STATUS_ERR_INVALID_ARG if driver has been misconfigured.
 */
enum status_code spi_master_vec_init(struct spi_master_vec_module *const module,
		Spi *const spi_instance, const struct spi_master_vec_config *const config)
{
	Assert(module);
	Assert(sercom);
	Assert(config);

	module->spi_instance = spi_instance;
	g_module = module;

#if (SAMG55)
	/* Enable the peripheral and set SPI mode. */
	flexcom_enable(BOARD_FLEXCOM_SPI);
	flexcom_set_opmode(BOARD_FLEXCOM_SPI, FLEXCOM_SPI);
#else
	/* Configure an SPI peripheral. */
	spi_enable_clock(spi_instance);
#endif
	spi_disable(spi_instance);
	spi_reset(spi_instance);
	spi_set_master_mode(spi_instance);
	spi_disable_mode_fault_detect(spi_instance);
#ifdef CONF_BOARD_SPI_NPCS0	
	spi_set_peripheral_chip_select_value(spi_instance, SPI_CHIP_PCS);
#endif
	spi_set_clock_polarity(spi_instance, SPI_CHIP_SEL, SPI_CLK_POLARITY);
	spi_set_clock_phase(spi_instance, SPI_CHIP_SEL, SPI_CLK_PHASE);
	spi_set_bits_per_transfer(spi_instance, SPI_CHIP_SEL, SPI_CSR_BITS_8_BIT);
	spi_set_baudrate_div(spi_instance, SPI_CHIP_SEL, (sysclk_get_cpu_hz() / config->baudrate));
	spi_set_transfer_delay(spi_instance, SPI_CHIP_SEL, SPI_DLYBS, SPI_DLYBCT);
	spi_enable(spi_instance);

	/* Get pointer to SPI PDC register base. */
	g_p_spi_pdc = spi_get_pdc_base(spi_instance);
	pdc_enable_transfer(g_p_spi_pdc, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);

	/* Initialize our instance and register interrupt handler + data */
	module->rx_bufdesc_ptr = NULL;
	module->tx_bufdesc_ptr = NULL;
	module->direction = SPI_DIRECTION_IDLE;
	module->status = STATUS_OK;
#ifdef CONF_SPI_MASTER_VEC_OS_SUPPORT
	CONF_SPI_MASTER_VEC_CREATE_SEMAPHORE(&module->busy_semaphore);
#endif

	/* Enable NVIC interrupts. */
	NVIC_SetPriority(SPI_IRQn, 3); //////////////////////////////////////////////////// FIXME
	NVIC_EnableIRQ(SPI_IRQn);

	return STATUS_OK;
}

/**
 * \brief Enable the SERCOM SPI module
 *
 * This function must be called after \ref spi_master_vec_init() before a
 * transfer can be started.
 *
 * \param[in,out] module Driver instance to operate on.
 */
void spi_master_vec_enable(const struct spi_master_vec_module *const module)
{
	Assert(module);
	Assert(module->spi_instance);

	Spi *const spi_hw = module->spi_instance;

	NVIC_EnableIRQ(SPI_IRQn);
	spi_enable(spi_hw);
}

/**
 * \brief Disable the SERCOM SPI module
 *
 * \param[in,out] module Driver instance to operate on.
 */
void spi_master_vec_disable(struct spi_master_vec_module *const module)
{
	Assert(module);
	Assert(module->sercom);

	Spi *const spi_hw = module->spi_instance;

	NVIC_DisableIRQ(SPI_IRQn);
	spi_disable(spi_hw);

	module->rx_bufdesc_ptr = NULL;
	module->tx_bufdesc_ptr = NULL;
	module->direction = SPI_DIRECTION_IDLE;
	module->status = STATUS_OK;
}

/**
 * \brief Start vectored I/O transfer
 *
 * This function initiates a uni- or bidirectional SPI transfer from/to any
 * number of data buffers. The transfer is interrupt-driven and will run in the
 * background, after this function has returned.
 *
 * The buffers to transmit from or receive into must be described in arrays of
 * buffer descriptors. These arrays \e must end with descriptors that specify
 * zero buffer length. The first descriptor in an array can \e not specify zero
 * length. The number of bytes to transmit and to receive do not have to be
 * equal.
 *
 * To initiate a unidirectional transfer, pass \c NULL as the address of either
 * buffer descriptor array, like this:
 * \code
 *     // Transmit some buffers
 *     spi_master_vec_transceive_buffer_job(&module, tx_buffers, NULL);
 *
 *     // Receive some buffers
 *     spi_master_vec_transceive_buffer_job(&module, NULL, rx_buffers);
 * \endcode
 *
 * \pre \ref spi_master_vec_init() and \ref spi_master_vec_enable() must have
 * been called before this function.
 *
 * \param[in,out] module Driver instance to operate on.
 * \param[in] tx_bufdescs address of buffer descriptor array for bytes to
 * transmit.
 * \arg \c NULL if the transfer is a simplex read.
 * \param[in,out] rx_bufdescs address of buffer descriptor array for storing
 * received bytes.
 * \arg \c NULL if the transfer is a simplex write.
 *
 * \return Status of transfer start.
 * \retval STATUS_OK if transfer was started.
 * \retval STATUS_BUSY if a transfer is already on-going.
 */
enum status_code spi_master_vec_transceive_buffer_job(
		struct spi_master_vec_module *const module,
		struct spi_master_vec_bufdesc tx_bufdescs[],
		struct spi_master_vec_bufdesc rx_bufdescs[])
{
	Assert(module);
	Assert(module->spi_instance);
	Assert(tx_bufdescs || rx_bufdescs);

	pdc_packet_t g_pdc_spi_tx_packet = {0, 0};
	pdc_packet_t g_pdc_spi_rx_packet = {0, 0};

	// FOR DEBUG ONLY!!!
	uint32_t tmp;
	for (uint32_t i = 0; tx_bufdescs[i].length; ++i) {
		tmp = (uint32_t)tx_bufdescs[i].data;
		if (tmp >= IFLASH_ADDR && tmp < IFLASH_ADDR + IFLASH_SIZE)
			while (1);
	}
	for (uint32_t i = 0; rx_bufdescs[i].length; ++i) {
		tmp = (uint32_t)rx_bufdescs[i].data;
		if (tmp >= IFLASH_ADDR && tmp < IFLASH_ADDR + IFLASH_SIZE)
			while (1);
	}

	cpu_irq_enter_critical();
	if (module->status == STATUS_BUSY) {
		cpu_irq_leave_critical();
		return STATUS_BUSY;
	} else {
		module->status = STATUS_BUSY;
		cpu_irq_leave_critical();
	}

#ifdef CONF_SPI_MASTER_VEC_OS_SUPPORT
	CONF_SPI_MASTER_VEC_TAKE_SEMAPHORE(&module->busy_semaphore);
#endif

	module->tx_bufdesc_ptr = tx_bufdescs;
	module->rx_bufdesc_ptr = rx_bufdescs;

	/* Prepare RX PDC transfer. */
	if (rx_bufdescs) {
		Assert(rx_bufdescs[0].length);
		g_pdc_spi_rx_packet.ul_addr = (uint32_t) module->rx_bufdesc_ptr->data;
		g_pdc_spi_rx_packet.ul_size = module->rx_bufdesc_ptr->length;
		g_pdc_spi_tx_packet.ul_addr = (uint32_t) module->rx_bufdesc_ptr->data;
		g_pdc_spi_tx_packet.ul_size = module->rx_bufdesc_ptr->length;
		++module->rx_bufdesc_ptr;
		pdc_rx_init(g_p_spi_pdc, 0, &g_pdc_spi_rx_packet);
	}

	/* Prepare TX PDC transfer. */
	if (tx_bufdescs) {
		Assert(tx_bufdescs[0].length);
		g_pdc_spi_tx_packet.ul_addr = (uint32_t) module->tx_bufdesc_ptr->data;
		if (g_pdc_spi_rx_packet.ul_size < module->tx_bufdesc_ptr->length)
			g_pdc_spi_tx_packet.ul_size = module->tx_bufdesc_ptr->length;
		++module->tx_bufdesc_ptr;
	}
	pdc_tx_init(g_p_spi_pdc, 0, &g_pdc_spi_tx_packet);


	if (g_pdc_spi_tx_packet.ul_size > g_pdc_spi_rx_packet.ul_size) {
		spi_enable_interrupt(module->spi_instance, SPI_IER_ENDTX);
	}
	else {
		spi_enable_interrupt(module->spi_instance, SPI_IER_ENDRX);
	}

	/* Start PDC transfer (mandatory to clear receive register). */
	SPI->SPI_RDR;
	SPI->SPI_RDR;
	pdc_enable_transfer(g_p_spi_pdc, PERIPH_PTCR_RXTEN | PERIPH_PTCR_TXTEN);
    /*ioport_set_pin_level(SPI_NPCS0_GPIO, IOPORT_PIN_LEVEL_LOW);//cs high(enable)
		if (rx_bufdescs[0].length) {
	        pdc_enable_transfer(g_p_spi_pdc, PERIPH_PTCR_RXTEN | PERIPH_PTCR_TXTEN);
        } else {
	        pdc_enable_transfer(g_p_spi_pdc, PERIPH_PTCR_TXTEN);
        }
    //ioport_set_pin_level(SPI_NPCS0_GPIO, IOPORT_PIN_LEVEL_HIGH);//cs high(disable)
*/
	return STATUS_OK;
}

/**
 * \internal
 * \brief Interrupt handler
 */
void SPI_Handler(void)
{
	Spi *const spi_hw = g_module->spi_instance;
	pdc_packet_t g_pdc_spi_tx_packet = {0, 0};
	pdc_packet_t g_pdc_spi_rx_packet = {0, 0};
        
    //wwd_irq();

	spi_read_status(spi_hw);
	spi_disable_interrupt(spi_hw, 0xffffffff);
	pdc_disable_transfer(g_p_spi_pdc, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);

	if (g_module->rx_bufdesc_ptr->length || g_module->tx_bufdesc_ptr->length) {

		/* Prepare RX PDC transfer. */
       // wwd_irq();
		
		if (g_module->rx_bufdesc_ptr->length) {
			g_pdc_spi_rx_packet.ul_addr = (uint32_t) g_module->rx_bufdesc_ptr->data;
			g_pdc_spi_rx_packet.ul_size = g_module->rx_bufdesc_ptr->length;
			g_pdc_spi_tx_packet.ul_addr = (uint32_t) g_module->rx_bufdesc_ptr->data;
			g_pdc_spi_tx_packet.ul_size = g_module->rx_bufdesc_ptr->length;
			++g_module->rx_bufdesc_ptr;
			pdc_rx_init(g_p_spi_pdc, 0, &g_pdc_spi_rx_packet);
		}

		/* Prepare TX PDC transfer. */
		if (g_module->tx_bufdesc_ptr->length) {
			g_pdc_spi_tx_packet.ul_addr = (uint32_t) g_module->tx_bufdesc_ptr->data;
			if (g_pdc_spi_rx_packet.ul_size < g_module->tx_bufdesc_ptr->length)
				g_pdc_spi_tx_packet.ul_size = g_module->tx_bufdesc_ptr->length;
			++g_module->tx_bufdesc_ptr;
		}
		pdc_tx_init(g_p_spi_pdc, 0, &g_pdc_spi_tx_packet);

		if (g_pdc_spi_tx_packet.ul_size > g_pdc_spi_rx_packet.ul_size) {
			spi_enable_interrupt(g_module->spi_instance, SPI_IER_ENDTX);
		}
		else {
			spi_enable_interrupt(g_module->spi_instance, SPI_IER_ENDRX);
		}

		/* Start PDC transfer (mandatory to clear receive register). */
		SPI->SPI_RDR;
		SPI->SPI_RDR;
		pdc_enable_transfer(g_p_spi_pdc, PERIPH_PTCR_RXTEN | PERIPH_PTCR_TXTEN);
        /*ioport_set_pin_level(SPI_NPCS0_GPIO, IOPORT_PIN_LEVEL_LOW);//cs high(enable)
		if (--g_module->rx_bufdesc_ptr->length) {
	        pdc_enable_transfer(g_p_spi_pdc, PERIPH_PTCR_RXTEN | PERIPH_PTCR_TXTEN);
        } else {
	        pdc_enable_transfer(g_p_spi_pdc, PERIPH_PTCR_TXTEN);
        }
        ioport_set_pin_level(SPI_NPCS0_GPIO, IOPORT_PIN_LEVEL_HIGH);//cs high(disable) 
        */
	}
	else {
		g_module->status = STATUS_OK;
#ifdef CONF_SPI_MASTER_VEC_OS_SUPPORT
		CONF_SPI_MASTER_VEC_GIVE_SEMAPHORE_FROM_ISR(&g_module->busy_semaphore);
#endif
	}
}
#endif
/**
 * @}
 */
