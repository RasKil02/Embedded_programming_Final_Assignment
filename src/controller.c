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
/*****************************    Includes    ******************************/
// For freeRTOS
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "emp_type.h"

// Own includes
#include "controller.h"
#include "LED_task.h"
#include "Keypad.h"
#include "lcd.h"
#include "encoder.h"
#include "uart.h"

// Data type
#include <string.h>

/*****************************    Defines    *******************************/
extern QueueHandle_t key_queue;
extern QueueHandle_t uart_queue_handler;
extern QueueHandle_t encoder_queue;
extern QueueHandle_t lcd_queue;
extern QueueHandle_t change_q;
extern QueueHandle_t purchased_products_q;
extern QueueHandle_t time_q;
extern QueueHandle_t controller_queue;

typedef enum {
    PROGRAM_START,
    C_IDLE,
    C_DISPLAY_PAYMENT_OPTIONS,
    C_DETERMINE_PAYMENT_METHOD,
    C_DISPLAY_AMOUNT_INSERTED,
    C_RETURNING_CASH,
    C_RETURN_CHANGE_LED,
} controller_state_t;

typedef struct {
    char coffee_type[50];
    int price;
    int amount;
    int time_of_day;
    int payment_type;
    int card_number;
} uart_product_t;

typedef enum
{ 
    LCD_IDLE,
    LCD_DISPLAY_CASH_OR_CARD,
    LCD_DISPLAY_CHOICE,
    LCD_DISPLAY_ENTER_CASH_INFO,
    LCD_DISPLAY_CASH_AMOUNT,
    LCD_DISPLAY_ENTER_CARD_NUMBER_AND_PIN,
    LCD_DISPLAY_CHOICE_IS_BEING_PRODUCED,
    LCD_DISPLAY_CHOICE_PRODUCED,
    LCD_RETURN_CASH,
} lcd_states;

typedef struct {
    lcd_states cmd;
    int value;
} lcd_msg_t;

/*****************************   Constants   *******************************/
#define INITIAL_BREWING_RATE  0.6       // price pr. cl
#define INCREASED_BREWING_RATE 1.45     // price pr. cl after 3 minutes
#define INITIAL_BREWING_TIME_MS 3000    // time in ms where the brewing rate changes¨
#define INITIAL_BREWING_TIME_S 3        // time in minutes where the brewing rate changes
#define MS_TO_S 1000
#define STANDARD_COFFEE_AMOUNT 1        // standard amount of coffee in cl for espresso and latte

/*****************************   Variables   ***********************/
const INT8U espresso = 15;
const INT8U latte = 27;
const INT8U filter = 3;

void controller_task(void *pvParameters)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    lcd_msg_t msg;
    controller_state_t state;

    INT8U user_choice;
    INT8U change;
    INT8U change_for_return = 0;

    // Initial state
    state = PROGRAM_START;

    while(1)
    {
        switch(state)
        {   case PROGRAM_START:
            {
                // Display welcome message and ask user to choose coffee
                msg.cmd = LCD_IDLE;
                msg.value = 0;
                xQueueSend(lcd_queue, &msg, portMAX_DELAY);
                state = C_IDLE;
                break;
            }
            
            case C_IDLE:
            {
                if (xQueueReceive(key_queue, &user_choice, 10 / portTICK_PERIOD_MS))
                {
                    msg.cmd = LCD_DISPLAY_CHOICE;
                    msg.value = user_choice;
                    xQueueSend(lcd_queue, &msg, portMAX_DELAY);
                    
                    vTaskDelay(2000 / portTICK_PERIOD_MS);

                    state = C_DISPLAY_PAYMENT_OPTIONS;
                }
                // Wait for user input
                break;
            }

            case C_DISPLAY_PAYMENT_OPTIONS :
            {
                msg.cmd = LCD_DISPLAY_CASH_OR_CARD;
                msg.value = 0;
                xQueueSend(lcd_queue, &msg, portMAX_DELAY);

                state = C_DETERMINE_PAYMENT_METHOD;
                break;
            }

            case C_DETERMINE_PAYMENT_METHOD :
            {
                if (xQueueReceive(key_queue, &user_choice, 10 / portTICK_PERIOD_MS))
                {
                    if (user_choice == '1') // Cash
                    {
                        msg.cmd = LCD_DISPLAY_ENTER_CASH_INFO;
                        msg.value = 0;
                        xQueueSend(lcd_queue, &msg, portMAX_DELAY);
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                        state = C_DISPLAY_AMOUNT_INSERTED;
                    }
                    else if (user_choice == '2') // Card
                    {
                        // handle later
                    }
                }
                break;
            }

            case C_DISPLAY_AMOUNT_INSERTED :
            {
                msg.cmd = LCD_DISPLAY_CASH_AMOUNT;
                msg.value = user_choice;

                xQueueSend(lcd_queue, &msg, pdMS_TO_TICKS(10));

                if (xQueueReceive(controller_queue, &change, pdMS_TO_TICKS(10))) 
                {
                    change_for_return = change;
                    state = C_RETURNING_CASH;
                }
                break;
            }
            
            case C_RETURNING_CASH :
            {
                msg.cmd = LCD_RETURN_CASH;
                msg.value = 0;
                xQueueSend(lcd_queue, &msg, pdMS_TO_TICKS(10));
                state = C_RETURN_CHANGE_LED;
                break;
            }
            
            case C_RETURN_CHANGE_LED : 
            {
                xQueueSend(change_q, &change_for_return, 10 / portTICK_RATE_MS);
                break;
            }

        }

    vTaskDelay(10 / portTICK_RATE_MS);

    }

}
