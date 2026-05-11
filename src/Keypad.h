/*****************************************************************************
* University of Southern Denmark
* Embedded Programming 
*
* MODULENAME.: key.h
*
* PROJECT....: Final Project - Embedded Programming
*
* DESCRIPTION: Test.
*
* Change Log:
******************************************************************************
* Date    Id    Change
* 2026-05-10
* --------------------
* 150321  MoH   Module created.
*
*****************************************************************************/

#ifndef _KEY_H
  #define _KEY_H

BOOLEAN get_keyboard( INT8U* );
extern void key_task(void *pvParameters);


#endif
