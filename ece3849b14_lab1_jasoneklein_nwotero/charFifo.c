/*
 * charFifo.c
 *
 *  Created on: Nov 9, 2014
 *      Author: Nicholas
 */

#include "charFifo.h"
#include "stdlib.h"

#define FIFO_WRAP(i) ((i < 0) ? (g_usFifoSize - i) : ((i >= g_usFifoSize) ? (i - g_usFifoSize) : i))
//#define ADC_BUFFER_WRAP(i) ((i) & (ADC_BUFFER_SIZE - 1)) // index wrapping macro

//char* g_pcFifo = NULL;
char g_pcFifo[10];
unsigned short g_usFifoHead = 0;
unsigned short g_usFifoTail = 0;
const unsigned short g_usFifoSize = 10;

unsigned char create_fifo(const unsigned short size){
//	g_usFifoSize = size; 	//Save size to global variable
//	free(g_pcFifo);			//Free memory allocated to old FIFO
//	char tempArray[size];
//	g_pcFifo = tempArray;
//	g_pcFifo = malloc(size * sizeof(char));	//Allocate memory for FIFO

	return 1;

//	if (g_pcFifo != NULL) {	//Ensure memory allocation was successful
//		return 1;
//	}
//	else {					//If it wasn't, return 0
//		return 0;
//	}
}



unsigned char fifo_put(char item) {
	unsigned short new_tail = FIFO_WRAP(g_usFifoTail + 1);	//Get the index in front of the tail

	if (new_tail != g_usFifoHead){		//If the index in front of the tail is not the head
		g_pcFifo[g_usFifoTail] = item;
		g_usFifoTail = new_tail;
		return 1;						//Success
	}
	else								//If the head is at the next index, FIFO is full
	{
		return 0;						//Fail
	}
}


unsigned char fifo_get(char* container){
	if (g_usFifoHead != g_usFifoTail){			//If the FIFO is not empty
		*container = g_pcFifo[g_usFifoHead];	//Put the oldest char into the container
		if (g_usFifoHead + 1 >= g_usFifoSize)	//If moving the head will cause an overflow
		{
			g_usFifoHead = 0;					//Wrap to zero
		}
		else{
			g_usFifoHead++;
		}
		return 1;								//Success
	}
	else{
		return 0;								//Fail, FIFO is empty
	}
}


