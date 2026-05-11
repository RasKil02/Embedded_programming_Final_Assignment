/*****************************************************************************
* University of Southern Denmark
* Embedded  Programming 
*
* MODULENAME.: LED_task.h
*
* PROJECT....: Final Assignment - Embedded Programming
*
* DESCRIPTION: Used for all LED related functions
*
* Change Log:
******************************************************************************
* Date    Id    Change
* 2026-05-10
* --------------------
* 150321  MoH   Module created.
*
*****************************************************************************/

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
extern INT8U uart_amount;

void LED_task(void *pvParameters);



/****************************** End Of Module *******************************/

#endif /* LED_TASK_H_ */
