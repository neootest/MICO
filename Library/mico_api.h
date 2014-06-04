/**
  ******************************************************************************
  * @file    mico_api.h 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file provides all the headers of APIs provided by MICO.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
  ******************************************************************************
  */ 

/** \mainpage MICO 

  This documentation describes the MICO APIs.
  It consists of:

     - MICO RTOS and Timer APIs       - mico_rtos.h
     - MICO Wi-Fi connectivith APIs   - mico_wlan.h
     - MICO BSD socket APIs           - mico_socket.h
     - MICO discribute callback APIs  - mico_callback.h
= */

#ifndef __MICO_API_H_
#define __MICO_API_H_

#include "Debug.h"

#include "mico_rtos.h"

#include "mico_wlan.h"

#include "mico_socket.h"

#include "mico_system_monitor.h"

#include "mico_callback.h"

#include "mico_algorithm.h"


/**
  * @brief  Get RF driver's version.
  * @note   Create a memery buffer to store the version characters.
  *         THe input buffer should be 40 bytes at least.
  * @note   This should be executed after mxchipInit().
  * @param  inVersion: Buffer address to store the RF driver. 
  * @param  inLength: Buffer size. 
  * @retval None
  */
void wlan_driver_version( char* inVersion, uint8_t inLength );


/**
  * @brief  Get MICO's version.
  * @param  None 
  * @retval The memory address that store the MICO's version characters.
  */
char* system_lib_version( void );


/**
  * @brief  Initialize the TCPIP stack thread, RF driver thread, and other
            supporting threads needed for wlan connection. Do some necessary
            initialization
  * @param  None 
  * @retval None
  */
void mxchipInit( void );

#endif /* __MICO_API_H_ */

