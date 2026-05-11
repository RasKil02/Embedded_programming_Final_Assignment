/*****************************************************************************
* University of Southern Denmark
* Embedded Programming 
*
* MODULENAME.: controller.h
*
* PROJECT....: Final Assignment - Embedded programming
*
* DESCRIPTION: Used as the central logic control unit for the system
*
* Change Log:
******************************************************************************
* Date    Id    Change
* 2026-05-10
* --------------------
* 150321  MoH   Module created.
*
*****************************************************************************/


#ifndef CONTROLLER_H_
#define CONTROLLER_H_

/***************************** Include files *******************************/

/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

#define MAX_PRODUCTS 100

typedef struct {
    INT8U coffee_type;
    INT8U price;
    INT8U amount;
    INT8U u_hour;
    INT8U u_min;
    INT8U payment_type;
    char u_card_number[17];
} uart_product_t;


extern INT8U espresso;
extern INT8U latte;
extern INT8U filter;
extern uart_product_t product_log[MAX_PRODUCTS];
extern INT8U purchase_nr;
extern INT8U hour;
extern INT8U min;
extern INT8U sec;


/*****************************   Functions   *******************************/
void controller_task(void *pvParameters);

/****************************** End Of Module *******************************/

#endif /* CONTROLLER_H_ */
