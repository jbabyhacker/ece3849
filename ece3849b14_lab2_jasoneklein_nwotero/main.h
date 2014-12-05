/*
 * main.h
 *
 *  Created on: Nov 7, 2014
 *      Author: Nicholas
 */

#ifndef MAIN_H_
#define MAIN_H_

// Includes from TI qs_eklm3s8962
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/lm3s8962.h"
#include "driverlib/sysctl.h"

//Includes for ADC
#include "driverlib/adc.h"

//Includes for OLED
#include "drivers/rit128x96x4.h"
#include "frame_graphics.h"
#include "utils/ustdlib.h"

//Includes for timer
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

//Includes for buttons
#include "driverlib/gpio.h"
#include "buttons.h"

//Includes for analog clock sin() and cos()
#include "math.h"

//Includes for NULL
#include "stdlib.h"

//Includes for float max
#include "float.h"

#include "kiss_fft.h"
#include "_kiss_fft_guts.h"

//Defines
#define BUTTON_CLOCK 200 // button scanning interrupt rate in Hz
#define M_PI 3.14159265358979323846f // Mathematical constant pi
#define ADC_BUFFER_SIZE 2048 // must be a power of 2
#define ADC_BUFFER_WRAP(i) ((i) & (ADC_BUFFER_SIZE - 1)) // index wrapping macro
#define SCREEN_WIDTH 128 // width of OLED screen in pixels
#define ADC_OFFSET 0 //512
#define VIN_RANGE 3.0
#define PIXELS_PER_DIV 12
#define ADC_BITS 10
#define BUTTON_BUFFER_SIZE 10
#define ADC_TO_VOLT(c) (2.0*((((float)VIN_RANGE / (float)(1 << ADC_BITS)) * (float)(c - ADC_OFFSET)) - 1.5))
#define VOLT_TO_ADC(v) ((((((float)(1 << ADC_BITS) / (float)VIN_RANGE) * (float)(v)) + ADC_OFFSET)/2.0) + 512.0)
#define BRIGHT 15 // bright pixel level
#define DIM 2 // dim pixel level
#define TRIGGER_X_POS 110 // trigger level symbol, position along x-axis, to center of symbol
#define TRIGGER_Y_POS 5 // trigger level symbol, position along y-axis, to center of symbol
#define TRIGGER_H_LINE 6 // trigger level symbol, length of horizontal lines, measured from center of symbol
#define TRIGGER_V_LINE 4 // trigger level symbol, length of vertical lines, measured from center of symbol
#define TRIGGER_ARROW_WIDTH 4 // trigger level symbol, arrow distance from center horizontally
#define TRIGGER_ARROW_HEIGHT 2 // trigger level symbol, arrow distance from center vertically

#define PI 3.14159265358979
#define NFFT 1024 // FFT length
#define KISS_FFT_CFG_SIZE (sizeof(struct kiss_fft_state)+sizeof(kiss_fft_cpx)*(NFFT-1))

//Structures
typedef struct {
	unsigned short x;
	unsigned short y;
} Point;

// Globals
unsigned long g_ulSystemClock; // system clock frequency in Hz
volatile unsigned long g_ulTime = 0; // time in hundredths of a second

volatile int g_iADCBufferIndex = ADC_BUFFER_SIZE - 1;  // latest sample index
volatile unsigned short g_pusADCBuffer[ADC_BUFFER_SIZE]; // circular buffer

//volatile Point g_ppWaveformBuffer[SCREEN_WIDTH];
volatile Point g_ppWaveformBuffer[NFFT];
//volatile int g_ppWaveformBuffer[NFFT];
volatile unsigned long g_ulADCErrors = 0; // number of missed ADC deadlines

//volatile unsigned short g_pusWaveformBuffer[SCREEN_WIDTH];
volatile unsigned char g_ucTriggerLevel = 0;
volatile int g_iTriggerDirection = 1;
volatile unsigned long g_ulTriggerSearchFail = 0;
//unsigned long g_ulCount_unloaded;// counts of iterable before interrupts are enabled
//float cpu_load;

volatile unsigned char g_ucSelectionIndex = 1; // index for selected top-screen gui element
volatile unsigned int g_uiMVoltsPerDiv = 500;	// miliVolts per division of the screen
volatile int g_iTriggerPixel = 0;// The number of pixels from the center of the screen the trigger level is on
volatile float g_fFScale;
volatile float g_fTriggerLevel;
const char * const g_ppcVoltageScaleStr[] = {"100 mV", "200 mV", "500 mV", "1 V"};

unsigned int g_uiTimescale = 24;

volatile int g_piSpectrumBuffer[SCREEN_WIDTH];
//volatile float g_piSpectrumBuffer[SCREEN_WIDTH];
//volatile float g_piSpectrumBuffer[NFFT/2];
volatile unsigned char g_ucSpectrumMode = 0;

volatile unsigned char g_ucBPPressedCount = 0;
volatile unsigned char g_ucUiTaskCount = 0;

Void adcSetup(Void);
Void buttonSetup(Void);
unsigned int triggerSearch(float triggerLevel, int direction, int samples);
Void drawTrigger(int direction);



#endif /* MAIN_H_ */
