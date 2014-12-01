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
 */
Void main() {
//    Task_Handle task;
//    Error_Block eb;
//
//    System_printf("enter main()\n");
//
//    Error_init(&eb);
//    task = Task_create(taskFxn, NULL, &eb);
//    if (task == NULL) {
//        System_printf("Task_create() failed!\n");
//        BIOS_exit(0);
//    }

	BIOS_start(); /* enable interrupts and start SYS/BIOS */
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
