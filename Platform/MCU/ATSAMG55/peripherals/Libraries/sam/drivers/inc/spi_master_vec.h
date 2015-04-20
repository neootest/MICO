/**
 * \file
 *
 * \brief SERCOM SPI master with vectored I/O driver include
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

#ifndef SPI_MASTER_VEC_H
#define SPI_MASTER_VEC_H

#include "compiler.h"
#include "spi.h"
#include "status_codes.h"
#include "conf_spi_master_vec.h"

/**
 * \defgroup asfdoc_samd20_sercom_spi_master_vec_group SAM D20 Serial Peripheral Interface Master Driver w/ Vectored I/O (SERCOM SPI)
 *
 * @{
 */

/**
 * \brief Driver configuration structure
 *
 * \sa asfdoc_samd20_sercom_spi_master_vec_special_considerations for more
 * information regarding SERCOM pad and pin MUX.
 */
struct spi_master_vec_config {
	/** Baud rate in Hertz. */
	uint32_t baudrate;
};

/**
 * \internal SPI transfer directions
 */
enum _spi_direction {
	SPI_DIRECTION_READ,
	SPI_DIRECTION_WRITE,
	SPI_DIRECTION_BOTH,
	SPI_DIRECTION_IDLE,
};

/**
 * \name Buffer Description
 * @{
 */

/** Buffer length container. */
typedef uint16_t spi_master_vec_buflen_t;

/** Buffer descriptor structure. */
struct spi_master_vec_bufdesc {
	/** Pointer to buffer start. */
	void *data;
	/** Length of buffer. */
	spi_master_vec_buflen_t length;
};

/** @} */

/** Driver instance. */
struct spi_master_vec_module {
	Spi *volatile spi_instance;
	volatile enum _spi_direction direction;
	volatile enum status_code status;
	volatile spi_master_vec_buflen_t rx_prev_length;
	volatile spi_master_vec_buflen_t tx_prev_length;
	uint8_t *volatile rx_head_ptr;
	uint8_t *volatile tx_head_ptr;
	volatile uint_fast8_t tx_lead_on_rx;
	struct spi_master_vec_bufdesc *volatile rx_bufdesc_ptr;
	struct spi_master_vec_bufdesc *volatile tx_bufdesc_ptr;
#ifdef CONF_SPI_MASTER_VEC_OS_SUPPORT
    //#ifndef USE_MICO_SEMA
	CONF_SPI_MASTER_VEC_SEMAPHORE_TYPE busy_semaphore;
    //#else
	//CONF_SPI_MASTER_VEC_SEMAPHORE_TYPE *busy_semaphore;
    //#endif
#endif
};

/**
 * \name Configuration and Initialization
 * @{
 */

/**
 * \brief Initialize configuration with default values.
 *
 * \param[out] config Configuration struct to initialize.
 */
static inline void spi_master_vec_get_config_defaults(
		struct spi_master_vec_config *const config)
{
	config->baudrate = 100000;
}

#ifdef __cplusplus
extern "C" {
#endif

enum status_code spi_master_vec_init(struct spi_master_vec_module *const module,
		Spi *const spi_instance, const struct spi_master_vec_config *const config);

/** @} */

/**
 * \name Enable/Disable and Reset
 * @{
 */

void spi_master_vec_enable(const struct spi_master_vec_module *const module);
void spi_master_vec_disable(struct spi_master_vec_module *const module);

/** @} */

/**
 * \name Read/Write and Status
 * @{
 */

enum status_code spi_master_vec_transceive_buffer_job(
		struct spi_master_vec_module *const module,
		struct spi_master_vec_bufdesc tx_bufdescs[],
		struct spi_master_vec_bufdesc rx_bufdescs[]);

/**
 * \brief Get current status of transfer.
 *
 * \param[in] module Driver instance to operate on.
 *
 * \return Current status of driver instance.
 * \retval STATUS_OK if idle and previous transfer succeeded.
 * \retval STATUS_BUSY if a transfer is ongoing.
 * \retval <other> if previous transfer failed.
 */
static inline enum status_code spi_master_vec_get_job_status(
		const struct spi_master_vec_module *const module)
{
	return module->status;
}

/**
 * \brief Get status of transfer upon job end.
 *
 * \param[in]  module Driver instance to operate on.
 *
 * \return Current status of driver instance.
 * \retval STATUS_OK if idle and previous transfer succeeded.
 * \retval <other> if previous transfer failed.
 */
static inline enum status_code spi_master_vec_get_job_status_wait(
		 struct spi_master_vec_module *const module)
{
	enum status_code status;

#ifdef CONF_SPI_MASTER_VEC_OS_SUPPORT
	CONF_SPI_MASTER_VEC_TAKE_SEMAPHORE(&module->busy_semaphore);
	status = spi_master_vec_get_job_status(module);
	CONF_SPI_MASTER_VEC_GIVE_SEMAPHORE(&module->busy_semaphore);
#else
	do {
		status = spi_master_vec_get_job_status(module);
	} while (status == STATUS_BUSY);
#endif

	return status;
}


/**
 * \brief Start vectored I/O transfer, wait for it to end.
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
 * \retval STATUS_OK if transfer succeeded.
 * \retval STATUS_BUSY if a transfer was already on-going.
 * \retval <other> if transfer failed.
 */
static inline enum status_code spi_master_vec_transceive_buffer_wait(
		struct spi_master_vec_module *const module,
		struct spi_master_vec_bufdesc tx_bufdescs[],
		struct spi_master_vec_bufdesc rx_bufdescs[])
{
	enum status_code status;

	status = spi_master_vec_transceive_buffer_job(module, tx_bufdescs,
			rx_bufdescs);

	if (status == STATUS_BUSY) {
		return status;
	}

	return spi_master_vec_get_job_status_wait(module);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SPI_MASTER_VEC_H */
