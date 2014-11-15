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
	if (!g_ucPortEButtonFlag) {
		g_ucPortEButtonFlag = 1;
	}
//	unsigned long presses = g_ulButtons;
//
//	ButtonDebounce((~GPIO_PORTE_DATA_R & GPIO_PIN_0) << 4 // "up" button
//	| (~GPIO_PORTE_DATA_R & GPIO_PIN_1) << 2 // "down" button
//	| (~GPIO_PORTE_DATA_R & GPIO_PIN_2) // "left" button
//			| (~GPIO_PORTE_DATA_R & GPIO_PIN_3) >> 2 // "right" button
//			| (~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1); // "select" button
//	fifo_put(~presses & g_ulButtons);
}

void PortF_Button_ISR(void) {
	GPIO_PORTF_ICR_R = GPIO_ICR_GPIO_M; // clear Port F interrupt flag
	if (!g_ucPortFButtonFlag) {
		g_ucPortFButtonFlag = 1;
	}
//	unsigned long presses = g_ulButtons;
//	ButtonDebounce((~GPIO_PORTE_DATA_R & GPIO_PIN_0) << 4 // "up" button
//	| (~GPIO_PORTE_DATA_R & GPIO_PIN_1) << 2 // "down" button
//	| (~GPIO_PORTE_DATA_R & GPIO_PIN_2) // "left" button
//			| (~GPIO_PORTE_DATA_R & GPIO_PIN_3) >> 2 // "right" button
//			| (~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1); // "select" button
//	fifo_put(~presses & g_ulButtons);
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

void timerSetup(void) {
	IntMasterDisable();
	// initialize timer 3 in one-shot mode for polled timing
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
	//TIMER3_CTL_R &= TIMER_CTL_TAOTE; // Enable timer3_A ADC trigger
	TimerDisable(TIMER3_BASE, TIMER_BOTH);
	TimerConfigure(TIMER3_BASE, TIMER_CFG_ONE_SHOT);
	TimerLoadSet(TIMER3_BASE, TIMER_A, g_ulSystemClock / 50 - 1); // 1 sec interval

	//IntMasterEnable();
}

unsigned long cpu_load_count(void) {
	unsigned long i = 0;
	TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER3_BASE, TIMER_A); // start one-shot timer
	while (!(TimerIntStatus(TIMER3_BASE, 0) & TIMER_TIMA_TIMEOUT))
		i++;
	return i;
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

	// initialize button FIFO
//	unsigned char success = create_fifo(BUTTON_BUFFER_SIZE);
//	if (!success) {
//		exit(0);
//	}
}

void adcSetup(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // enable the ADC
	SysCtlADCSpeedSet(SYSCTL_ADCSPEED_500KSPS); // specify 500ksps
	//ADC0_EMUX_R &= ADC_EMUX_EM3_TIMER; // ADC event on Timer3
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

void adjustableAdcSetup() {

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
		unsigned int curCount = g_pusADCBuffer[searchIndex];		//Accessing two members of a shared data structure non-atomically
		unsigned int prevCount = g_pusADCBuffer[searchIndex - 1];	//Shared data safe because ISR writes to different part of buffer
		float curVolt = ADC_TO_VOLT(curCount);
		float prevVolt = ADC_TO_VOLT(prevCount);

		//Is curVolt a trigger?
		if (((direction == 1) && 					//If looking for rising edge
				(curVolt >= triggerLevel) &&
				(prevVolt < triggerLevel))	//And the current and previous voltages are above/equal to and below the trigger
		|| ((direction == -1) && 			//Or, if looking for falling edge
				(curVolt <= triggerLevel) &&
				(prevVolt > triggerLevel))) { //And the current and previous voltages are below/equal to and above the trigger
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

void drawTrigger(int direction) {
	DrawLine(TRIGGER_X_POS - (direction * TRIGGER_H_LINE),
			TRIGGER_Y_POS + TRIGGER_V_LINE, TRIGGER_X_POS,
			TRIGGER_Y_POS + TRIGGER_V_LINE, BRIGHT); // draw bottom horizontal line
	DrawLine(TRIGGER_X_POS, TRIGGER_Y_POS + TRIGGER_V_LINE, TRIGGER_X_POS,
			TRIGGER_Y_POS - TRIGGER_V_LINE, BRIGHT); // draw vertical line
	DrawLine(TRIGGER_X_POS, TRIGGER_Y_POS - TRIGGER_V_LINE,
			TRIGGER_X_POS + (direction * TRIGGER_H_LINE),
			TRIGGER_Y_POS - TRIGGER_V_LINE, BRIGHT); // draw top horizontal line
	DrawLine(TRIGGER_X_POS - TRIGGER_ARROW_WIDTH,
			TRIGGER_Y_POS + (direction * TRIGGER_ARROW_HEIGHT), TRIGGER_X_POS,
			TRIGGER_Y_POS, BRIGHT); // draw left side of arrow
	DrawLine(TRIGGER_X_POS + TRIGGER_ARROW_WIDTH,
			TRIGGER_Y_POS + (direction * TRIGGER_ARROW_HEIGHT), TRIGGER_X_POS,
			TRIGGER_Y_POS, BRIGHT); // draw right side of arrow
}

float fScale;
/**
 * Adapted from Lab 0 handout by Professor Gene Bogdanov
 */
int main(void) {
	char pcStr[50]; // string buffer
	float cpu_load;
	unsigned long count_unloaded;
	unsigned long count_loaded;
	unsigned char selectionIndex = 1;
	unsigned int mvoltsPerDiv = 500;

	float triggerLevel = 1.5;

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
	timerSetup(); // configure timer
	count_unloaded = cpu_load_count();
	buttonSetup(); // configure buttons
	IntMasterEnable();

	while (true) {
		FillFrame(0); // clear frame buffer

		//Process button input
		char buttonPressed;
		unsigned char success = fifo_get(&buttonPressed);
		if (success) {
			switch (buttonPressed) {
			case 1: // "select" button
				g_ucTriggerDirection *= -1;
				break;
			case 2: // "right" button
				selectionIndex = (selectionIndex == 2) ? 2 : ++selectionIndex;
				break;
			case 4: // "left" button
				selectionIndex = (selectionIndex == 0) ? 0 : --selectionIndex;
				break;
			case 8: // "down" button
				if (selectionIndex == 0) {
					//Adjust Timescale
				} else if (selectionIndex == 1) { //Adjust pixel per ADC tick
					if (mvoltsPerDiv == 1000) {
						mvoltsPerDiv = 500;
					} else if (mvoltsPerDiv == 500) {
						mvoltsPerDiv = 200;
					} else if (mvoltsPerDiv == 200) {
						mvoltsPerDiv = 100;
					}
				} else {
					triggerLevel -= 0.5 * mvoltsPerDiv;
				}
				break;
			case 16: // "up" button
				if (selectionIndex == 0) {
					//Adjust Timescale
				} else if (selectionIndex == 1) { //Adjust pixel per ADC tick
					if (mvoltsPerDiv == 500) {
						mvoltsPerDiv = 1000;
					} else if (mvoltsPerDiv == 200) {
						mvoltsPerDiv = 500;
					} else if (mvoltsPerDiv == 100) {
						mvoltsPerDiv = 200;
					}
				} else {
					triggerLevel += 0.5 * mvoltsPerDiv;
				}
				break;
			}
		}
		fScale = ((float) (VIN_RANGE * 1000 * PIXELS_PER_DIV))
				/ ((float) ((1 << ADC_BITS) * mvoltsPerDiv));

		//Find trigger
		int triggerIndex = triggerSearch(triggerLevel, 1);

		//Copy, convert a screen's worth of data into a local buffer of points
		Point localADCBuffer[SCREEN_WIDTH];
		int i;
		for (i = 0; i < SCREEN_WIDTH; i++) {
			Point dataPoint;
			dataPoint.x = i;
			dataPoint.y = g_pusADCBuffer[ADC_BUFFER_WRAP(triggerIndex + i)];
			localADCBuffer[i] = dataPoint;
		}

		//Draw points using the cached data
		int j;
		for (j = 1; j < SCREEN_WIDTH; j++) {
			int y0 = FRAME_SIZE_Y / 2
					- ((localADCBuffer[j - 1].y - ADC_OFFSET) * fScale);
			int y1 = FRAME_SIZE_Y / 2
					- ((localADCBuffer[j].y - ADC_OFFSET) * fScale);

			DrawLine(localADCBuffer[j - 1].x, y0, localADCBuffer[j].x, y1,
					BRIGHT); // draw data points with lines
		}

		// Decorate screen grids
		unsigned short k;
		for (k = 0; k < FRAME_SIZE_X; k += PIXELS_PER_DIV) {
			unsigned short x = k + 5;
			unsigned short y = k;
			DrawLine(x, 0, x, FRAME_SIZE_Y, DIM); // draw vertical gridlines
			if (y < 96) {
				DrawLine(0, y, FRAME_SIZE_X, y, DIM); // draw horizontal gridlines
			}
		}

		//Draw selector rectangle
		unsigned char x1, x2, y1 = 0, y2 = 7;
		if (selectionIndex == 0) {
			x1 = 0;
			x2 = 30;
		} else if (selectionIndex == 1) {
			x1 = 50;
			x2 = 84;
		} else {
			x1 = 100;
			x2 = 120;
			y2 = 9;
		}
		DrawFilledRectangle(x1, y1, x2, y2, 5);

		//Measure CPU Load
		count_loaded = cpu_load_count();
		cpu_load = 1.0 - (float) count_loaded / count_unloaded; // compute CPU load

		unsigned int whole = (int) (cpu_load * 100);
		unsigned int frac = (int) (cpu_load * 1000 - whole * 10);

		usprintf(pcStr, "CPU Load: %02u.%01u\%\%", whole, frac); // convert CPU load to string
		DrawString(0, 86, pcStr, 15, false); // draw string to frame buffer

		//Draw timescale
		usprintf(pcStr, "%02uus", g_ucTimescale); // convert timescale to string
		DrawString(5, 0, pcStr, 15, false); // draw string to frame buffer

		//Draw voltage scale
		usprintf(pcStr, "%02umV", mvoltsPerDiv); // convert voltage scale to string
		DrawString(53, 0, pcStr, 15, false); // draw string to frame buffer

		drawTrigger(g_ucTriggerDirection);

		int voltToAdc = VOLT_TO_ADC(triggerLevel);
		float voltToAdcFscale = voltToAdc * fScale;
		int y = (int)((FRAME_SIZE_Y / 2) - voltToAdcFscale);

		DrawLine(0, y, FRAME_SIZE_X - 1, y, 10);

		// copy frame to the OLED screen
		RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);

	}

	return 0;
}

