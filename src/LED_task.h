/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: LED_task.h
*
* PROJECT....: EMP final assignment
*
* DESCRIPTION: Used for all LED related functions
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 040526  KOES    Module created.
*
*****************************************************************************/

/*
 * LED_task.h
 *
 *  Created on: 4. maj 2026
 *      Author: Karl
 */

#ifndef LED_TASK_H_
#define LED_TASK_H_


/***************************** Include files *******************************/

/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/
void blink_green_led(void);

void turn_on_yellow_led(void);
void turn_on_red_led(void);
void turn_on_green_led(void);

void turn_off_led(void);

void LED_init(void);
INT8U button_pushed(void);

void LED_task(void *pvParameters);



/****************************** End Of Module *******************************/

#endif /* LED_TASK_H_ */
