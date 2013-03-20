/*
    FreeRTOS V7.3.0 - Copyright (C) 2012 Real Time Engineers Ltd.


    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

/* Standard includes. */
#include "string.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+IO includes. */
#include "FreeRTOS_IO.h"

/* Place holder for calls to ioctl that don't use the value parameter. */
#define uarttstPARAMTER_NOT_USED			( ( void * ) 0 )

/* This parameter sets the length of the queue when character by character
queues are used for sending or receiving.  The queue is less than the length
of the string transmitted for test purposes.  Other demos that use character
queues have queues that are longer than the length of the string being
transmitted. */
#define uartstQUEUE_LENGTH					( ( void * ) 20 )

/* A block time that is deliberately too short to allow all the characters
in a string to be sent to a character by character queue. */
#define uartstVERY_SHORT_TX_BLOCK_TIME			( ( void * ) 6 )
#define uartstVERY_SHORT_RX_BLOCK_TIME			( ( void * ) 4 )

/* A block time that is many times longer than required to send and receive
complete messages. */
#define uartstVERY_LONG_BLOCK_TIME				( ( void * ) ( 500UL / portTICK_RATE_MS ) )

/* A block time that is long enough to allow another task to execute, but
no so long that the UART FIFO could be filled up, resulting in overrun
errors. */
#define uartstEXTREMELY_SHORT_BLOCK_TIME	( ( portTickType ) 1 )

/* The number of times each string is Txed/Rxed. */
#define uartstNUM_TEST_TRANSACTIONS			( 10 )

/* The size of the circular buffer used by the UART Rx interrupt when the Rx
is operating in zero copy mode.  This is large enough to hold twice the number
of characters that are contained in the longest string used during this test. */
#define uartstCIRCULAR_BUFFER_SIZE			( ( void * ) ( strlen( ( char * ) pcTestMessage3 ) * ( size_t ) 2U ) )

/* The allowable time margin when measuring block times. */
#define uartstTIME_MARGIN					( ( portTickType ) 2 )
/*-----------------------------------------------------------*/

/*
 * The tasks that implements the UART operation mode test functions.
 */
static void prvUARTTxOperationModesTestTask( void *pvParameters );
static void prvUARTRxOperationModesTestTask( void *pvParameters );

/*
 * Performs a similar function to prvCheckTestResult().  configASSERT() itself
 * has to be undefined in order to run through some of these scenarios.
 */
static void prvCheckTestResult( portBASE_TYPE xTestCondition, portBASE_TYPE xLine );

/*-----------------------------------------------------------*/

/* ***NOTE*** Buffers in this file are dimensioned using the length of
pcTestMessage3 as that is the longest of all the strings.  Therefore ensure
pcTestMessage3 always remains the longest. */
static const int8_t * const pcTestMessage1 = ( int8_t * ) "This is the string sent when the UART is in polling mode.  Not much to do here.\r\n";
static const int8_t * const pcTestMessage2 = ( int8_t * ) "This is the string sent when the UART is in zero copy mode.  There are quite a lot of characters in it to ensure the interrupt has to fire several times in order for the entire message to be transmitted.  The longer it is the better.\r\n";
static const int8_t * const pcTestMessage3 = ( int8_t * ) "This is the string sent when the UART uses a character by character Tx queue.  Again, the longer this is the better.  Character queue transmission and receiption is a very inefficient, although convenient, way of sending large amounts of data.  It is best used when small amounts of data are being transferred.\r\n";
static const int8_t * const pcTestMessage4 = ( int8_t * ) "Message sent with short block time abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!#';][{}:@~+_)(*&^%$£!`¬.\r\n";

static Peripheral_Descriptor_t xTestUART = NULL;
static xTaskHandle xRxTaskHandle = NULL;

/*-----------------------------------------------------------*/

void vUARTOperationModeTestsStart( void )
{
portBASE_TYPE xReturned;

	/* Create both tasks. */
	xReturned = xTaskCreate( prvUARTTxOperationModesTestTask,		/* The task that demonstrates the various modes in which the UART can transmit. */
							( const int8_t * const ) "UARTTx",		/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
							configMINIMAL_STACK_SIZE * 2,			/* The size of the stack allocated to the task. */
							NULL,									/* The parameter is not used, so NULL is passed. */
							configMAX_PRIORITIES - 2,				/* The priority allocated to the task.  This needs to be below the Rx task priority, otherwise the polling tests won't work. */
							NULL );									/* The handle to the created task is not required, so NULL is passed. */
	prvCheckTestResult( xReturned, __LINE__ );

	xReturned = xTaskCreate( prvUARTRxOperationModesTestTask,		/* The task that demonstrates the various modes in which the UART can receive. */
							( const int8_t * const ) "UARTRx",		/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
							configMINIMAL_STACK_SIZE * 2,			/* The size of the stack allocated to the task. */
							NULL,									/* The parameter is not used, so NULL is passed. */
							configMAX_PRIORITIES - 1,				/* The priority allocated to the task.  This needs to be higher than Tx task priority, otherwise the polling tests won't work. */
							&xRxTaskHandle );						/* The handle to the created task is not required, so NULL is passed. */
	prvCheckTestResult( xReturned, __LINE__ );
}
/*-----------------------------------------------------------*/

static void prvUARTTxOperationModesTestTask( void *pvParameters )
{
portBASE_TYPE xReturned, xTestLoops;
portTickType xTimeBefore, xTimeAfter;

	( void ) pvParameters;

	/* Open the UART port used for console input.  The second parameter
	(ulFlags) is not used in this case. */
	xTestUART = FreeRTOS_open( boardCOMMAND_CONSOLE_UART, ( uint32_t ) uarttstPARAMTER_NOT_USED );
	prvCheckTestResult( ( xTestUART != NULL ), __LINE__ );

	/*************************************************************************
	 * Test 1.  Polling Tx and polling Rx.
	 *************************************************************************/

	/* When this task starts, the Rx task will already have started as the
	Rx task has the higher priority.  The Rx task should have placed itself
	into the Suspended state to wait for the Tx task to be ready. */
	xReturned = xTaskIsTaskSuspended( xRxTaskHandle );
	prvCheckTestResult( xReturned, __LINE__ );

	/* Resume the Rx task.  It will start polling for received characters,
	but put itself in the Blocked state when no characters are present to
	ensure this task does not get starved of CPU time. */
	vTaskResume( xRxTaskHandle );

	/* The UART has been opened, but not yet configured.  It will
	use the default configuration, which is to poll rather than use interrupts,
	and to use 115200 baud.  Try writing out a message a few times to check the
	default functionality. */
	for( xTestLoops = 0; xTestLoops < uartstNUM_TEST_TRANSACTIONS; xTestLoops++ )
	{
		xReturned = FreeRTOS_write( xTestUART, pcTestMessage1, strlen( ( char * ) pcTestMessage1 ) );
		prvCheckTestResult( ( xReturned == ( portBASE_TYPE ) strlen( ( char * ) pcTestMessage1 ) ), __LINE__ );
	}


	for( ;; )
	{
		/* Allow the Rx task to complete previous test. */
		while( xTaskIsTaskSuspended( xRxTaskHandle ) != pdTRUE )
		{
			vTaskDelay( ( portTickType ) uartstVERY_SHORT_TX_BLOCK_TIME );
		}



		/**********************************************************************
		 * Test 2.  Interrupt driven, zero copy Tx, ring buffer Rx.
		 **********************************************************************/

		/* Now the UART configuration is going to change.  First set the baud
		rate to 115200 - which it is already as that is the default so this is
		just for test purposes.  Setting it to any other value would require
		the terminal baud rate to change too.  As the UART is still in polled
		mode there is no need to wait for it to be free. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlSET_SPEED, ( void * ) boardDEFAULT_UART_BAUD );
		prvCheckTestResult( xReturned, __LINE__ );

		/* Next, change the Tx usage model from straight polled mode to use
		zero copy buffers with interrupts.  In this mode, the UART will
		transmit characters directly from the buffer passed to the
		FreeRTOS_write() function.  This	call will cause UART interrupts to
		be enabled and the interrupt priority to be set to the minimum
		possible. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlUSE_ZERO_COPY_TX, uarttstPARAMTER_NOT_USED );
		prvCheckTestResult( xReturned, __LINE__ );

		/* Change the Rx usage model from polled to instead use a ring buffer.
		The	Rx driver created the circular buffer itself.  This call will cause
		UART interrupts to be enabled and the interrupt priority to be set to
		the	minimum possible. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlUSE_CIRCULAR_BUFFER_RX, uartstCIRCULAR_BUFFER_SIZE );
		prvCheckTestResult( xReturned, __LINE__ );

		/* By default, the UART interrupt priority will have been set to the
		lowest possible.  It must be kept at or below
		configMAX_LIBRARY_INTERRUPT_PRIORITY, but can be raised above its
		default priority. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlSET_INTERRUPT_PRIORITY, ( void * ) configMAX_LIBRARY_INTERRUPT_PRIORITY );
		prvCheckTestResult( xReturned, __LINE__ );

		/* Zero copy transmit requires a task to obtain exclusive access to the
		peripheral before attempting a FreeRTOS_write() call.  A mutex is by
		the FreeRTOS+IO code to grant access.  Attempting a write here, without
		first obtaining the mutex, should result in no bytes being
		transmitted.  Only attempt this if configASSERT() is not defined,
		otherwise attempting a write without first holding the mutex will cause
		an assertion. */
		/* _RB_ Test this path. */
		#ifndef configASSERT
		{
			xReturned = FreeRTOS_write( xTestUART, pcTestMessage2, strlen( ( char * ) pcTestMessage2 ) );
			prvCheckTestResult( ( xReturned == 0U ), __LINE__ );
		}
		#endif /* configASSERT(). */

		/* Unsuspend the Rx task ready for this test. */
		vTaskResume( xRxTaskHandle );

		for( xTestLoops = 0; xTestLoops < uartstNUM_TEST_TRANSACTIONS; xTestLoops++ )
		{
			/* Try writing a string again, this time using a zero copy
			interrupt driven mode.

			First obtain exclusive write access to the UART.  Obtaining write
			access not only ensure mutually exclusive access, it also ensures
			any previous transmit activity has been completed (important when
			using zero copy buffers to ensure a buffer being that is still
			being transmitted is not overwritten).  This task will wait
			indefinitely in the Blocked state while waiting for UART access to
			be granted,	so no CPU time is wasted polling. */
			xReturned = FreeRTOS_ioctl( xTestUART, ioctlOBTAIN_WRITE_MUTEX, ( void * ) uartstVERY_LONG_BLOCK_TIME );
			prvCheckTestResult( xReturned, __LINE__ );

			/* Write the string.  The write access mutex will be returned
			by the UART ISR once the entire string has been transmitted. */
			xReturned = FreeRTOS_write( xTestUART, pcTestMessage2, strlen( ( char * ) pcTestMessage2 ) );
			prvCheckTestResult( ( xReturned == ( portBASE_TYPE ) strlen( ( char * ) pcTestMessage2 ) ), __LINE__ );
		}

		/* Send the same string another uartstNUM_TEST_TRANSACTIONS times.  This
		time the Rx task will receive the characters one by one. */
		for( xTestLoops = 0; xTestLoops < uartstNUM_TEST_TRANSACTIONS; xTestLoops++ )
		{
			xReturned = FreeRTOS_ioctl( xTestUART, ioctlOBTAIN_WRITE_MUTEX, ( void * ) uartstVERY_LONG_BLOCK_TIME );
			prvCheckTestResult( xReturned, __LINE__ );

			xReturned = FreeRTOS_write( xTestUART, pcTestMessage2, strlen( ( char * ) pcTestMessage2 ) );
			prvCheckTestResult( ( xReturned == ( portBASE_TYPE ) strlen( ( char * ) pcTestMessage2 ) ), __LINE__ );
		}

		/* Take the write mutex again.  This will only be obtainable when the
		UART has completed transmitting the last string sent to it in the loop
		above.	 Normally the write mutex is released again by the UART ISR
		when it has	completed transmitting a string sent to it by a call to
		FreeRTOS_write().  In this case it is not going to be released, as it
		will be deleted when the UART mode is switched in the next part of
		this test anyway. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlOBTAIN_WRITE_MUTEX, ( void * ) uartstVERY_LONG_BLOCK_TIME );
		prvCheckTestResult( xReturned, __LINE__ );

		/* Allow the Rx task to complete test 2. */
		while( xTaskIsTaskSuspended( xRxTaskHandle ) != pdTRUE )
		{
			vTaskDelay( ( portTickType ) uartstVERY_SHORT_TX_BLOCK_TIME );
		}




		/*************************************************************************
		 * Test 3.  Character queue Rx/Tx
		 *************************************************************************/

		/* Change the Tx usage model from zero copy with interrupts to use a
		character by character queue.  Real applications should not change
		interrupt usage modes in this manner!  It is done here purely for test
		purposes.  Character by character Tx queues are a convenient and easy
		transmit method.  They can, however, be inefficient if a lot of data is
		being transmitted quickly.  This command will always result in UART
		interrupts becoming enabled, with the interrupt priority set to the
		minimum possible. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlUSE_CHARACTER_QUEUE_TX, uartstQUEUE_LENGTH );
		prvCheckTestResult( xReturned, __LINE__ );

		/* Set a maximum transmit block time to ensure tasks that perform a
		FreeRTOS_write() on a peripheral that is using an interrupt driven
		character queue transfer mode do not block indefinitely if their
		requested number of characters cannot be written to the queue. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlSET_TX_TIMEOUT, uartstVERY_LONG_BLOCK_TIME );
		prvCheckTestResult( xReturned, __LINE__ );

		/* Likewise, change the Rx usage model from circular buffer, to use a
		character by character queue.  Character by character Rx queue are a
		convenient receive method.  They can, however, be inefficient if a lot
		of data is being received quickly.  This command will always result in
		UART interrupts becoming enabled, with the interrupt priority set to
		the minimum	possible.  The Rx block time was set when the reception was
		set to use the circular buffer transfer mode. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlUSE_CHARACTER_QUEUE_RX, uartstQUEUE_LENGTH );
		prvCheckTestResult( xReturned, __LINE__ );

		/* By default, the UART interrupt priority will have been reset to the
		lowest possible when the usage mode was changed.  It must be at or kept
		below configMAX_LIBRARY_INTERRUPT_PRIORITY, but can be raised above its
		default	priority again. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlSET_INTERRUPT_PRIORITY, ( void * ) configMAX_LIBRARY_INTERRUPT_PRIORITY );
		prvCheckTestResult( xReturned, __LINE__ );

		/* Unsuspend the Rx task ready for this test. */
		vTaskResume( xRxTaskHandle );

		for( xTestLoops = 0; xTestLoops < uartstNUM_TEST_TRANSACTIONS; xTestLoops++ )
		{
			/* Try writing a string again, this time using a character by
			character Tx queue, still with interrupts enabled. */
			xReturned = FreeRTOS_write( xTestUART, pcTestMessage3, strlen( ( char * ) pcTestMessage3 ) );
			prvCheckTestResult( ( xReturned == ( portBASE_TYPE ) strlen( ( char * ) pcTestMessage3 ) ), __LINE__ );
		}

		/*************************************************************************
		 * Test 4.  Character queue Rx/Tx block times
		 *************************************************************************/

		/* Writing characters to a Tx queue has a block time.  This is the
		total time spent trying to write the characters to the Tx queue if the
		queue is already full.  If the task finds the queue full then it is
		held in the	Blocked state until space becomes available, so no CPU time
		is wasted polling.  If the block time expires before all the characters
		have been written to the queue then the FreeRTOS_write() call returns
		the number of characters that were successfully sent (as normal). */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlSET_TX_TIMEOUT, uartstVERY_SHORT_TX_BLOCK_TIME );
		prvCheckTestResult( xReturned, __LINE__ );

		for( xTestLoops = 0; xTestLoops < uartstNUM_TEST_TRANSACTIONS; xTestLoops++ )
		{
			/* Try writing a string again, still using the character by
			character queue, but this time using a very short block time so not
			all the	characters will make it into the queue. */
			xTimeBefore = xTaskGetTickCount();
			xReturned = FreeRTOS_write( xTestUART, pcTestMessage4, strlen( ( char * ) pcTestMessage4 ) );
			xTimeAfter = xTaskGetTickCount();

			prvCheckTestResult( ( xReturned != ( portBASE_TYPE ) strlen( ( char * ) pcTestMessage4 ) ), __LINE__ );
			prvCheckTestResult( ( ( xTimeAfter - xTimeBefore ) < ( ( portTickType ) uartstVERY_SHORT_TX_BLOCK_TIME + ( portTickType ) 2 ) ), __LINE__ );
			FreeRTOS_write( xTestUART, "\r\n", strlen( ( char * ) "\r\n" ) );
		}
	}
}
/*-----------------------------------------------------------*/

static void prvUARTRxOperationModesTestTask( void *pvParameters )
{
portBASE_TYPE xTestLoops, xReturned;
uint8_t *pcRxBuffer;
size_t xRxedChars, xStringLength, xReceivedCharacters;
const size_t xRxBufferSize = strlen( ( char * ) pcTestMessage3 ) + ( size_t ) 1;
portTickType xTimeBefore, xTimeAfter;

	( void ) pvParameters;

	/* Create a buffer that is large enough to receive the longest of the
	strings. */
	pcRxBuffer = pvPortMalloc( xRxBufferSize );
	prvCheckTestResult( ( pcRxBuffer != NULL ), __LINE__ );

	/* At this point the Tx task should not have executed, and therefore the
	UART should not have been opened. */
	prvCheckTestResult( ( xTestUART == NULL ), __LINE__);

	/*************************************************************************
	 * Test 1.  Polling Tx and polling Rx.
	 *************************************************************************/

	/* This task has the higher priority so will run before the Tx task.
	Suspend this task to allow the lower priority Tx task to run.  The Tx task
	will unsuspend (resume) this task when it is ready. */
	vTaskSuspend( NULL );

	/* The Tx task should have opened the UART before unsuspending this
	task. */
	prvCheckTestResult( ( xTestUART != NULL ), __LINE__ );

	/* Clear any characters that may already be in the UART buffers/FIFO. */
	while( FreeRTOS_read( xTestUART, &( pcRxBuffer[ 0 ] ), sizeof( uint8_t ) ) != 0 );

	/* The UART has been opened, but not yet configured.  It will
	use the default configuration, which is to poll rather than use interrupts,
	and to use 115200 baud.  Try reading out a message a few times to check the
	default functionality.  The task must block when no characters are
	available otherwise the lower priority Tx task will be starved of CPU
	time. */
	xStringLength = strlen( ( char * ) pcTestMessage1 );
	for( xTestLoops = 0; xTestLoops < uartstNUM_TEST_TRANSACTIONS; xTestLoops++ )
	{
		/* Clear the Rx buffer, then poll for characters one at a time, placing
		them in the Rx buffer. */
		memset( pcRxBuffer, 0x00, xRxBufferSize );
		xRxedChars = 0;
		while( xRxedChars < xStringLength )
		{
			xReturned = FreeRTOS_read( xTestUART, &( pcRxBuffer[ xRxedChars ] ), sizeof( uint8_t ) );

			/* It should only be possible to have received either 1 of zero
			bytes. */
			prvCheckTestResult( ( ( xReturned == 1 ) || ( xReturned == 0 ) ), __LINE__ );

			if( xReturned == 0 )
			{
				/* No characters were returned, so allow the Tx task to
				execute. */
				vTaskDelay( ( portTickType ) uartstEXTREMELY_SHORT_BLOCK_TIME );
			}
			else
			{
				xRxedChars++;
			}
		}

		/* The whole string should now have been received.  Is it correct? */
		xReturned = strcmp( ( char * ) pcRxBuffer, ( char * ) pcTestMessage1 );
		prvCheckTestResult( ( xReturned == 0 ), __LINE__ );
	}


	for( ;; )
	{
		/* Suspend again to wait for the Tx task to be ready for test 2. */
		vTaskSuspend( NULL );


		/*************************************************************************
		 * Test 2.  Interrupt driven, zero copy Tx, circular buffer Rx.
		 *************************************************************************/

		/* The Tx task will now have configured the UART to use interrupts and
		a circular buffer for receiving.  The default configuration will have
		an infinite block time.  Set this down, but leave the block time long
		enough for the entire string to be received. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlSET_RX_TIMEOUT, uartstVERY_LONG_BLOCK_TIME );
		prvCheckTestResult( xReturned, __LINE__ );

		xStringLength = strlen( ( char * ) pcTestMessage2 );
		for( xTestLoops = 0; xTestLoops < uartstNUM_TEST_TRANSACTIONS; xTestLoops++ )
		{
			/* Clear the Rx buffer. */
			memset( pcRxBuffer, 0x00, xRxBufferSize );

			/* Read as many characters as possible in one go, up to the total
			number of expected characters in the string being received. */
			xReturned = FreeRTOS_read( xTestUART, pcRxBuffer, xStringLength );

			prvCheckTestResult( ( xReturned == ( portBASE_TYPE ) xStringLength ), __LINE__ );

			/* The whole string should now have been received.  Is it
			correct? */
			xReturned = strcmp( ( char * ) pcRxBuffer, ( char * ) pcTestMessage2 );
			prvCheckTestResult( ( xReturned == 0 ), __LINE__ );
		}

		/* This time, try receiving the characters one at a time. */
		for( xTestLoops = 0; xTestLoops < uartstNUM_TEST_TRANSACTIONS; xTestLoops++ )
		{
			/* Clear the Rx buffer. */
			memset( pcRxBuffer, 0x00, xRxBufferSize );
			xReceivedCharacters = 0;

			/* Read one character at a time until the total number of expected
			characters in the string have been received. */
			while( xReceivedCharacters < xStringLength )
			{
				xReturned = FreeRTOS_read( xTestUART, &( pcRxBuffer[ xReceivedCharacters ] ), sizeof( uint8_t ) );
				prvCheckTestResult( ( xReturned == ( portBASE_TYPE ) sizeof( uint8_t ) ), __LINE__ );
				xReceivedCharacters += sizeof( uint8_t );

				/* Make sure not to get stuck here. */
				if( xReturned == 0 )
				{
					break;
				}
			}

			/* The whole string should now have been received.  Is it
			correct? */
			xReturned = strcmp( ( char * ) pcRxBuffer, ( char * ) pcTestMessage2 );
			prvCheckTestResult( ( xReturned == 0 ), __LINE__ );
		}

		/* Next, shorten the block time and try receiving more characters.  At
		this time, the Tx task will not be transmitting, so the read commands
		should	return after the configured block time has expired.

		First change the block time itself. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlSET_RX_TIMEOUT, uartstVERY_SHORT_TX_BLOCK_TIME );
		prvCheckTestResult( xReturned, __LINE__ );

		/* Attempt a read, taking note of the time both before and after the
		call to read(). */
		memset( pcRxBuffer, 0x00, xRxBufferSize );
		xTimeBefore = xTaskGetTickCount();
		xReturned = FreeRTOS_read( xTestUART, pcRxBuffer, xStringLength );
		prvCheckTestResult( ( ( xTaskGetTickCount() - xTimeBefore ) <= ( ( portTickType ) uartstVERY_SHORT_TX_BLOCK_TIME + uartstTIME_MARGIN ) ), __LINE__ );

		/* Nothing should have been returned. */
		prvCheckTestResult( ( xReturned == 0 ), __LINE__ );

		/* Repeat, using a different block time. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlSET_RX_TIMEOUT, ( void * ) ( ( portTickType ) uartstVERY_SHORT_TX_BLOCK_TIME * 2U ) );
		prvCheckTestResult( xReturned, __LINE__ );
		xTimeBefore = xTaskGetTickCount();
		xReturned = FreeRTOS_read( xTestUART, pcRxBuffer, xStringLength );
		prvCheckTestResult( ( ( xTaskGetTickCount() - xTimeBefore ) <= ( ( ( portTickType ) uartstVERY_SHORT_TX_BLOCK_TIME * 2U ) + uartstTIME_MARGIN ) ), __LINE__ );
		prvCheckTestResult( ( xReturned == 0 ), __LINE__ );

		/* Suspend again to wait for the Tx task to be ready for test 3. */
		vTaskSuspend( NULL );




		/*************************************************************************
		 * Test 3.  Character queue Rx/Tx
		 *************************************************************************/

		/* The Tx task will have set both the Rx and Tx models to use a
		character by character queue.  Set the Rx block time back up so it is
		long enough to read the entire string. */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlSET_RX_TIMEOUT, uartstVERY_LONG_BLOCK_TIME );
		prvCheckTestResult( xReturned, __LINE__ );

		xStringLength = strlen( ( char * ) pcTestMessage3 );
		for( xTestLoops = 0; xTestLoops < uartstNUM_TEST_TRANSACTIONS; xTestLoops++ )
		{
			/* Clear the Rx buffer. */
			memset( pcRxBuffer, 0x00, xRxBufferSize );

			/* Read as many characters as possible in one go, up to the total
			number of expected characters in the string being received. */
			xReturned = FreeRTOS_read( xTestUART, pcRxBuffer, xStringLength );

			prvCheckTestResult( ( xReturned == ( portBASE_TYPE ) xStringLength ), __LINE__ );

			/* The whole string should now have been received.  Is it correct? */
			xReturned = strcmp( ( char * ) pcRxBuffer, ( char * ) pcTestMessage3 );
			prvCheckTestResult( ( xReturned == 0 ), __LINE__ );
		}




		/*************************************************************************
		 * Test 4.  Character queue Rx/Tx block times
		 *************************************************************************/


		/* Reading characters from an Rx queue has a block time.  This is the
		total time spent trying to read characters from the Rx queue if the
		queue is already empty.  If the task finds the queue empty then it is
		held in the	Blocked state until data becomes available, so no CPU time
		is wasted polling.  If the block time expires before all the characters
		have been read from the queue then the FreeRTOS_read() call returns the
		number of characters that were successfully read (as normal). */
		xReturned = FreeRTOS_ioctl( xTestUART, ioctlSET_RX_TIMEOUT, uartstVERY_SHORT_RX_BLOCK_TIME );
		prvCheckTestResult( xReturned, __LINE__ );

		for( xTestLoops = 0; xTestLoops < uartstNUM_TEST_TRANSACTIONS; xTestLoops++ )
		{
			/* Try reading a string again, still using the character by
			character queue, but this time using a very short block time so not
			all the	characters will make it from the queue. */
			xTimeBefore = xTaskGetTickCount();
			xReturned = FreeRTOS_read( xTestUART, pcRxBuffer, xStringLength );
			xTimeAfter = xTaskGetTickCount();

			prvCheckTestResult( ( ( xReturned != 0 ) && ( xReturned != ( portBASE_TYPE ) xStringLength ) ), __LINE__ );
			prvCheckTestResult( ( ( xTimeAfter - xTimeBefore ) < ( ( portTickType ) uartstVERY_SHORT_RX_BLOCK_TIME + ( portTickType ) 2 ) ), __LINE__ );
		}

		/* Clear any characters that remain UART buffers/FIFO before
		continuing. */
		while( FreeRTOS_read( xTestUART, &( pcRxBuffer[ 0 ] ), sizeof( uint8_t ) ) != 0 );
		while( FreeRTOS_read( xTestUART, &( pcRxBuffer[ 0 ] ), sizeof( uint8_t ) ) != 0 );
		while( FreeRTOS_read( xTestUART, &( pcRxBuffer[ 0 ] ), sizeof( uint8_t ) ) != 0 );
	}
}
/*-----------------------------------------------------------*/

static void prvCheckTestResult( portBASE_TYPE xTestCondition, portBASE_TYPE xLine )
{
	/* If the test conditions indicates a failure, then just stop here so the
	line number can be inspected.  prvCheckTestResult() is not used in this
	case as some tests require configASSERT() to be undefined. */
	if( xTestCondition == 0 )
	{
		taskDISABLE_INTERRUPTS();
		for( ;; );
	}

	( void ) xLine;
}


