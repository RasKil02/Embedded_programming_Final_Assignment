/*****************************************************************************
* University of Southern Denmark
* Embedded Programming 
*
* MODULENAME.: encoder.c
*
* PROJECT....: Final Assignment - Embedded Programming
*
* DESCRIPTION: Driver for the rotary encoder. Reads the state of the encoder and sends it to a queue.
*
* Change Log:
*****************************************************************************
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
#include "emp_type.h"
#include "FreeRTOS.h"
#include "queue.h"

// Own includes
#include "encoder.h"


/*****************************    Defines    *******************************/
#define QUEUE_LEN 128

extern QueueHandle_t encoder_queue;
extern QueueHandle_t encoder_button_queue;
#define DIGI_A   0x20   // PA5
#define DIGI_B   0x40   // PA6
#define DIGI_P2  0x80   // PA7

#define IDLE               0
#define SEND_TO_LCD        1
#define SEND_FINAL_AMOUNT  2
/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/
INT16S amount = 0;

/*****************************   Functions   *******************************/

INT8U Encoder_getA( void )
/*****************************************************************************
*   Input    : - 
*   Output   : - 
*   Function : - Get the state of the encoder pin A (PA5)
******************************************************************************/
{
    return ( GPIO_PORTA_DATA_R & DIGI_A ) != 0;         // If result != 0 -> returns 1 (true)    -     else return 0 (false)      -     Is PA5 High
}

INT8U Encoder_getB( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function : - Get the state of the encoder pin B (PA6)
******************************************************************************/
{
    return ( GPIO_PORTA_DATA_R & DIGI_B ) != 0;         // If result != 0 -> returns 1 (true)    -     else return 0 (false)      -     Is PA6 High
}

INT8U Encoder_getButton( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function : - Get the state of the encoder button (PA7)
******************************************************************************/
{
    return ( GPIO_PORTA_DATA_R & DIGI_P2 ) == 0;
}

INT8U Encoder_readA( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function : - Read the state of the encoder pin A and send it to a queue
******************************************************************************/
{
    INT8U value_A = Encoder_getA();
    return ( xQueueSend( encoder_queue, &value_A, portMAX_DELAY ) );        // Send value to encoder_queue
}

INT8U Encoder_readB( void )
/*****************************************************************************
*   Input    : - 
*   Output   :
*   Function : - Read the state of the encoder pin B and send it to a queue
******************************************************************************/
{
    INT8U value_B = Encoder_getB();
    return( xQueueSend( encoder_queue, &value_B, portMAX_DELAY ) );         // Send value to encoder_queue
}

INT8U Encoder_readButton( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function : - Read the state of the encoder button and send it to a queue
******************************************************************************/
{

    INT8U value_button = Encoder_getButton();
    return( xQueueSend( encoder_button_queue, &value_button, 0 ) );         // Send value to encoder_button_queue
}

void Encoder_init( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function : - Initialize the GPIO pins for the encoder (PA5, PA6, PA7) as inputs with pull-up resistors
******************************************************************************/
{
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA;

    GPIO_PORTA_DIR_R &= ~( DIGI_A | DIGI_B | DIGI_P2 );                     
    GPIO_PORTA_DEN_R |=  ( DIGI_A | DIGI_B | DIGI_P2 );                  
    GPIO_PORTA_PUR_R |=  ( DIGI_A | DIGI_B | DIGI_P2 );
}

extern void encoder_task( void *pvParameters )
/*****************************************************************************
*   Input    : - pvParameters: Pointer to parameters for the task 
*   Output   : - 
*   Function : - Task function for reading the encoder state and sending it to a queue.
******************************************************************************/
{
    INT8U state = IDLE;
    INT8U lastA = Encoder_getA();

    while( 1 )
    {
        INT8U A = Encoder_getA();
        INT8U B = Encoder_getB();

        switch( state )
        {   
            case IDLE:                                                  // Wait for a change in the state of pin A
            {
                if( A != lastA )
                {
                    if( B == A )
                    {
                        amount += 5;
                    }
                    else
                    {
                        amount += 20;
                    }

                    state = SEND_TO_LCD;
                    lastA = A;
                }
                break;
            }

            case SEND_TO_LCD:                                           // Send the amount to the LCD queue     
            {
                xQueueSend( encoder_queue, &amount, portMAX_DELAY );    // Send the amount to the encoder_queue
                state = IDLE;
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));                                
    }
}
