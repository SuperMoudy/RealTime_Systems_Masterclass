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
#include "queue.h"



/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

// Constant to specify max number of characters in a message
#define MESSAGE_LEN 25

// Max Queue size
#define MESSAGE_QUEUE_LEN 10


// Type definitions
typedef struct
{
	//uint8_t sender_ID; // Number refers to the sender identity
	char message[MESSAGE_LEN + 1]; // Message body to be sent
	uint8_t message_len; // Actual length of the message
} Message_st;

// Global Variables
// Task Handlers
TaskHandle_t Button1_task_handle = NULL, Button2_task_handle = NULL;
TaskHandle_t String_task_handle = NULL, Consumer_task_handle = NULL;

// Queue Handlers
QueueHandle_t message_queue;

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/

// Prototypes for task codes
void Button1_task_code(void *);
void Button2_task_code(void *);
void String_task_code(void *);
void Consumer_task_code(void *);


/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	// Messages
	Message_st button1_message, button2_message, string_message;
	
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	
	// Create Message Queue
	message_queue = xQueueCreate(MESSAGE_QUEUE_LEN, sizeof(Message_st *));
	
	/* Create Tasks here */
	// Button 1 Task
	xTaskCreate(Button1_task_code, // Function that implements the task.
              "Button1 Task", // Text name for the task.
              100, // Stack size in words, not bytes.
              (void *) &button1_message, // Parameter passed into the task.
              1, // Priority at which the task is created.
              &Button1_task_handle ); // Used to pass out the created task's handle.
	
	// Button 2 Task					
	xTaskCreate(Button2_task_code, // Function that implements the task.
              "Button2 Task", // Text name for the task.
              100, // Stack size in words, not bytes.
              (void *) &button2_message, // Parameter passed into the task.
              1, // Priority at which the task is created.
              &Button2_task_handle ); // Used to pass out the created task's handle.
	
	// Periodic String Task					
	xTaskCreate(String_task_code, // Function that implements the task.
              "String Task", // Text name for the task.
              100, // Stack size in words, not bytes.
              (void *) &string_message, // Parameter passed into the task.
              1, // Priority at which the task is created.
              &String_task_handle ); // Used to pass out the created task's handle.
							
	// Consumer Task					
	xTaskCreate(Consumer_task_code, // Function that implements the task.
              "Consumer Task", // Text name for the task.
              100, // Stack size in words, not bytes.
              (void *) NULL, // Parameter passed into the task.
              1, // Priority at which the task is created.
              &Consumer_task_handle ); // Used to pass out the created task's handle.


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

void Button1_task_code(void *task_message)
{
	// Task Message
	Message_st *button1_message_ptr = (Message_st *)task_message;
	
	// Button States
	pinState_t button1_prev_state = GPIO_read(PORT_0, PIN0);
	pinState_t button1_curr_state = button1_prev_state;
	
	while(1)
	{
		// Read pin state
		button1_curr_state = GPIO_read(PORT_0, PIN0);
		
		// Detect state transition (rising/falling edge detection)
		if(button1_curr_state != button1_prev_state)
		{
			if(button1_curr_state == PIN_IS_HIGH) // prev is LOW -> rising edge
			{
				// Add message body to the packet
				strcpy(button1_message_ptr->message, "Button1 Rising Edge\n");
				
				// Add message length to the packet
				button1_message_ptr->message_len = (uint8_t) strlen(button1_message_ptr->message);
			}
			else // prev is HIGH, curr is LOW -> falling edge
			{
				// Add message body to the packet
				strcpy(button1_message_ptr->message, "Button1 Falling Edge\n");
				
				// Add message length to the packet
				button1_message_ptr->message_len = (uint8_t) strlen(button1_message_ptr->message);
			}
			
			// Send message to queue
			xQueueSend(message_queue, (void *) &button1_message_ptr, portMAX_DELAY);
		}
		
		// Save current state
		button1_prev_state = button1_curr_state;
		
		// Run every 1 ms
		vTaskDelay(1);
	}
}

void Button2_task_code(void *task_message)
{
	// Task Message
	Message_st *button2_message_ptr = (Message_st *)task_message;
	
	// Button States
	pinState_t button2_prev_state = GPIO_read(PORT_0, PIN1);
	pinState_t button2_curr_state = button2_prev_state;
	
	while(1)
	{
		// Read pin state
		button2_curr_state = GPIO_read(PORT_0, PIN1);
		
		// Detect state transition (rising/falling edge detection)
		if(button2_curr_state != button2_prev_state)
		{
			if(button2_curr_state == PIN_IS_HIGH) // prev is LOW -> rising edge
			{
				// Add message body to the packet
				strcpy(button2_message_ptr->message, "Button2 Rising Edge\n");
				
				// Add message length to the packet
				button2_message_ptr->message_len = strlen(button2_message_ptr->message);
			}
			else // prev is HIGH, curr is LOW -> falling edge
			{
				// Add message body to the packet
				strcpy(button2_message_ptr->message, "Button2 Falling Edge\n");
				
				// Add message length to the packet
				button2_message_ptr->message_len = strlen(button2_message_ptr->message);
			}
			
			// Send message to queue
			xQueueSend(message_queue, (void *) &button2_message_ptr, portMAX_DELAY);
		}
		
		// Save current state
		button2_prev_state = button2_curr_state;
		
		// Run every 1 ms
		vTaskDelay(1);
	}
}

void String_task_code(void *task_message)
{
	// Task Message
	Message_st *string_message_ptr = (Message_st *)task_message;
	
	// Add message body to the packet
	strcpy(string_message_ptr->message, "Periodic Message\n");
	
	// Add message length to the packet
	string_message_ptr->message_len = (uint8_t) strlen(string_message_ptr->message);
	
	while(1)
	{
		// Send message to queue
		xQueueSend(message_queue, (void *) &string_message_ptr, portMAX_DELAY);
		
		// Run every 100 ms
		vTaskDelay(100);
	}
}

void Consumer_task_code(void *task_parameters)
{
	// Task Message
	Message_st *consumer_message_ptr;
	
	while(1)
	{
		// Receive message from queue
		xQueueReceive(message_queue, (void *) &consumer_message_ptr, portMAX_DELAY);
			
		// Printing message
		vSerialPutString((const signed char *)consumer_message_ptr->message, consumer_message_ptr->message_len);
		
		// Run every 50 ms
		vTaskDelay(50);
	}
}

