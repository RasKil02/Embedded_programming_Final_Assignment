/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: leds.c
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
* 050128  KA    Module created.
*
*****************************************************************************/

/***************************** Include files *******************************/
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "Task.h"
#include "queue.h"
#include "semphr.h"
#include "emp_type.h"
//#include "glob_def.h"
//#include "status_led.h"


/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

void red_led_init(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :
*****************************************************************************/
{
  INT8S dummy;
  // Enable the GPIO port that is used for the on-board LED.
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;

  // Do a dummy read to insert a few cycles after enabling the peripheral.
  dummy = SYSCTL_RCGC2_R;

  GPIO_PORTF_DIR_R |= 0x02;
  GPIO_PORTF_DEN_R |= 0x02;
}

void yellow_led_init(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :
*****************************************************************************/
{
  INT8S dummy;
  // Enable the GPIO port that is used for the on-board LED.
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;

  // Do a dummy read to insert a few cycles after enabling the peripheral.
  dummy = SYSCTL_RCGC2_R;

  GPIO_PORTF_DIR_R |= 0x04;
  GPIO_PORTF_DEN_R |= 0x04;
}

void green_led_init(void)
/*****************************************************************************
*   Input    :  -
*   Output   :  -
*   Function :
*****************************************************************************/
{
  INT8S dummy;
  // Enable the GPIO port that is used for the on-board LED.
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;

  // Do a dummy read to insert a few cycles after enabling the peripheral.
  dummy = SYSCTL_RCGC2_R;

  GPIO_PORTF_DIR_R |= 0x08;
  GPIO_PORTF_DEN_R |= 0x08;
}


void red_led_task(void *pvParameters)
{
  INT16U adc_value;
  portTickType delay;

  red_led_init();

  while(1)
  {
    // Toggle red led
    GPIO_PORTF_DATA_R ^= 0x02;
    adc_value = get_adc();
    delay = 1000 - adc_value / 5;
    vTaskDelay( delay / portTICK_RATE_MS); // wait 100-1000 ms. (200-1000)
  }
}

void yellow_led_task(void *pvParameters)
{
  yellow_led_init();

  while(1)
  {
    // Toggle yellow led
    GPIO_PORTF_DATA_R ^= 0x04;
    vTaskDelay( 1000 / portTICK_RATE_MS); // wait 1000 ms.
  }
}

void green_led_task(void *pvParameters)
{
  green_led_init();

  while(1)
  {
    // Toggle green led
    GPIO_PORTF_DATA_R ^= 0x08;
    vTaskDelay( 1500 / portTICK_RATE_MS); // wait 1500 ms.
  }
}


/****************************** End Of Module *******************************/




