/*
 * main.c
 *
 * Authors: Jason Klein and Nicholas Otero
 * Date: 12/5/2014
 * ECE 3829 Lab 3 LM3S2110
 */

#include "main.h"

void main(void) {
	// configure clock for 25 MHz
	if (REVISION_IS_A2) {
		SysCtlLDOSet(SYSCTL_LDO_2_75V);
	}
	SysCtlClockSet(
			SYSCTL_SYSDIV_8 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_8MHZ);

	g_ulSystemClock = SysCtlClockGet();

	IntMasterDisable();
	NetworkInit();
	ComparatorSetup();
	PeriodicTimerSetup();
	CaptureTimerSetup();
	IntMasterEnable();

	while (1) {

	}
}

void Timer0A_ISR() {
	static long previous = 0;
	// clear interrupt flag
	//if (TIMER0_ICR_R & TIMER_MIS_CAEMIS == TIMER_MIS_CAEMIS){
	//	TIMER0_ICR_R &= ~TIMER_ICR_CAECINT;
	//}
	TIMER0_ICR_R = TIMER_ICR_CAECINT;
	//TIMER0_CTL_R = TIMER_CTL_TAEN;

	if (g_ucPeriodInit) { //This should only execute on the first measurement
		g_ucPeriodInit = 0;
		previous = TIMER0_TAR_R; //Captured timer value
	} else {
		long recent = TIMER0_TAR_R; //Captured timer value
		g_ucPeriodIndex = BUFFER_WRAP(++g_ucPeriodIndex);
		g_ulDiff = ((previous - recent) & 0xffff);
		g_pulPeriodMeasurements[g_ucPeriodIndex] = g_ulDiff
				/ (g_ulSystemClock / 1000000);
		previous = recent;
	}

	g_ulTimer0Counter++;
}
//
void Timer1A_ISR() {
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;

	g_ulTimer1Counter++;

	int i;
	int n = 0;
	long freqCount = 0;
	//Iterate through each period measured by the other timer ISR. Note, the shared variable is accessed
	//repeatedly. This is not a shared data bug. g_ucPeriodIndex will only increment, and this loop
	//need to keep up with the new values.  The loop will go no further than what is about to get written.
	for (i = g_ucFreqIndex; (i != BUFFER_WRAP(g_ucPeriodIndex + 1)); BUFFER_WRAP(++i)) {
		if (g_pulPeriodMeasurements[i]) {
			n++;
			freqCount += 1000000 / g_pulPeriodMeasurements[i];
		}
	}
	g_ucFreqIndex = BUFFER_WRAP(g_ucPeriodIndex);

	if (n) {
		g_ulFrequencyMeasurement = freqCount / n;
		NetworkTx(g_ulFrequencyMeasurement);
	}
}
void ComparatorSetup() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_COMP0);

	//No ADC interrupt, internal reference, not inverted output
	ComparatorConfigure(COMP_BASE, 0,
			COMP_TRIG_NONE | COMP_ASRCP_REF | COMP_OUTPUT_NORMAL);
	ComparatorRefSet(COMP_BASE, COMP_REF_1_5125V); // 1.5125V internal reference
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_DIR_MODE_HW); // C0- input
	GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA,
			GPIO_PIN_TYPE_ANALOG);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIODirModeSet(GPIO_PORTD_BASE, GPIO_PIN_7, GPIO_DIR_MODE_HW); // C0o output
	GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA,
			GPIO_PIN_TYPE_STD);

}

void CaptureTimerSetup() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

	GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_DIR_MODE_HW); // CCP0 input
	GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA,
			GPIO_PIN_TYPE_STD);

	TimerDisable(TIMER0_BASE, TIMER_BOTH);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME);
	TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
	TimerLoadSet(TIMER0_BASE, TIMER_A, 0xffff);
	TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
	TimerEnable(TIMER0_BASE, TIMER_A);
	IntPrioritySet(INT_TIMER0A, 0);
	IntEnable(INT_TIMER0A);

}

void PeriodicTimerSetup() {
	IntMasterDisable();
	// configure timer for buttons
	unsigned long ulDivider, ulPrescaler;
	// initialize a general purpose timer for periodic interrupts
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	TimerDisable(TIMER1_BASE, TIMER_BOTH);
	TimerConfigure(TIMER1_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC); //Periodic timer

	ulPrescaler = (g_ulSystemClock / POLL_RATE - 1) >> 16; // prescaler for a 16-bit timer
	ulDivider = g_ulSystemClock / (POLL_RATE * (ulPrescaler + 1)) - 1; // 16-bit divider (timer load value)
	TimerLoadSet(TIMER1_BASE, TIMER_A, ulDivider); //Set starting count of timer
	TimerPrescaleSet(TIMER1_BASE, TIMER_A, ulPrescaler);	//Prescale the timer
	TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);//enable the timer's interrupts
	TimerEnable(TIMER1_BASE, TIMER_A);		//enable timer peripheral interrupts
	// initialize interrupt controller to respond to timer interrupts
	IntPrioritySet(INT_TIMER1A, 32); // 0 = highest priority, 32 = next lower
	IntEnable(INT_TIMER1A);
}
