/*
 *  ======== main.c ========
 */

#include <xdc/std.h>

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

#include <ti/sysbios/BIOS.h>

#include "main.h"

/*
 *  ======== taskFxn ========
 */
//void taskFxn(UArg a0, UArg a1)
//{
//    System_printf("enter taskFxn()\n");
//
//    Task_sleep(10);
//
//    System_printf("exit taskFxn()\n");
//}
/*
 *  ======== main ========
 */void main() {
//	char pcStr[50]; 						// string buffer
//	float cpu_load;							// percentage of CPU loaded


//	unsigned char selectionIndex = 1;// index for selected top-screen gui element
//	unsigned int mvoltsPerDiv = 500;	// miliVolts per division of the screen
//	int triggerPixel = 0;// The number of pixels from the center of the screen the trigger level is on
//	float fScale;				// scaling factor to map ADC counts to pixels

// initialize the clock generator, from TI qs_eklm3s8962
	if (REVISION_IS_A2) {
		SysCtlLDOSet(SYSCTL_LDO_2_75V);
	}
	SysCtlClockSet(
			SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_8MHZ);
//
//	g_ulSystemClock = SysCtlClockGet();
//	g_uiTimescale = 24;

//	g_ulCount_unloaded = cpu_load_count();

	IntMasterDisable();
	buttonSetup();
	adcSetup();
	IntMasterEnable();

	BIOS_start(); /* enable interrupts and start SYS/BIOS */
}

/**
 * ADC Interrupt service routine. Stores ADC value in the global
 * circular buffer.  If the ADC FIFO overflows, count as a fault.
 * Adapted from Lab 1 handout by Professor Gene Bogdanov
 */
void ADCSampler_Hwi(void) {
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
 * Timer 0 Interrupt service routine. Polls button flags and
 * then debounces values read.
 */
void ButtonPoller_Clock(void) {
// 	TIMER0_ICR_R = TIMER_ICR_TATOCINT; // clear interrupt
	unsigned long presses = g_ulButtons;

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
	if (presses & 0x000000FF) {
		//TODO: Add "presses" to mailbox. Make sure this works
		++g_ucBPPressedCount;
		Mailbox_post(ButtonMailbox_Handle, &presses, BIOS_NO_WAIT);
	}
}

void UserInput_Task(UArg arg0, UArg arg1) {
	int triggerDirection = 1;
	unsigned char selectionIndex = 1;
	unsigned int mvoltsPerDiv = 500;
	int triggerPixel = 0;

	for (;;) {
		unsigned long presses;

		Mailbox_pend(ButtonMailbox_Handle, &presses, BIOS_WAIT_FOREVER);
		++g_ucUiTaskCount;

		switch (presses) {
		case 1: // "select" button pressed
			if (selectionIndex == 2) {
				triggerDirection *= -1; //Change direction of change for trigger
			} else {
				//Switch between spectrum and waveform
				if (g_ucSpectrumMode == 0){
					g_ucSpectrumMode = 1;	//Spectrum
				}
				else {
					g_ucSpectrumMode = 0;	//Waveform
				}
			}
			break;
		case 2: // "right" button pressed
			selectionIndex = (selectionIndex == 2) ? 2 : ++selectionIndex; //Select gui element to the right, if one exists
			break;
		case 4: // "left" button pressed
			selectionIndex = (selectionIndex == 0) ? 0 : --selectionIndex; //Select gui element to the left, if one exists
			break;
		case 8: // "down" button pressed
			if (selectionIndex == 0) {
				//Adjust Timescale
//					g_uiTimescale =
//							(g_uiTimescale == 24) ? 24 : (g_uiTimescale - 2);
//					setupSampleTimer(g_uiTimescale); 	//Reset sampling timer
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
		case 16: // "up" button pressed
			if (selectionIndex == 0) {
				//Adjust Timescale
//					g_uiTimescale =
//							(g_uiTimescale == 1000) ?
//									1000 : (g_uiTimescale + 2);
//					adcSetup();									//Reset ADC
//					setupSampleTimer(g_uiTimescale);		//Reset sample timer
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
		//TODO: Modify oscilloscope settings
		g_iTriggerDirection = triggerDirection;
		g_ucSelectionIndex = selectionIndex;
		g_uiMVoltsPerDiv = mvoltsPerDiv;
		g_iTriggerPixel = triggerPixel;
	}
}

void Display_Task(UArg arg0, UArg arg1) {
	// initialize the OLED display, from TI qs_eklm3s8962
	RIT128x96x4Init(3500000);

	int triggerDirection;
	unsigned int mvoltsPerDiv;
	int triggerPixel;
	float fScale;
	unsigned char selectionIndex;
	float triggerLevel;
	char pcStr[50]; 						// string buffer

	for (;;) {
		Semaphore_pend(Draw_Sem, BIOS_WAIT_FOREVER);

		// get shared data variables
		triggerDirection = g_iTriggerDirection;
		mvoltsPerDiv = g_uiMVoltsPerDiv;
		triggerPixel = g_iTriggerPixel;
		fScale = g_fFScale;
		selectionIndex = g_ucSelectionIndex;
		triggerLevel = g_fTriggerLevel;

		//TODO: Draw one frame to the OLED screen (controls and waveform)
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

		if (g_ucSpectrumMode == 0) {				//Draw waveform
			//Draw points using the cached data
			int j;
			for (j = 1; j < SCREEN_WIDTH; j++) {
				// draw data points with lines
				DrawLine(g_ppWaveformBuffer[j - 1].x,
						g_ppWaveformBuffer[j - 1].y, g_ppWaveformBuffer[j].x,
						g_ppWaveformBuffer[j].y, BRIGHT);
			}
		} else { //Draw FFT
			int j;
			for (j = 2; j < SCREEN_WIDTH-1; j++) {
				// draw data points with lines
				DrawLine(j - 1, g_piSpectrumBuffer[j - 1], j, g_piSpectrumBuffer[j], BRIGHT);
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

//
//		//In order to have the decimal value drawn to the screen, the whole and fractional parts
//		//need to be extracted from the floating point number
//		Semaphore_pend(CPU_Sem, BIOS_WAIT_FOREVER);
//		unsigned int whole = (int) (cpu_load * 100);
//		unsigned int frac = (int) (cpu_load * 1000 - whole * 10);
//		Semaphore_post(CPU_Sem);

//		usprintf(pcStr, "CPU Load: %02u.%01u\%\%", whole, frac); // convert CPU load to string
//		DrawString(0, 86, pcStr, 15, false); // draw string to frame buffer

		//Draw timescale
		usprintf(pcStr, "%02uus", g_uiTimescale); // convert timescale to string
		DrawString(5, 0, pcStr, 15, false); // draw string to frame buffer
//
//		//Draw voltage scale
		usprintf(pcStr, "%02umV", mvoltsPerDiv); // convert voltage scale to string
		DrawString(53, 0, pcStr, 15, false); // draw string to frame buffer

		//Draw trigger icon and trigger line
		drawTrigger(triggerDirection);
		int voltToAdc = VOLT_TO_ADC(triggerLevel);
		float voltToAdcFscale = voltToAdc * fScale;
		int y = (int) ((FRAME_SIZE_Y / 2) - triggerPixel);
		DrawLine(0, y, FRAME_SIZE_X - 1, y, 10);

		// copy frame to the OLED screen
		RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);

		Semaphore_post(Wave_Sem);
	}
}

void Waveform_Task(UArg arg0, UArg arg1) {
	float fScale; // scaling factor to map ADC counts to pixels
	int triggerDirection;
	unsigned int mvoltsPerDiv;
	int triggerPixel;

	for (;;) {
		//TODO: Pend on sempaphore from Display_Task()
		Semaphore_pend(Wave_Sem, BIOS_WAIT_FOREVER);

		// get shared data variables
		triggerDirection = g_iTriggerDirection;
		mvoltsPerDiv = g_uiMVoltsPerDiv;
		triggerPixel = g_iTriggerPixel;

		//TODO: Search for trigger in the ADC buffer
		//Determine scaling factor
		fScale = ((float) (VIN_RANGE * 1000 * PIXELS_PER_DIV))
				/ ((float) ((1 << ADC_BITS) * mvoltsPerDiv));

		//Find trigger
		float triggerLevel = (3.0 / (1 << ADC_BITS)) * (triggerPixel / fScale);
		int triggerIndex = triggerSearch(triggerLevel, triggerDirection, NFFT);

		g_fFScale = fScale;
		g_fTriggerLevel = triggerLevel;

		//Copy, convert a screen's worth of data into a local buffer of points
		Point tempBuffer[NFFT];
		int i;
		for (i = 0; i < NFFT; i++) {
			Point dataPoint;
			dataPoint.x = i;
			dataPoint.y =
					g_pusADCBuffer[ADC_BUFFER_WRAP(triggerIndex + i - NFFT/2)];
			tempBuffer[i] = dataPoint;
		}

		int j;
		for (j = 1; j < NFFT; j++) {
			int adcY0 = 2
					* (tempBuffer[j - 1].y
							- ((1 << ADC_BITS) / 3.0) * (1.5)); //Scale previous sample
			int adcY1 =
					2
							* (tempBuffer[j].y
									- ((1 << ADC_BITS) / 3.0) * (1.5)); //Scale current sample

			g_ppWaveformBuffer[j - 1].y = FRAME_SIZE_Y / 2
					- ((adcY0 - ADC_OFFSET) * fScale);
			g_ppWaveformBuffer[j-1].x = tempBuffer[j-1].x;
			g_ppWaveformBuffer[j].y = FRAME_SIZE_Y / 2
					- ((adcY1 - ADC_OFFSET) * fScale);
			g_ppWaveformBuffer[j].x = tempBuffer[j].x;
		}

		if (g_ucSpectrumMode == 0) {
			Semaphore_post(Draw_Sem);
		} else {
			Semaphore_post(FFT_Sem);
		}
	}
}

void FFT_Task(UArg arg0, UArg arg1) {
	for (;;) {
		Semaphore_pend(FFT_Sem, BIOS_WAIT_FOREVER);

		static char kiss_fft_cfg_buffer[KISS_FFT_CFG_SIZE]; // Kiss FFT config memory
		size_t buffer_size = KISS_FFT_CFG_SIZE;
		kiss_fft_cfg cfg; // Kiss FFT config
		static kiss_fft_cpx in[NFFT], out[NFFT]; // complex waveform and spectrum buffers
		int i;
		cfg = kiss_fft_alloc(NFFT, 0, kiss_fft_cfg_buffer, &buffer_size); // init Kiss FFT

		for (i = 0; i < NFFT; i++) { // generate an input waveform
			in[i].r = g_ppWaveformBuffer[i].y; // real part of waveform
			in[i].i = 0; // imaginary part of waveform
		}
		kiss_fft(cfg, in, out); // compute FFT

//		float min = FLT_MAX;
		int j;
//		for (j = 0; j < SCREEN_WIDTH; j++) {
//			if(out[j].i < min) {
//				min = out[j].i;
//			}
//		}

		/*********************************************************************
		 * AREAS TO INVESTIGATE:
		 * - g_ppWaveformBuffer[] is of length 128
		 *
		 *********************************************************************/


		int k;
		for (j = 0; j < SCREEN_WIDTH; j++){
//			g_piSpectrumBuffer[j] = /*(int)(log10((*/out[j].i /*- min)+1.001)*7.0 + 10.0)*/;
//			float sum = 0.0;
//			for(k = 0; k < 8; k++) {
//				sum += out[j+k].i;
//			}

//			g_piSpectrumBuffer[j] = (int)(log10((out[j].i - min)+1.001)*7.0 + 10.0);
			g_piSpectrumBuffer[j] = log10(powf(out[j].r,2) + powf(out[j].i,2))*-10 + 96;
		}



		Semaphore_post(Draw_Sem);
	}
}

//void Idle_Task(void) {
//	unsigned long count_loaded;	// counts of iterable after interrupts are enabled
//
//	for (;;) {
//		//Measure CPU Load
//		count_loaded = cpu_load_count();
//
//		Semaphore_pend(CPU_Sem, BIOS_WAIT_FOREVER);
//		cpu_load = 1.0 - (float) count_loaded / g_ulCount_unloaded; // compute CPU load
//		Semaphore_post(CPU_Sem);
//	}
//}

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
//	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 0); // specify the trigger for Timer
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_ALWAYS, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0,
			ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0); // in the 0th step, sample channel 0
	ADCIntEnable(ADC0_BASE, 0); // enable ADC interrupt from sequence 0
	ADCSequenceEnable(ADC0_BASE, 0); // enable the sequence. it is now sampling
//	IntPrioritySet(INT_ADC0SS0, 0); // 0 = highest priority
//	IntEnable(INT_ADC0SS0); // enable ADC0 interrupts
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
