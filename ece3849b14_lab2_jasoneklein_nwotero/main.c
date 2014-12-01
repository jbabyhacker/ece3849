/*
 *  ======== main.c ========
 */

#include <xdc/std.h>

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/knl/Task.h>

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
	char pcStr[50]; 						// string buffer
	float cpu_load;							// percentage of CPU loaded
	unsigned long count_unloaded;// counts of iterable before interrupts are enabled
	unsigned long count_loaded;	// counts of iterable after interrupts are enabled
	unsigned char selectionIndex = 1;// index for selected top-screen gui element
	unsigned int mvoltsPerDiv = 500;	// miliVolts per division of the screen
	int triggerPixel = 0;// The number of pixels from the center of the screen the trigger level is on
	float fScale;				// scaling factor to map ADC counts to pixels

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

	IntMasterDisable();
	adcSetup();
	buttonSetup();
	IntMasterEnable();

	BIOS_start(); /* enable interrupts and start SYS/BIOS */
}

/**
 * ADC Interrupt service routine. Stores ADC value in the global
 * circular buffer.  If the ADC FIFO overflows, count as a fault.
 * Adapted from Lab 1 handout by Professor Gene Bogdanov
 */
Void ADC_Sampler() {
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
 */
Void Button_Poller() {
// 	TIMER0_ICR_R = TIMER_ICR_TATOCINT; // clear interrupt
	unsigned long presses = g_ulButtons;

//	if (g_ucPortEButtonFlag || g_ucPortFButtonFlag) {
//		// button debounce
//		ButtonDebounce((~GPIO_PORTE_DATA_R & GPIO_PIN_0) << 4 // "up" button
//		| (~GPIO_PORTE_DATA_R & GPIO_PIN_1) << 2 // "down" button
//		| (~GPIO_PORTE_DATA_R & GPIO_PIN_2) // "left" button
//				| (~GPIO_PORTE_DATA_R & GPIO_PIN_3) >> 2 // "right" button
//				| (~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1); // "select" button
//		presses = ~presses & g_ulButtons; // button press detector
//
//		//Note, we could make this one statement, but it is expanded for readability
//		//Determine which buttons are pressed
//		if (presses & 1) { // "select" button pressed
//			fifo_put(1);
//			g_ucPortFButtonFlag = 0; // reset flag
//		}
//
//		if (presses & 2) { // "Right" button pressed
//			fifo_put(2);
//			g_ucPortEButtonFlag = 0; // reset flag
//		}
//
//		if (presses & 4) { // "Left" button pressed
//			fifo_put(3);
//			g_ucPortEButtonFlag = 0; // reset flag
//		}
//
//		if (presses & 8) { // "Down" button pressed
//			fifo_put(4);
//			g_ucPortEButtonFlag = 0; // reset flag
//		}
//
//		if (presses & 16) { // "Up" button pressed
//			fifo_put(5);
//			g_ucPortEButtonFlag = 0; // reset flag
//		}
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
Void adcSetup(void) {
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
 */
Void buttonSetup(void) {
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
