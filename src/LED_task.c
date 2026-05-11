/*****************************************************************************
* University of Southern Denmark
* Embedded Programming 
*
* MODULENAME.: LED_task.c
*
* PROJECT....: Final Assignment - Embedded programming
*
* DESCRIPTION: Task for controlling the LEDs on the coffee machine. 
*              The LEDs indicate the current state of the machine and provide feedback to the user.
*
* Change Log:
******************************************************************************
* Date    Id    Change
* 2026-05-10
* --------------------
* 150321  MoH   Module created.
*
*****************************************************************************/

/***************************** Include files *******************************/
// for freeRTOS
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "emp_type.h"

// Own includes
#include "LED_task.h"
#include "controller.h"


/*****************************    Defines    *******************************/
#define QUEUE_LEN 128
#define YELLOW_LED 0x0b
#define GREEN_LED 0x07
#define RED_LED 0x0d
#define ESPRESSO_LATTE_GRIND_TIME 7500
#define ESPRESSO_LATTE_BREW_TIME 14000
#define LATTE_FROTH_TIME 6200
#define FILTER_COFFEE_FAST_RATE 1.45f   // cl/s

extern QueueHandle_t change_q;
extern QueueHandle_t purchased_products_q;
extern QueueHandle_t time_q;
extern QueueHandle_t led_to_controller_q;

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
    ESPRESSO,                       // 1
    LATTE,                          // 2
    FILTER                          // 3, C has automatically assigned 1,2 and 3.
} product_t;

typedef struct {
    product_t product;
    int prepaid_amount; // in kr.
} product_msg_t;


/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/
void LED_init(void)
/*****************************************************************************
*   Input    :  - 
*   Output   :  - 
*   Function :  - Initializes the GPIO pins for the LEDs. Sets PF1, PF2, and PF3 as outputs and enables digital functionality.
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
*   Function :  - Blinks the green LED once. Turns it on for 500 ms and then off for 500 ms.
*****************************************************************************/
{
    GPIO_PORTF_DATA_R = GREEN_LED;   // ON
    vTaskDelay(500 / portTICK_RATE_MS);

    GPIO_PORTF_DATA_R = 0x0E;   // OFF
    vTaskDelay(500 / portTICK_RATE_MS);
}

void turn_on_green_led(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  - Turns on the green LED. 
*****************************************************************************/
{
    GPIO_PORTF_DATA_R = GREEN_LED;
}

void turn_on_red_led(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  - Turns on the red LED.
*****************************************************************************/
{
    GPIO_PORTF_DATA_R = RED_LED;
}

void turn_on_yellow_led(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  - Turns on the yellow LED.
*****************************************************************************/
{
    GPIO_PORTF_DATA_R = YELLOW_LED;
}

void turn_off_led(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  - Turns off all LEDs.
*****************************************************************************/
{
    GPIO_PORTF_DATA_R = 0x0E;                   // OFF (active low)
}

INT8U button_pushed()
{
    return (GPIO_PORTF_DATA_R & 0x01) >> 4;     // Returns 1 if button is pushed, 0 if not
}


void LED_task(void *pvParameters)
/*****************************************************************************
*   Input    :  - 
*   Output   :  -
*   Function :  - Main task function for controlling the LEDs. 
*                 Implements a state machine that reacts to messages from the controller task and user interactions (button presses). 
*                 The LEDs indicate the current state of the machine and provide feedback to the user.
*****************************************************************************/
{

    LED_init();
    turn_off_led();

    state_t STATE = IDLE;
    int time = 0;
    int time_inactive = 0;
    float prepaid_amount = 0.0f;        // Placeholder
    product_msg_t product_msg;
    INT8U change_value = 0;
    INT8U change = 0;
    static int time_left_grind = 0;
    static int time_left_brew = 0;
    static int time_left_froth = 0;
    float rate = 0.6f;                  // cl/s

    INT8U message_for_controller;
    INT8U uart_amount = 0;

    while(1)
    {
        switch(STATE)
        {
            case IDLE:                                                          // Wait for either a change message or a product selection message
            {
                // Check change FIRST (priority)
                if (xQueueReceive(change_q, &change, pdMS_TO_TICKS(10)))       
                {
                    change_value = change;
                    STATE = RETURN_CASH;
                    break;
                }

                // Check product selection
                if (xQueueReceive(purchased_products_q, &product_msg, pdMS_TO_TICKS(10)))
                {
                    prepaid_amount = product_msg.prepaid_amount;

                    if (product_msg.product == ESPRESSO || product_msg.product == LATTE)
                    {
                        STATE = GRIND_ESPRESSO_LATTE;
                    }
                    else if (product_msg.product == FILTER)
                    {
                        STATE = FILTER_COFFEE;
                    }
                }

                break;
            }

            case RETURN_CASH:                                       // Blink the green LED for each unit of change to return, then return to IDLE.
            {
                if (change_value > 0)
                {
                    blink_green_led();
                    change_value--;
                }

                else
                {
                    change_value = 0;
                    STATE = IDLE;
                }

                break;
            }


            case GRIND_ESPRESSO_LATTE:                              // Grind the coffee for a fixed amount of time, then transition to brewing. The yellow LED is on during grinding.
            {
                if (time_left_grind == 0)
                {
                    time_left_grind = ESPRESSO_LATTE_GRIND_TIME;
                }

                turn_on_yellow_led();

                vTaskDelay(90 / portTICK_RATE_MS);
                time_left_grind -= 100;

                if (time_left_grind <= 0)
                {
                    turn_off_led();
                    time_left_grind = 0;
                    STATE = BREW_ESPRESSO_LATTE;
                }

                break;
            }

            case FILTER_COFFEE:                                     // For filter coffee, the user needs to hold the button to brew. The yellow LED is on while the button is held. If the button is released for more than 5 seconds, or if the prepaid amount runs out, the machine returns to IDLE.
            {
                // This returns 1 if the button was pushed and 0 if not
                if ((GPIO_PORTF_DATA_R & 0x01) == 0)                // Button pushed
                {
                    time_inactive = 0;

                    if (time < 3000)
                    {
                        turn_on_yellow_led();
                        // rate stays the same
                        uart_amount += 0.1 * rate;

                    }
                    if (time > 3000)
                    {
                        turn_on_yellow_led();
                        rate = FILTER_COFFEE_FAST_RATE;             
                        uart_amount += 0.1 * rate;
                    }
                }

                if ((GPIO_PORTF_DATA_R & 0x01) != 0)    // Button not pushed
                {
                    turn_off_led();
                    time_inactive += 100;
                }

                if (prepaid_amount <= 0.0f)             // If the prepaid amount runs out, stop brewing and return to IDLE
                {
                    turn_off_led();
                    message_for_controller = 1;
                    xQueueSend(led_to_controller_q, &message_for_controller, pdMS_TO_TICKS(10));
                    STATE = IDLE;
                    // xQueueSend(time_q, &time, 0); // Send time to controller task for logging
                    time = 0;
                    time_inactive = 0;
                }

                if (time_inactive > 5000)               // If the button has been released for more than 5 seconds, stop brewing and return to IDLE
                {
                    turn_off_led();
                    message_for_controller = 1;
                    xQueueSend(led_to_controller_q, &message_for_controller, pdMS_TO_TICKS(10));
                    STATE = IDLE;
                    // xQueueSend(time_q, &time, 0); // Send time to controller task for logging
                    time = 0;
                    time_inactive = 0;
                }

                vTaskDelay(90 / portTICK_RATE_MS);
                time += 100;
                prepaid_amount -= filter / 30.0f;; 
                break;
            }

            case BREW_ESPRESSO_LATTE:                   // Brew the espresso or latte for a fixed amount of time. The red LED is on during brewing. After brewing, transition to either IDLE (for espresso) or FROTH_MILK (for latte).
            {
                if (time_left_brew == 0)
                {
                    time_left_brew = ESPRESSO_LATTE_BREW_TIME;
                }

                turn_on_red_led();

                vTaskDelay(90 / portTICK_RATE_MS);
                time_left_brew -= 100;

                if (time_left_brew <= 0)
                {
                    turn_off_led();
                    time_left_brew = 0;

                    if (product_msg.product == ESPRESSO)
                    {
                        message_for_controller = 1;
                        xQueueSend(led_to_controller_q, &message_for_controller, pdMS_TO_TICKS(10));
                        STATE = IDLE;
                    }
                    else if (product_msg.product == LATTE)
                    {
                        STATE = FROTH_MILK;
                    }
                }

                break;
            }

            case FROTH_MILK:                            // Froth the milk for a fixed amount of time. The green LED is on during frothing. After frothing, transition to IDLE.
            {
                if (time_left_froth == 0)
                {
                    time_left_froth = LATTE_FROTH_TIME;
                }

                turn_on_green_led();

                vTaskDelay(90 / portTICK_RATE_MS);
                time_left_froth -= 100;

                if (time_left_froth <= 0)               // After frothing, transition to IDLE
                {
                    turn_off_led();
                    time_left_froth = 0;
                    message_for_controller = 1;
                    xQueueSend(led_to_controller_q, &message_for_controller, pdMS_TO_TICKS(10));
                    STATE = IDLE;
                }

                break;
            }

            default:
                // runs if none match
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Remember that this timing needs to be fit into the state machine
    }
}


/****************************** End Of Module *******************************/
