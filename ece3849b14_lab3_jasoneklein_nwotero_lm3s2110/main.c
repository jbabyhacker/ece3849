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
 SysCtlClockSet(SYSCTL_SYSDIV_8 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
 SYSCTL_XTAL_8MHZ);

 while (1) {

 }
}
