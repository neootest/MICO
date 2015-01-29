/**
 *******************************************************************************
 * @file    sdram.h
 * @author  Robert
 * @version V1.0.0
 * @date    24-July-2013
 * @brief	SDRMC(Synchronous Dynamic Random Access Memory Controller) is a controller to control
 *			and switch information between CPU & sdram memory chip.For the sake of driving most of
 *			SDRAM chips,function compatibility,efficiency reliability will be took into account.
 * Changelog:
 *******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#ifndef	__SDRAM_H__
#define	__SDRAM_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * support SDRAM chip currently
 */
enum
{
    SDRAM_TYPE_UNKNOWN,
    SDRAM_TYPE_MT48LC4M16A2,
    SDRAM_TYPE_MT48LC8M16A2,
    SDRAM_TYPE_HY57V281620,
    SDRAM_TYPE_HY57V641620,
    SDRAM_TYPE_K4S281632K,
    SDRAM_TYPE_K4S641632H,
    SDRAM_TYPE_EM639165TS,
    SDRAM_TYPE_EM638165TS,
    SDRAM_TYPE_M5S2128188, //VCC = 1V2
    SDRAM_TYPE_EM636165TS,
    SDRAM_TYPE_HY57V161610D,
    SDRAM_TYPE_K4S161622D,
    SDRAM_TYPE_OTHER,
};

/**
 * sdram control command
 */
enum
{
    SDRAM_CMD_GOTO_SLEEP = 0x01,	//put SDRAM into sleep work mode
    SDRAM_CMD_GOTO_STANDBY,			//put SDRAM into standby work mode
    SDRAM_CMD_WAKEUP,				//wake SDRAM up from sleep or standby work mode
    SDRAM_CMD_GET_STATUS,			//speed down access memory
    SDRAM_CMD_CLR_STATUS,			//speed down access memory
    SDRAM_CMD_GET_CAP,				//get sdram capacity in byte
    SDRAM_CMD_MODULE_ENDIS,
    SDRAM_CMD_USR_CFG,
    SDRAM_MASTER_CFG,
};

/**
 * @brief
 *   The SdramInit() function initialize the sdram chip key parameter with row referred to Rows
 * (11~13), column referred to Cols(8 ~ 1024), banks referred to Banks(2 or 4) and column latency(CL)
 * referred to CL(2 or 3).
 *
 * @param	NONE
 *
 * @return
 *   Upon successful completion,SdramInit() function shall return 0.
 *   Otherwise,a negative error code will be returned.
 *
 * ERRORS
 *   EINVAL	Invalid parmeter,for example,Rows,Cols,Banks and CL,too big or small,and so on
 * @note
 */
extern int32_t SdramInit(uint8_t Rows, uint8_t Cols, uint8_t Banks, uint8_t CL);

/**
 * @brief
 *   The SdramIOctl() function control SDRAM action including but not limited to configration,
 * sleep,wake up,auto-refresh and other high performance characteristics.
 *
 * @param	Cmd	I/O control command
 * @param	Opt	the control command argument if the command require
 *
 * @return
 *   Upon successful completion,SdramIOctl() function shall return the result like
 * SDRAM_CMD_GET_XXXX(such as SDRAM_CMD_GET_STATUS) or return zero like
 * SDRAM_CMD_SET_XXXX(such as SDRAM_CMD_SET_STATUS).
 * Otherwise,a negative error code will be returned.
 *
 * ERRORS
 *   ENOSYS The command is not available currently.
 *   EINVAL Invalid parameter,especially for SDRAM_CMD_GET_XXXX.
 * @note
 */
extern int32_t SdramIOctl(int32_t Cmd, uint32_t Opt);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//_SDRAM_H_
