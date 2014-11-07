/*
 * main.c
 *
 * Authors: Jason Klein and Nicholas Otero
 * Date: 10/29/2014
 * ECE 3829 Lab 0
 */

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

//Defines
#define BUTTON_CLOCK 200 // button scanning interrupt rate in Hz
#define M_PI 3.14159265358979323846f // Mathematical constant pi
#define ADC_BUFFER_SIZE 2048 // must be a power of 2
#define ADC_BUFFER_WRAP(i) ((i) & (ADC_BUFFER_SIZE - 1)) // index wrapping macro
// Globals
unsigned long g_ulSystemClock; // system clock frequency in Hz
volatile unsigned long g_ulTime = 0; // time in hundredths of a second
volatile unsigned char g_clockSelect = 1; // switch between analog and digital clock display
volatile int g_iADCBufferIndex = ADC_BUFFER_SIZE - 1;  // latest sample index
volatile unsigned short g_pusADCBuffer[ADC_BUFFER_SIZE]; // circular buffer
volatile unsigned long g_ulADCErrors = 0; // number of missed ADC deadlines

//Structures
typedef struct {
	short x;
	short y;
} Point;

/**
 * Timer 0 interrupt service routine
 * Adapted from Lab 0 handout by Professor Gene Bogdanov
 */
void TimerISR(void) {
	static int tic = false;
	static int running = true;
	unsigned long presses = g_ulButtons;
	TIMER0_ICR_R = TIMER_ICR_TATOCINT; // clear interrupt flag
	// button debounce
	ButtonDebounce((~GPIO_PORTE_DATA_R & GPIO_PIN_0) << 4 // "up" button
	| (~GPIO_PORTE_DATA_R & GPIO_PIN_1) << 2 // "down" button
	| (~GPIO_PORTE_DATA_R & GPIO_PIN_2) // "left" button
			| (~GPIO_PORTE_DATA_R & GPIO_PIN_3) >> 2 // "right" button
			| (~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1); // "select" button
	presses = ~presses & g_ulButtons; // button press detector

	//Note, we could make this one statement, but it is expanded for readability
	//Determine which buttons are pressed
	if (presses & 1) { // "select" button pressed
		running = !running; // stop the TimerISR() from updating g_ulTime
	}

	if (presses & 2) { // "Right" button pressed
		g_ulTime = 0; // reset clock time to 0
	}

	if (presses & 4) { // "Left" button pressed
		g_clockSelect = !g_clockSelect; // switch clock display
	}

	if (presses & 8) { // "Down" button pressed
		g_ulTime = 0; // reset clock time to 0
	}

	if (presses & 16) { // "Up" button pressed
		g_ulTime = 0; // reset clock time to 0
	}

	if (running) {
		if (tic) {
			g_ulTime++; // increment time every other ISR call
			tic = false;
		} else
			tic = true;
	}
}

void ADC_ISR(void) {
	ADC_ISC_R = ADC_ISC_IN0; // clear ADC sequence0 interrupt flag in the ADCISC register
	if (ADC0_OSTAT_R & ADC_OSTAT_OV0) { // check for ADC FIFO overflow
		g_ulADCErrors++; // count errors - step 1 of the signoff
		ADC0_OSTAT_R = ADC_OSTAT_OV0; // clear overflow condition
	}
	int buffer_index = ADC_BUFFER_WRAP(g_iADCBufferIndex + 1);
	g_pusADCBuffer[buffer_index] = ADC_SSFIFO0_R & ADC_SSFIFO0_DATA_M; // read sample from the ADC sequence0 FIFO
	g_iADCBufferIndex = buffer_index;
}

/**
 * Configures timer to update at 200HZ
 * Obtained from Lab 0 handout by Professor Gene Bogdanov
 */
void timerSetup(void) {
	unsigned long ulDivider, ulPrescaler;
	// initialize a general purpose timer for periodic interrupts
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerDisable(TIMER0_BASE, TIMER_BOTH);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
	// prescaler for a 16-bit timer
	ulPrescaler = (g_ulSystemClock / BUTTON_CLOCK - 1) >> 16;
	// 16-bit divider (timer load value)
	ulDivider = g_ulSystemClock / (BUTTON_CLOCK * (ulPrescaler + 1)) - 1;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ulDivider);
	TimerPrescaleSet(TIMER0_BASE, TIMER_A, ulPrescaler);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER0_BASE, TIMER_A);
	// initialize interrupt controller to respond to timer interrupts
	IntPrioritySet(INT_TIMER0A, 32); // 0 = highest priority, 32 = next lower
	IntEnable(INT_TIMER0A);
}

/**
 * Configures "select", "up", "down", "left", "right" buttons for input
 * Adapted from Lab 0 handout by Professor Gene Bogdanov
 */
void buttonSetup(void) {
	// configure GPIO used to read the state of the on-board push buttons
	// configures select button
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA,
			GPIO_PIN_TYPE_STD_WPU);

	// configure GPIO used to read the state of the on-board push buttons
	// configures "up", "down", "left", "right" buttons
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPadConfigSet(GPIO_PORTE_BASE,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
			GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

void adcSetup(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // enable the ADC
	SysCtlADCSpeedSet(SYSCTL_ADCSPEED_500KSPS); // specify 500ksps
	ADCSequenceDisable(ADC0_BASE, 0); // choose ADC sequence 0; disable before configuring
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_ALWAYS, 0); // specify the "Always" trigger
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0,
			ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0); // in the 0th step, sample channel 0
	// enable interrupt, and make it the end of sequence
	ADCIntEnable(ADC0_BASE, 0); // enable ADC interrupt from sequence 0
	ADCSequenceEnable(ADC0_BASE, 0); // enable the sequence. it is now sampling
	IntPrioritySet(INT_ADC0SS0, 0); // 0 = highest priority
	IntEnable(INT_ADC0SS0); // enable ADC0 interrupts
}

//void adcSetupHex(void) {
//	SYSCTL_RCGC0_R |= SYSCTL_RCGC0_ADC;
////	ADC_SSPRI_R |=
//	ADC_ACTSS_R &= ~ADC_ACTSS_ASEN0;
//	ADC_EMUX_R |= ADC_EMUX_EM0_ALWAYS;
//	ADC_SSMUX0_R |= (ADC_SSMUX0_MUX0_M | ADC_SSMUX0_MUX1_M | ADC_SSMUX0_MUX2_M
//			| ADC_SSMUX0_MUX3_M | ADC_SSMUX0_MUX4_M | ADC_SSMUX0_MUX5_M
//			| ADC_SSMUX0_MUX6_M | ADC_SSMUX0_MUX7_M);
//	ADC_SSCTL0_R
//
//}

/**
 * Computes the cartesian coordinate of the seconds
 * for the analog clock given the radius and angle
 */
Point calcCoord(unsigned short radius, float angle) {
	Point p;
	p.x = (float) radius * cos(angle);
	p.y = (float) radius * sin(angle);
	return p;
}

/**
 * Adapted from Lab 0 handout by Professor Gene Bogdanov
 */
int main(void) {
	char pcStr[50]; // string buffer
	unsigned long ulTime; // local copy of g_ulTime
	unsigned short centiseconds;
	unsigned short seconds;
	unsigned short minutes;
	unsigned short radius = 47;
	short offsetX = 63; // center of display in x direction
	short offsetY = 47; // center of display in y direction
	Point points[60]; // stores coordinates of tick marks

	// pre-calculates coordinates of analog clock tick marks
	// using floating point math so that floating point math
	// is not computed in the main loop
	unsigned short j;
	for (j = 0; j < 60; j++) {
		points[j] = calcCoord(radius - 6,
				M_PI * 2.0f * (float) (j - 15) * 6.0f / 360.0f);
		points[j].x += offsetX; // offsets to center drawing
		points[j].y += offsetY;
	}

	// initialize the clock generator, from TI qs_eklm3s8962
	if (REVISION_IS_A2) {
		SysCtlLDOSet(SYSCTL_LDO_2_75V);
	}

	SysCtlClockSet(
			SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_8MHZ);

	g_ulSystemClock = SysCtlClockGet();

	RIT128x96x4Init(3500000); // initialize the OLED display, from TI qs_eklm3s8962

	buttonSetup(); // configure buttons
	timerSetup(); // configure timer
	adcSetup(); // configure ADC
	IntMasterEnable();

	while (true) {
		FillFrame(0); // clear frame buffer

		ulTime = g_ulTime; // read volatile global only once
		centiseconds = ulTime % 100; // hundredths of a second
		seconds = (ulTime / 100) % 60; // seconds
		minutes = (ulTime / 100) / 60; // minutes

		if (g_clockSelect) {
			usprintf(pcStr, "Time = %02u:%02u:%02u", minutes, seconds,
					centiseconds); // convert time to string
			DrawString(0, 0, pcStr, 15, false); // draw string to frame buffer
		} else {
			DrawCircle(offsetX, offsetY, radius, 15); // draw clock circle
			short i;
			unsigned level = 15;
			for (i = 0; i < 60; i++) {
				level = i % 5 == 0 ? 15 : 10; // make every 5th point darker
				DrawPoint(points[i].x, points[i].y, level); // draw clock tick marks
			}

			DrawLine(offsetX, offsetY, points[seconds].x, points[seconds].y,
					15); // draw seconds hand
			DrawLine(offsetX, offsetY, points[minutes].x, points[minutes].y,
					10); // draw minutes hand
		}

		// copy frame to the OLED screen
		RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);
	}

	return 0;
}

