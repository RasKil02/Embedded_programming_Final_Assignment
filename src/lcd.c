/*****************************************************************************
 * University of Southern Denmark
 * Embedded Programming
 *
 * MODULENAME.: lcd.c
 *
 * PROJECT....: Final Assignment - Embedded Programming
 *
 * DESCRIPTION: Driver for LCD display
 *
 * Change Log:
 ******************************************************************************
* Date    Id    Change
* 2026-05-10
* --------------------
* 150321  MoH   Module created.
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
#include "controller.h"
#include "lcd.h"
#include "encoder.h"
#include <stdio.h>

/*****************************    Defines    *******************************/
#define QUEUE_LEN 128

extern QueueHandle_t lcd_queue;
extern QueueHandle_t encoder_queue;
extern QueueHandle_t encoder_button_queue;
extern QueueHandle_t change_q;
extern QueueHandle_t controller_queue;

#define FALSE 0
#define TRUE 1

typedef enum
{
    LCD_ENTER_CURRENT_TIME,
    LCD_START_SCREEN,
    LCD_IDLE,
    LCD_CHOOSE_UART_OPTION,
    LCD_UART_REPORT,
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


typedef struct
{
    lcd_states cmd;
    int value;
    int value2;
} lcd_msg_t;

/*****************************   Constants   *******************************/
const INT8U LCD_init_sequense[] =
    {
        0x30, // Reset
        0x30, // Reset
        0x30, // Reset
        0x20, // Set 4bit interface
        0x28, // 2 lines Display
        0x0C, // Display ON, Cursor OFF, Blink OFF
        0x06, // Cursor Increment
        0x01, // Clear Display
        0x02, // Home
        0xFF  // stop
};

/*****************************   Variables   *******************************/
INT8U LCD_init;

/*****************************   Functions   *******************************/

void move_LCD(INT8U x, INT8U y)
/*****************************************************************************
 *   Input    : - x: column (0-15), y: row (0-1)
 *   Output   : -
 *   Function : - Move cursor to given position.
 ******************************************************************************/
{
    INT8U pos = (y == 0) ? (0x80 + x) : (0x80 + 0x40 + x);
    wr_ctrl_LCD(pos);
}

void wr_ctrl_LCD_low(INT8U Ch)
/*****************************************************************************
 *   Input    : - Ch: Mask for low part of control data
 *   Output   : -
 *   Function : Write low part of control data to LCD.
 ******************************************************************************/
{
    INT8U temp;
    volatile int i;

    temp = GPIO_PORTC_DATA_R & 0x0F;
    temp = temp | ((Ch & 0x0F) << 4);
    GPIO_PORTC_DATA_R = temp;
    for (i = 0; i < 10000; i)
        i++;
    GPIO_PORTD_DATA_R &= 0xFB;          // Select Control mode, write
    for (i = 0; i < 10000; i)
        i++;
    GPIO_PORTD_DATA_R |= 0x08;          // Set E High

    for (i = 0; i < 10000; i)
        i++;

    GPIO_PORTD_DATA_R &= 0xF7;          // Set E Low

    for (i = 0; i < 10000; i)
        i++;
}

void wr_ctrl_LCD_high(INT8U Ch)
/*****************************************************************************
 *   Input    : - Ch: Mask for high part of control data
 *   Output   : -
 *   Function : Write high part of control data to LCD.
 ******************************************************************************/
{
    wr_ctrl_LCD_low((Ch & 0xF0) >> 4);
}

void out_LCD_low(INT8U Ch)
/*****************************************************************************
 *   Input    : - Ch: Mask for low part of character data
 *   Output   : -
 *   Function : Send low part of character to LCD.
 *              This function works only in 4 bit data mode.
 ******************************************************************************/
{
    INT8U temp;

    temp = GPIO_PORTC_DATA_R & 0x0F;
    GPIO_PORTC_DATA_R = temp | ((Ch & 0x0F) << 4);
    // GPIO_PORTD_DATA_R &= 0x7F;        // Select write
    GPIO_PORTD_DATA_R |= 0x04; // Select data mode
    GPIO_PORTD_DATA_R |= 0x08; // Set E High
    GPIO_PORTD_DATA_R &= 0xF7; // Set E Low
}

void out_LCD_high(INT8U Ch)
/*****************************************************************************
 *   Input    : - Ch: Mask for high part of character data
 *   Output   : -
 *   Function : Send high part of character to LCD.
 *              This function works only in 4 bit data mode.
 ******************************************************************************/
{
    out_LCD_low((Ch & 0xF0) >> 4);
}

void wr_ctrl_LCD(INT8U Ch)
/*****************************************************************************
 *   Input    : - Ch: Control data to write to LCD
 *   Output   : -
 *   Function : Write control data to LCD.
 ******************************************************************************/
{
    static INT8U Mode4bit = FALSE;
    INT16U i;

    wr_ctrl_LCD_high(Ch);
    if (Mode4bit)
    {
        for (i = 0; i < 10000; i++)
            ;
        wr_ctrl_LCD_low(Ch);
    }
    else
    {
        if ((Ch & 0x30) == 0x20)
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
    wr_ctrl_LCD(0x01);
}

void home_LCD()
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : Return cursor to the home position.
 ******************************************************************************/
{
    wr_ctrl_LCD(0x02);
}

void Set_cursor(INT8U Ch)
/*****************************************************************************
 *   Input    : New Cursor position
 *   Output   : -
 *   Function : Place cursor at given position.
 ******************************************************************************/
{
    wr_ctrl_LCD(Ch);
}

void out_LCD(INT8U Ch)
/*****************************************************************************
 *   Input    : - Ch: Character data to write to LCD
 *   Output   : -
 *   Function : Write control data to LCD.
 ******************************************************************************/
{
    INT16U i;

    out_LCD_high(Ch);
    for (i = 0; i < 1000; i++)
        ;
    out_LCD_low(Ch);
}

void lcd_init()
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : - Initialize LCD by sending a sequence of control commands.
 ******************************************************************************/
{
    LCD_init = 0;

    while (LCD_init_sequense[LCD_init] != 0xFF)
    {
        wr_ctrl_LCD(LCD_init_sequense[LCD_init++]);
        vTaskDelay(5 / portTICK_RATE_MS);
    }
}

void lcd_print(char *str)
/*****************************************************************************
 *   Input    : - str: String to print on LCD
 *   Output   : -
 *   Function : Print a string on LCD, starting from the current cursor position.
 ******************************************************************************/
{
    int pos1 = 0;
    int pos2 = 0;

    while (*str)
    {
        move_LCD(pos1, pos2);
        out_LCD(*str);
        str++;
        pos1++;
        if (pos1 > 15)
        {
            pos1 = 0;
            pos2++;
            if (pos2 > 1)
            {
                pos2 = 0;
            }
        }
    }
}

void slide_text(char *str)
/*****************************************************************************
 *   Input    : - str: String to slide on LCD
 *   Output   : -
 *   Function : - Slide a string across the LCD, starting from the leftmost position and moving to the right.
 ******************************************************************************/
{
    int offset = 0;
    while (1)
    {
        clr_LCD();
        home_LCD();

        move_LCD(offset, 0);
        lcd_print(str);

        vTaskDelay(3000 / portTICK_RATE_MS);

        offset++;

        if (offset > 15)                            // If the offset exceeds the width of the LCD, reset it to 0 to start sliding from the beginning again.
        {
            offset = 0;
        }
    }
}

void lcd_task(void *pvParameters)
/*****************************************************************************
 *   Input    : - pvParameters: Pointer to task parameters (not used in this implementation)
 *   Output   : -
 *   Function : - Main task function for handling LCD operations.
 ******************************************************************************/
{
    lcd_msg_t event;
    lcd_init();
    int cash = 0;
    char cash_c[16];
    char price_buffer[16];
    int choice;
    INT8U change = 0;
    INT16S encoder_value;
    INT8U last_button = 0;
    INT8U price = 0;
    INT8U dummy = 0;

    while (1)
    {
        if (xQueueReceive(lcd_queue, &event, pdMS_TO_TICKS(10)))        // Check for new LCD events with a timeout of 10 ms
        {
            switch (event.cmd)                                          // Handle different LCD commands based on the event's command type
            {
            case LCD_ENTER_CURRENT_TIME :                               // Initial screen to set the current time
            {
                clr_LCD();
                home_LCD();
                lcd_print("Enter time: HHMM");
                break;
            }

            case LCD_START_SCREEN :                                     // Start screen with options to order coffee or access UART features
            {
                clr_LCD();
                home_LCD();
                lcd_print("SW1:Order coffeeSW2:uart");
                break;
            }
            case LCD_IDLE:                                              // Idle screen prompting the user to choose a coffee type
            {
                clr_LCD();
                home_LCD();
                lcd_print("Choose Coffee:  1: E 2: L 3: F");
                break;
            }

            case LCD_CHOOSE_UART_OPTION :                              // Screen to choose between changing the price or getting a report via UART
            {
                clr_LCD();
                home_LCD();
                lcd_print("SW1:Change priceSW2: Get log");
                break;
            }

            case LCD_UART_REPORT :                                      // Screen indicating that a report has been sent to Putty via UART
            {
                clr_LCD();
                home_LCD();
                lcd_print("Report sent to  putty v uart");
                break;
            }

            case LCD_UART_PRODUCT:                                      // Screen indicating that a product has been sent to Putty via UART
            {
                clr_LCD();
                home_LCD();
                lcd_print("1: E  2: L  3: FUse putty");
                break;
            }

            case LCD_UART_PRICE:                                        // Screen indicating that the price has been sent to Putty via UART
            {
                clr_LCD();
                home_LCD();
                lcd_print("Enter new price");
                break;
            }

            case LCD_SHOWCASE_NEW_PRICE:                                // Screen showcasing the new price after it has been set via UART
            {
                clr_LCD();
                home_LCD();
                lcd_print("New price set!");

                vTaskDelay(pdMS_TO_TICKS(1000));                        // Showcase new price for 2 sec

                break;
            }

            case LCD_DISPLAY_CASH_OR_CARD:                              // Screen prompting the user to choose between paying with cash or card
            {
                clr_LCD();
                home_LCD();
                lcd_print("Pay with:       1: Cash 2: Card");           // 16 char and then it switches lines
                break;
            }

            case LCD_DISPLAY_CHOICE:                                    // Screen displaying the user's coffee choice based on their selection
            {
                clr_LCD();
                home_LCD();

                choice = event.value;

                if (choice == '1')
                {
                    lcd_print("You chose:      Espresso");
                }
                else if (choice == '2')
                {
                    lcd_print("You chose:      Latte");
                }
                else if (choice == '3')
                {
                    lcd_print("You chose:      Filter");
                }

                break;
            }

            case LCD_DISPLAY_ENTER_CASH_INFO:                           // Screen prompting the user to use the encoder to insert cash.
            {
                clr_LCD();
                home_LCD();

                lcd_print("Use encoder to  insert cash");
                break;
            }

            case LCD_DISPLAY_CASH_AMOUNT:                               // Screen displaying the amount of cash inserted by the user and allowing them to confirm their payment with a button press.
            {
                BOOLEAN BUTTON_PRESSED = 0;
                if (dummy == 0)
                {
                    choice = event.value;

                    if (choice == '1')
                        price = espresso;

                    if (choice == '2')
                        price = latte;

                    if (choice == '3')
                        price = filter;

                    cash = 0;

                    clr_LCD();
                    home_LCD();

                    dummy = 1;
                }


                if ((GPIO_PORTF_DATA_R & 0x01) == 0)                    // Check if the button is pressed (active low)
                {
                    if (cash >= price)
                    {
                        clr_LCD();
                        home_LCD();

                        lcd_print("       paid");

                        move_LCD(0, 1);

                        sprintf(cash_c, "%d DKK", cash);
                        cash_c[15] = '\0';
                        lcd_print(cash_c);

                        change = cash - price;

                        xQueueSend(controller_queue, &change, pdMS_TO_TICKS(10));       // Send the change amount to the controller task
                        xQueueReset(encoder_queue);                                     // Reset the encoder queue to clear any remaining values

                        xQueueReset(lcd_queue);
                        dummy = 0;
                        amount = 0;
                        BUTTON_PRESSED = 1;
                    }
                    else
                    {
                        clr_LCD();
                        home_LCD();

                        lcd_print("     too low");

                        move_LCD(0, 1);

                        sprintf(cash_c, "%d DKK", cash);
                        lcd_print(cash_c);
                        event.cmd = LCD_DISPLAY_CASH_AMOUNT;
                        xQueueSend(lcd_queue, &event, 0);
                    }
                }

                if (xQueueReceive(encoder_queue,                        // Check for encoder values
                                  &encoder_value,
                                  0))
                {
                    cash = encoder_value;

                    clr_LCD();
                    home_LCD();

                    sprintf(cash_c, "%d DKK", cash);
                    cash_c[15] = '\0';
                    lcd_print(&cash_c[0]);
                }
                if (BUTTON_PRESSED == 0)
                {
                    event.cmd = LCD_DISPLAY_CASH_AMOUNT;
                    xQueueSend(lcd_queue, &event, 0);
                }
                break;
            }

            case LCD_DISPLAY_ENTER_CARD_NUMBER_AND_PIN:             // Screen prompting the user to enter their card number and PIN for card payment.
            {
                clr_LCD();
                home_LCD();
                lcd_print("Please Enter    card nr. & PIN:");
                break;
            }

            case LCD_DISPLAY_CHOICE_IS_BEING_PRODUCED:              // Screen indicating that the user's coffee choice is being produced.
            {
                clr_LCD();
                home_LCD();
                lcd_print("Dispensing...");
                break;
            }

            case LCD_DISPLAY_CHOICE_PRODUCED:                       // Screen indicating that the user's coffee choice has been produced and prompting them to remove their coffee.
            {
                clr_LCD();
                home_LCD();
                lcd_print("Remove coffee       (SW1)");
                break;
            }

            case LCD_RETURN_CASH:                                   // Screen indicating that the user's cash is being returned.
            {
                clr_LCD();
                home_LCD();
                lcd_print("Returning       change...");
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
            }

            case LCD_PLACE_CUP:                                     // Screen prompting the user to place their cup for the coffee to be dispensed using SW1.
            {
                clr_LCD();
                home_LCD();
                lcd_print("Please place cup    (SW1)");
            }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
