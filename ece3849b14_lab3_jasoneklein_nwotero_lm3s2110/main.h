/*
 * main.h
 *
 * Authors: Jason Klein and Nicholas Otero
 * Date: 12/5/2014
 * ECE 3829 Lab 3
 */

#ifndef MAIN_H_
#define MAIN_H_

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

void ComparatorSetup();

#endif /* MAIN_H_ */
