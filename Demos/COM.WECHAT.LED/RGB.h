/*
 HSB Color, 
 Library that converts HSB color values to RGB colors.
 Created by Julio Terra, June 4, 2011.
 
 Header File Name: HSBColor.h 
 Implementation File Name: HSBColor.h 
 
 */

#ifndef HSBColor_h
#define HSBColor_h


#define H2R_MAX_RGB_val 255.0

void H2R_HSBtoRGB(float hue, float sat, float bright, float *color);

void OpenLED_RGB(float *color);
void CloseLED_RGB();

//void OpenLED_W(float Size);
//void CloseLED_W();
//
//void OpenLED_C(float Size);
//void CloseLED_C();

#endif
