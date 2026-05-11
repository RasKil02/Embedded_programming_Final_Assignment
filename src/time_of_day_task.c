/*****************************************************************************
* University of Southern Denmark
* Embedded Programming
*
* MODULENAME.: time_of_day_task.c
*
* PROJECT....: Final Assignment - Embedded Programming
*
* DESCRIPTION: Simple real-time clock task
*
*****************************************************************************
* Date    Id    Change
* 2026-05-10
* --------------------
* 150321  MoH   Module created.
*
*****************************************************************************/

/***************************** Include files *******************************/

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Own includes
#include "time_of_day_task.h"


/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

void time_of_day_task(void *pvParameters)
{
    TickType_t xLastWakeTime;

    // Initialize with current tick count
    xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        sec++;

        if(sec >= 60)
        {
            sec = 0;
            min++;
        }

        if(min >= 60)
        {
            min = 0;
            hour++;
        }

        if(hour >= 24)
        {
            hour = 0;
        }

        // Precise periodic timing
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}