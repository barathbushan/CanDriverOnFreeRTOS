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
#include "timers.h"

/* FreeRTOS+IO includes. */
#include "FreeRTOS_IO.h"

/* Library includes. */
#include "lpc17xx_gpio.h"

/* Example includes. */
#include "GPIO-output-and-software-timers.h"

/* The period assigned to the xGPIOTimer software timer. prvGPIOTimerCallback
will be called every gpioSOFTARE_TIMER_PERIOD_MS milliseconds. */
#define gpioSOFTWARE_TIMER_PERIOD_MS		( 200 / portTICK_RATE_MS )

/*
 * The callback function assigned to xGPIOTimer.
 */
static void prvGPIOTimerCallback( xTimerHandle xTimer );

/* The timer variable itself. */
static xTimerHandle xGPIOTimer = NULL;

/* An array that contains the port number of each used LED. */
const uint8_t ucLEDPorts[ boardNUM_SOFTWARE_TIMER_LEDS ] = boardSOFTWARE_TIMER_LED_PORT_INITIALISER;

/* An array that contains the bit numbers of each used LED. */
uint32_t ulLEDBits[ boardNUM_SOFTWARE_TIMER_LEDS ] = boardSOFTWARE_TIMER_LED_INITIALISER;

/*-----------------------------------------------------------*/

void vGPIOSoftwareTimersStart( void )
{
int32_t lx;

	/* Set the LED outputs to output. */
	for( lx = 0; lx < boardNUM_SOFTWARE_TIMER_LEDS; lx++ )
	{
		GPIO_SetDir( ucLEDPorts[ lx ], ulLEDBits[ lx ], boardGPIO_OUTPUT );
	}

    /* Create the timer, if it has not already been created by a previous call
    to this function. */
    if( xGPIOTimer == NULL )
    {
		xGPIOTimer = xTimerCreate(	( const int8_t * ) "GPIOTmr", 	/* Just a text name to associate with the timer, useful for debugging, but not used by the kernel. */
									gpioSOFTWARE_TIMER_PERIOD_MS,		/* The period of the timer. */
									pdTRUE,								/* This timer will autoreload, so uxAutoReload is set to pdTRUE. */
									NULL,								/* The timer ID is not used, so can be set to NULL. */
									prvGPIOTimerCallback );				/* The callback function executed each time the timer expires. */
    }

    /* Sanity check that the timer was actually created. */
    configASSERT( xGPIOTimer );

    /* Start the timer.  If this is called before the scheduler is started then
    the block time will automatically get changed to 0 (from portMAX_DELAY). */
    lx = xTimerStart( xGPIOTimer, portMAX_DELAY );

    /* Sanity check that the start command was sent to the timer command
    queue. */
    configASSERT( lx );
}
/*-----------------------------------------------------------*/

static void prvGPIOTimerCallback( xTimerHandle xTimer )
{
static uint8_t ucLED = 0U, ucState = boardLED_ON;

	/* In this case the parameter is not used, so ensure compiler warnings
	are not generated. */
	( void ) xTimer;

	/* Turn all the LEDs on, one by one, then, when all are on, turn each LED
	off again one by one. */
	if( ucState == boardLED_ON )
	{
		/* Turn the next LED on. */
		GPIO_SetValue( ucLEDPorts[ ucLED ], ulLEDBits[ ucLED ] );
	}
	else
	{
		/* Turn the next LED off. */
		GPIO_ClearValue( ucLEDPorts[ ucLED ], ulLEDBits[ ucLED ] );
	}

	ucLED++;
	if( ucLED >= boardNUM_SOFTWARE_TIMER_LEDS )
	{
		/* All the LEDs have now either been turned on, or off.  Go back to the
		first LED and invert the setting. */
		ucLED = 0U;

		if( ucState == boardLED_ON )
		{
			ucState = boardLED_OFF;
		}
		else
		{
			ucState = boardLED_ON;
		}
	}
}
/*-----------------------------------------------------------*/




