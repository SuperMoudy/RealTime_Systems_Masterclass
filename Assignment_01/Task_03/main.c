/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

// Constant for Toggling state
#define NO_TOGGLE 0

// Type definitions
typedef struct
{
	pinX_t pin_num;
	pinState_t pin_state;
	int toggle_rate;
} LED_data_st;

// Global Variables
TaskHandle_t LED_handle = NULL, Button_handle = NULL;


/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/


// Prototypes for task codes
void LED_task_code(void*);
void Button_task_code(void*);


/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	// Local Variables
	LED_data_st LED1 = {PIN1, PIN_IS_LOW, NO_TOGGLE};
	
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();

	
  /* Create Tasks here */
	
	// LED Task
	xTaskCreate(LED_task_code, // Function that implements the task.
              "LED Task", // Text name for the task.
              50, // Stack size in words, not bytes.
              (void*)&LED1, // Parameter passed into the task.
              1, // Priority at which the task is created.
              &LED_handle ); // Used to pass out the created task's handle.
	
	// Button Task					
	xTaskCreate(Button_task_code, // Function that implements the task.
              "Button Task", // Text name for the task.
              50, // Stack size in words, not bytes.
              (void*)&LED1, // Parameter passed into the task.
              1, // Priority at which the task is created.
              &Button_handle ); // Used to pass out the created task's handle.


	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/

void LED_task_code(void *data)
{
	while(1)
	{
		// Write current pin state
		GPIO_write(PORT_0, ((LED_data_st *)data)->pin_num, ((LED_data_st *)data)->pin_state);
		
		// Check if toggling is disabled
		if(((LED_data_st *)data)->toggle_rate == NO_TOGGLE)
		{
			// Suspend this task as the pin state is low
			vTaskSuspend(NULL);
			//vTaskDelay(10);
		}
		else // Toggling is enabled
		{
			// Toggle pin state
			((LED_data_st *)data)->pin_state ^= 1;
			
			// Apply toggle rate
			vTaskDelay(((LED_data_st *)data)->toggle_rate);
		}
	}
}


void Button_task_code(void *data)
{
	// Initial State
	unsigned long duration = 0;
	pinState_t pin0_prev_state = GPIO_read(PORT_0, PIN0);
	pinState_t pin0_curr_state = pin0_prev_state;
	
	while(1)
	{
		// Read pin 0 state
		pin0_curr_state = GPIO_read(PORT_0, PIN0);
		
		// Check pin 0 state
		if(pin0_curr_state == PIN_IS_HIGH)
		{
			// Continue measuring press duration
			duration += 10;
		}
		else // PIN_IS_LOW
		{
			if(pin0_prev_state == PIN_IS_HIGH)
			{
				if(duration >= 0 && duration < 2000)
				{
					// Disable toggling
					((LED_data_st *)data)->toggle_rate = NO_TOGGLE;
				
					// Clear pin (switch LED off)
					((LED_data_st *)data)->pin_state = PIN_IS_LOW;
				}
				else if(duration >= 2000 && duration < 4000)
				{
					// Re-enable toggling with a rate
					((LED_data_st *)data)->toggle_rate = 400;
				
					// Bring LED Task again to the schedular
					vTaskResume(LED_handle);
				}
				else // >= 4000
				{
					// Re-enable toggling with a rate
					((LED_data_st *)data)->toggle_rate = 100;
				
					// Bring LED Task again to the schedular
					vTaskResume(LED_handle);
				}
			}
			
			// Reset press duration
			duration = 0;
		}
		
		// Prepare for state transition (save last state)
		pin0_prev_state = pin0_curr_state;
		
		// Check every 10 ms
		vTaskDelay(10);
	}
}

