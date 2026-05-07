/*****************************************************************************
* University of Southern Denmark
* Embedded Programming (EMP)
*
* MODULENAME.: encoder.c
*
* PROJECT....: EMP
*
* DESCRIPTION: See module specification file (.h-file).
*
* Change Log:
*****************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 150321  MoH   Module created.
*
*****************************************************************************/

/***************************** Include files *******************************/
// for freeRTOS
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "emp_type.h"
#include "tmodel.h"
#include "FreeRTOS.h"
#include "queue.h"

// Own includes
#include "encoder.h"

using namespace std;

/*****************************    Defines    *******************************/
#define QUEUE_LEN 128

extern QueueHandle_t encoder_queue;

#define DIGI_A   0x20   // PA5
#define DIGI_B   0x40   // PA6
#define DIGI_P2  0x80   // PA7

#define IDLE               0
#define SEND_TO_LCD        1
#define SEND_FINAL_AMOUNT  2
/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

INT8U Encoder_getA( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function :
******************************************************************************/
{
    return ( GPIO_PORTA_DATA_R & DIGI_A ) != 0;         // If result != 0 -> returns 1 (true)    -     else return 0 (false)      -     Is PA5 High
}

INT8U Encoder_getB( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function :
******************************************************************************/
{
    return ( GPIO_PORTA_DATA_R & DIGI_B ) != 0;         // If result != 0 -> returns 1 (true)    -     else return 0 (false)      -     Is PA6 High
}

INT8U Encoder_getButton( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function :
******************************************************************************/
{
    return ( GPIO_PORTA_DATA_R & DIGI_P2 ) == 0;
}

INT8U Encoder_readA( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function :
******************************************************************************/
{
    INT8U value_A = Encoder_getA();
    return ( xQueueSend( encoder_queue, &value_A, portMAX_DELAY ) );        // Send value to queue
}

INT8U Encoder_readB( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function :
******************************************************************************/
{
    INT8U value_B = Encoder_getB();
    return( xQueueSend( encoder_queue, &value_B, portMAX_DELAY ) );         // Send value to queue
}

INT8U Encoder_readButton( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function :
******************************************************************************/
{

    INT8U value_button = Encoder_getButton();
    return( xQueueSend( encoder_queue, &value_button, portMAX_DELAY ) );    // Send value to queue
}

void Encoder_init( void )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function :
******************************************************************************/
{
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA;

    GPIO_PORTA_DIR_R &= ~( DIGI_A | DIGI_B | DIGI_P2 );
    GPIO_PORTA_DEN_R |=  ( DIGI_A | DIGI_B | DIGI_P2 );
    GPIO_PORTA_PUR_R |=  ( DIGI_A | DIGI_B | DIGI_P2 );
}

extern void encoder_task( void *pvParameters )
/*****************************************************************************
*   Input    :
*   Output   :
*   Function :
******************************************************************************/
{
    INT8U state = IDLE;
    INT8U lastA = Encoder_getA();
    INT16S amount = 0;

    while( 1 )
    {
        INT8U A = Encoder_getA();
        INT8U B = Encoder_getB();
        INT8U button = Encoder_getButton();

        switch( state )
        {
            case IDLE:
            {
                if( A != lastA )
                {
                    if( A == B )
                    {
                        amount++;
                    }
                    else
                    {
                        amount--;
                    }

                    state = SEND_TO_LCD;
                    lastA = A;
                }

                if( button == 1 )
                {
                    state = SEND_FINAL_AMOUNT;
                }
                break;
            }

            case SEND_TO_LCD:
            {
                xQueueSend( encoder_queue, &amount, portMAX_DELAY );
                state = IDLE;
                break;
            }

            case SEND_FINAL_AMOUNT:
            {
                xQueueSend( encoder_queue, &amount, portMAX_DELAY );
                state = IDLE;
                break;
            }
        }

        vTaskDelay( pdMS_TO_TICKS( 10 ) );
    }
}