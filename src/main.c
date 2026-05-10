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
#define USERTASK_STACK_SIZE 120 
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
QueueHandle_t encoder_button_queue;
QueueHandle_t controller_queue;
QueueHandle_t led_to_controller_q;

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

/***************************    Functions     **********************************/
void init_gpio(void)
/*****************************************************************************
*   Input    :
*   Output   :
*   Function : The super loop.
******************************************************************************/
{
  int dummy;

  // Enable the GPIO port that is used for the on-board LED.
  SYSCTL_RCGC2_R  =  SYSCTL_RCGC2_GPIOA |SYSCTL_RCGC2_GPIOC | SYSCTL_RCGC2_GPIOD | SYSCTL_RCGC2_GPIOE |SYSCTL_RCGC2_GPIOF;
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART0;

  // Do a dummy read to insert a few cycles after enabling the peripheral.
  dummy = SYSCTL_RCGC2_R;

  // Unlock the GPIO Pin which enables write access to the GPIO
  GPIO_PORTF_LOCK_R=0x4C4F434B;

  // Commit register (GPIOAFSEL, GPIOPUR, GPIOPDR, GPIODEN)
  GPIO_PORTF_CR_R=0xFF;

  // Set the direction as output (PF1, PF2 and PF3).
  GPIO_PORTA_DIR_R = 0x1C;
  GPIO_PORTC_DIR_R = 0xF0;
  GPIO_PORTD_DIR_R = 0x4C;
  GPIO_PORTF_DIR_R = 0x0E;

  // Enable the GPIO pins for digital function (PF0, PF1, PF2, PF3, PF4).
  GPIO_PORTA_DEN_R = 0x1C;
  GPIO_PORTC_DEN_R = 0xF0;
  GPIO_PORTD_DEN_R = 0x4C;
  GPIO_PORTE_DEN_R = 0x0F;
  GPIO_PORTF_DEN_R = 0x1F;

  // Enable internal pull-up (PF0 and PF4).
  GPIO_PORTF_PUR_R = 0x11;
}

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
  init_gpio();
  Encoder_init();
  uart0_init(19200, 8, 1, 0);
}

int main(void)
{
    setupHardware();

    lcd_queue = xQueueCreate(10, sizeof(lcd_msg_t));
    key_queue =  xQueueCreate( 10, sizeof( INT8U ) );
    change_q = xQueueCreate(10, sizeof(INT8U));
    purchased_products_q = xQueueCreate(10, sizeof(product_msg_t));
    time_q = xQueueCreate(10, sizeof(int));
    encoder_queue = xQueueCreate( 10, sizeof( INT16S ) );
    encoder_button_queue = xQueueCreate( 10, sizeof( INT8U ) );
    controller_queue = xQueueCreate(10, sizeof( INT8U ));
    led_to_controller_q = xQueueCreate(10, sizeof( INT8U ));
    uart_queue_handler = xQueueCreate( 10, sizeof( INT8U ) );

    xTaskCreate( key_task, "Keyboard_task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
    xTaskCreate( LED_task, "LED task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate( lcd_task, "LCD task", 512, NULL, LOW_PRIO, NULL );
    xTaskCreate( encoder_task, "encoder task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate( controller_task, "controller task", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL);
    xTaskCreate( uart_tx_task, "UART_tx", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );
    xTaskCreate( uart_rx_task, "UART_rx", USERTASK_STACK_SIZE, NULL, LOW_PRIO, NULL );

    vTaskStartScheduler();
    return 0;
}
