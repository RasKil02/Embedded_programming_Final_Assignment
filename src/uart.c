/*****************************************************************************
* University of Southern Denmark
* Embedded Programming
*
* MODULENAME.: emp.c
*
* PROJECT....: Final Assignment - Embedded Programming
*
* DESCRIPTION: This file implements a UART driver for FreeRTOS.
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
#include "systick_frt.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Own includes
#include "uart.h"

/*****************************    Defines    *******************************/
#define QUEUE_LEN 128

extern QueueHandle_t uart_queue_handler;

/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

BOOLEAN uart0_put_q( INT8U ch )
/*****************************************************************************
*   Input    :  - ch: The character to be sent via UART.
*   Output   :  - 
*   Function :  - This function sends a character to the UART transmit queue. It will block until there is space in the queue.
*****************************************************************************/
{
    xQueueSend( uart_queue_handler, &ch, portMAX_DELAY );
    return( 1 );
}

BOOLEAN uart0_get_q( INT8U *pch )
/*****************************************************************************
*   Input    :  - pch: Pointer to a variable where the received character will be stored.
*   Output   :  - 
*   Function :  - This function receives a character from the UART receive queue. It will block until there is a character available in the queue.
*****************************************************************************/
{
    return( xQueueReceive( uart_queue_handler, pch, portMAX_DELAY ) );
}

BOOLEAN uart0_rx_rdy()
{
    return !(UART0_FR_R & UART_FR_RXFE);
}

BOOLEAN uart0_tx_rdy()
/*****************************************************************************
*   Input    :  - 
*   Output   :  - 
*   Function :  - This function checks if the UART transmit buffer is empty and ready to accept a new character. It returns true if the buffer is empty, and false otherwise.
*****************************************************************************/
{
  return( UART0_FR_R & UART_FR_TXFE );
}

void uart0_putc( INT8U ch )
/*****************************************************************************
*   Input    :  - ch: The character to be sent via UART.
*   Output   :  -  
*   Function :  - This function sends a character directly to the UART data register. 
*****************************************************************************/
{
  UART0_DR_R = ch;
}

void uart0_puts(char *str)
{
    while(*str)
    {
        while(!uart0_tx_rdy())
        {
        }

        uart0_putc(*str);
        str++;
    }
}

INT32U lcrh_databits( INT8U number_of_databits )
/*****************************************************************************
*   Input    :  - number_of_databits: The desired number of data bits (5, 6, 7, or 8).
*   Output   :
*   Function : sets bit 5 and 6 according to the wanted number of data bits.
*               5: bit5 = 0, bit6 = 0.
*               6: bit5 = 1, bit6 = 0.
*               7: bit5 = 0, bit6 = 1.
*               8: bit5 = 1, bit6 = 1  (default).
*              all other bits are returned = 0
******************************************************************************/
{
  if(( number_of_databits < 5 ) || ( number_of_databits > 8 ))
    number_of_databits = 8;
  return(( (INT32U)number_of_databits - 5 ) << 5 );  // Control bit 5-6, WLEN
}

INT32U lcrh_stopbits( INT8U number_of_stopbits )
/*****************************************************************************
*   Input    : - number_of_stopbits: The desired number of stop bits (1 or 2).
*   Output   :
*   Function : sets bit 3 according to the wanted number of stop bits.
*               1 stpobit:  bit3 = 0 (default).
*               2 stopbits: bit3 = 1.
*              all other bits are returned = 0
******************************************************************************/
{
  if( number_of_stopbits == 2 )
    return( 0x00000008 );       // return bit 3 = 1
  else
    return( 0x00000000 );       // return all zeros
}

INT32U lcrh_parity( INT8U parity )
/*****************************************************************************
*   Input    : - parity: The desired parity mode ('e' for even, 'o' for odd, '0' for mark, '1' for space, 'n' for none).
*   Output   :
*   Function : sets bit 1, 2 and 7 to the wanted parity.
*               'e':  00000110b.
*               'o':  00000010b.
*               '0':  10000110b.
*               '1':  10000010b.
*               'n':  00000000b.
*              all other bits are returned = 0
******************************************************************************/
{
  INT32U result;

  switch( parity )
  {
    case 'e':
      result = 0x00000006;
      break;
    case 'o':
      result = 0x00000002;
      break;
    case '0':
      result = 0x00000086;
      break;
    case '1':
      result = 0x00000082;
      break;
    case 'n':
    default:
      result = 0x00000000;
  }
  return( result );
}

void uart0_fifos_enable()
/*****************************************************************************
*   Input    :
*   Output   :
*   Function : Enable the tx and rx fifos
******************************************************************************/
{
  UART0_LCRH_R  |= 0x00000010;
}

void uart0_fifos_disable()
/*****************************************************************************
*   Input    :
*   Output   :
*   Function : Enable the tx and rx fifos
******************************************************************************/
{
  UART0_LCRH_R  &= 0xFFFFFFEF;
}

extern void uart0_init( INT32U baud_rate, INT8U databits, INT8U stopbits, INT8U parity )
/*****************************************************************************
*   Input    : - baud_rate: The desired baud rate for UART communication.
*               - databits: The desired number of data bits.
*               - stopbits: The desired number of stop bits.
*               - parity: The desired parity mode.
*   Output   :
*   Function : - This function initializes the UART0 module with the specified baud rate, number of data bits, stop bits, and parity. 
******************************************************************************/
{
  INT32U BRD;

  #ifndef E_PORTA
  #define E_PORTA
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA;                 // Enable clock for Port A
  #endif

  #ifndef E_UART0
  #define E_UART0
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART0;                 // Enable clock for UART 0
  #endif

  GPIO_PORTA_AFSEL_R |= 0x00000003;
  GPIO_PORTA_DIR_R   |= 0x00000002;
  GPIO_PORTA_DEN_R   |= 0x00000003;
  GPIO_PORTA_PUR_R   |= 0x00000002;

  BRD = 64000000 / baud_rate;
  UART0_IBRD_R = BRD / 64;
  UART0_FBRD_R = BRD & 0x0000003F;

  UART0_LCRH_R  = lcrh_databits( databits );
  UART0_LCRH_R += lcrh_stopbits( stopbits );
  UART0_LCRH_R += lcrh_parity( parity );

  uart0_fifos_disable();

  UART0_CTL_R  |= (UART_CTL_UARTEN | UART_CTL_TXE );  // Enable UART
}

BOOLEAN uart0_getc(INT8U *data)
{
    if (!(UART0_FR_R & UART_FR_RXFE))
    {
        *data = (INT8U)(UART0_DR_R & 0xFF);
        return 1;
    }

    return 0;
}

extern void uart_rx_task(void *pvParameters)
{
/*****************************************************************************
*   Input    :
*   Output   :
*   Function : - This function continuously checks for incoming data on the UART. 
*                If data is available, it reads the character and sends it to a FreeRTOS queue for processing by other tasks.
******************************************************************************/
    INT8U ch;

    while(1)
    {
        if(uart0_getc(&ch))
        {
            xQueueSend(uart_queue_handler,
                       &ch,
                       portMAX_DELAY);
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

extern void uart_tx_task( void *pvParameters )
/*****************************************************************************
*   Input    : - pvParameters: Pointer to task parameters.
*   Output   :
*   Function : - This function continuously waits for characters to be sent to the UART transmit queue.
******************************************************************************/
{
    INT8U ch;

    while( 1 )
    {

        if( xQueueReceive( uart_queue_handler, &ch, portMAX_DELAY) == pdPASS)     // If possible to get data from queue
        {
            while( !uart0_tx_rdy() )    // checks if the uart buffer is empty and ready                                 // Wait as long UART is not ready to send
            {
                const TickType_t Delay = 1 / portTICK_PERIOD_MS;            // 1ms Delay
                vTaskDelay( Delay );
            }

            UART0_DR_R = ch;                                                // Writes to UART hardware
        }
    }
}

/****************************** End Of Module *******************************/

