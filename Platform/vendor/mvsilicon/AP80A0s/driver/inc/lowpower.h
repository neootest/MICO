/**
  *****************************************************************************
  * @file:			powerkey.h
  * @author			Ingrid Chen
  * @version		V1.0.0
  * @data			14-June-2013
  * @Brief			Lowpower module driver header file.
  ******************************************************************************
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
  */

#ifndef __LOWPOWER_H__
#define	__LOWPOWER_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

#define RST_MODE_0 0
#define RST_MODE_1 1
#define SOFT_PD_EN 2
#define SOFT_PD_DIS 3
#define RTC32K_WAKEUP_ENABLE 4
#define RTC32K_WAKEUP_DISABLE 5

void SetLpMode(uint8_t Mode);
/**
 * @Brief:	Set Lowpower module in non-Button mode
 * @Param:	None
 * @Return:	None
 * @Note@:
 */
void LpSetBypassMode(void);

/**
 * @Brief:	Set Lowpower module in Slider-Button mode
 * @Param:	None
 * @Return:	None
 * @Note@:
 */
void LpSetHwMode(void);

/**
 * @Brief:	Set Lowpower module in Push-Button mode
 * @Param:	None
 * @Return:	None
 * @Note@:
 */
void LpSetSwMode(void);

/**
 * @Brief:	Set mcu_off_seq to to power down core
 * @Param:	None
 * @Return:	None
 * @Note@:
 */
void LpSetMcuOffSeq(void);

/**
 * @Brief:	Clear mcu_off_seq to make sure the core is power on
 * @Param:	None
 * @Return:	None
 * @Note@:
 */
void LpClrMcuOffSeq(void);

/**
 * @Brief:	Get mcu_off_seq status
 * @Param:	None
 * @Return:	TURE or FALSE
 * @Note@:
 */
bool LpGetMcuOffSeq(void);

/**
 * @Brief:	Clear Onkey triggle flag
 * @Param:	None
 * @Return:	None
 * @Note@:
 */
void LpClrOnkeyTrg(void);

/**
 * @Brief:	Clear 8 second reset flag
 * @Param:	None
 * @Return:	None
 * @Note@:
 */
void LpClrRst8s(void);

/**
 * @Brief:	Clear wakeup flag
 * @Param:	None
 * @Return:	None
 * @Note@:
 */
void LpClrWakeCore(void);

/**
 * @Brief:	Get pad_onkey status is pressed or not
 * @Param:	None
 * @Return:	TURE or FALSE
 * @Note@:
 */
bool LpGetOnkeyReg(void);

/**
 * @Brief:	Get Sonoff flag is set or not
 * @Param:	None
 * @Return:	TURE or FALSE
 * @Note@:
 */
bool LpGetSOnOff(void);

/**
 * @Brief:	Get the time pad_onkey pressed has lasted for onkey_trg_cnt_max time or not
 * @Param:	None
 * @Return:	TURE or FALSE
 * @Note@:
 */
bool LpGetOnkeyTrg(void);

/**
 * @Brief:	Get  osc32k is steady to be used to count or not
 * @Param:	None
 * @Return:	TURE or FALSE
 * @Note@:
 */
bool LpGetOsc32KReady(void);

/**
 * @Brief:	Get 8s reset had happened or not
 * @Param:	None
 * @Return:	TURE or FALSE
 * @Note@:
 */
bool LpGetRst8s(void);

/**
 * @Brief: Set how long Powerkey = 1 lasts to power up the core.
 * @Param: Maxcnt 0~0xFF
 * @Return: None
 * @Note@: the time is equal to (8*onkey_trg_cnt_max+0.5625)ms
 */
void LpSetOnkeyCnt(uint8_t Maxcnt);


#ifdef  __cplusplus
}
#endif//__cplusplus

#endif
