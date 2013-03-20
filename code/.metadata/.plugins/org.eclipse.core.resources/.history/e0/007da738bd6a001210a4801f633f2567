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

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"

/*
 * Implements the run-time-stats command.
 */
static portBASE_TYPE prvTaskStatsCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * Implements the task-stats command.
 */
static portBASE_TYPE prvRunTimeStatsCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * Implements the echo-three-parameters command.
 */
static portBASE_TYPE prvThreeParameterEchoCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * Implements the echo-parameters command.
 */
static portBASE_TYPE prvMultiParameterEchoCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * Implements the create-task command.
 */
static portBASE_TYPE prvCreateTaskCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * Implements the delete-task command.
 */
static portBASE_TYPE prvDeleteTaskCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * The task that is created by the create-task command.
 */
static void prvCreatedTask( void *pvParameters );

/*
 * Holds the handle of the task created by the create-task command.
 */
static xTaskHandle xCreatedTaskHandle = NULL;

/* Structure that defines the "run-time-stats" command line command.   This
generates a table that shows how much run time each task has */
static const CLI_Command_Definition_t prvRunTimeStatsCommandDefinition =
{
	( const int8_t * const ) "run-time-stats", /* The command string to type. */
	( const int8_t * const ) "run-time-stats:\r\n Displays a table showing how much processing time each FreeRTOS task has used\r\n\r\n",
	prvRunTimeStatsCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the "task-stats" command line command.  This generates
a table that gives information on each task in the system. */
static const CLI_Command_Definition_t prvTaskStatsCommandDefinition =
{
	( const int8_t * const ) "task-stats", /* The command string to type. */
	( const int8_t * const ) "task-stats:\r\n Displays a table showing the state of each FreeRTOS task\r\n\r\n",
	prvTaskStatsCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the "echo_3_parameters" command line command.  This
takes exactly three parameters that the command simply echos back one at a
time. */
static const CLI_Command_Definition_t prvThreeParameterEchoCommandDefinition =
{
	( const int8_t * const ) "echo-3-parameters",
	( const int8_t * const ) "echo-3-parameters <param1> <param2> <param3>:\r\n Expects three parameters, echos each in turn\r\n\r\n",
	prvThreeParameterEchoCommand, /* The function to run. */
	3 /* Three parameters are expected, which can take any value. */
};

/* Structure that defines the "echo_parameters" command line command.  This
takes a variable number of parameters that the command simply echos back one at
a time. */
static const CLI_Command_Definition_t prvMultiParameterEchoCommandDefinition =
{
	( const int8_t * const ) "echo-parameters",
	( const int8_t * const ) "echo-parameters <...>:\r\n Take variable number of parameters, echos each in turn\r\n\r\n",
	prvMultiParameterEchoCommand, /* The function to run. */
	-1 /* The user can enter any number of commands. */
};

/* Structure that defines the "create-task" command line command.  This takes a
single parameter that is passed into a newly created task.  The task then
periodically writes to the console.  The parameter must be a numerical value. */
static const CLI_Command_Definition_t prvCreateTaskCommandDefinition =
{
	( const int8_t * const ) "create-task",
	( const int8_t * const ) "create-task <param>:\r\n Creates a new task that periodically writes the parameter to the CLI output\r\n\r\n",
	prvCreateTaskCommand, /* The function to run. */
	1 /* A single parameter should be entered. */
};

/* Structure that defines the "delete-task" command line command.  This deletes
the task that was previously created using the "create-command" command. */
static const CLI_Command_Definition_t prvDeleteTaskCommandDefinition =
{
	( const int8_t * const ) "delete-task",
	( const int8_t * const ) "delete-task:\r\n Deletes the task created by the create-task command\r\n\r\n",
	prvDeleteTaskCommand, /* The function to run. */
	0 /* A single parameter should be entered. */
};

/*-----------------------------------------------------------*/

void vRegisterCLICommands( void )
{
	/* Register all the command line commands defined immediately above. */
	FreeRTOS_CLIRegisterCommand( &prvTaskStatsCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &prvRunTimeStatsCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &prvThreeParameterEchoCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &prvMultiParameterEchoCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &prvCreateTaskCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &prvDeleteTaskCommandDefinition );
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvTaskStatsCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
const int8_t *const pcTaskTableHeader = ( int8_t * ) "Task          State  Priority  Stack	#\r\n************************************************\r\n";

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Generate a table of task stats. */
	strcpy( ( char * ) pcWriteBuffer, ( char * ) pcTaskTableHeader );
	vTaskList( pcWriteBuffer + strlen( ( char * ) pcTaskTableHeader ) );

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvRunTimeStatsCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
const int8_t * const pcStatsTableHeader = ( int8_t * ) "Task            Abs Time      % Time\r\n****************************************\r\n";

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Generate a table of task stats. */
	strcpy( ( char * ) pcWriteBuffer, ( char * ) pcStatsTableHeader );
	vTaskGetRunTimeStats( pcWriteBuffer + strlen( ( char * ) pcStatsTableHeader ) );

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvThreeParameterEchoCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
int8_t *pcParameterString;
portBASE_TYPE xParameterStringLength, xReturn;
static portBASE_TYPE xParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	if( xParameterNumber == 0 )
	{
		/* The first time the function is called after the command has been
		entered just a header string is returned. */
		sprintf( ( char * ) pcWriteBuffer, "The three parameters were:\r\n" );

		/* Next time the function is called the first parameter will be echoed
		back. */
		xParameterNumber = 1L;

		/* There is more data to be returned as no parameters have been echoed
		back yet. */
		xReturn = pdPASS;
	}
	else
	{
		/* Obtain the parameter string. */
		pcParameterString = ( int8_t * ) FreeRTOS_CLIGetParameter
									(
										pcCommandString,		/* The command string itself. */
										xParameterNumber,		/* Return the next parameter. */
										&xParameterStringLength	/* Store the parameter string length. */
									);

		/* Sanity check something was returned. */
		configASSERT( pcParameterString );

		/* Return the parameter string. */
		memset( pcWriteBuffer, 0x00, xWriteBufferLen );
		sprintf( ( char * ) pcWriteBuffer, "%d: ", xParameterNumber );
		strncat( ( char * ) pcWriteBuffer, ( const char * ) pcParameterString, xParameterStringLength );
		strncat( ( char * ) pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

		/* If this is the last of the three parameters then there are no more
		strings to return after this one. */
		if( xParameterNumber == 3L )
		{
			/* If this is the last of the three parameters then there are no more
			strings to return after this one. */
			xReturn = pdFALSE;
			xParameterNumber = 0L;
		}
		else
		{
			/* There are more parameters to return after this one. */
			xReturn = pdTRUE;
			xParameterNumber++;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvMultiParameterEchoCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
int8_t *pcParameterString;
portBASE_TYPE xParameterStringLength, xReturn;
static portBASE_TYPE xParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	if( xParameterNumber == 0 )
	{
		/* The first time the function is called after the command has been
		entered just a header string is returned. */
		sprintf( ( char * ) pcWriteBuffer, "The parameters were:\r\n" );

		/* Next time the function is called the first parameter will be echoed
		back. */
		xParameterNumber = 1L;

		/* There is more data to be returned as no parameters have been echoed
		back yet. */
		xReturn = pdPASS;
	}
	else
	{
		/* Obtain the parameter string. */
		pcParameterString = ( int8_t * ) FreeRTOS_CLIGetParameter
									(
										pcCommandString,		/* The command string itself. */
										xParameterNumber,		/* Return the next parameter. */
										&xParameterStringLength	/* Store the parameter string length. */
									);

		if( pcParameterString != NULL )
		{
			/* Return the parameter string. */
			memset( pcWriteBuffer, 0x00, xWriteBufferLen );
			sprintf( ( char * ) pcWriteBuffer, "%d: ", xParameterNumber );
			strncat( ( char * ) pcWriteBuffer, ( const char * ) pcParameterString, xParameterStringLength );
			strncat( ( char * ) pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

			/* There might be more parameters to return after this one. */
			xReturn = pdTRUE;
			xParameterNumber++;
		}
		else
		{
			/* No more parameters were found.  Make sure the write buffer does
			not contain a valid string. */
			pcWriteBuffer[ 0 ] = 0x00;

			/* No more data to return. */
			xReturn = pdFALSE;

			/* Start over the next time this command is executed. */
			xParameterNumber = 0;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvCreateTaskCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
int8_t *pcParameterString;
portBASE_TYPE xParameterStringLength;
static const int8_t *pcSuccessMessage = ( int8_t * ) "Task created\r\n";
static const int8_t *pcFailureMessage = ( int8_t * ) "Task not created\r\n";
static const int8_t *pcTaskAlreadyCreatedMessage = ( int8_t * ) "The task has already been created. Execute the delete-task command first.\r\n";
int32_t lParameterValue;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Obtain the parameter string. */
	pcParameterString = ( int8_t * ) FreeRTOS_CLIGetParameter
								(
									pcCommandString,		/* The command string itself. */
									1,						/* Return the first parameter. */
									&xParameterStringLength	/* Store the parameter string length. */
								);

	/* Turn the parameter into a number. */
	lParameterValue = ( int32_t ) atol( ( const char * ) pcParameterString );

	/* Attempt to create the task. */
	if( xCreatedTaskHandle != NULL )
	{
		strcpy( ( char * ) pcWriteBuffer, ( const char * ) pcTaskAlreadyCreatedMessage );
	}
	else
	{
		if( xTaskCreate( prvCreatedTask, ( const signed char * ) "Created", configMINIMAL_STACK_SIZE,  ( void * ) lParameterValue, tskIDLE_PRIORITY, &xCreatedTaskHandle ) == pdPASS )
		{
			strcpy( ( char * ) pcWriteBuffer, ( const char * ) pcSuccessMessage );
		}
		else
		{
			strcpy( ( char * ) pcWriteBuffer, ( const char * ) pcFailureMessage );
		}
	}

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvDeleteTaskCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
static const int8_t *pcSuccessMessage = ( int8_t * ) "Task deleted\r\n";
static const int8_t *pcFailureMessage = ( int8_t * ) "The task was not running.  Execute the create-task command first.\r\n";

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* See if the task is running. */
	if( xCreatedTaskHandle != NULL )
	{
		vTaskDelete( xCreatedTaskHandle );
		xCreatedTaskHandle = NULL;
		strcpy( ( char * ) pcWriteBuffer, ( const char * ) pcSuccessMessage );
	}
	else
	{
		strcpy( ( char * ) pcWriteBuffer, ( const char * ) pcFailureMessage );
	}

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

static void prvCreatedTask( void *pvParameters )
{
int32_t lParameterValue;
static uint8_t pucLocalBuffer[ 60 ];
void vOutputString( const uint8_t * const pucMessage );

	/* Cast the parameter to an appropriate type. */
	lParameterValue = ( int32_t ) pvParameters;

	memset( ( void * ) pucLocalBuffer, 0x00, sizeof( pucLocalBuffer ) );
	sprintf( ( char * ) pucLocalBuffer, "Created task running.  Received parameter %d\r\n\r\n", ( long ) lParameterValue );
	vOutputString( pucLocalBuffer );

	for( ;; )
	{
		vTaskDelay( portMAX_DELAY );
	}
}
/*-----------------------------------------------------------*/

