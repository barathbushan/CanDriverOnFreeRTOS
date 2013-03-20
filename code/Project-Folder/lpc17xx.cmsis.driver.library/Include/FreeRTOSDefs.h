/* Define USE_FREERTOS on the command line, in lpc17xx_libcfg_default.h, or in
this file to make the driver library FreeRTOS and multitasking aware. */

#define USE_FREERTOS

#ifdef USE_FREERTOS

	/* Include the real definitions of the FreeRTOS functions used in the
	driver library. */
	#include "FreeRTOS.h"
	#include "task.h"

#else /* USE_FREERTOS */

	/* Stub out the FreeRTOS functions/macros that are used (unguarded) in the
	driver library. */
	#define taskENTER_CRITICAL()
	#define taskEXIT_CRITICAL()

#endif /* USE_FREERTOS */

	
	
