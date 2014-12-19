/*
 * main.c
 *
 * Authors: Jason Klein and Nicholas Otero
 * Date: 12/5/2014
 * ECE 3829 Lab 3 LM3S2110
 */

#include "main.h"

volatile float periodSum = 0;
volatile unsigned int numSamples = 0;

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

/**
 * Function: Timer0A_ISR
 * ----------------------------
 *   Timer to capture timer values to compute the clock cycles per period
 *
 *   returns: nothing
 */
void Timer0A_ISR() {
	static long previous = 0;
	TIMER0_ICR_R = TIMER_ICR_CAECINT;

	if (g_ucPeriodInit) { //This should only execute on the first measurement
		g_ucPeriodInit = 0;
		previous = TIMER0_TAR_R; //Captured timer value
	} else {
		long recent = TIMER0_TAR_R; //Captured timer value
		g_ulDiff = ((previous - recent) & 0xffff);
		periodSum += g_ulDiff;
		numSamples++;
		previous = recent;
	}
}

/**
 * Function: Timer1A_ISR
 * ----------------------------
 *   Timer to compute frequency.
 *
 *   returns: nothing
 */
void Timer1A_ISR() {
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;

	int i;
	int n = 0;
	long freqCount = 0;

	float avgPeriod = periodSum / numSamples;
	g_ulFrequencyMeasurement = ((1 / avgPeriod) * g_ulSystemClock) * 1000;
	periodSum = 0;
	numSamples = 0;
	NetworkTx(g_ulFrequencyMeasurement);
}

/**
 * Function: ComparatorSetup
 * ----------------------------
 *   Configures the hardware comparator to create a square wave
 *
 *   returns: nothing
 */
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

/**
 * Function: CaptureTimerSetup
 * ----------------------------
 *   Configures the timer to trigger on rising edge
 *
 *   returns: nothing
 */
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

/**
 * Function: PeriodicTimerSetup
 * ----------------------------
 *   Configures the timer to run at 10 Hz
 *
 *   returns: nothing
 */
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
