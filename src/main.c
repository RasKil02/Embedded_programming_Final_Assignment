/**
 * main.c
 */

 /***************************    Includes     **********************************/
#include "FreeRTOS.h"
#include "tm4c123gh6pm.h"
#include "emp_type.h"
#include "task.h"
#include "systick_frt.h"

#include <stdint.h>

#include "LED_task.h"
#include "Keypad.h"
#include "controller.h"
#include "lcd.h"
#include "encoder.h"
#include "uart.h"

/***************************    Defines     **********************************/
#define USERTASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define IDLE_PRIO 0
#define LOW_PRIO  1
#define MED_PRIO  2
#define HIGH_PRIO 3

QueueHandle_t key_queue;
QueueHandle_t uart_queue_handler;
QueueHandle_t encoder_queue;
QueueHandle_t lcd_queue;
QueueHandle_t change_q;
QueueHandle_t purchased_products_q;
QueueHandle_t time_q;

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

=======
typedef enum {
    NO_PRODUCT = 0,
    ESPRESSO,           // 1
    LATTE,              // 2
    FILTER              // 3, C has automatically assigned 1,2 and 3.
} product_t;

/***************************    Functions     **********************************/
static void setupHardware(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :
*****************************************************************************/
{
  // TODO: Put hardware configuration and initialisation in here

  // Warning: If you do not initialize the hardware clock, the timings will be inaccurate
  init_systick();
  LED_init();
}

int main(void)
{
    setupHardware();

    key_queue =  xQueueCreate( 10, sizeof( INT8U ) ); // Is this correct?
    uart_queue_handler = xQueueCreate( 10, sizeof( INT8U ) );
    encoder_queue = xQueueCreate( 10, sizeof( INT8U ) );
    lcd_queue = xQueueCreate(10, sizeof(lcd_msg_t));
    change_q = xQueueCreate(10, sizeof(int));
    purchased_products_q = xQueueCreate(10, sizeof(product_t));
    time_q = xQueueCreate(10, sizeof(int));

    xTaskCreate( uart_tx_task, "UART_tx", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
    xTaskCreate( uart_rx_task, "UART_rx", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
    xTaskCreate( key_task, "Keyboard_task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
    xTaskCreate( controller_task, "controller task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate( LED_task, "LED task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate( lcd_task, "LCD task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate( encoder_task, "encoder task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL);

    vTaskStartScheduler();
    return 0;
}
