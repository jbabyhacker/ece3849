/*
 * fifo.h
 *
 *  Created on: Nov 9, 2014
 *      Author: Nicholas
 */

#ifndef FIFO_H_
#define FIFO_H_

/**
 * Function: create_fifo
 * ----------------------------
 *   This function creates a FIFO data structure of given size. This FIFO can be accessed using the below functions.
 *
 *   size: the size of the FIFO, as an unsigned short
 *
 *   returns: 1 if the FIFO was created successfully, 0 if not
 */
unsigned char create_fifo(unsigned short size);

/**
 * Function: fifo_put
 * ----------------------------
 *   This function puts the given item in the FIFO
 *
 *   item: the char to be stored in the FIFO
 *
 *   returns: 1 if the put was successful, or 0 if it was not.
 */
unsigned char fifo_put(char item);

/**
 * Function: fifo_get
 * ----------------------------
 *   This function removes and returns the oldest item in the FIFO
 *
 *   container: a pointer to a variable to store the retrieved char
 *
 *   returns: 1 if successful, 0 if the FIFO is full
 */
unsigned char fifo_get(char* container);


#endif /* FIFO_H_ */
