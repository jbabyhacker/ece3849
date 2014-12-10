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
		g_ulADCErrors++; // count errors - step 1 of the signoff
		ADC0_OSTAT_R = ADC_OSTAT_OV0; // clear overflow condition
	}
	int buffer_index = ADC_BUFFER_WRAP(g_iADCBufferIndex + 1);
	g_pusADCBuffer[buffer_index] = ADC_SSFIFO0_R & ADC_SSFIFO0_DATA_M; // read sample from the ADC sequence0 FIFO
	g_iADCBufferIndex = buffer_index; // set the new buffer index
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

			break;
		case 2: // "right" button pressed
			break;
		case 4: // "left" button pressed
			break;
		case 8: // "down" button pressed
			break;
		case 16: // "up" button pressed
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

		// copy frame to the OLED screen
		RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);

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
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0,
			ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0); // in the 0th step, sample channel 0
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
