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
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"

// More Application includes
#include "semphr.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

// Macro to change int to char
#define INT_TO_CHAR(number) ((char)(number + '0'))

// Constant to indicate mutex polling
#define NO_TIMEOUT ((TickType_t) 0)

// Global Variables
TaskHandle_t task1_handle = NULL, task2_handle = NULL;
SemaphoreHandle_t mutex = NULL;


/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/

// Prototypes for task codes
void task1_code(void *);
void task2_code(void *);


/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	
	// Create Mutex
	mutex = xSemaphoreCreateMutex();
	
  	/* Create Tasks here */
	// Task 1
	xTaskCreate(task1_code, // Function that implements the task.
              "Task1 with period 100", // Text name for the task.
              50, // Stack size in words, not bytes.
              (void *) NULL, // Parameter passed into the task.
              2, // Priority at which the task is created.
              &task1_handle ); // Used to pass out the created task's handle.
	
	// Task 2					
	xTaskCreate(task2_code, // Function that implements the task.
              "Task2 with period 500", // Text name for the task.
              50, // Stack size in words, not bytes.
              (void *) NULL, // Parameter passed into the task.
              1, // Priority at which the task is created.
              &task2_handle ); // Used to pass out the created task's handle.


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

void task1_code(void *task_parameters)
{
	// Message body
	char message[] = "Task 1 - Message x\n";
	
	// Message length (calculated once to save runtime)
	uint8_t message_len = (uint8_t)strlen(message);
	
	// Message ID (variable in message body from 0 to 9)
	uint8_t message_ID = 0;
	
	while(1)
	{
		// Take Mutex to access the shared resource (UART1)
		// NO_TIMEOUT to poll, portMAX_DELAY to block
		if(xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
		{
			for(message_ID = 0; message_ID < 10; message_ID++)
			{
				// Update message ID in message body
				message[message_len - 2] = INT_TO_CHAR(message_ID);
			
				// Send Message
				vSerialPutString((const signed char *)message, message_len);
			
				// Send every 100 ms
				vTaskDelay(100);
				
			} // for
			
			// Give back the mutex so that the shared resource 
			// can be accessed by other tasks
			xSemaphoreGive(mutex);
			
		} // if
		
		// Block task
		vTaskDelay(1);
		
	} // while
}


void task2_code(void *task_parameters)
{
	// Message body
	char message[] = "Task 2 - Message x\n";
	
	// Message length (calculated once to save runtime)
	uint8_t message_len = (uint8_t)strlen(message);
	
	// Message ID (variable in message body from 0 to 9)
	uint8_t message_ID = 0;
	
	// Loop index to simulate heavy load
	volatile uint32_t heavy_load_index;
	
	while(1)
	{
		// Take Mutex to access the shared resource (UART1)
		// NO_TIMEOUT to poll, portMAX_DELAY to block
		if(xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
		{
			for(message_ID = 0; message_ID < 10; message_ID++)
			{
				// Update message ID in message body
				message[message_len - 2] = INT_TO_CHAR(message_ID);
			
				// Send Message
				vSerialPutString((const signed char *)message, message_len);
			
				// Heavy Load loop
				for(heavy_load_index = 0; heavy_load_index < 100000; heavy_load_index++);
			
				// Send every 500 ms
				vTaskDelay(500);
				
			} // for
			
			// Give back the mutex so that the shared resource 
			// can be accessed by other tasks
			xSemaphoreGive(mutex);
			
		} // if
		
		// Block task
		vTaskDelay(1);
		
	} // while
}

