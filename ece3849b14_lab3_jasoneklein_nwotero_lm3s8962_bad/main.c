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

}
