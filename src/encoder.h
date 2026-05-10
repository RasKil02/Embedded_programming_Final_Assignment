/*****************************************************************************
* University of Southern Denmark
* Embedded Programming (EMP)
*
* MODULENAME.: encoder.h
*
* PROJECT....: EMP
*
* DESCRIPTION: Header file for encoder module
*
*****************************************************************************/

#ifndef ENCODER_H_
#define ENCODER_H_

/***************************** Include files *******************************/
#include <stdint.h>
#include "emp_type.h"
#include "FreeRTOS.h"
#include "queue.h"

/*****************************    Defines    *******************************/
#define IDLE               0
#define SEND_TO_LCD        1
#define SEND_FINAL_AMOUNT  2

/*****************************   Constants   *******************************/
extern QueueHandle_t encoder_queue;

/****************************    variables   *******************************/
extern INT16S amount;

/*****************************   Functions   *******************************/

// Initialization
void Encoder_init( void );

// Read raw GPIO values
INT8U Encoder_getA( void );
INT8U Encoder_getB( void );
INT8U Encoder_getButton( void );

// Send values to queue
INT8U Encoder_readA( void );
INT8U Encoder_readB( void );
INT8U Encoder_readButton( void );

// FreeRTOS task
void encoder_task( void *pvParameters );

#endif /* ENCODER_H_ */
