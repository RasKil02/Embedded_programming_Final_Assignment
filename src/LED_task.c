/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: LED_task.c
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
* 040526  KOES    Module created.
*
*****************************************************************************/


/*
 * LED_task.c
 *
 *  Created on: 4. maj 2026
 *      Author: Karl
 */

/***************************** Include files *******************************/
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "emp_type.h"
//#include "glob_def.h"
//#include "binary.h"
#include "status_led.h"


/*****************************    Defines    *******************************/
extern QueueHandle_t change_q; 
extern QueueHandle_t purchased_products_q;
extern QueueHandle_t time_q;


typedef enum {
    IDLE,
    RETURN_CASH,
    GRIND_ESPRESSO_LATTE,
    FILTER_COFFEE,
    BREW_ESPRESSO_LATTE,
    FROTH_MILK
} state_t;

typedef enum {
    NO_PRODUCT = 0,
    ESPRESSO,           // 1
    LATTE,              // 2
    FILTER              // 3, C has automatically assigned 1,2 and 3.
} product_t;

// Placeholders for succesful build:

/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/
void LED_init(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  -
*****************************************************************************/
{
    INT8S dummy;

    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;
    dummy = SYSCTL_RCGC2_R;

    // Enable all 3 LED pins at once
    GPIO_PORTF_DIR_R |= 0x0E;  // PF1, PF2, PF3
    GPIO_PORTF_DEN_R |= 0x0E;
}

void blink_green_led(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  -
*****************************************************************************/
{
    GPIO_PORTF_DATA_R = 0x08;   // ON
    vTaskDelay(200 / portTICK_RATE_MS);

    GPIO_PORTF_DATA_R = 0x00;   // OFF
    vTaskDelay(200 / portTICK_RATE_MS);
}

void turn_on_green_led(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  -
*****************************************************************************/
{
    GPIO_PORTF_DATA_R = 0x08;
}

void turn_on_red_led(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  -
*****************************************************************************/
{
    GPIO_PORTF_DATA_R = 0x02;
}

void turn_on_yellow_led(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  -
*****************************************************************************/
{
    GPIO_PORTF_DATA_R = 0x04;
}

void turn_off_led(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  -
*****************************************************************************/
{
    GPIO_PORTF_DATA_R = 0x00;
}



void LED_task(void *pvParameters)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  -
*****************************************************************************/
{

    LED_init();

    state_t STATE = IDLE;
    int time = 0;
    int time_inactive = 0;
    float prepaid_amount = 0.0f; // Placeholder
    product_t product;
    int change;

    while(1)
    {
        switch(STATE)
        {
            case IDLE:
            {
                // Check change FIRST (priority)
                if (xQueueReceive(change_q, &change, 0)) // This queue has not been created yet, note this if statement will return pdTRUE if it has a value
                {
                    STATE = RETURN_CASH;
                    break;
                }

                // Check product selection
                if (xQueueReceive(purchased_products_q, &product, 0)) // This queue has not been created yet
                {
                    if (product == ESPRESSO || product == LATTE)
                    {
                        STATE = GRIND_ESPRESSO_LATTE;
                    }
                    else if (product == FILTER)
                    {
                        STATE = FILTER_COFFEE;
                    }
                }

                break;
            }

            case RETURN_CASH:
            {
                static int counter = 0;

                if (counter == 0)
                {
                    counter = change;
                }

                if (counter > 0)
                {
                    blink_green_led();   // one blink step
                    counter--;
                }
                else
                {
                    STATE = IDLE;
                }

                break;
            }


            case GRIND_ESPRESSO_LATTE:
            {
                static int time_left_grind = 0;

                if (time_left_grind == 0)
                {
                    time_left_grind = 7500;
                }

                turn_on_yellow_led();

                vTaskDelay(100);
                time_left_grind -= 100;

                if (time_left_grind <= 0)
                {
                    turn_off_led();
                    time_left_grind = 0;
                    STATE = BREW_ESPRESSO_LATTE;
                }

                break;
            }

            case FILTER_COFFEE:
            {
                float rate = 0.6f; // cl/s

                // This returns 1 if the button was pushed and 0 if not
                if (!(GPIO_PORTF_DATA_R & 0x10))
                {
                    time_inactive = 0;

                    if (time < 3000)
                    {
                        turn_on_yellow_led();
                        // rate stays the same
                    }
                    if (time > 3000)
                    {
                        turn_on_yellow_led();
                        rate = 1.45; // cl/s
                    }
                }

                if ((GPIO_PORTF_DATA_R & 0x10))
                {
                    time_inactive += 100;
                }

                if (prepaid_amount <= 0.0f)
                {
                    turn_off_led();
                    STATE = IDLE;
                    xQueueSend(time_q, &time, 0); // Send time to controller task for logging
                    time = 0;
                    time_inactive = 0;
                }

                if (time_inactive >= 5000)
                {
                    turn_off_led();
                    STATE = IDLE;
                    xQueueSend(time_q, &time, 0); // Send time to controller task for logging
                    time = 0;
                    time_inactive = 0;
                }

                vTaskDelay(90);
                time += 100;
                prepaid_amount -= 0.1; // kr.

                break;
            }

            case BREW_ESPRESSO_LATTE:
            {
                static int time_left_brew = 0;

                if (time_left_brew == 0)
                {
                    time_left_brew = 14000;
                }

                turn_on_red_led();

                vTaskDelay(100);
                time_left_brew -= 100;

                if (time_left_brew <= 0)
                {
                    turn_off_led();
                    time_left_brew = 0;

                    if (product == ESPRESSO)
                    {
                        STATE = IDLE;
                    }
                    else if (product == LATTE)
                    {
                        STATE = FROTH_MILK;
                    }
                }

                break;
            }

            case FROTH_MILK:
            {
                static int time_left_froth = 0;

                if (time_left_froth == 0)
                {
                    time_left_froth = 6200;
                }

                turn_on_green_led();

                vTaskDelay(100);
                time_left_froth -= 100;

                if (time_left_froth <= 0)
                {
                    turn_off_led();
                    time_left_froth = 0;
                    STATE = IDLE;
                }

                break;
            }

            default:
                // runs if none match
                break;
        }

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}


/****************************** End Of Module *******************************/




