/**
  ******************************************************************************
  * @file    RemoteTcpClient.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   Create a TCP client thread, and connect to a remote server.
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

#include "MICOAppDefine.h"
#include "MICODefine.h"
//#include "stm32f4xx.h"
#include "RGB.h"

#define rgb_log(M, ...) custom_log("rgb", M, ##__VA_ARGS__)
#define rgb_log_trace() custom_log_trace("rgb")

float constrain(float value, float min, float max){
  if(value >= max)
    return max;
  if(value <=min )
    return min;
  return value;
}

float Percent(float value){
  return value = (((float)value / 255.0) * 100.0);
}

void H2R_HSBtoRGB(float hue, float sat, float bright, float *color) {
		
  // constrain all input variables to expected range
  hue = constrain(hue, 0, 360);
  sat = constrain(sat, 0, 100);
  bright = constrain(bright, 0, 100);

  // define maximum value for RGB array elements
  float max_rgb_val = H2R_MAX_RGB_val;

  // convert saturation and brightness value to decimals and init r, g, b variables
  float sat_f = (float)sat / 100.0;
  float bright_f = (float)bright / 100.0;        
  float r, g, b;
    
  // If brightness is 0 then color is black (achromatic)
  // therefore, R, G and B values will all equal to 0
  if (bright <= 0) {      
    color[0] = 0;
    color[1] = 0;
    color[2] = 0;
  } 
	
  // If saturation is 0 then color is gray (achromatic)
  // therefore, R, G and B values will all equal the current brightness
  if (sat <= 0) {      
    color[0] = bright_f * max_rgb_val;
    color[1] = bright_f * max_rgb_val;
    color[2] = bright_f * max_rgb_val;
  } 
    
  // if saturation and brightness are greater than 0 then calculate 
  // R, G and B values based on the current hue and brightness
  else {  
    if (hue >= 0 && hue < 120) {
      float hue_primary = 1.0 - ((float)hue / 120.0);
      float hue_secondary = (float)hue / 120.0;
      float sat_primary = (1.0 - hue_primary) * (1.0 - sat_f);
      float sat_secondary = (1.0 - hue_secondary) * (1.0 - sat_f);
      float sat_tertiary = 1.0 - sat_f;
      r = (bright_f * max_rgb_val) * (hue_primary + sat_primary);
      g = (bright_f * max_rgb_val) * (hue_secondary + sat_secondary);
      b = (bright_f * max_rgb_val) * sat_tertiary;  
    }
    else if (hue >= 120 && hue < 240) {
      float hue_primary = 1.0 - (((float)hue-120.0) / 120.0);
      float hue_secondary = ((float)hue-120.0) / 120.0;
      float sat_primary = (1.0 - hue_primary) * (1.0 - sat_f);
      float sat_secondary = (1.0 - hue_secondary) * (1.0 - sat_f);
      float sat_tertiary = 1.0 - sat_f;
      r = (bright_f * max_rgb_val) * sat_tertiary;  
      g = (bright_f * max_rgb_val) * (hue_primary + sat_primary);
      b = (bright_f * max_rgb_val) * (hue_secondary + sat_secondary);
    }
    else if (hue >= 240 && hue <= 360) {
      float hue_primary = 1.0 - (((float)hue-240.0) / 120.0);
      float hue_secondary = ((float)hue-240.0) / 120.0;
      float sat_primary = (1.0 - hue_primary) * (1.0 - sat_f);
      float sat_secondary = (1.0 - hue_secondary) * (1.0 - sat_f);
      float sat_tertiary = 1.0 - sat_f;
      r = (bright_f * max_rgb_val) * (hue_secondary + sat_secondary);
      g = (bright_f * max_rgb_val) * sat_tertiary;  
      b = (bright_f * max_rgb_val) * (hue_primary + sat_primary);
    }
    
    color[0] = r;
    color[1] = g;
    color[2] = b;

  }
  color[0] = Percent(color[0]);
  color[1] = Percent(color[1]);
  color[2] = Percent(color[2]);

}

void OpenLED_RGB(float *color){
    MicoPwmInitialize( (mico_pwm_t)MICO_PWM_R, 50, (float)color[0]);
    MicoPwmStart( (mico_pwm_t)MICO_PWM_R);
    
    MicoPwmInitialize( (mico_pwm_t)MICO_PWM_G, 50, (float)color[1]);
    MicoPwmStart( (mico_pwm_t)MICO_PWM_G);
    
    MicoPwmInitialize( (mico_pwm_t)MICO_PWM_B, 50, (float)color[2]);
    MicoPwmStart( (mico_pwm_t)MICO_PWM_B);
}

void CloseLED_RGB(){
  MicoPwmStop((mico_pwm_t)MICO_PWM_R);
  MicoPwmStop((mico_pwm_t)MICO_PWM_G);
  MicoPwmStop((mico_pwm_t)MICO_PWM_B);
}

//void OpenLED_W(float Size){
//  MicoPwmInitialize( (mico_pwm_t)MICO_PWM_W, 50, 100 - (float)Size);
//  MicoPwmStart( (mico_pwm_t)MICO_PWM_W);
//}
//
//void CloseLED_W(){
//  MicoPwmStop((mico_pwm_t)MICO_PWM_W);
//}
//
//void OpenLED_C(float Size){
//  MicoPwmInitialize( (mico_pwm_t)MICO_PWM_C, 50, 100 - (float)Size);
//  MicoPwmStart( (mico_pwm_t)MICO_PWM_C); 
//}
//
//void CloseLED_C(){
//  MicoPwmStop((mico_pwm_t)MICO_PWM_C);
//}







