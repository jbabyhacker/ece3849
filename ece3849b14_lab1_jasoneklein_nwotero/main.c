/*
 * main.c
 *
 * Authors: Jason Klein and Nicholas Otero
 * Date: 10/29/2014
 * ECE 3829 Lab 0
 */

#include "main.h"

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

/**
 * ADC Interrupt service routine. Stores ADC value in the global
 * circular buffer.  If the ADC FIFO overflows, count as a fault.
 * Adapted from Lab 1 handout by Professor Gene Bogdanov
 */
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

/**
 * Function: triggerSearch
 * ----------------------------
 *   This function searches for a trigger in the ADC buffer and returns the index of the trigger
 *   when found. If one is not found, this function will return the index half a buffer away from the newest sample
 *
 *   triggerLevel: the magnitude, in ADC counts, of the trigger
 *   direction: the direction of change of a trigger, either +1 for rising or -1 for falling
 *
 *   returns: the index of the trigger, or the index half a buffer from the newest sample if a trigger is not found
 */
unsigned int triggerSearch(int triggerLevel, int direction){
	unsigned int startIndex = g_iADCBufferIndex - (SCREEN_WIDTH / 2); //start half a screen width from the most recent sample
	unsigned int searchIndex = startIndex;
	unsigned int searched = 0; //This avoids doing math dealing with the buffer wrap for deciding when to give up

	while (1) {
		//Look for trigger,break loop if found
		int curCount = g_pusADCBuffer[searchIndex]; //Accessing two members of a shared data structure non-atomically
		int prevCount = g_pusADCBuffer[searchIndex - 1]; //Shared data safe because ISR writes to different part of buffer

		//Is curCount a trigger?
		if ((curCount == triggerLevel) && (direction * (curCount - prevCount) > 0)) {
			return searchIndex;
		}

		//If we have searched half of the buffer, give up. Else, move the search index back
		if (searched == (ADC_BUFFER_SIZE / 2)) {
			return ADC_BUFFER_WRAP(g_iADCBufferIndex - (ADC_BUFFER_SIZE / 2));;
		}
		else {
			searchIndex = ADC_BUFFER_WRAP(--searchIndex);
			searched++;
		}
	}
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
		//points[j] = calcCoord(radius - 6,
			//	M_PI * 2.0f * (float) (j - 15) * 6.0f / 360.0f);
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

