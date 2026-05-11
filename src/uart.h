/*****************************************************************************
* University of Southern Denmark
* Embedded Programming (EMP)
*
* MODULENAME.: uart.h
*
* PROJECT....: Final Assignment - Embedded Programming
*
* DESCRIPTION: Header file for uart module.
*
* Change Log:
******************************************************************************
* Date    Id    Change
* 2026-05-10
* --------------------
* 150321  MoH   Module created.
*
*****************************************************************************/

#ifndef _UART_H
  #define _UART_H

/***************************** Include files *******************************/
#include "emp_type.h"
#include "uart.h"
#include "systick_frt.h"
#include "FreeRTOS.h"
#include "task.h"
/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/
BOOLEAN uart0_put_q( INT8U );
BOOLEAN uart0_get_q( INT8U* );
BOOLEAN uart0_getc(INT8U *data);
void uart0_puts(char *str);

void uart_tx_task(void *pvParameters);
void uart_rx_task(void *pvParameters);


extern void uart0_init( INT32U, INT8U, INT8U, INT8U );
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Initialize uart 0
******************************************************************************/


/****************************** End Of Module *******************************/
#endif

