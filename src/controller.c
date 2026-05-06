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
#include "LED.task.h"
#include "Keypad.h"
#include "lcd.h"
#include "encoder.h"
#include "uart.h"
#include <string.h>

/*****************************    Defines    *******************************/
typedef enum {
    IDLE,
    PAYMENT,
    CHANGE_PRODUCT_PRICE,
    RECEIVING_CASH,
    RETURN_CASH,
    PRODUCE_CHOICE,
    LOG_PRODUCT_CHOICE,
    WAIT_FOR_CARD,
} state_t;

typedef enum {
    LCD_DISPLAY_CHOICE,
    RETURN_CASH_LCD
} lcd_cmd_t;

typedef struct {
    lcd_cmd_t cmd;
    int value;
} lcd_msg_t;

typedef struct {
    string coffe_type,
    int price,
    int amount,
    int time_of_day,
    int payment_type,
    int card_number
} uart_product_t;

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
    state_t STATE = IDLE;
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
                msg.cmd = LCD_DISPLAY_CHOICE;
                msg.value = input;

                xQueueSend(lcd_queue, &msg, 0);   // Send choice of coffee to LCD task
                
                if (xQueueReceive(key_queue, &input, 0))
                {   
                    if (input == '1')
                    {
                        STATE = RECEIVING_CASH;
                    }
                    
                    if (input == '2')
                    {
                        STATE = WAIT_FOR_CARD;
                    }
                }
                break;
            }
            
            case RECEIVING_CASH :
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
                        STATE = RETURN_CASH;
                    }
                }
                break;
            }

            case RETURN_CASH :
            {
                msg.cmd = RETURN_CASH_LCD;
                msg.value = change_price;

                xQueueSend(lcd_queue, &msg, 0); 
                xQueueSend(change_q, &change_price, 0); // Send change to LED task

                STATE = PRODUCE_CHOICE;

                break;
            }

            case PRODUCE_CHOICE :
            {
                xQueueSend(purchased_products_q, &input, 0); // Send product choice to LED task
                STATE = LOG_PRODUCT_CHOICE;
                break;
            }

            case WAIT_FOR_CARD :
            {
                xQueueReceive(key_queue, &card_number, 0);  // Receive card number from UART task
                xQueueReceive(key_queue, &PIN, 0);        // Receive PIN from UART task

                if (card_number % 2 == 1 || PIN % 2 == 0)
                {
                    // Card rejected
                    STATE = IDLE;
                }
                if (card_number % 2 == 0 || PIN % 2 == 1)
                {
                    // Card rejected
                    STATE = IDLE;
                }
                else
                {
                    // Card accepted
                    msg.cmd = LCD_DISPLAY_CHOICE;
                    msg.value = input;

                    xQueueSend(lcd_queue, &msg, 0);   // Send choice of coffee to LCD task
                    STATE = PRODUCE_CHOICE;
                }

                break;
            }

            case LOG_PRODUCT_CHOICE :
            {
                if (uart_product.price == espresso)
                {
                    uart_product.coffe_type = "Espresso";
                    uart_product.amount = STANDARD_COFFEE_AMOUNT;
                    uart_product.price = price;
                    uart_product.time_of_day = 0;
                    uart_product.payment_type = input; // This is 1 or 2, logic will be handled in UART task
                }
                else if (uart_product.price == latte)
                {
                    uart_product.coffe_type = "Latte";
                    uart_product.amount = STANDARD_COFFEE_AMOUNT; 
                    uart_product.price = price;
                    uart_product.time_of_day = 0;
                    uart_product.payment_type = input; // This is 1 or 2, logic will be handled in UART task
                }
                else if (uart_product.price == filter)
                {
                    uart_product.coffe_type = "Filter";
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
                STATE = IDLE;
                break;
            }

            default:
                // runs if none match
                break;
        }

        vTaskDelay(10 / portTICK_RATE_MS);

    }
}


