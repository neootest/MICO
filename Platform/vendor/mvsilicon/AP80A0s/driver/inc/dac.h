/**
 *******************************************************************************
 * @file    dac.h
 * @author  Wang Yun
 * @version V1.0.0
 * @date    30-June-2014
 * @brief   dac module c header file
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

#ifndef __DAC_H__
#define __DAC_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

#define MDAC_DATA_FROM_DEC                      0
#define MDAC_DATA_FROM_I2S                      1

#define MDAC_MUTE_EN                            1
#define MDAC_MUTE_DIS                           0

#define MDAC_DITHER_POW                         0x1400
/**
 * DAC(mixer) input channle
 */
typedef enum _DAC_CHANNEL
{
    DAC_CH_NONE	  = 0,
    DAC_CH_FM1_L  = 0x01,
    DAC_CH_FM2_L  = 0x02,
    DAC_CH_DECD_L = 0x04,
    DAC_CH_LINE_L = 0x08,
    DAC_CH_FM1_R  = 0x10,
    DAC_CH_FM2_R  = 0x20,
    DAC_CH_DECD_R = 0x40,
    DAC_CH_LINE_R = 0x80,
    DAC_CH_PGA_L  = 0x100,
    DAC_CH_PGA_R  = 0x200,
    DAC_CH_MASK   = 0x3FF
} DAC_CHANNEL;


/*****************************************************************************
*                   		Digital part of DAC
******************************************************************************/

/**
 * @brief	Disable DAC.
 * @details	Close DAC interpolate filter.
 * @param	None
 * @return	None
 * @note
 */
void DacDisable(void);

/**
 * @brief	Enable DAC.
 * @details	Open CODAC module clock & interpolate filter.
 * @param	None
 * @return	None
 * @note
 */
void DacEnable(void);

/**
 * @brief	Set the sample rate of CODAC
 * @param	SampleRate -> Sample rate the the DAC(8/11.025/12/16/22.05/24/32/44.1/48 KHz)
 * 			Mode	   -> MCLK mode(USB_MODE/NORMAL_MODE)
 * @return  None
 * @note	1).外接ClassD芯片时，调用DacAdcSampleRateSet(SampRate, NORMAL_MODE)和
 *			   ClkMclkSelBySampleRate(SampRate, NORMAL_MODE)函数完成采样率和MCLK时钟配置。
 *			2).外接CLASSD芯片时8000/11025/12000三种采样率下，有可能需要将DAC输出的模拟信号
 *			   通过芯片内置的ADC模块重新采样并输出IIS信号给ClassD芯片。注意在这种模式下PHUB
 *			   需要开启OFIFOIN_MIX_DACOUT通路,同时需要通过配置I2S_CTL0寄存器(I2S_SEL位置1)
 *			   开启ADCIN to IISOUT通路。这种情况下DAC和ADC的采样率不一致，IIS模块的采样率
 *			   必须与ADC保持一致，8000/12000两种采样率下ADC/IIS的采样率应该配置为48KHz，
 *			   11025采样率下ADC/IIS的采样率应该配置为44100Hz。
 */
void DacAdcSampleRateSet(uint32_t SampleRate, uint8_t Mode);

/**
 * @brief  	Get the sample rate of CODAC.
 * @param	None
 * @return 	sample rate of codac 
 * @note	
 */
uint32_t DacAdcSampleRateGet(void);


/**
 * @brief  	DAC config.
 * @param	DatDirSel	-> Dac frame-sync signal select.
 *			ClkMode		-> MCLK mode(USB_MODE/NORMAL_MODE).
 * @return 	None
 * @note	DatDirSel must be MDAC_DATA_FROM_DEC when phub sync_rdwr_en_mode=2'b01,
 *			otherwise,must be MDAC_DATA_FROM_I2S
 */
void DacConfig(uint8_t DatDirSel, uint8_t ClkMode);

/**
 * @brief	Sidetone config.
 * @param	SdtgL	-> sidetone left channel value.
 * @param	SdtgR	-> sidetone right channel value.
 * @return	None
 */
void DacSidetoneConfig(uint8_t SdtgL, uint8_t SdtgR);

/**
 * @brief	DAC (interpolate filter) volume set
 * @param   LVol	-> [0x001:-72dB, 0xFFF:0dB]
 * @param   RVol	-> [0x001:-72dB, 0xFFF:0dB]
 * @return	None
 * @note    Digital adc mute When value equals 0x0 
 * @note	Valid only for OFIFOIN_MIX_DACOUT phub path
 */
void DacVolumeSet(uint16_t LVol, uint16_t RVol);

/**
 * @brief	DAC soft-mute on/off control
 * @param	LeftMuteEn	-> set to TRUE to mute the output of the Left channel DAC.
 * @param 	RightMuteEn	-> set to TRUE to mute the output of the Right channel DAC.
 * @return	None
 * @note
 */
void DacSoftMuteSet(bool LeftMuteEn, bool RightMuteEn);

/**
 * @brief	DAC mono config
 * @param	IsMonoEn	-> set to TRUE to enable Mono
 			IsMonoShift	-> TRUE: mono output to left channel, shift 180 to right channel
 * @return	None
 * @note	IsMonoShift is only valid when IsMonoEn set to TRUE.
 */
void DacMonoConfig(bool IsMonoEn, bool IsMonoShift);

/**
 * @brief  adjust DAC clock frequnecy(sample rate)
 * @param  AdjEn TRUE: enable, FALSE: disable dac samplerate adjustment function 
 * @param  Ppm range:[-524287,524287] >0: clock speed up val*0.2ppm  <0:clock slow down val*0.2ppm  
 * @return None
*/
void DacSampRateAdjust(bool AdjEn, int32_t Ppm);

/**
 * @brief 	Check if the DAC's clock mode is USB mode(12MHz) or not.
 * @param	None
 * @retval	Return TRUE for USB mode, otherwise return FALSE.
 * @note
 */
bool DacIsUsbMode(void);

/**
 * @brief 	not use capasity,chip drive earphone directly
 * @param	None
 * @return	None
 */
void DacNoUseCapacityToPlay(void);

/**
 * @brief 	Save the current state of CODEC clock & Open it.
 * @param	IsFastCharge	-> set to TRUE for fast-charge.
 * @return	None
 * @note
 */
void CodecClockEnable(void);

/**
 * @brief 	Restore CODEC clock state.
 * @param	IsFastCharge	-> set to TRUE for fast-charge.
 * @return	None
 * @note
 */
void CodecClockRestore(void);

/**
 * @brief 	codec module DAC section analog volume set.
 * @param	LVol	-> 0:23dB 1:0dB 2:-6dB 3:-12dB
 * @param	RVol	-> 0:23dB 1:0dB 2:-6dB 3:-12dB
 * @return	None
 * @pre
 * @note
 */
void CodecDacAnaVolSet(uint8_t LVol, uint8_t RVol);

/**
 * @brief	Codec module mix section mute on/off control by directly setting mute
 * @param	LeftMuteEn	-> set to TRUE to mute the output of the Left channel DAC.
 * @param 	RightMuteEn	-> set to TRUE to mute the output of the Right channel DAC.
 * @return	None
 * @note
 */
void CodecMixDirectMuteSet(bool LeftMuteEn, bool RightMuteEn);

/**
 * @brief	Codec mute on/off control
 * @param	LeftMuteEn	-> set to TRUE to mute the output of the Left channel DAC.
 * @param 	RightMuteEn	-> set to TRUE to mute the output of the Right channel DAC.
 * @return	None
 * @note
 */
void CodecDacMuteSet(bool LeftMuteEn, bool RightMuteEn);

/**
 * @brief 	Initialize Codec module Dac section.
 * @param	IsFastCharge	-> set to TRUE for fast-charge.
 * @return	Return TRUE if successful, or FALSE otherwise.
 * @pre
 * @note
 */
bool CodecDacInit(bool IsFastCharge);

/**
 * @brief 	Deinitialize Codec module Dac section.
 * @param	IsFastCharge	-> set to TRUE for fast-charge.
 * @return	Return TRUE if successful, or FALSE otherwise.
 */
bool CodecDacDeinit(bool IsFastCharge);

/**
 * @brief 	Codec channel selection.
 * @param	Channel			-> Indicate that which channel will strobe to the MIXER.
 * @return	Return TRUE if successful, or FALSE otherwise.
 * @pre		The CodecDacInit() function must be executed previously.
 * @note
 */
bool CodecDacChannelSel(uint16_t Channel);

/**
 * @brief 	Codec module dac section channel selection and will not change mute or 
            unmute mode.
 * @param	Channel			-> Indicate that which channel will strobe to the MIXER.
 * @return	Return TRUE if successful, or FALSE otherwise.
 * @pre		The CodecDacInit() function must be executed previously.
 * @note    The function is different with function named CodecDacChannelSel,it will 
            not exit mute mode when a connection is established and it will not into
            mute mode when a disconnection is established.
 */
bool CodecDacChannelSelNotChangeMode(uint16_t Channel);
/**
 * @brief  only use Line in analog path
 * @param  VolLeft	-> [0x00:-53.26dB, 0x3F:21.36dB]
 * @param  VolLeft	-> [0x00:-53.26dB, 0x3F:21.36dB]
 * @return NONE
 */
void CodecDacLineInGainConfig(uint8_t VolLeft, uint8_t VolRight);

/**
 * @brief  only use FM in analog path
 * @param  VolLeft	-> [0x00:-53.26dB, 0x3F:21.36dB]
 * @param  VolLeft	-> [0x00:-53.26dB, 0x3F:21.36dB]
 * @return NONE
 */
void CodecDacFmInGainConfig(uint8_t VolLeft, uint8_t VolRight);

/**
 * @brief  only use MIC in analog path
 * @param  VolLeft	-> [0x00:-53.26dB, 0x3F:21.36dB]
 * @param  VolLeft	-> [0x00:-53.26dB, 0x3F:21.36dB]
 * @return NONE
 */
void CodecDacMicInGainConfig(uint8_t VolLeft, uint8_t VolRight);

/**
 * @brief 	judge whether DAC digital section is mute
 * @param	None
 * @return	Mute is return TRUE otherwise return FALSE
 */
bool CodecDacIsSoftMute(void);

/**
 * @brief 	judge whether DAC analog section is mute
 * @param	None
 * @return	Mute is return TRUE otherwise return FALSE
 */
bool CodecDacIsAnaMute(void);

/**
 * @brief  enable mic bias
 * @param  BiasEn 1: enable 0:disable
 * @return None
 */
void CodecMicBiasEn(bool BiasEn);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif	//__DAC_H__
