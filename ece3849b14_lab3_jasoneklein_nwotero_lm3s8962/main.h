/*
 * main.h
 *
 * Authors: Jason Klein and Nicholas Otero
 * Date: 12/5/2014
 * ECE 3829 Lab 2
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

//Includes for Fast Fourier Transform

//Includes for SYS/BIOS
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>
#include <ti/sysbios/BIOS.h>

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
#define BRIGHT 15 // bright pixel level
#define DIM 2 // dim pixel level
#define TRIGGER_X_POS 110 // trigger level symbol, position along x-axis, to center of symbol
#define TRIGGER_Y_POS 5 // trigger level symbol, position along y-axis, to center of symbol
#define TRIGGER_H_LINE 6 // trigger level symbol, length of horizontal lines, measured from center of symbol
#define TRIGGER_V_LINE 4 // trigger level symbol, length of vertical lines, measured from center of symbol
#define TRIGGER_ARROW_WIDTH 4 // trigger level symbol, arrow distance from center horizontally
#define TRIGGER_ARROW_HEIGHT 2 // trigger level symbol, arrow distance from center vertically

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

volatile unsigned long g_ulADCErrors = 0; // number of missed ADC deadlines

volatile Point g_ppWaveformBuffer[SCREEN_WIDTH];
volatile unsigned char g_ucSelectionIndex = 1; // index for selected top-screen gui element
volatile unsigned int g_uiMVoltsPerDiv = 500;	// miliVolts per division of the screen
volatile int g_iTriggerDirection = 1;
volatile int g_iTriggerPixel = 0;// The number of pixels from the center of the screen the trigger level is on
unsigned int g_uiTimescale = 24;
volatile unsigned long g_ulTriggerSearchFail = 0;

volatile unsigned long g_ulAdcTime = 0;
volatile unsigned long g_ulAdcCount = 0;

Void adcSetup(Void);
Void buttonSetup(Void);
unsigned int triggerSearch(float triggerLevel, int direction, int samples);
void drawTrigger(int direction);

#endif /* MAIN_H_ */
