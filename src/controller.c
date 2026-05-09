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

typedef enum {
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
  LCD_DISPLAY_ENTER_CARD_NUMBER_AND_PIN,
  LCD_RETURN_CASH,
  LCD_DISPLAY_CHOICE,
  LCD_DISPLAY_CHOICE_IS_BEING_PRODUCED,
  LCD_DISPLAY_CHOICE_PRODUCED,
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

