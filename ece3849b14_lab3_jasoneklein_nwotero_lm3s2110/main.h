/*
 * main.h
 *
 * Authors: Jason Klein and Nicholas Otero
 * Date: 12/5/2014
 * ECE 3829 Lab 3
 */

#ifndef MAIN_H_
#define MAIN_H_

//Includes
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "lm3s2110.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/comp.h"
#include "driverlib/timer.h"
#include "network.h"

//Defines
#define BUFFER_SIZE 255
#define BUFFER_WRAP(i) (i % BUFFER_SIZE)

#define POLL_RATE 100

//Prototypes
void ComparatorSetup();
void CaptureTimerSetup();
void PeriodicTimerSetup();

//Globals
unsigned long g_ulSystemClock;
volatile unsigned long g_pulPeriodMeasurements[BUFFER_SIZE];
volatile unsigned long g_ulFrequencyMeasurement;
volatile unsigned char g_ucPeriodIndex = 0;
volatile unsigned char g_ucFreqIndex = 0;
volatile unsigned char g_ucPeriodInit = 1;

#endif /* MAIN_H_ */
