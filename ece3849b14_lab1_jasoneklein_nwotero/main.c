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
	TIMER3_ICR_R = TIMER_ICR_TATOCINT; // clear interrupt flag
}

void PortE_Button_ISR(void) {
	GPIO_PORTE_ICR_R = GPIO_ICR_GPIO_M; // clear Port E interrupt flag
	if (!g_ucPortEButtonFlag) {
		g_ucPortEButtonFlag = 1;
	}
}

void PortF_Button_ISR(void) {
	GPIO_PORTF_ICR_R = GPIO_ICR_GPIO_M; // clear Port F interrupt flag
	if (!g_ucPortFButtonFlag) {
		g_ucPortFButtonFlag = 1;
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
//	g_ulAdcSamples++;
}

void TIMER_0_ISR(void) {
	TIMER0_ICR_R = TIMER_ICR_TATOCINT;
//	g_ulAdcSampleRate = g_ulAdcSamples;
//	g_ulAdcSamples = 0;
	unsigned long presses = g_ulButtons;

	if (g_ucPortEButtonFlag || g_ucPortFButtonFlag) {
		// button debounce
		ButtonDebounce((~GPIO_PORTE_DATA_R & GPIO_PIN_0) << 4 // "up" button
		| (~GPIO_PORTE_DATA_R & GPIO_PIN_1) << 2 // "down" button
		| (~GPIO_PORTE_DATA_R & GPIO_PIN_2) // "left" button
				| (~GPIO_PORTE_DATA_R & GPIO_PIN_3) >> 2 // "right" button
				| (~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1); // "select" button
		presses = ~presses & g_ulButtons; // button press detector
//		fifo_put(presses);

		//Note, we could make this one statement, but it is expanded for readability
		//Determine which buttons are pressed
		if (presses & 1) { // "select" button pressed
			//			running = !running; // stop the TimerISR() from updating g_ulTime
			fifo_put(1);
			g_ucPortFButtonFlag = 0;
		}

		if (presses & 2) { // "Right" button pressed
			fifo_put(2);
			g_ucPortEButtonFlag = 0;
		}

		if (presses & 4) { // "Left" button pressed
			fifo_put(3);
			g_ucPortEButtonFlag = 0;
		}

		if (presses & 8) { // "Down" button pressed
			fifo_put(4);
			g_ucPortEButtonFlag = 0;
		}

		if (presses & 16) { // "Up" button pressed
			fifo_put(5);
			g_ucPortEButtonFlag = 0;
		}
	}
}

void setupSampleTimer(unsigned long timeScale) {
	// configure timer 0
	unsigned long ulDivider, ulPrescaler;
	unsigned long desiredFreq = ((12 * 1000) / timeScale) * 1000;
	// initialize a general purpose timer for periodic interrupts
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	TimerIntDisable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	TimerDisable(TIMER1_BASE, TIMER_BOTH);
	TimerConfigure(TIMER1_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
	// prescaler for a 16-bit timer
	ulPrescaler = (g_ulSystemClock / desiredFreq - 1) >> 16;
	// 16-bit divider (timer load value)
	ulDivider = g_ulSystemClock / (desiredFreq * (ulPrescaler + 1)) - 1;
	TimerLoadSet(TIMER1_BASE, TIMER_A, ulDivider);
//	TimerLoadSet(TIMER1_BASE, TIMER_A, 100);
	TimerPrescaleSet(TIMER1_BASE, TIMER_A, ulPrescaler);
	TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	TimerControlTrigger(TIMER1_BASE, TIMER_A, true);

	TimerEnable(TIMER1_BASE, TIMER_A);
	// initialize interrupt controller to respond to timer interrupts
	//IntPrioritySet(INT_TIMER1A, 0); // 0 = highest priority, 32 = next lower
	//IntEnable(INT_TIMER1A);
}

/**
 * Function: timerSetup
 * ----------------------------
 *   This function sets up the timer for polling and debouncing buttons as well as the
 *   timer used to measure CPU load
 *
 *   returns: nothing
 */
void timerSetup(void) {
	IntMasterDisable();

	//Initialize timer for CPU load measurement
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);	// initialize timer 3 in one-shot mode for polled timing
	TimerDisable(TIMER3_BASE, TIMER_BOTH);			//Disables the timer
	TimerConfigure(TIMER3_BASE, TIMER_CFG_ONE_SHOT);	//Configure timer for one shot mode
	TimerLoadSet(TIMER3_BASE, TIMER_A, g_ulSystemClock / 50 - 1); // 1 sec interval


	// configure timer for buttons
	unsigned long ulDivider, ulPrescaler;
	// initialize a general purpose timer for periodic interrupts
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerDisable(TIMER0_BASE, TIMER_BOTH);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC); //Periodic timer

	ulPrescaler = (g_ulSystemClock / BUTTON_CLOCK - 1) >> 16; // prescaler for a 16-bit timer
	ulDivider = g_ulSystemClock / (BUTTON_CLOCK * (ulPrescaler + 1)) - 1;// 16-bit divider (timer load value)
	TimerLoadSet(TIMER0_BASE, TIMER_A, ulDivider);			//Set starting count of timer
	TimerPrescaleSet(TIMER0_BASE, TIMER_A, ulPrescaler);	//Prescale the timer
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);		//enable the timer's interrupts
	TimerEnable(TIMER0_BASE, TIMER_A);						//enable timer peripheral interrupts
	// initialize interrupt controller to respond to timer interrupts
	IntPrioritySet(INT_TIMER0A, 32); // 0 = highest priority, 32 = next lower
	IntEnable(INT_TIMER0A);
}

/**
 * Function: cpu_load_count
 * ----------------------------
 *   This function counts how many times a while loop can iterate in the time it takes for a
 *   peripheral timer to count down. This measures CPU load.
 *
 *   returns: nothing
 */
unsigned long cpu_load_count(void) {
	unsigned long i = 0;
	TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);	//Clear the timers count
	TimerEnable(TIMER3_BASE, TIMER_A); 				// start one-shot timer
	//Count as much as possible in the amount of time it takes
	//for the counter to count down
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

	IntPrioritySet(INT_GPIOE, 64); // set Port E priority: 0 = highest priority, 32 = next lower
	IntPrioritySet(INT_GPIOF, 64); // set Port F priority: 0 = highest priority, 32 = next lower
	GPIOPinIntEnable(GPIO_PORTE_BASE,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3); // enable interrupts on Port E
	GPIOPinIntEnable(GPIO_PORTF_BASE, GPIO_PIN_1); // enable interrupts on Port F

	// initialize button FIFO
//	unsigned char success = create_fifo(BUTTON_BUFFER_SIZE);
//	if (!success) {
//		exit(0);
//	}
}

/**
 * Function: adcSetup
 * ----------------------------
 *   This function configures the ADC 0 peripheral to convert samples at a speed of 500ksps using the
 *   sequencer 0 module.  Samples are triggered by a general purpose timer and interrupts are generated
 *   by the ADC for each sample taken.  These interrupts are of the highest priority
 *
 *   returns: nothing
 */
void adcSetup(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // enable the ADC
	SysCtlADCSpeedSet(SYSCTL_ADCSPEED_500KSPS); // specify 500ksps
	ADCSequenceDisable(ADC0_BASE, 0); // choose ADC sequence 0; disable before configuring
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 0); // specify the trigger for Timer
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0,
			ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0); // in the 0th step, sample channel 0
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
//	unsigned int triggerLevelAdc = (((triggerLevel/2.0)+1.5)/3.0)*(1 << ADC_BITS);
	unsigned int triggerLevelAdc = ((int)(triggerLevel*170.667)) + 512;

	while (1) {
		//Look for trigger,break loop if found
		unsigned int curCount = g_pusADCBuffer[searchIndex]; //Accessing two members of a shared data structure non-atomically
		unsigned int prevCount = g_pusADCBuffer[searchIndex - 1]; //Shared data safe because ISR writes to different part of buffer
//		float curVolt = ADC_TO_VOLT(curCount);
//		float prevVolt = ADC_TO_VOLT(prevCount);

		//Is curVolt a trigger?
		if (((direction == 1) && 					//If looking for rising edge
				(curCount >= triggerLevelAdc) && (prevCount < triggerLevelAdc))	//And the current and previous voltages are above/equal to and below the trigger
		|| ((direction == -1) && 			//Or, if looking for falling edge
				(curCount <= triggerLevelAdc) && (prevCount > triggerLevelAdc))) { //And the current and previous voltages are below/equal to and above the trigger
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
 * Function: drawTrigger
 * ----------------------------
 *   This function draws the trigger icon which depicts the direction of change being used for the
 *   triggerSearch function
 *
 *   direction: the direction of change of a trigger, either (int) +1 for rising or -1 for falling
 *
 *   returns: nothing
 */
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

/**
 * Adapted from Lab 0 handout by Professor Gene Bogdanov
 */
int main(void) {
	char pcStr[50]; 						// string buffer
	float cpu_load;							// percentage of CPU loaded
	unsigned long count_unloaded;			// counts of iterable before interrupts are enabled
	unsigned long count_loaded;				// counts of iterable after interrupts are enabled
	unsigned char selectionIndex = 1;		// index for selected top-screen gui element
	unsigned int mvoltsPerDiv = 500;		// miliVolts per division of the screen
	int triggerPixel = 0;					// The number of pixels from the center of the screen the trigger level is on
	float fScale;							// scaling factor to map ADC counts to pixels

	// initialize the clock generator, from TI qs_eklm3s8962
	if (REVISION_IS_A2) {
		SysCtlLDOSet(SYSCTL_LDO_2_75V);
	}
	SysCtlClockSet(
			SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_8MHZ);

	g_ulSystemClock = SysCtlClockGet();
	g_uiTimescale = 24;

	// initialize the OLED display, from TI qs_eklm3s8962
	RIT128x96x4Init(3500000);

	adcSetup(); 						// configure ADC
	timerSetup(); 						// configure timer
	setupSampleTimer(g_uiTimescale); 	// configure adjustable timer which triggers ADC samples
	count_unloaded = cpu_load_count();	// measure the number of iterations a non-loaded system can complete
	buttonSetup(); 						// configure buttons
	IntMasterEnable();					// Enable interrupt controller

	// Main program loop
	while (true) {
		FillFrame(0); // clear  OLED frame buffer

		//Process button input
		char buttonPressed = 0;
		unsigned char success = fifo_get(&buttonPressed); 	//Read the pushed button
		if (success) {										//If a button was pressed, decide which
			switch (buttonPressed) {
			case 1: // "select" button
				g_iTriggerDirection *= -1;	//Change direction of change for trigger
				break;
			case 2: // "right" button
				selectionIndex = (selectionIndex == 2) ? 2 : ++selectionIndex; //Select gui element to the right, if one exists
				break;
			case 3: // "left" button
				selectionIndex = (selectionIndex == 0) ? 0 : --selectionIndex; //Select gui element to the left, if one exists
				break;
			case 4: // "down" button
				if (selectionIndex == 0) {
					//Adjust Timescale
					g_uiTimescale =
							(g_uiTimescale == 24) ? 24 : (g_uiTimescale - 2);
					setupSampleTimer(g_uiTimescale); 							//Reset sampling timer
				} else if (selectionIndex == 1) { //Adjust pixel per ADC tick
					if (mvoltsPerDiv == 1000) {
						mvoltsPerDiv = 500;
					} else if (mvoltsPerDiv == 500) {
						mvoltsPerDiv = 200;
					} else if (mvoltsPerDiv == 200) {
						mvoltsPerDiv = 100;
					}
				} else {
					triggerPixel -= 2;	//Move the trigger line down
				}
				break;
			case 5: // "up" button
				if (selectionIndex == 0) {
					//Adjust Timescale
					g_uiTimescale =
							(g_uiTimescale == 1000) ?
									1000 : (g_uiTimescale + 2);
					adcSetup();									//Reset ADC
					setupSampleTimer(g_uiTimescale);			//Reset sample timer
				} else if (selectionIndex == 1) {
					//Adjust pixel per ADC tick
					if (mvoltsPerDiv == 500) {
						mvoltsPerDiv = 1000;
					} else if (mvoltsPerDiv == 200) {
						mvoltsPerDiv = 500;
					} else if (mvoltsPerDiv == 100) {
						mvoltsPerDiv = 200;
					}
				} else {
					triggerPixel += 2;	//Move trigger line up two pixels
				}
				break;
			}
		}

		//Determine scaling factor
		fScale = ((float) (VIN_RANGE * 1000 * PIXELS_PER_DIV))
				/ ((float) ((1 << ADC_BITS) * mvoltsPerDiv));

		//Find trigger
		float triggerLevel = (3.0 / (1 << ADC_BITS))
				* (triggerPixel / fScale);
		int triggerIndex = triggerSearch(triggerLevel, g_iTriggerDirection);

		//Copy, convert a screen's worth of data into a local buffer of points
		Point localADCBuffer[SCREEN_WIDTH];
		int i;
		for (i = 0; i < SCREEN_WIDTH; i++) {
			Point dataPoint;
			dataPoint.x = i;
			dataPoint.y = g_pusADCBuffer[ADC_BUFFER_WRAP(triggerIndex + i - SCREEN_WIDTH/2)];
			localADCBuffer[i] = dataPoint;
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

		//Draw points using the cached data
		int j;
		for (j = 1; j < SCREEN_WIDTH; j++) {
			int adcY0 = 2 * (localADCBuffer[j - 1].y - ((1 << ADC_BITS) / 3.0) * (1.5)); //Scale previous sample
			int adcY1 = 2 * (localADCBuffer[j].y - ((1 << ADC_BITS) / 3.0) * (1.5));		//Scale current sample

			int y0 = FRAME_SIZE_Y / 2 - ((adcY0 - ADC_OFFSET) * fScale);
			int y1 = FRAME_SIZE_Y / 2 - ((adcY1 - ADC_OFFSET) * fScale);

			// draw data points with lines
			DrawLine(localADCBuffer[j - 1].x, y0, localADCBuffer[j].x, y1,
					BRIGHT);
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

		//In order to have the decimal value drawn to the screen, the whole and fractional parts
		//need to be extracted from the floating point number
		unsigned int whole = (int) (cpu_load * 100);
		unsigned int frac = (int) (cpu_load * 1000 - whole * 10);

		usprintf(pcStr, "CPU Load: %02u.%01u\%\%", whole, frac); // convert CPU load to string
		DrawString(0, 86, pcStr, 15, false); // draw string to frame buffer

		//Draw timescale
		usprintf(pcStr, "%02uus", g_uiTimescale); // convert timescale to string
		DrawString(5, 0, pcStr, 15, false); // draw string to frame buffer

		//Draw voltage scale
		usprintf(pcStr, "%02umV", mvoltsPerDiv); // convert voltage scale to string
		DrawString(53, 0, pcStr, 15, false); // draw string to frame buffer

		//Draw trigger icon and trigger line
		drawTrigger(g_iTriggerDirection);
		int voltToAdc = VOLT_TO_ADC(triggerLevel);
		float voltToAdcFscale = voltToAdc * fScale;
		int y = (int) ((FRAME_SIZE_Y / 2) - triggerPixel);
		DrawLine(0, y, FRAME_SIZE_X - 1, y, 10);

		// copy frame to the OLED screen
		RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);

	}

	return 0;
}

