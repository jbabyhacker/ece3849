/*
 * main.c
 *
 * Authors: Jason Klein and Nicholas Otero
 * Date: 12/5/2014
 * ECE 3829 Lab 2
 */

#include "main.h"

void main() {
	// initialize the clock generator, from TI qs_eklm3s8962
	if (REVISION_IS_A2) {
		SysCtlLDOSet(SYSCTL_LDO_2_75V);
	}
	SysCtlClockSet(
			SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_8MHZ); // set clock

	IntMasterDisable(); // disable interrupts
	buttonSetup(); // setup buttons
	adcSetup(); // setup ADC
	NetworkInit();
	IntMasterEnable(); // enable interrupts

	BIOS_start(); /* enable interrupts and start SYS/BIOS */
}

/**
 * Hwi Function: ADCSampler_Hwi
 * ----------------------------
 *   Stores ADC value in the global circular buffer.
 *   If the ADC FIFO overflows, count as a fault.
 *
 *   Adapted from Lab 1 handout by Professor Gene Bogdanov
 *
 *   returns: nothing
 */
void ADCSampler_Hwi(void) {
	ADC_ISC_R = ADC_ISC_IN0; // clear ADC sequence0 interrupt flag in the ADCISC register
	if (ADC0_OSTAT_R & ADC_OSTAT_OV0) { // check for ADC FIFO overflow
		g_ulADCErrors++; // count errors
		ADC0_OSTAT_R = ADC_OSTAT_OV0; // clear overflow condition
	}

	g_ulAdcTime++;

	int buffer_index = ADC_BUFFER_WRAP(g_iADCBufferIndex + 1);
	while((ADC0_SSFSTAT0_R & ADC_SSFSTAT0_EMPTY) != ADC_SSFSTAT0_EMPTY){
		g_pusADCBuffer[buffer_index] = ADC_SSFIFO0_R & ADC_SSFIFO0_DATA_M; // read sample from the ADC sequence0 FIFO
		buffer_index = ADC_BUFFER_WRAP(++buffer_index);
	}

	g_iADCBufferIndex = buffer_index - 1; // set the new buffer index
}

/**
 * Clock Function: ButtonPoller_Clock
 * ----------------------------
 *   Polls button values and debounces them to determine if
 *   a button is actually pressed.
 *
 *   returns: nothing
 */
void ButtonPoller_Clock(void) {
	unsigned long presses = g_ulButtons;
	g_ulAdcCount = g_ulAdcTime;
	g_ulAdcTime = 0;

	// button debounce
	ButtonDebounce((~GPIO_PORTE_DATA_R & GPIO_PIN_0) << 4 // "up" button
	| (~GPIO_PORTE_DATA_R & GPIO_PIN_1) << 2 // "down" button
	| (~GPIO_PORTE_DATA_R & GPIO_PIN_2) // "left" button
			| (~GPIO_PORTE_DATA_R & GPIO_PIN_3) >> 2 // "right" button
			| (~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1); // "select" button
	presses = ~presses & g_ulButtons; // button press detector

	// presses & 1 == true --> "select" button pressed
	// presses & 2 == true --> "right" button pressed
	// presses & 4 == true --> "left" button pressed
	// presses & 8 == true --> "down" button pressed
	// presses & 16 == true --> "up" button pressed
	if (presses & 0x000000FF) { // if any button is pressed
		Mailbox_post(ButtonMailbox_Handle, &presses, BIOS_NO_WAIT); // post to ButtonMailbox_Handle mailbox
	}
}

/**
 * Task Function: UserInput_Task
 * ----------------------------
 *   Process button data and then change appropriate oscilloscope settings.
 *   Pends on ButtonMailbox_Handle for ButtonPoller_Clock to add items to this Mailbox.
 *
 *   returns: nothing
 */
void UserInput_Task(UArg arg0, UArg arg1) {
	for (;;) {
		unsigned long presses;

		// wait until an item exists in the Mailbox
		Mailbox_pend(ButtonMailbox_Handle, &presses, BIOS_WAIT_FOREVER); // get user input

		switch (presses) {
		case 1: // "select" button pressed
			if (g_ucSelectionIndex == 2) {
				g_iTriggerDirection *= -1; //Change direction of change for trigger
			} else {
				//Switch between spectrum and waveform
//				if (g_ucSpectrumMode == 0) {
//					g_ucSpectrumMode = 1;	//Spectrum
//				} else {
//					g_ucSpectrumMode = 0;	//Waveform
//				}
			}
			break;
		case 2: // "right" button pressed
			g_ucSelectionIndex =
					(g_ucSelectionIndex == 2) ? 2 : ++g_ucSelectionIndex; //Select gui element to the right, if one exists
			break;
		case 4: // "left" button pressed
			g_ucSelectionIndex =
					(g_ucSelectionIndex == 0) ? 0 : --g_ucSelectionIndex; //Select gui element to the left, if one exists
			break;
		case 8: // "down" button pressed
			if (g_ucSelectionIndex == 0) {
				//Adjust Timescale
			} else if (g_ucSelectionIndex == 1) { //Adjust pixel per ADC tick
				if (g_uiMVoltsPerDiv == 1000) {
					g_uiMVoltsPerDiv = 500;
				} else if (g_uiMVoltsPerDiv == 500) {
					g_uiMVoltsPerDiv = 200;
				} else if (g_uiMVoltsPerDiv == 200) {
					g_uiMVoltsPerDiv = 100;
				}
			} else {
				g_iTriggerPixel -= 2;	//Move the trigger line down
			}
			break;
		case 16: // "up" button pressed
			if (g_ucSelectionIndex == 0) {
				//Adjust Timescale
			} else if (g_ucSelectionIndex == 1) {
				//Adjust pixel per ADC tick
				if (g_uiMVoltsPerDiv == 500) {
					g_uiMVoltsPerDiv = 1000;
				} else if (g_uiMVoltsPerDiv == 200) {
					g_uiMVoltsPerDiv = 500;
				} else if (g_uiMVoltsPerDiv == 100) {
					g_uiMVoltsPerDiv = 200;
				}
			} else {
				g_iTriggerPixel += 2;	//Move trigger line up two pixels
			}
			break;
		}
	}
}

/**
 * Task Function: Display_Task
 * ----------------------------
 *   Draws waveform, waveform settings, and FFT to the display.
 *   Pends on Draw_Sem. Upon loop iteration completion, posts to Wave_Sem
 *
 *   returns: nothing
 */
void Display_Task(UArg arg0, UArg arg1) {
	// initialize the OLED display, from TI qs_eklm3s8962
	RIT128x96x4Init(3500000);

	char pcStr[50]; // string buffer

	for (;;) {
		Semaphore_pend(Draw_Sem, BIOS_WAIT_FOREVER);

		FillFrame(0); // clear  OLED frame buffer

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
			// draw data points with lines
			DrawLine(g_ppWaveformBuffer[j - 1].x, g_ppWaveformBuffer[j - 1].y,
					g_ppWaveformBuffer[j].x, g_ppWaveformBuffer[j].y, BRIGHT);
		}

		//Draw selector rectangle
		unsigned char x1, x2, y1 = 0, y2 = 7;
		if (g_ucSelectionIndex == 0) {
			x1 = 0;
			x2 = 30;
		} else if (g_ucSelectionIndex == 1) {
			x1 = 50;
			x2 = 84;
		} else {
			x1 = 100;
			x2 = 120;
			y2 = 9;
		}
		DrawFilledRectangle(x1, y1, x2, y2, 5);

		//Draw timescale
		usprintf(pcStr, "%02uus", g_uiTimescale); // convert timescale to string
		DrawString(5, 0, pcStr, 15, false); // draw string to frame buffer

		//Draw voltage scale
		usprintf(pcStr, "%02umV", g_uiMVoltsPerDiv); // convert voltage scale to string
		DrawString(53, 0, pcStr, 15, false); // draw string to frame buffer

		//Draw frequency to screen
		usprintf(pcStr, "%u Hz", g_ulFrequency);
		DrawString(5, 80, pcStr, 15, false);

		//Draw trigger icon and trigger line
		drawTrigger(g_iTriggerDirection);
		int y = (int) ((FRAME_SIZE_Y / 2) - g_iTriggerPixel);
		DrawLine(0, y, FRAME_SIZE_X - 1, y, 10);

		// copy frame to the OLED screen
		RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);

		Semaphore_post(Wave_Sem);
	}
}

/**
 * Task Function: Waveform_Task
 * ----------------------------
 *   Processes ADC buffer, g_pusADCBuffer, and sets g_ppWaveformBuffer for
 *   Display_Task() and FFT_Task().
 *   Pends on Wave_Sem. Upon loop iteration completion, posts to
 *   Draw_Sem or FFT_Sem if FFT mode is disabled or enabled respectively.
 *
 *   returns: nothing
 */
void Waveform_Task(UArg arg0, UArg arg1) {
	float fScale; // scaling factor to map ADC counts to pixels

	for (;;) {
		// Pend on sempaphore
		Semaphore_pend(Wave_Sem, BIOS_WAIT_FOREVER);

		//Determine scaling factor
		fScale = ((float) (VIN_RANGE * 1000 * PIXELS_PER_DIV))
				/ ((float) ((1 << ADC_BITS) * g_uiMVoltsPerDiv));

		//Find trigger
		float triggerLevel = (3.0 / (1 << ADC_BITS))
				* (g_iTriggerPixel / fScale);
		int triggerIndex = triggerSearch(triggerLevel, g_iTriggerDirection,
				SCREEN_WIDTH);

		//Copy NFFT, 1024 samples, into local buffer
		Point tempBuffer[SCREEN_WIDTH];
		int i;
		for (i = 0; i < SCREEN_WIDTH; i++) {
			Point dataPoint;
			dataPoint.x = i;
			dataPoint.y =
					g_pusADCBuffer[ADC_BUFFER_WRAP(triggerIndex + i - SCREEN_WIDTH/2)];
			tempBuffer[i] = dataPoint;
		}

		int j;
		for (j = 1; j < SCREEN_WIDTH; j++) {
			int adcY0 = 2
					* (tempBuffer[j - 1].y - ((1 << ADC_BITS) / 3.0) * (1.5)); //Scale previous sample
			int adcY1 = 2 * (tempBuffer[j].y - ((1 << ADC_BITS) / 3.0) * (1.5)); //Scale current sample

			g_ppWaveformBuffer[j - 1].y = FRAME_SIZE_Y / 2
					- ((adcY0 - ADC_OFFSET) * fScale);
			g_ppWaveformBuffer[j - 1].x = tempBuffer[j - 1].x;
			g_ppWaveformBuffer[j].y = FRAME_SIZE_Y / 2
					- ((adcY1 - ADC_OFFSET) * fScale);
			g_ppWaveformBuffer[j].x = tempBuffer[j].x;
		}

		Semaphore_post(Draw_Sem);
	}
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
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_ALWAYS, 0); // always trigger
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0); // in the 0th step, sample channel 0
	ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_CH0 | ADC_CTL_IE);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
	ADCIntEnable(ADC0_BASE, 0); // enable ADC interrupt from sequence 0
	ADCSequenceEnable(ADC0_BASE, 0); // enable the sequence. it is now sampling
}

/**
 * Function: buttonSetup
 * ----------------------------
 *   Configures "select", "up", "down", "left", "right" buttons for input
 *   Adapted from Lab 0 handout by Professor Gene Bogdanov
 *
 *   returns: nothing
 */
void buttonSetup(void) {
	// configure GPIO used to read the state of the on-board push buttons
	// configures "up", "down", "left", "right" buttons
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); // enable General Purpose IO Port E
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3); // set GPIO pins 0-3 as input
	GPIOPadConfigSet(GPIO_PORTE_BASE,
			GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
			GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	// configure GPIO used to read the state of the on-board push buttons
	// configures select button
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // enable General Purpose IO Port F
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1); // set GPIO pin 1 as input
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA,
			GPIO_PIN_TYPE_STD_WPU);
}

/**
 * Function: triggerSearch
 * ----------------------------
 *   This function searches for a trigger in the ADC buffer and returns the index of the trigger
 *   when found. If one is not found, this function will return the index half a buffer away from the newest sample
 *
 *   triggerLevel: the floating-point magnitude, in Volts, of the trigger
 *   direction: the direction of change of a trigger, either (int) +1 for rising or -1 for falling
 *   samples: the number of samples to search through
 *
 *   returns: the index of the trigger, or the index half a buffer from the newest sample if a trigger is not found
 */
unsigned int triggerSearch(float triggerLevel, int direction, int samples) {
	unsigned int startIndex = g_iADCBufferIndex - (samples / 2); //start half a screen width from the most recent sample
	unsigned int searchIndex = startIndex;
	unsigned int searched = 0; //This avoids doing math dealing with the buffer wrap for deciding when to give up
	//	unsigned int triggerLevelAdc = (((triggerLevel/2.0)+1.5)/3.0)*(1 << ADC_BITS);
	unsigned int triggerLevelAdc = ((int) (triggerLevel * 170.667)) + 512;

	while (1) {
		//Look for trigger,break loop if found
		unsigned int curCount = g_pusADCBuffer[searchIndex]; //Accessing two members of a shared data structure non-atomically
		unsigned int prevCount = g_pusADCBuffer[searchIndex - 1]; //Shared data safe because ISR writes to different part of buffer
		//		float curVolt = ADC_TO_VOLT(curCount);
		//		float prevVolt = ADC_TO_VOLT(prevCount);

		//Is curVolt a trigger?
		if (((direction == 1) && 					//If looking for rising edge
				(curCount >= triggerLevelAdc) && (prevCount < triggerLevelAdc))	//And the current and previous voltages are above/equal to and below the trigger
				|| ((direction == -1)
						&& 			//Or, if looking for falling edge
						(curCount <= triggerLevelAdc)
						&& (prevCount > triggerLevelAdc))) { //And the current and previous voltages are below/equal to and above the trigger
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
