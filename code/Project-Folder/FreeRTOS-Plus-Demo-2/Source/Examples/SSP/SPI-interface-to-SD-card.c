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
#include "stdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+IO includes. */
#include "FreeRTOS_IO.h"

/* Library includes. */
#include "lpc17xx_gpio.h"

/* Example includes. */
#include "SPI-interface-to-SD-card.h"
#include "FreeRTOS_CLI.h"

/* File system headers. */
#include "diskio.h"
#include "ff.h"

/* Only a single drive is supported. */
#define spiDRIVE_NUMBER					( 0 )

/* Delay used between attempts to mount the drive. */
#define spiLONG_DELAY					( 1000 / portTICK_RATE_MS )

/* 8.3 format, plus null terminator. */
#define spiMAX_FILE_NAME_LEN 13

/* By the SD card specification, this cannot be above 2048. */
#define spiRAM_BUFFER_SIZE 				2000

/* Just used in FreeRTOS_ioctl() calls as the third parameter when the third
parameter is not actually required. */
#define cmdPARAMTER_NOT_USED			( ( void * ) 0 )

/* The buffers for the various transfer control methods are dimensioned to
equal the RAM buffer size. */
#define spiBUFFER_SIZE					( spiRAM_BUFFER_SIZE )

/* A one second delay, specified in ticks. */
#define spiONE_SECOND					( 1000UL / portTICK_RATE_MS )

/*-----------------------------------------------------------*/

/*
 * The task that demonstrates how to create and read files using all supported
 * FreeRTOS+IO transfer methods.
 */
static void prvSPIToSDCardTask( void *pvParameters );

/*
 * Called when a file operation fails - just to assist debugging.
 */
static void prvFileOperationFailed( int32_t lLine );

/*
 * Creates n files on the SD card, where each file is a different size and
 * contains different data.
 */
static void prvWriteFilesToDisk( int8_t *pcPartFileName, FIL *pxFile );

/*
 * Reads back the files created during the execution of prvWriteFilesToDisk()
 * and checks that each file content is as expected.
 */
static void prvReadBackCreatedFiles( int8_t *pcPartFileName, FIL *pxFile );

/*
 * Implements the file system "dir" command, accessible through a command
 * console.  See the definition of the xDirCommand command line input structure
 * below.  This function is not necessarily reentrant.  It must not be used by
 * more than one task at a time.
 */
static portBASE_TYPE prvDirCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * Implements the file system "del" command, accessible through a command
 * console.  See the definition of the xDelCommand command line input structure
 * below.  This function is not necessarily reentrant.  It must not be used by
 * more than one task at a time.
 */
static portBASE_TYPE prvDelCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * Implements the file system "copy" command, accessible through a command
 * console.  See the definition of the xCopyCommand command line input
 * structure below.  This function is not necessarily reentrant.  It must not
 * be used by more than one task at a time.
 */
static portBASE_TYPE prvCopyCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*-----------------------------------------------------------*/

/* The file system structure - the file system implementation includes a mutex
object that ensures consistent access to this variable. */
static FATFS xFatfs;

/* These objects are large, so not stored on the stack.  They are used by
the functions that implement command line commands - and as only one command
line command can be executing at any one time - there is no need to protect
access to the variables using a mutex or other mutual exclusion method. */
static FIL xCommandLineFile1, xCommandLineFile2;

/* A buffer used to both create content to write to disk, and read content back
from a disk.  Note there is no mutual exclusion on this buffer, so it must not
be accessed outside of the prvSPIToSDCardTask() task. */
static uint8_t cRAMBuffer[ spiRAM_BUFFER_SIZE ];

/* The RAM buffer is used by two tasks, so protect it using a mutex. */
static xSemaphoreHandle xRamBufferMutex = NULL;

/* Structure that defines the "dir" command line command. */
static const CLI_Command_Definition_t xDirCommand =
{
	( const int8_t * const ) "dir",
	( const int8_t * const ) "dir:\r\n Displays the name and size of each file in the root directory\r\n\r\n",
	prvDirCommand,
	0
};

/* Structure that defines the "del" command line command. */
static const CLI_Command_Definition_t xDelCommand =
{
	( const int8_t * const ) "del",
	( const int8_t * const ) "del <filename>:\r\n Deletes <filename> from the disk\r\n\r\n",
	prvDelCommand,
	1
};

/* Structure that defines the "del" command line command. */
static const CLI_Command_Definition_t xCopyCommand =
{
	( const int8_t * const ) "copy",
	( const int8_t * const ) "copy <filename1> <filename2>:\r\n Copies <filename1> to <filename2>, creating or overwriting <filename2>\r\n\r\n",
	prvCopyCommand,
	2
};

/*-----------------------------------------------------------*/

void vStartSPIInterfaceToSDCardTask( void )
{
	/* Create the prvSPIToSDCardTask task.  This just creates
	spiFILES_TO_CREATE unique files on SD drive number 0, checks the files
	were created correctly, then deletes itself. */
	xTaskCreate( 	prvSPIToSDCardTask,						/* The task that uses the SSP in SPI mode to implement a file system disk IO driver. */
					( const int8_t * const ) "FSys", 		/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					configFILE_SYSTEM_DEMO_STACK_SIZE,		/* The size of the stack allocated to the task. */
					NULL,									/* The parameter is not used, so NULL is passed. */
					configFILE_SYSTEM_DEMO_TASK_PRIORITY,	/* The priority allocated to the task. */
					NULL );									/* A handle to the task being created is not required, so just pass in NULL. */

	/* Register the file commands with the command interpreter. */
	FreeRTOS_CLIRegisterCommand( &xDirCommand );
	FreeRTOS_CLIRegisterCommand( &xDelCommand );
	FreeRTOS_CLIRegisterCommand( &xCopyCommand );

	/* Create the mutex that protects the shared RAM buffer. */
	xRamBufferMutex = xSemaphoreCreateMutex();
	configASSERT( xRamBufferMutex );
}
/*-----------------------------------------------------------*/



static void prvSPIToSDCardTask( void *pvParameters )
{
Peripheral_Descriptor_t xSPIPortUsedByMMCDriver;
extern Peripheral_Descriptor_t xMMCGetSPIPortHandle( void );
FIL *pxFile = NULL;

	( void ) pvParameters;

	/* Wait until the disk is present. */
	while( disk_initialize( spiDRIVE_NUMBER ) != FR_OK )
	{
		vTaskDelay( spiLONG_DELAY );

		/* Is the SD card inserted?  Are the jumpers set correctly to connect
		the SD card to the SPI bus?  See the web documentation for jumper
		settings. */
		configASSERT( ( volatile void * ) NULL );
	}

	/* Mount the drive.  This need not be in a loop. */
	while( f_mount( spiDRIVE_NUMBER, &xFatfs ) != FR_OK )
	{
		/* The drive could not be mounted.  Wait a while, then try again. */
		vTaskDelay( spiLONG_DELAY );
	}

	/* Initialising the disk will have opened an SSP port in SPI mode.  Obtain
	the handle to the opened port from the MMC driver so it can be manipulated
	from this test/demo file. */
	xSPIPortUsedByMMCDriver = xMMCGetSPIPortHandle();

	/* The FIL object is too large to store on the stack of this task.  It is
	instead created dynamically here, and it is never freed because if the
	allocation succeeds the FIL object is used for the lifetime of the task. */
	pxFile = pvPortMalloc( sizeof( FIL ) );
	configASSERT( pxFile );

	if( pxFile != NULL )
	{
		/* Mounting the drive will have opened the SPI port in polled mode.
		Demonstrate FreeRTOS+IO accessing an SPI port (in the SD card driver
		implemented in mmc.c) in polled mode by creating a set of files, then
		reading the files back to ensure they were created correctly. */
		prvWriteFilesToDisk( ( int8_t * ) "poll", pxFile );
		prvReadBackCreatedFiles( ( int8_t * ) "poll", pxFile );

		/* Now set the SPI Tx to use zero copy mode and Rx to use circular buffer
		mode, before creating and reading back the files again. */
		FreeRTOS_ioctl( xSPIPortUsedByMMCDriver, ioctlUSE_ZERO_COPY_TX, cmdPARAMTER_NOT_USED );
		FreeRTOS_ioctl( xSPIPortUsedByMMCDriver, ioctlUSE_CIRCULAR_BUFFER_RX, ( void * ) spiBUFFER_SIZE );

		/* The SPI port is now fully interrupt driven.	 Demonstrate FreeRTOS+IO
		accessing an SPI port in this mode (in the SD card driver implemented in
		mmc.c) by creating the set of files again, then again reading the files
		back to ensure they were created correctly. */
		prvWriteFilesToDisk( ( int8_t * ) "int", pxFile );
		prvReadBackCreatedFiles( ( int8_t * ) "int", pxFile );

		/* Now set the SPI Tx to use interrupt driven character queue Rx and Tx. */
		FreeRTOS_ioctl( xSPIPortUsedByMMCDriver, ioctlUSE_CHARACTER_QUEUE_TX, ( void * ) spiBUFFER_SIZE );
		FreeRTOS_ioctl( xSPIPortUsedByMMCDriver, ioctlUSE_CHARACTER_QUEUE_RX, ( void * ) spiBUFFER_SIZE );
		FreeRTOS_ioctl( xSPIPortUsedByMMCDriver, ioctlSET_RX_TIMEOUT, ( void * ) 5000UL );
		FreeRTOS_ioctl( xSPIPortUsedByMMCDriver, ioctlSET_TX_TIMEOUT, ( void * ) 5000UL );

		/* The SPI port is fully interrupt driven, and using FreeRTOS queues to
		send and receive data into and out of the SPI interrupt.  NOTE: This is not
		an efficient method when large blocks of data are being transferred - it is
		done here purely to demonstrate the method on an SPI port. */
		prvWriteFilesToDisk( ( int8_t * ) "Q", pxFile );
		prvReadBackCreatedFiles( ( int8_t * ) "Q", pxFile );
	}

	/* From here on the files exist, and disk and directory structure queried
	using the command interpreter.  There is nothing more for this task to do.
	The task could be deleted, but in this case it is left running so its stack
	usage can be inspected. */
	for( ;; )
	{
		vTaskDelay( 1000 );
	}
}
/*-----------------------------------------------------------*/

static void prvFileOperationFailed( int32_t lLine )
{
	/* Force an assertion fail so the line that failed can be inspected in the
	debugger. */
	configASSERT( lLine == 0L );

	/* Remove compiler options when configASSERT() is not defined. */
	( void ) lLine;
}
/*-----------------------------------------------------------*/

static void prvWriteFilesToDisk( int8_t *pcPartFileName, FIL *pxFile )
{
portBASE_TYPE xFileNumber, xWriteNumber;
TCHAR cFileName[ spiMAX_FILE_NAME_LEN ];
const portBASE_TYPE xMaxFiles = 20;
UINT xBytesWritten;
const portTickType xMaxDelay = 500UL / portTICK_RATE_MS;

	/* Create xMaxFiles files.  Each created file will be
	( xFileNumber * spiRAM_BUFFER_SIZE ) bytes in length, and filled
	with a different repeating character. */
	for( xFileNumber = 1; xFileNumber < xMaxFiles; xFileNumber++ )
	{
		/* Generate a file name. */
		sprintf( cFileName, "%d:/%03d%s.txt", spiDRIVE_NUMBER, xFileNumber, ( char * ) pcPartFileName );

		/* Open the file, creating the file if it does not already exist. */
		if( f_open( pxFile, cFileName, ( FA_CREATE_ALWAYS | FA_WRITE ) ) != FR_OK )
		{
			prvFileOperationFailed( __LINE__ );
		}

		/* About to use the RAM buffer, ensure this task has exclusive access
		to it while it is in use. */
		if( xSemaphoreTake( xRamBufferMutex, xMaxDelay ) == pdPASS )
		{
			/* Fill the RAM buffer with data that will be written to the file. */
			memset( cRAMBuffer, ( int ) ( '0' + xFileNumber ), spiRAM_BUFFER_SIZE );

			/* Write the RAM buffer to the opened file a number of times.  The
			number of times the RAM buffer is written to the file depends on the
			file number, so the length of each created file will be different. */
			for( xWriteNumber = 0; xWriteNumber < xFileNumber; xWriteNumber++ )
			{
				if( f_write( pxFile, cRAMBuffer, ( spiRAM_BUFFER_SIZE - xWriteNumber ), &xBytesWritten ) != FR_OK )
				{
					prvFileOperationFailed( __LINE__ );
				}

				if( xBytesWritten != ( UINT ) ( spiRAM_BUFFER_SIZE - xWriteNumber ) )
				{
					prvFileOperationFailed( __LINE__ );
				}
			}
			
			/* Must return the mutex! */
			xSemaphoreGive( xRamBufferMutex );
		}

		/* Close the file. */
		if( f_close( pxFile ) != FR_OK )
		{
			prvFileOperationFailed( __LINE__ );
		}
	}
}
/*-----------------------------------------------------------*/

static void prvReadBackCreatedFiles( int8_t *pcPartFileName, FIL *pxFile )
{
portBASE_TYPE xFileNumber, xReadNumber, xByte;
const portBASE_TYPE xMaxFiles = 20;
static TCHAR cFileName[ spiMAX_FILE_NAME_LEN ];
UINT xBytesRead;
const portTickType xMaxDelay = 500UL / portTICK_RATE_MS;

	/* Create xMaxFiles files.  Each created file will be
	( xFileNumber * spiRAM_BUFFER_SIZE ) bytes in length, and filled
	with a different repeating character. */
	for( xFileNumber = 1; xFileNumber < xMaxFiles; xFileNumber++ )
	{
		/* Generate a file name. */
		sprintf( cFileName, "%d:/%03d%s.txt", spiDRIVE_NUMBER, xFileNumber, ( char * ) pcPartFileName );

		/* Open the file for reading. */
		if( f_open( pxFile, cFileName, FA_READ ) != FR_OK )
		{
			prvFileOperationFailed( __LINE__ );
		}

		/* Open a file and read its contents into the RAM buffer.  The contents
		of the file as well as the file length can be derived from the
		xFileNumber variable, as this is how the files were created in the
		first place within the prvWriteFilesToDisk() function (implemented in
		this file). */
		for( xReadNumber = 0; xReadNumber < xFileNumber; xReadNumber++ )
		{
			/* About to use the RAM buffer, ensure this task has exclusive access
			to it while it is in use. */
			if( xSemaphoreTake( xRamBufferMutex, xMaxDelay ) == pdPASS )
			{
				/* Zero out the RAM buffer as a starting condition. */
				memset( cRAMBuffer, 0, spiRAM_BUFFER_SIZE );

				if( f_read( pxFile, cRAMBuffer, ( spiRAM_BUFFER_SIZE - xReadNumber ), &xBytesRead ) != FR_OK )
				{
					prvFileOperationFailed( __LINE__ );
				}

				if( xBytesRead != ( UINT ) ( spiRAM_BUFFER_SIZE - xReadNumber ) )
				{
					prvFileOperationFailed( __LINE__ );
				}

				/* Check each byte in the buffer is as expected. */
				for( xByte = 0; xByte < ( spiRAM_BUFFER_SIZE - xReadNumber ); xByte++ )
				{
					if( cRAMBuffer[ xByte ] != ( '0' + xFileNumber ) )
					{
						prvFileOperationFailed( __LINE__ );
					}
				}
				
				/* Must give the mutex back! */
				xSemaphoreGive( xRamBufferMutex );
			}			
		}

		/* Attempting to read the file now should return 0 bytes. */
		if( f_read( pxFile, cRAMBuffer, spiRAM_BUFFER_SIZE, &xBytesRead ) != FR_OK )
		{
			prvFileOperationFailed( __LINE__ );
		}

		if( xBytesRead != 0 )
		{
			prvFileOperationFailed( __LINE__ );
		}

		/* Close the file. */
		if( f_close( pxFile ) != FR_OK )
		{
			prvFileOperationFailed( __LINE__ );
		}
	}
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvDirCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
static portBASE_TYPE xDirectoryOpen = pdFALSE;
static FILINFO xFileInfo;
static DIR xDirectory;
portBASE_TYPE xReturn;

	/* This assumes pcWriteBuffer is long enough. */
	( void ) pcCommandString;

	if( xDirectoryOpen == pdFALSE )
	{
		/* This is the first time the function has been called this run of the
		dir command.  Ensure the directory is open. */
		if( f_opendir( &xDirectory, "/" ) == FR_OK )
		{
			xDirectoryOpen = pdTRUE;
		}
		else
		{
			xDirectoryOpen = pdFALSE;
			xReturn = pdFALSE;
		}
	}

	if( xDirectoryOpen == pdTRUE )
	{
		/* Read the next file. */
		if( f_readdir( &xDirectory, &xFileInfo ) == FR_OK )
		{
			if( xFileInfo.fname[ 0 ] != '\0' )
			{
				/* There is at least one more file name to return. */
				snprintf( ( char * ) pcWriteBuffer, xWriteBufferLen, "%s\t\t%d\r\n", xFileInfo.fname, xFileInfo.fsize );
				xReturn = pdTRUE;

			}
			else
			{
				/* There are no more file names to return.   Reset the read
				pointer ready for the next time this directory is read. */
				f_readdir( &xDirectory, NULL );
				xReturn = pdFALSE;
				xDirectoryOpen = pdFALSE;
				pcWriteBuffer[ 0 ] = 0x00;
			}
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvDelCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
int8_t *pcParameter;
portBASE_TYPE xParameterStringLength;
const unsigned portBASE_TYPE uxFirstParameter = 1U;

	/* This assumes pcWriteBuffer is long enough. */
	( void ) xWriteBufferLen;

	/* Obtain the name of the file being deleted. */
	pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter( pcCommandString, uxFirstParameter, &xParameterStringLength );

	/* Terminate the parameter string. */
	pcParameter[ xParameterStringLength ] = 0x00;

	if( f_unlink( ( const TCHAR * ) pcParameter ) != FR_OK )
	{
		snprintf( ( char * ) pcWriteBuffer, xWriteBufferLen, "Could not delete %s\r\n\r\n", pcParameter );
	}

	/* There is only ever one string to return. */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvCopyCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
int8_t *pcParameter1, *pcParameter2;
portBASE_TYPE xParameter1StringLength, xParameter2StringLength, xFinished = pdFALSE;
const unsigned portBASE_TYPE uxFirstParameter = 1U, uxSecondParameter = 2U;
UINT xBytesRead, xBytesWritten;
const portTickType xMaxDelay = 500UL / portTICK_RATE_MS;

	( void ) xWriteBufferLen;

	/* Obtain the name of the source file, and the length of its name. */
	pcParameter1 = ( int8_t * ) FreeRTOS_CLIGetParameter( pcCommandString, uxFirstParameter, &xParameter1StringLength );

	/* Obtain the name of the destination file, and the length of its name. */
	pcParameter2 = ( int8_t * ) FreeRTOS_CLIGetParameter( pcCommandString, uxSecondParameter, &xParameter2StringLength );

	/* Terminate both file names. */
	pcParameter1[ xParameter1StringLength ] = 0x00;
	pcParameter2[ xParameter2StringLength ] = 0x00;

	/* Open the source file. */
	if( f_open( &xCommandLineFile1, ( const TCHAR * ) pcParameter1, ( FA_OPEN_EXISTING | FA_READ ) ) == FR_OK )
	{
		/* Open the destination file. */
		if( f_open( &xCommandLineFile2, ( const TCHAR * ) pcParameter2, ( FA_CREATE_ALWAYS | FA_WRITE ) ) == FR_OK )
		{
			while( xFinished == pdFALSE )
			{
				/* About to use the RAM buffer, ensure this task has exclusive access
				to it while it is in use. */
				if( xSemaphoreTake( xRamBufferMutex, xMaxDelay ) == pdPASS )
				{
					if( f_read( &xCommandLineFile1, cRAMBuffer, sizeof( cRAMBuffer ), &xBytesRead ) == FR_OK )
					{
						if( xBytesRead != 0U )
						{
							if( f_write( &xCommandLineFile2, cRAMBuffer, xBytesRead, &xBytesWritten ) == FR_OK )
							{
								if( xBytesWritten < xBytesRead )
								{
									snprintf( ( char * ) pcWriteBuffer, xWriteBufferLen, "Error writing to %s, disk full?\r\n\r\n", pcParameter2 );
									xFinished = pdTRUE;
								}
							}
							else
							{
								snprintf( ( char * ) pcWriteBuffer, xWriteBufferLen, "Error during copy\r\n\r\n" );
								xFinished = pdTRUE;
							}
						}
						else
						{
							/* EOF. */
							xFinished = pdTRUE;
						}
					}
					else
					{
						snprintf( ( char * ) pcWriteBuffer, xWriteBufferLen, "Error during copy\r\n\r\n" );
						xFinished = pdTRUE;
					}
					
					/* Must give the mutex back! */
					xSemaphoreGive( xRamBufferMutex );
				}				
			}

			/* Close both files. */
			f_close( &xCommandLineFile1 );
			f_close( &xCommandLineFile2 );
		}
		else
		{
			snprintf( ( char * ) pcWriteBuffer, xWriteBufferLen, "Could not open or create %s\r\n\r\n", pcParameter2 );

			/* Close the source file. */
			f_close( &xCommandLineFile1 );
		}
	}
	else
	{
		snprintf( ( char * ) pcWriteBuffer, xWriteBufferLen, "Could not open %s\r\n\r\n", pcParameter1 );
	}

	return pdFALSE;
}
/*-----------------------------------------------------------*/



