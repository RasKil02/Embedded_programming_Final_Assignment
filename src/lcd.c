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
    while(*str)
    {
        out_LCD(*str);
        str++;
    }
}

void init_lcd_hardware(void)
{
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOC;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD;

  volatile int delay = SYSCTL_RCGC2_R; // allow clock to start

  GPIO_PORTC_DIR_R |= 0xF0; // PC4–PC7 output
  GPIO_PORTC_DEN_R |= 0xF0;

  GPIO_PORTD_DIR_R |= 0x0C; // PD2, PD3 output (adjust if needed)
  GPIO_PORTD_DEN_R |= 0x0C;
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

        vTaskDelay(300 / portTICK_RATE_MS);

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

            out_LCD('C');
            move_LCD(1,0);
            out_LCD('h');
            move_LCD(2,0);
            out_LCD('o');
            move_LCD(3,0);
            out_LCD('o');
            move_LCD(4,0);
            out_LCD('s');
            move_LCD(5,0);
            out_LCD('e');

            move_LCD(7,0);
            out_LCD('C');
            move_LCD(8,0);
            out_LCD('o');
            move_LCD(9,0);
            out_LCD('f');
            move_LCD(10,0);
            out_LCD('f');
            move_LCD(11,0);
            out_LCD('e');
            move_LCD(12,0);
            out_LCD('e');
            move_LCD(13,0);
            out_LCD(':');

            move_LCD(0,1);
            out_LCD('E');
            move_LCD(1,1);
            out_LCD(':');
            move_LCD(2,1);
            out_LCD('1');

            move_LCD(4,1);
            out_LCD('L');
            move_LCD(5,1);
            out_LCD(':');
            move_LCD(6,1);
            out_LCD('2');

            move_LCD(8,1);
            out_LCD('F');
            move_LCD(9,1);
            out_LCD(':');
            move_LCD(10,1);
            out_LCD('3');

            break;
        }

        case LCD_DISPLAY_CASH_OR_CARD :
        {
            clr_LCD();
            home_LCD();
            break;
        }

        case LCD_DISPLAY_CHOICE :
        {
            clr_LCD();
            home_LCD();
            out_LCD('W');

            /*
            lcd_print("You chose:");
            move_LCD(0,1);
            int choice = event.value; // 1, 2 or 3
            if (choice == '1')
            {
                lcd_print("E15DKK");

            }
            else if (choice == '2')
            {
                lcd_print("L27DKK");
            }
            else if (choice == '3')
            {
                lcd_print("F3DKKCL");
            }
            */
            break;
        }

        case LCD_DISPLAY_ENTER_CARD_NUMBER_AND_PIN :
        {
            clr_LCD();
            home_LCD();
            lcd_print("Enter card number:");
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
            lcd_print("Remove");
            move_LCD(6,0);
            lcd_print("coffee");
            break;

        }

        case LCD_RETURN_CASH :
        {
            clr_LCD();
            home_LCD();
            lcd_print("Returning");
            move_LCD(0,1);
            lcd_print("change");
            break;
        }
      }
      vTaskDelay(10 / portTICK_RATE_MS);
    }
  }
}
