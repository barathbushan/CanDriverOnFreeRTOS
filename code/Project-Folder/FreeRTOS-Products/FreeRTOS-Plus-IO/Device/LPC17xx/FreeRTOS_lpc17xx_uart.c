/*
 * FreeRTOS+IO V1.0.1 (C) 2012 Real Time Engineers ltd.
 *
 * FreeRTOS+IO is an add-on component to FreeRTOS.  It is not, in itself, part 
 * of the FreeRTOS kernel.  FreeRTOS+IO is licensed separately from FreeRTOS, 
 * and uses a different license to FreeRTOS.  FreeRTOS+IO uses a dual license
 * model, information on which is provided below:
 *
 * - Open source licensing -
 * FreeRTOS+IO is a free download and may be used, modified and distributed
 * without charge provided the user adheres to version two of the GNU General
 * Public license (GPL) and does not remove the copyright notice or this text.
 * The GPL V2 text is available on the gnu.org web site, and on the following
 * URL: http://www.FreeRTOS.org/gpl-2.0.txt
 *
 * - Commercial licensing -
 * Businesses and individuals who wish to incorporate FreeRTOS+IO into
 * proprietary software for redistribution in any form must first obtain a low
 * cost commercial license - and in-so-doing support the maintenance, support
 * and further development of the FreeRTOS+IO product.  Commercial licenses can
 * be obtained from http://shop.freertos.org and do not require any source files
 * to be changed.
 *
 * FreeRTOS+IO is distributed in the hope that it will be useful.  You cannot
 * use FreeRTOS+IO unless you agree that you use the software 'as is'.
 * FreeRTOS+IO is provided WITHOUT ANY WARRANTY; without even the implied
 * warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. Real Time Engineers Ltd. disclaims all conditions and terms, be they
 * implied, expressed, or statutory.
 *
 * 1 tab == 4 spaces!
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/FreeRTOS-Plus
 *
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* IO library includes. */
#include "FreeRTOS_IO.h"
#include "IOUtils_Common.h"
#include "FreeRTOS_uart.h"

/* Hardware setup peripheral driver includes.  The includes for the UART itself
is already included from FreeRTOS_IO_BSP.h. */
#include "lpc17xx_pinsel.h"

/* The bits in the FIFOLVL register that represent the Tx Fifo level. */
#define uartTX_FIFO_LEVEL_MASK		( 0xf00UL )

/* The TEMT bit in the line status register. */
#define uartTX_BUSY_MASK			( 1UL << 6UL )

/*-----------------------------------------------------------*/

/*
 * Write as many characters as possible from *ppucBuffer into the UART FIFO.
 */
static inline size_t prvFillFifoFromBuffer( LPC_UART_TypeDef * const pxUART, uint8_t **ppucBuffer, const size_t xTotalBytes );

/*-----------------------------------------------------------*/

/* Stores the transfer control structures that are currently in use by the
supported UART ports. */
static Transfer_Control_t *pxTxTransferControlStructs[ boardNUM_UARTS ] = { NULL };
static Transfer_Control_t *pxRxTransferControlStructs[ boardNUM_UARTS ] = { NULL };

/* Stores the IRQ numbers of the supported UART ports. */
static const IRQn_Type xIRQ[] = { UART0_IRQn, UART1_IRQn, UART2_IRQn, UART3_IRQn };

/*-----------------------------------------------------------*/

portBASE_TYPE FreeRTOS_UART_open( Peripheral_Control_t * const pxPeripheralControl )
{
PINSEL_CFG_Type xPinConfig;
UART_CFG_Type xUARTConfig;
UART_FIFO_CFG_Type xUARTFIFOConfig;
LPC_UART_TypeDef * const pxUART = ( LPC_UART_TypeDef * const ) diGET_PERIPHERAL_BASE_ADDRESS( pxPeripheralControl );
portBASE_TYPE xReturn;
const uint8_t cPeripheralNumber = diGET_PERIPHERAL_NUMBER( pxPeripheralControl );

	/* Sanity check the peripheral number. */
	if( cPeripheralNumber < boardNUM_UARTS )
	{
		pxPeripheralControl->read = FreeRTOS_UART_read;
		pxPeripheralControl->write = FreeRTOS_UART_write;
		pxPeripheralControl->ioctl = FreeRTOS_UART_ioctl;

		/* Setup the pins for the UART being used. */
		taskENTER_CRITICAL();
		{
			boardCONFIGURE_UART_PINS( cPeripheralNumber, xPinConfig );

			/* Set up the default UART configuration. */
			xUARTConfig.Baud_rate = boardDEFAULT_UART_BAUD;
			xUARTConfig.Databits = UART_DATABIT_8;
			xUARTConfig.Parity = UART_PARITY_NONE;
			xUARTConfig.Stopbits = UART_STOPBIT_1;
			UART_Init( pxUART, &xUARTConfig );

			/* Enable the FIFO. */
			xUARTFIFOConfig.FIFO_ResetRxBuf = ENABLE;
			xUARTFIFOConfig.FIFO_ResetTxBuf = ENABLE;
			xUARTFIFOConfig.FIFO_DMAMode = DISABLE;
			xUARTFIFOConfig.FIFO_Level = UART_FIFO_TRGLEV2;
			UART_FIFOConfig( pxUART, &xUARTFIFOConfig );

			/* Enable Tx. */
			UART_TxCmd( pxUART, ENABLE );
		}
		taskEXIT_CRITICAL();

		xReturn = pdPASS;
	}
	else
	{
		xReturn = pdFAIL;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

size_t FreeRTOS_UART_write( Peripheral_Descriptor_t const pxPeripheral, const void *pvBuffer, const size_t xBytes )
{
Peripheral_Control_t * const pxPeripheralControl = ( Peripheral_Control_t * const ) pxPeripheral;
size_t xReturn = 0U;
LPC_UART_TypeDef * const pxUART = ( LPC_UART_TypeDef * const ) diGET_PERIPHERAL_BASE_ADDRESS( ( ( Peripheral_Control_t * const ) pxPeripheral ) );
int8_t cPeripheralNumber;

	if( diGET_TX_TRANSFER_STRUCT( pxPeripheralControl ) == NULL )
	{
		#if ioconfigUSE_UART_POLLED_TX == 1
		{
			/* No FreeRTOS objects exist to allow transmission without blocking
			the	task, so just send out by polling.  No semaphore or queue is
			used here, so the application must ensure only one task attempts to
			make a polling write at a time. */
			xReturn = UART_Send( pxUART, ( uint8_t * ) pvBuffer, ( size_t ) xBytes, BLOCKING );

			/* The UART is set to polling mode, so may as well poll the busy bit
			too.  Change to interrupt driven mode to avoid wasting CPU time here. */
			while( UART_CheckBusy( pxUART ) != RESET );
		}
		#endif /* ioconfigUSE_UART_POLLED_TX */
	}
	else
	{
		/* Remember which transfer control structure is being used.
		The Tx interrupt will use this to continue to write data to the
		Tx FIFO/UART until the length member of the structure reaches
		zero. */
		cPeripheralNumber = diGET_PERIPHERAL_NUMBER( pxPeripheralControl );
		pxTxTransferControlStructs[ cPeripheralNumber  ] = diGET_TX_TRANSFER_STRUCT( pxPeripheralControl );

		switch( diGET_TX_TRANSFER_TYPE( pxPeripheralControl ) )
		{
			case ioctlUSE_ZERO_COPY_TX :

				#if ioconfigUSE_UART_ZERO_COPY_TX == 1
				{
					/* The implementation of the zero copy write uses a semaphore
					to indicate whether a write is complete (and so the buffer
					being written free again) or not.  The semantics of using a
					zero copy write dictate that a zero copy write can only be
					attempted by a task, once the semaphore has been successfully
					obtained by that task.  This ensure that only one task can
					perform a zero copy write at any one time.  Ensure the semaphore
					is not currently available, if this function has been called
					without it being obtained first then it is an error. */
					configASSERT( xIOUtilsGetZeroCopyWriteMutex( pxPeripheralControl, ioctlOBTAIN_WRITE_MUTEX, 0U ) == 0 );
					xReturn = xBytes;
					ioutilsINITIATE_ZERO_COPY_TX
						(
							pxPeripheralControl,
							UART_TxCmd( pxUART, DISABLE ),	/* Disable peripheral function. */
							UART_TxCmd( pxUART, ENABLE ), 	/* Enable peripheral function. */
							prvFillFifoFromBuffer( pxUART, ( uint8_t ** ) &( pvBuffer ), xBytes ), /* Write to peripheral function. */
							pvBuffer, 						/* Data source. */
							xReturn							/* Number of bytes to be written. This will get set to zero if the write mutex is not held. */
						);
				}
				#endif /* ioconfigUSE_UART_ZERO_COPY_TX */
				break;


			case ioctlUSE_CHARACTER_QUEUE_TX :

				#if ioconfigUSE_UART_TX_CHAR_QUEUE == 1
				{
					/* The queue allows multiple tasks to attempt to write
					bytes, but ensures only the highest priority of these tasks
					will actually succeed.  If two tasks of equal priority
					attempt to write simultaneously, then the application must
					ensure mutual exclusion, as time slicing could result in
					the strings being sent to the queue being interleaved. */
					ioutilsBLOCKING_SEND_CHARS_TO_TX_QUEUE
						(
							pxPeripheralControl,
							( pxUART->LSR & uartTX_BUSY_MASK ) == uartTX_BUSY_MASK,  /* Peripheral busy condition. */
							pxUART->THR = ucChar,				/* Peripheral write function. */
							( ( uint8_t * ) pvBuffer ),			/* Data source. */
							xBytes, 							/* Number of bytes to be written. */
							xReturn );
				}
				#endif /* ioconfigUSE_UART_TX_CHAR_QUEUE */
				break;


			default :

				/* Other methods can be implemented here.  For now set the
				stored transfer structure back to NULL as nothing is being
				sent. */
				configASSERT( xReturn );
				pxTxTransferControlStructs[ cPeripheralNumber ] = NULL;

				/* Prevent compiler warnings when the configuration is set such
				that the following parameters are not used. */
				( void ) pvBuffer;
				( void ) xBytes;
				( void ) pxUART;
				break;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

size_t FreeRTOS_UART_read( Peripheral_Descriptor_t const pxPeripheral, void * const pvBuffer, const size_t xBytes )
{
Peripheral_Control_t * const pxPeripheralControl = ( Peripheral_Control_t * const ) pxPeripheral;
size_t xReturn = 0U;
LPC_UART_TypeDef * const pxUART = ( LPC_UART_TypeDef * const ) diGET_PERIPHERAL_BASE_ADDRESS( ( ( Peripheral_Control_t * const ) pxPeripheral ) );

	if( diGET_RX_TRANSFER_STRUCT( pxPeripheralControl ) == NULL )
	{
		#if ioconfigUSE_UART_POLLED_RX == 1
		{
			/* No FreeRTOS objects exist to allow reception without blocking
			the task, so just receive by polling.  No semaphore or queue is
			used here, so the application must ensure only one task attempts
			to make a polling read at a time. */
			xReturn = UART_Receive( pxUART, pvBuffer, xBytes, NONE_BLOCKING );
		}
		#endif /* ioconfigUSE_UART_POLLED_RX */
	}
	else
	{
		/* Sanity check the array index. */
		configASSERT( diGET_PERIPHERAL_NUMBER( pxPeripheralControl ) < ( int8_t ) ( sizeof( xIRQ ) / sizeof( IRQn_Type ) ) );

		switch( diGET_RX_TRANSFER_TYPE( pxPeripheralControl ) )
		{
			case ioctlUSE_CIRCULAR_BUFFER_RX :

				#if ioconfigUSE_UART_CIRCULAR_BUFFER_RX == 1
				{
					/* There is nothing to prevent multiple tasks attempting to
					read the circular buffer at any one time.  The implementation
					of the circular buffer uses a semaphore to indicate when new
					data is available, and the semaphore will ensure that only the
					highest priority task that is attempting a read will actually
					receive bytes. */
					ioutilsRECEIVE_CHARS_FROM_CIRCULAR_BUFFER
						(
							pxPeripheralControl,
							UART_IntConfig( pxUART, UART_INTCFG_RBR, DISABLE ),	/* Disable peripheral. */
							UART_IntConfig( pxUART, UART_INTCFG_RBR, ENABLE ), 	/* Enable peripheral. */
							( ( uint8_t * ) pvBuffer ),							/* Data destination. */
							xBytes,												/* Bytes to read. */
							xReturn												/* Number of bytes read. */
						);
				}
				#endif /* ioconfigUSE_UART_CIRCULAR_BUFFER_RX */
				break;


			case ioctlUSE_CHARACTER_QUEUE_RX :

				#if ioconfigUSE_UART_RX_CHAR_QUEUE == 1
				{
					/* The queue allows multiple tasks to attempt to read
					bytes, but ensures only the highest priority of these
					tasks will actually receive bytes.  If two tasks of equal
					priority attempt to read simultaneously, then the
					application must ensure mutual exclusion, as time slicing
					could result in the string being received being partially
					received by each task. */
					xReturn = xIOUtilsReceiveCharsFromRxQueue( pxPeripheralControl, ( uint8_t * ) pvBuffer, xBytes );
				}
				#endif /* ioconfigUSE_UART_RX_CHAR_QUEUE */
				break;


			default :

				/* Other methods can be implemented here. */
				configASSERT( xReturn );

				/* Prevent compiler warnings when the configuration is set such
				that the following parameters are not used. */
				( void ) pvBuffer;
				( void ) xBytes;
				( void ) pxUART;
				break;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static inline size_t prvFillFifoFromBuffer( LPC_UART_TypeDef * const pxUART, uint8_t **ppucBuffer, const size_t xTotalBytes )
{
size_t xBytesSent = 0U;

	/* This function is only used by zero copy transmissions, so mutual
	exclusion is already taken care of by the fact that a task must first
	obtain a semaphore before initiating a zero copy transfer.  The semaphore
	is part of the zero copy structure, not part of the application. */
	while( ( pxUART->FIFOLVL & uartTX_FIFO_LEVEL_MASK ) != uartTX_FIFO_LEVEL_MASK )
	{
		if( xBytesSent >= xTotalBytes )
		{
			break;
		}
		else
		{
			pxUART->THR = **ppucBuffer;
			( *ppucBuffer )++;
			xBytesSent++;
		}
	}

	return xBytesSent;
}
/*-----------------------------------------------------------*/

portBASE_TYPE FreeRTOS_UART_ioctl( Peripheral_Descriptor_t pxPeripheral, uint32_t ulRequest, void *pvValue )
{
Peripheral_Control_t * const pxPeripheralControl = ( Peripheral_Control_t * const ) pxPeripheral;
UART_CFG_Type xUARTConfig;
uint32_t ulValue = ( uint32_t ) pvValue;
const int8_t cPeripheralNumber = diGET_PERIPHERAL_NUMBER( ( ( Peripheral_Control_t * const ) pxPeripheral ) );
LPC_UART_TypeDef * pxUART = ( LPC_UART_TypeDef * ) diGET_PERIPHERAL_BASE_ADDRESS( ( ( Peripheral_Control_t * const ) pxPeripheral ) );
portBASE_TYPE xReturn = pdPASS;

	/* Sanity check the array index. */
	configASSERT( cPeripheralNumber < ( int8_t ) ( sizeof( xIRQ ) / sizeof( IRQn_Type ) ) );

	taskENTER_CRITICAL();
	{
		switch( ulRequest )
		{
			case ioctlUSE_INTERRUPTS :

				if( ulValue == pdFALSE )
				{
					NVIC_DisableIRQ( xIRQ[ cPeripheralNumber ] );
				}
				else
				{
					/* Enable the Rx and Tx interrupt. */
					UART_IntConfig( pxUART, UART_INTCFG_RBR, ENABLE );
					UART_IntConfig( pxUART, UART_INTCFG_THRE, ENABLE );

					/* Enable the interrupt and set its priority to the minimum
					interrupt priority.  A separate command can be issued to raise
					the priority if desired. */
					NVIC_SetPriority( xIRQ[ cPeripheralNumber ], configMIN_LIBRARY_INTERRUPT_PRIORITY );
					NVIC_EnableIRQ( xIRQ[ cPeripheralNumber ] );

					/* If the Rx is configured to use interrupts, remember the
					transfer control structure that should be used.  A reference
					to the Tx transfer control structure is taken when a write()
					operation is actually performed. */
					pxRxTransferControlStructs[ cPeripheralNumber ] = pxPeripheralControl->pxRxControl;
				}
				break;


			case ioctlSET_SPEED :

				/* Set up the default UART configuration. */
				xUARTConfig.Baud_rate = ulValue;
				xUARTConfig.Databits = UART_DATABIT_8;
				xUARTConfig.Parity = UART_PARITY_NONE;
				xUARTConfig.Stopbits = UART_STOPBIT_1;
				UART_Init( pxUART, &xUARTConfig );
				break;


			case ioctlSET_INTERRUPT_PRIORITY :

				/* The ISR uses ISR safe FreeRTOS API functions, so the priority
				being set must be lower than (ie numerically larger than)
				configMAX_LIBRARY_INTERRUPT_PRIORITY. */
				configASSERT( ulValue >= configMAX_LIBRARY_INTERRUPT_PRIORITY );
				NVIC_SetPriority( xIRQ[ cPeripheralNumber ], ulValue );
				break;


			default :

				xReturn = pdFAIL;
				break;
		}
	}
	taskEXIT_CRITICAL();

	return xReturn;
}
/*-----------------------------------------------------------*/


#if ioconfigINCLUDE_UART != 1
	/* If the UART driver is not being used, rename the interrupt handler.  This
	will prevent it being installed in the vector table.  The linker will then
	identify it as unused code, and remove it from the binary image. */
	#define UART3_IRQHandler Unused_UART3_IRQHandler
#endif /* ioconfigINCLUDE_UART */
#if 0
void UART3_IRQHandler( void )
{
uint32_t ulInterruptSource, ulReceived;
const uint32_t ulRxInterrupts = ( UART_IIR_INTID_RDA | UART_IIR_INTID_CTI );
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
const unsigned portBASE_TYPE uxUARTNumber = 3UL;
Transfer_Control_t *pxTransferStruct;

	/* Determine the interrupt source. */
	ulInterruptSource = UART_GetIntId( LPC_UART3 );

	if( ( ulInterruptSource & ulRxInterrupts ) != 0UL )
	{
		pxTransferStruct = pxRxTransferControlStructs[ uxUARTNumber ];
		if( pxTransferStruct != NULL )
		{
			switch( diGET_TRANSFER_TYPE_FROM_CONTROL_STRUCT( pxTransferStruct ) )
			{
				case ioctlUSE_CIRCULAR_BUFFER_RX :

					#if ioconfigUSE_UART_CIRCULAR_BUFFER_RX == 1
					{
						ioutilsRX_CHARS_INTO_CIRCULAR_BUFFER_FROM_ISR(
																	pxTransferStruct, 	/* The structure that contains the reference to the circular buffer. */
																	( ( LPC_UART3->LSR & UART_LSR_RDR ) != 0 ), 	/* While loop condition. */
																	LPC_UART3->RBR,			/* Register holding the received character. */
																	ulReceived,
																	xHigherPriorityTaskWoken
																);
					}
					#endif /* ioconfigUSE_UART_CIRCULAR_BUFFER_RX */
					break;


				case ioctlUSE_CHARACTER_QUEUE_RX :

					#if ioconfigUSE_UART_RX_CHAR_QUEUE == 1
					{
						ioutilsRX_CHARS_INTO_QUEUE_FROM_ISR( pxTransferStruct, ( ( LPC_UART3->LSR & UART_LSR_RDR ) != 0 ), LPC_UART3->RBR, ulReceived, xHigherPriorityTaskWoken );
					}
					#endif /* ioconfigUSE_UART_RX_CHAR_QUEUE */
					break;


				default :

					/* This must be an error.  Force an assert. */
					configASSERT( xHigherPriorityTaskWoken );
					break;
			}
		}
	}

	if( ( ulInterruptSource & UART_IIR_INTID_THRE ) != 0UL )
	{
		/* The transmit holding register is empty.  Is there any more data
		to send? */
		pxTransferStruct = pxTxTransferControlStructs[ uxUARTNumber ];
		if( pxTransferStruct != NULL )
		{
			switch( diGET_TRANSFER_TYPE_FROM_CONTROL_STRUCT( pxTransferStruct ) )
			{
				case ioctlUSE_ZERO_COPY_TX:

					#if ioconfigUSE_UART_ZERO_COPY_TX == 1
					{
						iouitlsTX_CHARS_FROM_ZERO_COPY_BUFFER_FROM_ISR( pxTransferStruct, ( ( LPC_UART3->FIFOLVL & uartTX_FIFO_LEVEL_MASK ) != uartTX_FIFO_LEVEL_MASK ), ( LPC_UART3->THR = ucChar ), xHigherPriorityTaskWoken );
					}
					#endif /* ioconfigUSE_UART_ZERO_COPY_TX */
					break;


				case ioctlUSE_CHARACTER_QUEUE_TX:

					#if ioconfigUSE_UART_TX_CHAR_QUEUE == 1
					{
						ioutilsTX_CHARS_FROM_QUEUE_FROM_ISR( pxTransferStruct, ( UART_FIFOLVL_TXFIFOLVL( LPC_UART3->FIFOLVL ) != ( UART_TX_FIFO_SIZE - 1 ) ), ( LPC_UART3->THR = ucChar ), xHigherPriorityTaskWoken );
					}
					#endif /* ioconfigUSE_UART_TX_CHAR_QUEUE */
					break;


				default :

					/* This must be an error.  Force an assert. */
					configASSERT( xHigherPriorityTaskWoken );
					break;
			}
		}
	}

	/* The ulReceived parameter is not used by the UART ISR. */
	( void ) ulReceived;

	/* If lHigherPriorityTaskWoken is now equal to pdTRUE, then a context
	switch should be performed before the interrupt exists.  That ensures the
	unblocked (higher priority) task is returned to immediately. */
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
#endif
