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

void PortE_Button_ISR(void) {
	GPIO_PORTE_ICR_R = GPIO_ICR_GPIO_M; // clear Port E interrupt flag

	unsigned long presses = g_ulButtons;

	ButtonDebounce((~GPIO_PORTE_DATA_R & GPIO_PIN_0) << 4 // "up" button
	| (~GPIO_PORTE_DATA_R & GPIO_PIN_1) << 2 // "down" button
	| (~GPIO_PORTE_DATA_R & GPIO_PIN_2) // "left" button
			| (~GPIO_PORTE_DATA_R & GPIO_PIN_3) >> 2); // "right" button
//	presses = ~presses & g_ulButtons; // button press detector

	unsigned char buffer_index = ADC_BUFFER_WRAP(g_cPortEBufferIndex + 1);
	g_pucPortEButtonBuffer[buffer_index] = ~presses & g_ulButtons;
	g_cPortEBufferIndex = buffer_index;

//	if (presses & 2) { // "Right" button pressed
//		g_ulTime = 0; // reset clock time to 0
//	}
//
//	if (presses & 4) { // "Left" button pressed
//		g_clockSelect = !g_clockSelect; // switch clock display
//	}
//
//	if (presses & 8) { // "Down" button pressed
//		g_ulTime = 0; // reset clock time to 0
//	}
//
//	if (presses & 16) { // "Up" button pressed
//		g_ulTime = 0; // reset clock time to 0
//	}
}

void PortF_Button_ISR(void) {
	GPIO_PORTF_ICR_R = GPIO_ICR_GPIO_M; // clear Port F interrupt flag

	unsigned long presses = g_ulButtons;
	ButtonDebounce((~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1); // "select" button
	presses = ~presses & g_ulButtons;

	unsigned char buffer_index = ADC_BUFFER_WRAP(g_cPortEBufferIndex + 1);
	g_pucPortFButtonBuffer[buffer_index] = ~presses & g_ulButtons;
	g_cPortFBufferIndex = buffer_index;
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
	// configures "up", "down", "left", "right" buttons
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPadConfigSet(GPIO_PORTE_BASE,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
			GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	// configure GPIO used to read the state of the on-board push buttons
	// configures select button
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA,
			GPIO_PIN_TYPE_STD_WPU);

	// configure GPIO to trigger interrupts on "up", "down", "left", "right" buttons
	GPIOPortIntRegister(GPIO_PORTE_BASE, PortE_Button_ISR); // set interrupt handler
	GPIOIntTypeSet(GPIO_PORTE_BASE,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
			GPIO_FALLING_EDGE); // trigger on "up", "down", "left", "right" buttons on falling edge

	// configure GPIO to trigger interrupts on "select" button
	GPIOPortIntRegister(GPIO_PORTF_BASE, PortF_Button_ISR);
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_FALLING_EDGE); // trigger "select" on falling edge

	IntPrioritySet(INT_GPIOE, 32); // set Port E priority: 0 = highest priority, 32 = next lower
	IntPrioritySet(INT_GPIOF, 32); // set Port F priority: 0 = highest priority, 32 = next lower
	GPIOPinIntEnable(GPIO_PORTE_BASE,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3); // enable interrupts on Port E
	GPIOPinIntEnable(GPIO_PORTF_BASE, GPIO_PIN_1); // enable interrupts on Port F
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
 *   triggerLevel: the floating-point magnitude, in Volts, of the trigger
 *   direction: the direction of change of a trigger, either (int) +1 for rising or -1 for falling
 *
 *   returns: the index of the trigger, or the index half a buffer from the newest sample if a trigger is not found
 */
unsigned int triggerSearch(float triggerLevel, int direction) {
	unsigned int startIndex = g_iADCBufferIndex - (SCREEN_WIDTH / 2); //start half a screen width from the most recent sample
	unsigned int searchIndex = startIndex;
	unsigned int searched = 0; //This avoids doing math dealing with the buffer wrap for deciding when to give up

	while (1) {
		//Look for trigger,break loop if found
		unsigned int curCount = g_pusADCBuffer[searchIndex];
		unsigned int prevCount = g_pusADCBuffer[searchIndex - 1];
		float curVolt = ADC_TO_VOLT(curCount); //Accessing two members of a shared data structure non-atomically
		float prevVolt = ADC_TO_VOLT(prevCount); //Shared data safe because ISR writes to different part of buffer
		float epsilon = 0.001;

		//Is curVolt a trigger?
		if (((direction == 1) && 					//If looking for rising edge
				(curVolt >= triggerLevel) && (prevVolt < triggerLevel))	//And the current and previous voltages are above/equal to and below the trigger
		|| ((direction == -1) && 			//Or, if looking for falling edge
				(curVolt <= triggerLevel) && (prevVolt > triggerLevel))) { //And the current and previous voltages are below/equal to and above the trigger
			return searchIndex;
		}

		//If we have searched half of the buffer, give up. Else, move the search index back
		if (searched == (ADC_BUFFER_SIZE / 2)) {
			g_ulTriggerSearchFail++;
			return ADC_BUFFER_WRAP(g_iADCBufferIndex - (ADC_BUFFER_SIZE / 2));;
		} else {
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

	// initialize the clock generator, from TI qs_eklm3s8962
	if (REVISION_IS_A2) {
		SysCtlLDOSet(SYSCTL_LDO_2_75V);
	}

	SysCtlClockSet(
			SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_8MHZ);

	g_ulSystemClock = SysCtlClockGet();

	RIT128x96x4Init(3500000); // initialize the OLED display, from TI qs_eklm3s8962

	adcSetup(); // configure ADC
//	timerSetup(); // configure timer
	buttonSetup(); // configure buttons
	IntMasterEnable();

	while (true) {
		FillFrame(0); // clear frame buffer

		//Find trigger
		int triggerIndex = triggerSearch(1.5, 1);

		//Copy, convert a screen's worth of data into a local buffer of points
		Point localADCBuffer[SCREEN_WIDTH];
		float fVoltsPerDiv = 1;
		float fScale = (VIN_RANGE * PIXELS_PER_DIV)
				/ ((1 << ADC_BITS) * fVoltsPerDiv);
		int i;
		for (i = 0; i < SCREEN_WIDTH; i++) {
			Point dataPoint;
			dataPoint.x = i;
			dataPoint.y = g_pusADCBuffer[ADC_BUFFER_WRAP(triggerIndex + i)];
			localADCBuffer[i] = dataPoint;
		}

		//Draw points using the cached data
		unsigned short level = 15;
		int j;
		for (j = 1; j < SCREEN_WIDTH; j++) {
			int y0 = FRAME_SIZE_Y / 2
					- (int) round(
							(((int) localADCBuffer[j - 1].y) - ADC_OFFSET)
									* fScale);
			int y1 = FRAME_SIZE_Y / 2
					- (int) round(
							(((int) localADCBuffer[j].y) - ADC_OFFSET)
									* fScale);

			DrawLine(localADCBuffer[j - 1].x, y0, localADCBuffer[j].x, y1,
					level); // draw data points with lines
		}

		// Decorate screen grids
		unsigned short k;
		for (k = 0; k < FRAME_SIZE_X; k += PIXELS_PER_DIV) {
			unsigned short x = k + 5;
			unsigned short y = k;
			DrawLine(x, 0, x, FRAME_SIZE_Y, 2); // draw vertical gridlines
			if (y < 96) {
				DrawLine(0, y, FRAME_SIZE_X, y, 2); // draw horizontal gridlines
			}
		}

		// copy frame to the OLED screen
		RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);
	}

	return 0;
}

