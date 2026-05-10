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
#include <stdlib.h>

/*****************************    Defines    *******************************/
extern QueueHandle_t key_queue;
extern QueueHandle_t uart_queue_handler;
extern QueueHandle_t encoder_queue;
extern QueueHandle_t lcd_queue;
extern QueueHandle_t change_q;
extern QueueHandle_t purchased_products_q;
extern QueueHandle_t time_q;
extern QueueHandle_t controller_queue;
extern QueueHandle_t led_to_controller_q;

typedef enum {
    PROGRAM_START,
    C_INIT,
    C_UART,
    C_CHANGE_PRICE,
    C_SHOWCASE_NEW_PRICE,
    C_IDLE,
    C_DISPLAY_PAYMENT_OPTIONS,
    C_DETERMINE_PAYMENT_METHOD,
    C_DISPLAY_AMOUNT_INSERTED,
    C_RETURNING_CASH,
    C_RETURN_CHANGE_LED,
    C_ENTER_CARD_NUMBER_AND_PIN,
    C_SEND_WAIT_FOR_CUP,
    C_WAIT_FOR_CUP,
    C_DISPLAY_DISPENSING,
    C_PRODUCE_CHOICE,
    C_LISTEN_UNTIL_FINISHED,
    C_LISTEN_FOR_COFFEE_REMOVED,
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
    LCD_START_SCREEN,
    LCD_IDLE,
    LCD_UART_PRODUCT,
    LCD_UART_PRICE,
    LCD_SHOWCASE_NEW_PRICE,
    LCD_DISPLAY_CASH_OR_CARD,
    LCD_DISPLAY_CHOICE,
    LCD_DISPLAY_ENTER_CASH_INFO,
    LCD_DISPLAY_CASH_AMOUNT,
    LCD_DISPLAY_ENTER_CARD_NUMBER_AND_PIN,
    LCD_DISPLAY_CHOICE_IS_BEING_PRODUCED,
    LCD_DISPLAY_CHOICE_PRODUCED,
    LCD_RETURN_CASH,
    LCD_PLACE_CUP,
} lcd_states;

typedef struct {
    lcd_states cmd;
    int value;
    int value2;
} lcd_msg_t;

typedef enum {
    NO_PRODUCT = 0,
    ESPRESSO,           // 1
    LATTE,              // 2
    FILTER              // 3, C has automatically assigned 1,2 and 3.
} product_t;

typedef struct {
    product_t product;
    int prepaid_amount; // in kr.
} product_msg_t;

/*****************************   Constants   *******************************/
#define INITIAL_BREWING_RATE  0.6       // price pr. cl
#define INCREASED_BREWING_RATE 1.45     // price pr. cl after 3 minutes
#define INITIAL_BREWING_TIME_MS 3000    // time in ms where the brewing rate changes¨
#define INITIAL_BREWING_TIME_S 3        // time in minutes where the brewing rate changes
#define MS_TO_S 1000
#define STANDARD_COFFEE_AMOUNT 1        // standard amount of coffee in cl for espresso and latte

/*****************************   Variables   ***********************/
INT8U espresso = 15;
INT8U latte = 27;
INT8U filter = 3;

void controller_task(void *pvParameters)
{
    lcd_msg_t msg;
    controller_state_t state;
    product_msg_t order;

    INT8U user_choice;
    INT8U change;
    INT8U change_for_return = 0;
    INT8U cardNr;
    INT8U drink_chosen;
    INT8U message;
    INT8U uart_input;
    INT8U u_coffee_choice;
    INT8U u_new_price;

    // Initial state
    state = PROGRAM_START;

    while(1)
    {
        switch(state)
        {   case PROGRAM_START:
            {
                msg.cmd = LCD_START_SCREEN;
                msg.value = 0;
                xQueueSend(lcd_queue, &msg, portMAX_DELAY);

                state = C_INIT;
                break;
            }

            case C_INIT : 
            {
                if ((GPIO_PORTF_DATA_R & 0x10) == 0)
                {
                    msg.cmd = LCD_IDLE;
                    msg.value = 0;
                    xQueueSend(lcd_queue, &msg, portMAX_DELAY);
                    state = C_IDLE;
                }

                if ((GPIO_PORTF_DATA_R & 0x01) == 0)
                {
                    msg.cmd = LCD_UART_PRODUCT;
                    msg.value = 0;
                    xQueueSend(lcd_queue, &msg, portMAX_DELAY);
                    state = C_UART;
                }
                break;
            }

            case C_UART :
            {
                if (xQueueReceive(uart_queue_handler, &uart_input, pdMS_TO_TICKS(1)))
                {
                    if (uart_input == '1') 
                    {
                        u_coffee_choice = 1; // Espresso
                        state = C_CHANGE_PRICE;
                        msg.cmd = LCD_UART_PRICE;
                        msg.value = 0;
                        xQueueSend(lcd_queue, &msg, pdMS_TO_TICKS(10));
                        xQueueReset(uart_queue_handler);
                    }
                    if (uart_input == '2')
                    {
                        u_coffee_choice = 2; // Latte
                        state = C_CHANGE_PRICE;
                        msg.cmd = LCD_UART_PRICE;
                        msg.value = 0;
                        xQueueSend(lcd_queue, &msg, pdMS_TO_TICKS(10));
                        xQueueReset(uart_queue_handler);
                    }
                    if (uart_input == '3')
                    {
                        u_coffee_choice = 3; // Filter
                        state = C_CHANGE_PRICE;
                        msg.cmd = LCD_UART_PRICE;
                        msg.value = 0;
                        xQueueSend(lcd_queue, &msg, pdMS_TO_TICKS(10));
                        xQueueReset(uart_queue_handler);
                    }
                }
                break;
            }
            
            case C_CHANGE_PRICE :
            {
                static INT8U digit_count = 0;
                static int temp_price = 0;

                if (xQueueReceive(uart_queue_handler,
                                &uart_input,
                                pdMS_TO_TICKS(1)))
                {
                    // Ensure received char is digit
                    if (uart_input >= '0' && uart_input <= '9')
                    {
                        temp_price = temp_price * 10;
                        temp_price += (uart_input - '0');

                        digit_count++;
                    }

                    // Full 3-digit price received
                    if (digit_count >= 3)
                    {
                        u_new_price = temp_price;

                        if (u_coffee_choice == 1)
                        {
                            espresso = u_new_price;
                        }

                        if (u_coffee_choice == 2)
                        {
                            latte = u_new_price;
                        }

                        if (u_coffee_choice == 3)
                        {
                            filter = u_new_price;
                        }

                        // reset parser
                        digit_count = 0;
                        temp_price = 0;

                        state = C_SHOWCASE_NEW_PRICE;
                    }
                }

                break;
            }

            case C_SHOWCASE_NEW_PRICE :
            {
                msg.cmd = LCD_SHOWCASE_NEW_PRICE;
                xQueueSend(lcd_queue, &msg, 10 / portTICK_PERIOD_MS);
                state = PROGRAM_START;
                break;
            }

            case C_IDLE:
            {
                if (xQueueReceive(key_queue, &user_choice, portMAX_DELAY))
                {
                    drink_chosen = user_choice;
                    msg.cmd = LCD_DISPLAY_CHOICE;
                    msg.value = user_choice;
                    xQueueSend(lcd_queue, &msg, portMAX_DELAY);

                    vTaskDelay(2000 / portTICK_PERIOD_MS);

                    state = C_DISPLAY_PAYMENT_OPTIONS;
                }
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
                        msg.cmd = LCD_DISPLAY_ENTER_CARD_NUMBER_AND_PIN;
                        msg.value = 0;
                        xQueueSend(lcd_queue, &msg, portMAX_DELAY);
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                        state = C_ENTER_CARD_NUMBER_AND_PIN;
                    }
                }
                break;
            }

            case C_DISPLAY_AMOUNT_INSERTED :
            {
                msg.cmd = LCD_DISPLAY_CASH_AMOUNT;
                msg.value = user_choice;

                xQueueSend(lcd_queue, &msg, pdMS_TO_TICKS(1));

                if (xQueueReceive(controller_queue, &change, portMAX_DELAY))
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
                xQueueSend(lcd_queue, &msg, 10 / portTICK_RATE_MS);

                state = C_RETURN_CHANGE_LED;

                break;
            }


            case C_RETURN_CHANGE_LED :
            {
                xQueueSend(change_q, &change_for_return, pdMS_TO_TICKS(10));
                state = C_SEND_WAIT_FOR_CUP;
                break;
            }

            case C_ENTER_CARD_NUMBER_AND_PIN :
            {
                char card_buffer[21];
                char card_number[17];
                char pin[5];

                INT8U cardNr_and_PIN;
                INT8U index = 0;

                // Receive 20 digits total
                while(index < 20)
                {
                    if(xQueueReceive(key_queue,
                                    &cardNr_and_PIN,
                                    portMAX_DELAY))
                    {
                        card_buffer[index] = cardNr_and_PIN;
                        index++;
                    }
                }

                // Null terminate full buffer
                card_buffer[20] = '\0';

                // Extract first 16 digits -> card number
                memcpy(card_number, card_buffer, 16);
                card_number[16] = '\0';

                // Extract last 4 digits -> PIN
                memcpy(pin, &card_buffer[16], 4);
                pin[4] = '\0';

                // Determine even/odd
                // Card parity determined by LAST digit only
                int card_evenness = (card_number[15] - '0') % 2;

                // PIN parity
                int pin_evenness = atoi(pin) % 2;

                // Accept only if both are same
                if(card_evenness == pin_evenness)
                {
                    state = C_SEND_WAIT_FOR_CUP;
                }
                else
                {
                    // Stay in same state and retry
                    state = C_ENTER_CARD_NUMBER_AND_PIN;
                }

                break;
            }


            case C_SEND_WAIT_FOR_CUP :
            {
                msg.cmd = LCD_PLACE_CUP;
                msg.value = 0;
                xQueueSend(lcd_queue, &msg, pdMS_TO_TICKS(10));
                state = C_WAIT_FOR_CUP;
                break;
            }

            case C_WAIT_FOR_CUP :
            {
                if ((GPIO_PORTF_DATA_R & 0x10) == 0)
                {
                    state = C_DISPLAY_DISPENSING;
                }
                break;
            }

            case C_DISPLAY_DISPENSING :
            {
                msg.cmd = LCD_DISPLAY_CHOICE_IS_BEING_PRODUCED;
                msg.value = 0;
                xQueueSend(lcd_queue, &msg, pdMS_TO_TICKS(10));
                state = C_PRODUCE_CHOICE;
            }

            case C_PRODUCE_CHOICE :
            {
                if (drink_chosen == '1')
                {
                    order.product = ESPRESSO;
                }
                if (drink_chosen == '2')
                {
                    order.product = LATTE;
                }
                if (drink_chosen == '3')
                {
                    order.product = FILTER;
                }

                order.prepaid_amount = 30;
                xQueueSend(purchased_products_q, &order, pdMS_TO_TICKS(10));

                state = C_LISTEN_UNTIL_FINISHED;
                break;
            }

            case C_LISTEN_UNTIL_FINISHED :
            {
                if (xQueueReceive(led_to_controller_q, &message, pdMS_TO_TICKS(10)))
                {
                    msg.cmd = LCD_DISPLAY_CHOICE_PRODUCED;
                    msg.value = 0;
                    xQueueSend(lcd_queue, &msg, pdMS_TO_TICKS(10));
                    state = C_LISTEN_FOR_COFFEE_REMOVED;
                }
                break;
            }

            case C_LISTEN_FOR_COFFEE_REMOVED :
            {
                if ((GPIO_PORTF_DATA_R & 0x10) == 0)
                {
                    vTaskDelay(pdMS_TO_TICKS(200));
                    state = PROGRAM_START;
                }
            }

        }

    vTaskDelay(pdMS_TO_TICKS(1));

    }

}
