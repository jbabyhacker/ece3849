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
#define DIGITAL_CLOCK // comment this out to enable analog clock
// Globals
unsigned long g_ulSystemClock; // system clock frequency in Hz
volatile unsigned long g_ulTime = 0; // time in hundredths of a second

//Structures
typedef struct {
	short x;
	short y;
} Point;

/**
 * Timer 0 interrupt service routine
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

	if (presses & 4) { // "Down" button pressed
		g_ulTime = 0; // reset clock time to 0
	}

	if (presses & 8) { // "Left" button pressed
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
/**
 * Configures timer to update at 200HZ
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
	IntPrioritySet(INT_TIMER0A, 0); // 0 = highest priority, 32 = next lower
	IntEnable(INT_TIMER0A);
	IntMasterEnable();
}

/**
 * Configures "select", "up", "down", "left", "right" buttons for input
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
				M_PI * 2.0f * (float) j * 6.0f / 360.0f);
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

	while (true) {
		FillFrame(0); // clear frame buffer

		ulTime = g_ulTime; // read volatile global only once
		centiseconds = ulTime % 100; // hundredths of a second
		seconds = (ulTime / 100) % 60; // seconds
		minutes = (ulTime / 100) / 60; // minutes

#ifdef DIGITAL_CLOCK
		usprintf(pcStr, "Time = %02u:%02u:%02u", minutes, seconds,
				centiseconds); // convert time to string
		DrawString(0, 0, pcStr, 15, false); // draw string to frame buffer
#else
		DrawCircle(offsetX, offsetY, radius, 15); // draw clock circle
		short i;
		unsigned level = 15;
		for (i = 0; i < 60; i++) {
			level = i % 5 == 0 ? 15 : 10; // make every 5th point darker
			DrawPoint(points[i].x, points[i].y, level); // draw clock tick marks
		}

		DrawLine(offsetX, offsetY, points[seconds].x, points[seconds].y, 15); // draw seconds hand
		DrawLine(offsetX, offsetY, points[minutes].x, points[minutes].y, 10); // draw minutes hand
#endif
		// copy frame to the OLED screen
		RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);
	}

	return 0;
}

