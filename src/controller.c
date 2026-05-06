/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: controller.c
*
* PROJECT....: ECP
*
* DESCRIPTION: See module specification file (.h-file).
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 060526  KOES    Module created.
*
*****************************************************************************/


/*
 * controller.c
 *
 *  Created on: 6. maj 2026
 *      Author: Karl
 */

/*****************************    Defines    *******************************/
typedef enum {
    IDLE,
    PAYMENT,
    CHANGE_PRODUCT_PRICE,
    RECEIVING_CASH,
    RETURN_CASH,
    PRODUCE_CHOICE,
    LOG_PRODUCT_CHOICE,
    WAIT_FOR_PASSWORD,
} state_t;

/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

void controller_task(void *pvParameters)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  -
*****************************************************************************/
{
    state_t STATE = IDLE;
    INT8U input;
    INT8U data;

    while(1)
    {
        switch(STATE)
        {
            case IDLE :
            {

                if (xQueueReceive(key_queue, &input, 0))
                {
                    STATE = PAYMENT;
                }

                if (xQueueReceive(uart_queue_handler, &data, 0))
                {
                    STATE = CHANGE_PRODUCT_PRICE;
                }

                break;
            }

            case CHANGE_PRODUCT_PRICE :
            {
                // Dont know what to put here
                break;
            }

            case PAYMENT :
            {

                break;
            }

            case RECEIVING_CASH :
            {
                break;
            }

            case RETURN_CASH :
            {
                break;
            }

            case PRODUCE_CHOICE:
            {
                break;
            }

            case LOG_PRODUCT_CHOICE :
            {

                break;
            }

            case WAIT_FOR_PASSWORD :
            {
                break;
            }

            default:
                // runs if none match
                break;
        }

        vTaskDelay(10 / portTICK_RATE_MS);

    }
}


