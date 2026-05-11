/*****************************************************************************
* University of Southern Denmark
* Embedded Programming 
*
* MODULENAME.: time_of_day_task.h
*
* PROJECT....: Final Assignment - Embedded Programming
*
* DESCRIPTION: Simple real-time clock task
*
*****************************************************************************
* Date    Id    Change
* 2026-05-10
* --------------------
* 150321  MoH   Module created.
*
*****************************************************************************/

#ifndef TIME_OF_DAY_TASK_H_
#define TIME_OF_DAY_TASK_H_

/***************************** Include files *******************************/
#include <stdint.h>
#include "emp_type.h"
#include "controller.h"

/*****************************   Variables   *******************************/


/*****************************   Functions   *******************************/

// Initializes time values
void time_init(INT8U start_hour, INT8U start_min);

// FreeRTOS task
void time_of_day_task(void *pvParameters);

#endif /* TIME_OF_DAY_TASK_H_ */