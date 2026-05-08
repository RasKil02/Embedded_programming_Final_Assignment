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
// For freeRTOS
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "emp_type.h"

// Own includes
#include "lcd.h"


/*****************************    Defines    *******************************/
#define QUEUE_LEN   128

extern QueueHandle_t lcd_queue;

#define FALSE 0
#define TRUE 1

typedef enum {
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
const INT8U LCD_init_sequense[]=
{
  0x30,     // Reset
  0x30,     // Reset
  0x30,     // Reset
  0x20,     // Set 4bit interface
  0x28,     // 2 lines Display
  0x0C,     // Display ON, Cursor OFF, Blink OFF
  0x06,     // Cursor Increment
  0x01,     // Clear Display
  0x02,   // Home
  0xFF      // stop
};

/*****************************   Variables   *******************************/
//INT8U LCD_buf[QUEUE_LEN];
//INT8U LCD_buf_head = 0;
//INT8U LCD_buf_tail = 0;
//INT8U LCD_buf_len  = 0;

INT8U LCD_init;



void move_LCD(INT8U x, INT8U y)
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : -
******************************************************************************/
{
    INT8U pos = (y == 0) ? (0x80 + x) : (0x80 + 0x40 + x);
    wr_ctrl_LCD(pos);
}

void wr_ctrl_LCD_low( INT8U Ch )
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Write low part of control data to LCD.
******************************************************************************/
{
  INT8U temp;
  volatile int i;

  temp = GPIO_PORTC_DATA_R & 0x0F;
  temp  = temp | ((Ch & 0x0F) << 4);
  GPIO_PORTC_DATA_R  = temp;
  for( i=0; i<1000; i )
      i++;
  GPIO_PORTD_DATA_R &= 0xFB;        // Select Control mode, write
  for( i=0; i<1000; i )
      i++;
  GPIO_PORTD_DATA_R |= 0x08;        // Set E High

  for( i=0; i<1000; i )
      i++;

  GPIO_PORTD_DATA_R &= 0xF7;        // Set E Low

  for( i=0; i<1000; i )
      i++;
}

void wr_ctrl_LCD_high( INT8U Ch )
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Write high part of control data to LCD.
******************************************************************************/
{
  wr_ctrl_LCD_low(( Ch & 0xF0 ) >> 4 );
}

void out_LCD_low( INT8U Ch )
/*****************************************************************************
*   Input    : Mask
*   Output   : -
*   Function : Send low part of character to LCD.
*              This function works only in 4 bit data mode.
******************************************************************************/
{
  INT8U temp;

  temp = GPIO_PORTC_DATA_R & 0x0F;
  GPIO_PORTC_DATA_R  = temp | ((Ch & 0x0F) << 4);
  //GPIO_PORTD_DATA_R &= 0x7F;        // Select write
  GPIO_PORTD_DATA_R |= 0x04;        // Select data mode
  GPIO_PORTD_DATA_R |= 0x08;        // Set E High
  GPIO_PORTD_DATA_R &= 0xF7;        // Set E Low
}

void out_LCD_high( INT8U Ch )
/*****************************************************************************
*   Input    : Mask
*   Output   : -
*   Function : Send high part of character to LCD.
*              This function works only in 4 bit data mode.
******************************************************************************/
{
  out_LCD_low((Ch & 0xF0) >> 4);
}

void wr_ctrl_LCD( INT8U Ch )
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Write control data to LCD.
******************************************************************************/
{
  static INT8U Mode4bit = FALSE;
  INT16U i;

  wr_ctrl_LCD_high( Ch );
  if( Mode4bit )
  {
    for(i=0; i<1000; i++);
    wr_ctrl_LCD_low( Ch );
  }
  else
  {
    if( (Ch & 0x30) == 0x20 )
      Mode4bit = TRUE;
  }
}

void clr_LCD()
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Clear LCD.
******************************************************************************/
{
  wr_ctrl_LCD( 0x01 );
}

void home_LCD()
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Return cursor to the home position.
******************************************************************************/
{
  wr_ctrl_LCD( 0x02 );
}

void Set_cursor( INT8U Ch )
/*****************************************************************************
*   Input    : New Cursor position
*   Output   : -
*   Function : Place cursor at given position.
******************************************************************************/
{
  wr_ctrl_LCD( Ch );
}

void out_LCD( INT8U Ch )
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Write control data to LCD.
******************************************************************************/
{
  INT16U i;

  out_LCD_high( Ch );
  for(i=0; i<1000; i++);
  out_LCD_low( Ch );
}

void lcd_init()
{
    LCD_init = 0;

    while(LCD_init_sequense[LCD_init] != 0xFF)
    {
        wr_ctrl_LCD(LCD_init_sequense[LCD_init++]);
        vTaskDelay(5 / portTICK_RATE_MS);
    }
}

void lcd_print(char *str)
{
  int pos1 = 0;
  int pos2 = 0;

    while(*str)
    {
        move_LCD(pos1, pos2);
        out_LCD(*str);
        str++;
        pos1++;
        if(pos1 > 15)
        {
            pos1 = 0;
            pos2++;
            if(pos2 > 1)
            {
                pos2 = 0;
            }
        }
    }
}

void slide_text(char *str)
{
    int offset = 0;
    while (1)
    {
        clr_LCD();
        home_LCD();

        move_LCD(offset, 0);   // flyt startposition
        lcd_print(str);        // print hele string

        vTaskDelay(3000 / portTICK_RATE_MS);

        offset++;

        if (offset > 15)   // LCD bredde (typisk 16)
        {
            offset = 0;
        }
    }
}

void lcd_task(void *pvParameters)
/*****************************************************************************
*   Input    :
*   Output   :
*   Function :
******************************************************************************/
{
  lcd_msg_t event;
  lcd_init();

  while(1)
  {
    if(xQueueReceive(lcd_queue, &event, portMAX_DELAY))
    {
      switch(event.cmd)
      {
        case LCD_IDLE :
        {
            clr_LCD();
            home_LCD();
            lcd_print("Choose Coffee:  1: E 2: L 3: F");
            break;
        }

        case LCD_DISPLAY_CASH_OR_CARD :
        {
            clr_LCD();
            home_LCD();
            lcd_print("Pay with:       1: Cash 2: Card"); // 15 char and then it switches lines
            break;
        }

        case LCD_DISPLAY_CHOICE :
        {
            clr_LCD();
            home_LCD();

            int choice = event.value; // 1, 2 or 3

            if (choice == 1)
            {
                lcd_print("You chose:      E15DKK");

            }
            else if (choice == 2)
            {
                lcd_print("You chose:      L27DKK");
            }
            else if (choice == 3)
            {
                lcd_print("You chose:      F3DKKCL");
            }

            break;
        }

        case LCD_DISPLAY_ENTER_CARD_NUMBER_AND_PIN :
        {
            clr_LCD();
            home_LCD();
            lcd_print("Please Enter    card nr. & PIN:");
            break;
        }


        case LCD_DISPLAY_CHOICE_IS_BEING_PRODUCED :
        {
            clr_LCD();
            home_LCD();
            lcd_print("Dispensing...");
            break;
        }


        case LCD_DISPLAY_CHOICE_PRODUCED :
        {
            clr_LCD();
            home_LCD();
            lcd_print("Remove coffee");
            break;
        }

        case LCD_RETURN_CASH :
        {
            clr_LCD();
            home_LCD();
            lcd_print("Returning       change...");
            break;
        }
      }
      vTaskDelay(10 / portTICK_RATE_MS);
    }
  }
}
