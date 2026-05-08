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

// Data stype
#include <string.h>

/*****************************    Defines    *******************************/

extern QueueHandle_t key_queue;
extern QueueHandle_t uart_queue_handler;
extern QueueHandle_t encoder_queue;
extern QueueHandle_t lcd_queue;
extern QueueHandle_t change_q;
extern QueueHandle_t purchased_products_q;
extern QueueHandle_t time_q;

typedef enum {
    C_IDLE,
    C_PAYMENT,
    C_CHANGE_PRODUCT_PRICE,
    C_RECEIVING_CASH,
    C_RETURN_CASH,
    C_PRODUCE_CHOICE,
    C_LOG_PRODUCT_CHOICE,
    C_WAIT_FOR_CARD,
} controller_state_t;

typedef struct {
    Char coffee_type[50];
    int price;
    int amount;
    int time_of_day;
    int payment_type;
    int card_number;
} uart_product_t;

typedef struct {
    lcd_states cmd;
    int value;
} lcd_msg_t;

typedef enum
{
  LCD_IDLE,
  LCD_DISPLAY_CASH_OR_CARD,
  LCD_DISPLAY_ENTER_CARD_NUMBER_AND_PIN,
  LCD_RETURN_CASH,
  LCD_DISPLAY_CHOICE,
  LCD_DISPLAY_CHOICE_IS_BEING_PRODUCED,
  LCD_DISPLAY_CHOICE_PRODUCED,
} lcd_states;

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


/*****************************   Functions   *******************************/

void controller_task(void *pvParameters)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :  -
*****************************************************************************/
{
    controller_state_t STATE = C_IDLE;
    lcd_msg_t msg;
    uart_product_t uart_product;
    INT8U input;
    INT8U data;
    INT8U money;
    INT8U price;
    INT8U amount_coffee;
    INT8U time_spent_brewing;
    static INT8U change_price = 0;
    INT8U card_number;
    INT8U PIN;

    while(1)
    {
        switch(STATE)
        {
            case C_IDLE :
            {

                if (xQueueReceive(key_queue, &input, 0))
                {
                    STATE = C_PAYMENT;
                }

                if (xQueueReceive(uart_queue_handler, &data, 0))
                {
                    STATE = C_CHANGE_PRODUCT_PRICE;
                }

                break;
            }

            case C_CHANGE_PRODUCT_PRICE :
            {
                // Dont know what to put here
                break;
            }

            case C_PAYMENT :
            {
                msg.cmd = LCD_DISPLAY_CHOICE;
                msg.value = input;

                xQueueSend(lcd_queue, &msg, 0);   // Send choice of coffee to LCD task

                if (xQueueReceive(key_queue, &input, 0))
                {
                    if (input == '1')
                    {
                        STATE = C_RECEIVING_CASH;
                    }

                    if (input == '2')
                    {
                        STATE = C_WAIT_FOR_CARD;
                    }
                }
                break;
            }

            case C_RECEIVING_CASH :
            {
                if (xQueueReceive(key_queue, &input, 0))
                {
                    if (input == '1')
                    {
                        price = espresso;
                    }

                    if (input == '2')
                    {
                        price = latte;
                    }

                    if (input == '3')
                    {
                        price = filter;
                    }
                }

                if (xQueueReceive(encoder_queue, &money, 0))
                {
                    if (money >= price)
                    {
                        change_price = money - price;
                        STATE = C_RETURN_CASH;
                    }
                }
                break;
            }

            case C_RETURN_CASH :
            {
                msg.cmd = LCD_RETURN_CASH;
                msg.value = change_price;

                xQueueSend(lcd_queue, &msg, 0);
                xQueueSend(change_q, &change_price, 0); // Send change to LED task

                STATE = C_PRODUCE_CHOICE;

                break;
            }

            case C_PRODUCE_CHOICE :
            {
                xQueueSend(purchased_products_q, &input, 0); // Send product choice to LED task
                STATE = C_LOG_PRODUCT_CHOICE;
                break;
            }

            case C_WAIT_FOR_CARD :
            {
                xQueueReceive(key_queue, &card_number, 0);  // Receive card number from UART task
                xQueueReceive(key_queue, &PIN, 0);        // Receive PIN from UART task

                if (card_number % 2 == 1 || PIN % 2 == 0)
                {
                    // Card rejected
                    STATE = C_IDLE;
                }
                if (card_number % 2 == 0 || PIN % 2 == 1)
                {
                    // Card rejected
                    STATE = C_IDLE;
                }
                else
                {
                    // Card accepted
                    msg.cmd = LCD_DISPLAY_CHOICE;
                    msg.value = input;

                    xQueueSend(lcd_queue, &msg, 0);   // Send choice of coffee to LCD task
                    STATE = C_PRODUCE_CHOICE;
                }

                break;
            }

            case C_LOG_PRODUCT_CHOICE :
            {
                if (uart_product.price == espresso)
                {
                    strcpy(uart_product.coffe_type, "Espresso");
                    uart_product.amount = STANDARD_COFFEE_AMOUNT;
                    uart_product.price = price;
                    uart_product.time_of_day = 0;
                    uart_product.payment_type = input; // This is 1 or 2, logic will be handled in UART task
                }
                else if (uart_product.price == latte)
                {
                    strcpy(uart_product.coffe_type, "Latte");
                    uart_product.amount = STANDARD_COFFEE_AMOUNT;
                    uart_product.price = price;
                    uart_product.time_of_day = 0;
                    uart_product.payment_type = input; // This is 1 or 2, logic will be handled in UART task
                }
                else if (uart_product.price == filter)
                {
                    strcpy(uart_product.coffe_type, "Filter");
                    xQueueReceive(time_q, &time_spent_brewing, 0); // Receive time of day from LED task

                    if (time_spent_brewing < INITIAL_BREWING_TIME_MS)
                    {
                        amount_coffee = INITIAL_BREWING_RATE * time_spent_brewing / MS_TO_S;
                        uart_product.amount = amount_coffee;
                    }
                    else
                    {
                        amount_coffee = INITIAL_BREWING_RATE * INITIAL_BREWING_TIME_S + INCREASED_BREWING_RATE * (time_spent_brewing - INITIAL_BREWING_TIME_MS) / MS_TO_S;
                        uart_product.amount = amount_coffee;
                    }

                    uart_product.price = amount_coffee * filter;
                    uart_product.time_of_day = 0;
                    uart_product.payment_type = input; // This is 1 or 2, logic will be handled in UART task

                    if (input == '1') // cash
                    {
                        uart_product.card_number = 0;
                    }

                    if (input == '2') // card payment
                    {
                        uart_product.card_number = card_number;
                    }
                }

                xQueueSend(uart_queue_handler, &uart_product, 0); // Send product choice to UART task
                STATE = C_IDLE;
                break;
            }

            default:
                // runs if none match
                break;
        }

        vTaskDelay(10 / portTICK_RATE_MS);

    }
}


