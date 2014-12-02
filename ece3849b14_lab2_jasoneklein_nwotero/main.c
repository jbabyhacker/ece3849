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
//Void taskFxn(UArg a0, UArg a1)
//{
//    System_printf("enter taskFxn()\n");
//
//    Task_sleep(10);
//
//    System_printf("exit taskFxn()\n");
//}
/*
 *  ======== main ========
 */Void main() {
//	char pcStr[50]; 						// string buffer
//	float cpu_load;							// percentage of CPU loaded
//	unsigned long count_unloaded;// counts of iterable before interrupts are enabled
//	unsigned long count_loaded;	// counts of iterable after interrupts are enabled
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

// initialize the OLED display, from TI qs_eklm3s8962
	RIT128x96x4Init(3500000);

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
 */Void ADCSampler_Hwi(Void) {
//	ADC_ISC_R = ADC_ISC_IN0; // clear ADC sequence0 interrupt flag in the ADCISC register
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
 */Void ButtonPoller_Clock(Void) {
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
		Mailbox_post(ButtonMailbox_Handle, &presses, BIOS_NO_WAIT);
	}
}

// UserInput_Task(), Display_Task(), and Waveform_Task() all share one semaphore

Void UserInput_Task(UArg arg0, UArg arg1) {
	for (;;) {
		unsigned long presses;

		/*
		 * I use this approach because purely using Mailbox_pend()
		 * doesn't work. When an item is added to the Mailbox,
		 * Mailbox_pend() should continue, but in actuality, it doesn't. WTF
		 */
		Mailbox_pend(ButtonMailbox_Handle, &presses, BIOS_WAIT_FOREVER);
//		Int val = Mailbox_getNumPendingMsgs(ButtonMailbox_Handle);
//		if (val > 0) {
//			Mailbox_pend(ButtonMailbox_Handle, &presses, BIOS_WAIT_FOREVER);
		switch (presses) {
		case 1: // "select" button pressed
			g_iTriggerDirection *= -1; //Change direction of change for trigger
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
//					g_uiTimescale =
//							(g_uiTimescale == 24) ? 24 : (g_uiTimescale - 2);
//					setupSampleTimer(g_uiTimescale); 	//Reset sampling timer
			} else if (g_ucSelectionIndex == 1) { //Adjust pixel per ADC tick
				if (g_uiMvoltsPerDiv == 1000) {
					g_uiMvoltsPerDiv = 500;
				} else if (g_uiMvoltsPerDiv == 500) {
					g_uiMvoltsPerDiv = 200;
				} else if (g_uiMvoltsPerDiv == 200) {
					g_uiMvoltsPerDiv = 100;
				}
			} else {
				g_iTriggerPixel -= 2;	//Move the trigger line down
			}
			break;
		case 16: // "up" button pressed
			if (g_ucSelectionIndex == 0) {
				//Adjust Timescale
//					g_uiTimescale =
//							(g_uiTimescale == 1000) ?
//									1000 : (g_uiTimescale + 2);
//					adcSetup();									//Reset ADC
//					setupSampleTimer(g_uiTimescale);		//Reset sample timer
			} else if (g_ucSelectionIndex == 1) {
				//Adjust pixel per ADC tick
				if (g_uiMvoltsPerDiv == 500) {
					g_uiMvoltsPerDiv = 1000;
				} else if (g_uiMvoltsPerDiv == 200) {
					g_uiMvoltsPerDiv = 500;
				} else if (g_uiMvoltsPerDiv == 100) {
					g_uiMvoltsPerDiv = 200;
				}
			} else {
				g_iTriggerPixel += 2;	//Move trigger line up two pixels
			}
			break;
		}
		Semaphore_post(UserInputWaveformDisplay_Handle);
//		val = 0;
//		}
		//TODO: Modify oscilloscope settings
		//TODO: Signal Display_Task() with a semaphore
	}
}

Void Display_Task(UArg arg0, UArg arg1) {
	for (;;) {
//		Int val = Semaphore_getCount(UserInputWaveformDisplay_Handle);
//		if(val > 0) {
		Semaphore_pend(UserInputWaveformDisplay_Handle, BIOS_WAIT_FOREVER);
//		}

		int i = 0;
		i++;

		//TODO: Pend on sempahore from UserInput_Task() OR UserInput_Task();
		//TODO: Draw one frame to the OLED screen (controls and waveform)
		//TODO: Signal Waveform_Task()
	}
}

Void Waveform_Task(UArg arg0, UArg arg1) {
	for (;;) {
		//TODO: Pend on sempaphore from Display_Task()
		//TODO: Search for trigger in the ADC buffer
		//TODO: Copy triggered waveform into waveform buffer
		//TODO: Signal Display_Task()
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
 */Void adcSetup(Void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // enable the ADC
	SysCtlADCSpeedSet(SYSCTL_ADCSPEED_500KSPS); // specify 500ksps
	ADCSequenceDisable(ADC0_BASE, 0); // choose ADC sequence 0; disable before configuring
//	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 0); // specify the trigger for Timer
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_ALWAYS, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0,
			ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0); // in the 0th step, sample channel 0
//	ADCIntEnable(ADC0_BASE, 0); // enable ADC interrupt from sequence 0
	ADCSequenceEnable(ADC0_BASE, 0); // enable the sequence. it is now sampling
//	IntPrioritySet(INT_ADC0SS0, 0); // 0 = highest priority
//	IntEnable(INT_ADC0SS0); // enable ADC0 interrupts
}

/**
 * Configures "select", "up", "down", "left", "right" buttons for input
 * Adapted from Lab 0 handout by Professor Gene Bogdanov
 */Void buttonSetup(Void) {
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
