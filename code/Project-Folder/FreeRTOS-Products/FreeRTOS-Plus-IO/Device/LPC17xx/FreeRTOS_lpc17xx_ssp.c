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
#include "FreeRTOS_ssp.h"

/* Hardware setup peripheral driver includes.  The includes for the SSP itself
is already included from FreeRTOS_IO_BSP.h. */
#include "lpc17xx_pinsel.h"

/* The maximum number of characters that can be in the Rx FIFO. */
#define sspMAX_FIFO_DEPTH				( 8 )

/* All the interrupts associated with receiving on the SSP. */
#define sspALL_SSP_RX_INTERRUPTS 		( SSP_INTCFG_RT | SSP_INTCFG_RX | SSP_INTCFG_ROR )

/* The interrupts that are associated with data having been received on the
SSP. */
#define sspRX_DATA_AVAILABLE_INTERRUPTS	( SSP_INTCFG_RT | SSP_INTCFG_RX )

/* A definition of configSPI_INTERRUPT_PRIORITY is required for compilation,
even if FreeRTOSIOConfig.h is configured to not include any interrupt driven
methods of transacting data. */
#ifndef configSPI_INTERRUPT_PRIORITY
	#define configSPI_INTERRUPT_PRIORITY configMIN_LIBRARY_INTERRUPT_PRIORITY
#endif /* configSPI_INTERRUPT_PRIORITY */

/*-----------------------------------------------------------*/

/*
 * Write bytes from ppucBuffer into the pxSSP FIFO until either all the bytes
 * have been written, or the FIFO is full.
 */
static size_t prvFillFifoFromBuffer( LPC_SSP_TypeDef * const pxSSP, uint8_t **ppucBuffer, const size_t xTotalBytes );

/*-----------------------------------------------------------*/

/* A structure is maintained for each possible Tx session on each possible SSP
port, and each possible Rx session on each possible SSP port.  These structures
maintain the state of the Tx or Rx transfer, on the assumption that a Tx or
Rx cannot complete immediately. */
static Transfer_Control_t *pxTxTransferControlStructs[ boardNUM_SSPS ] = { NULL };
static Transfer_Control_t *pxRxTransferControlStructs[ boardNUM_SSPS ] = { NULL };

/* Writing to the SSP will also cause bytes to be received.  If the only
purpose of writing is to send data, and whatever is received during the
send can be junked, then ulRecieveActive[ x ] will be set to false.  When the
purpose of sending is to solicit incoming data, and Rxed data should be
stored, then ulReceiveActive[ x ] will be set to true. */
static volatile uint32_t ulReceiveActive[ boardNUM_SSPS ] = { pdFALSE };

/* Maintain a structure that holds the configuration of each SSP port.  This
allows a single configuration parameter to be changed at a time, as the
configuration for all the other parameters can be read from the stored
structure rather than queried from the peripheral itself. */
static SSP_CFG_Type xSSPConfigurations[ boardNUM_SSPS ];

/* The CMSIS interrupt number definitions for the SSP ports. */
static const IRQn_Type xIRQ[ boardNUM_SSPS ] = { SSP0_IRQn, SSP1_IRQn };

/*-----------------------------------------------------------*/

portBASE_TYPE FreeRTOS_SSP_open( Peripheral_Control_t * const pxPeripheralControl )
{
PINSEL_CFG_Type xPinConfig;
portBASE_TYPE xReturn = pdFAIL;
LPC_SSP_TypeDef * const pxSSP = ( LPC_SSP_TypeDef * const ) diGET_PERIPHERAL_BASE_ADDRESS( pxPeripheralControl );
SSP_DATA_SETUP_Type *pxSSPTransferDefinition;
const int8_t cPeripheralNumber = diGET_PERIPHERAL_NUMBER( pxPeripheralControl );
volatile uint16_t usJunkIt;

	/* Sanity check the peripheral number. */
	if( cPeripheralNumber < boardNUM_SSPS )
	{
		/* Polled mode is used by default.  Create the structure used to
		transfer SSP data in polled mode. */
		pxSSPTransferDefinition = ( SSP_DATA_SETUP_Type * ) pvPortMalloc( sizeof( SSP_DATA_SETUP_Type ) );

		if( pxSSPTransferDefinition != NULL )
		{
			/* Create the transfer control structures in which references to
			pxSSPTransferDefinition will be stored. */
			vIOUtilsCreateTransferControlStructure( &( pxPeripheralControl->pxTxControl ) );
			vIOUtilsCreateTransferControlStructure( &( pxPeripheralControl->pxRxControl ) );

			if( ( pxPeripheralControl->pxTxControl != NULL ) && ( pxPeripheralControl->pxTxControl != NULL ) )
			{
				pxPeripheralControl->read = FreeRTOS_SSP_read;
				pxPeripheralControl->write = FreeRTOS_SSP_write;
				pxPeripheralControl->ioctl = FreeRTOS_SSP_ioctl;
				pxPeripheralControl->pxTxControl->pvTransferState = pxSSPTransferDefinition;
				pxPeripheralControl->pxTxControl->ucType = ioctlUSE_POLLED_TX;
				pxPeripheralControl->pxRxControl->pvTransferState = NULL;
				pxPeripheralControl->pxRxControl->ucType = ioctlUSE_POLLED_RX;

				taskENTER_CRITICAL();
				{
					/* Setup the pins for the SSP being used. */
					boardCONFIGURE_SSP_PINS( cPeripheralNumber, xPinConfig );

					/* Set up the default SSP configuration. */
					SSP_ConfigStructInit( &( xSSPConfigurations[ cPeripheralNumber ] ) );
					SSP_Init( pxSSP, &( xSSPConfigurations[ cPeripheralNumber ] ) );
					SSP_Cmd( pxSSP, ENABLE );

					/* Clear data in Rx Fifo. */
					while( ( pxSSP->SR & SSP_SR_RNE ) != 0 )
					{
						usJunkIt = pxSSP->DR;
					}
				}
				taskEXIT_CRITICAL();

				xReturn = pdPASS;
			}
			else
			{
				/* Could not create one of other transfer control structure,
				so free	the created LPC_SSP_TypeDef typedef structures and any
				transfer control structures that were created before
				exiting. */
				if( pxPeripheralControl->pxTxControl != NULL )
				{
					vPortFree( pxPeripheralControl->pxTxControl );
					pxPeripheralControl->pxTxControl = NULL;
				}

				if( pxPeripheralControl->pxRxControl != NULL )
				{
					vPortFree( pxPeripheralControl->pxRxControl );
					pxPeripheralControl->pxRxControl = NULL;
				}

				vPortFree( pxSSPTransferDefinition );
			}
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

size_t FreeRTOS_SSP_write( Peripheral_Descriptor_t const pxPeripheral, const void *pvBuffer, const size_t xBytes )
{
Peripheral_Control_t * const pxPeripheralControl = ( Peripheral_Control_t * const ) pxPeripheral;
size_t xReturn = 0U;
LPC_SSP_TypeDef * const pxSSP = ( LPC_SSP_TypeDef * const ) diGET_PERIPHERAL_BASE_ADDRESS( ( ( Peripheral_Control_t * const ) pxPeripheral ) );
SSP_DATA_SETUP_Type *pxSSPTransferDefinition;
const uint32_t ulPeripheralNumber = ( uint32_t ) diGET_PERIPHERAL_NUMBER( ( ( Peripheral_Control_t * const ) pxPeripheral ) );

	/* Remember which transfer control structure is being used, so if
	an interrupt is being used, it can continue the same transfer until
	all data has been transmitted. */
	pxTxTransferControlStructs[ ulPeripheralNumber ] = diGET_TX_TRANSFER_STRUCT( pxPeripheralControl );

	switch( diGET_TX_TRANSFER_TYPE( pxPeripheralControl ) )
	{
		case ioctlUSE_POLLED_TX :

			#if ioconfigUSE_SSP_POLLED_TX == 1
			{
				/* Configure the transfer data.  No semaphore or queue is used
				here, so the application must ensure only one task attempts to
				make a polling write at a time. */
				pxSSPTransferDefinition = ( SSP_DATA_SETUP_Type * ) diGET_TX_TRANSFER_STATE( pxPeripheralControl );
				configASSERT( pxSSPTransferDefinition );
				pxSSPTransferDefinition->tx_data = ( void * ) pvBuffer;
				pxSSPTransferDefinition->rx_data = NULL;
				pxSSPTransferDefinition->length  = ( uint32_t ) xBytes;
				xReturn = SSP_ReadWrite( pxSSP, pxSSPTransferDefinition, SSP_TRANSFER_POLLING );
			}
			#endif /* ioconfigUSE_SSP_POLLED_TX */

			/* The transfer struct is set back to NULL as the Tx is complete
			before the above call to SSP_ReadWrite() completes. */
			pxTxTransferControlStructs[ ulPeripheralNumber ] = NULL;
			break;


		case ioctlUSE_ZERO_COPY_TX :

			#if ioconfigUSE_SSP_ZERO_COPY_TX == 1
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
						SSP_IntConfig( pxSSP, sspRX_DATA_AVAILABLE_INTERRUPTS, DISABLE ),		/* Disable interrupt. */
						SSP_IntConfig( pxSSP, sspRX_DATA_AVAILABLE_INTERRUPTS, ENABLE ), 		/* Enable interrupt. */
						prvFillFifoFromBuffer( pxSSP, ( uint8_t ** ) &( pvBuffer ), xBytes ), 	/* Write to peripheral function.  The buffer is passed in by address as the pointer is incremented. */
						pvBuffer, 																/* Data source. */
						xReturn																	/* Number of bytes to be written.  This will get set to zero if the write mutex is not held. */
					);
			}
			#endif /* ioconfigUSE_SSP_ZERO_COPY_TX */

			/* Remove compiler warnings in case the above is #defined out. */
			( void ) prvFillFifoFromBuffer;
			break;


		case ioctlUSE_CHARACTER_QUEUE_TX :

			#if ioconfigUSE_SSP_TX_CHAR_QUEUE == 1
			{
				/* The queue allows multiple tasks to attempt to write bytes,
				but ensures only the highest priority of these tasks will
				actually succeed.  If two tasks of equal priority attempt to
				write simultaneously, then the application must ensure mutual
				exclusion, as time slicing could result in the strings being
				sent to the queue becoming interleaved. */
				xReturn = xIOUtilsSendCharsToTxQueue( pxPeripheralControl, ( ( uint8_t * ) pvBuffer ), xBytes );

				ioutilsFILL_FIFO_FROM_TX_QUEUE(
						pxPeripheralControl,
						SSP_IntConfig( pxSSP, sspRX_DATA_AVAILABLE_INTERRUPTS, DISABLE ),	/* Disable Rx interrupt. */
						SSP_IntConfig( pxSSP, sspRX_DATA_AVAILABLE_INTERRUPTS, ENABLE ), 	/* Enable Rx interrupt. */
						sspMAX_FIFO_DEPTH, 													/* Bytes to write to the FIFO. */
						( pxSSP->SR & SSP_STAT_TXFIFO_NOTFULL ) != 0UL,						/* FIFO not full. */
						pxSSP->DR = SSP_DR_BITMASK( ucChar ) );							  	/* Tx function. */
			}
			#endif /* ioconfigUSE_SSP_RX_CHAR_QUEUE */
			break;


		default :

			/* Other methods can be implemented here.  For now, set the stored
			Tx structure back to NULL as nothing is being sent. */
			pxTxTransferControlStructs[ ulPeripheralNumber ] = NULL;
			configASSERT( xReturn );

			/* Prevent compiler warnings when the configuration is set such
			that the following parameters are not used. */
			( void ) pvBuffer;
			( void ) xBytes;
			( void ) pxSSP;
			( void ) pxSSPTransferDefinition;
			break;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

size_t FreeRTOS_SSP_read( Peripheral_Descriptor_t const pxPeripheral, void * const pvBuffer, const size_t xBytes )
{
Peripheral_Control_t * const pxPeripheralControl = ( Peripheral_Control_t * const ) pxPeripheral;
size_t xReturn = 0U;
LPC_SSP_TypeDef * const pxSSP = ( LPC_SSP_TypeDef * const ) diGET_PERIPHERAL_BASE_ADDRESS( ( ( Peripheral_Control_t * const ) pxPeripheral ) );
SSP_DATA_SETUP_Type *pxSSPTransferDefinition;
const int8_t cPeripheralNumber = diGET_PERIPHERAL_NUMBER( ( ( Peripheral_Control_t * const ) pxPeripheral ) );

	/* Sanity check the array index. */
	configASSERT( cPeripheralNumber < ( int8_t ) ( sizeof( xIRQ ) / sizeof( IRQn_Type ) ) );

	switch( diGET_RX_TRANSFER_TYPE( pxPeripheralControl ) )
	{
		case ioctlUSE_POLLED_RX :

			#if ioconfigUSE_SSP_POLLED_RX == 1
			{
				/* Configure the transfer data.  No semaphore or queue is used
				here, so the application must ensure only one task attempts to
				make a polling read at a time.  *NOTE* the Tx transfer state
				is used, as the SSP requires a Tx to occur for any data to be
				received. */
				pxSSPTransferDefinition = ( SSP_DATA_SETUP_Type * ) diGET_TX_TRANSFER_STATE( pxPeripheralControl );
				configASSERT( pxSSPTransferDefinition );
				pxSSPTransferDefinition->tx_data = NULL;
				pxSSPTransferDefinition->rx_data = ( void * ) pvBuffer;
				pxSSPTransferDefinition->length  = ( uint32_t ) xBytes;
				xReturn = SSP_ReadWrite( pxSSP, pxSSPTransferDefinition, SSP_TRANSFER_POLLING );
			}
			#endif /* ioconfigUSE_SSP_POLLED_RX */
			break;


		case ioctlUSE_CIRCULAR_BUFFER_RX :
			/* _RB_ This relies on Tx being configured to zero copy mode. */
			#if ioconfigUSE_SSP_CIRCULAR_BUFFER_RX == 1
			{
				/* There is nothing to prevent multiple tasks attempting to
				read the circular buffer at any one time.  The implementation
				of the circular buffer uses a semaphore to indicate when new
				data is available, and the semaphore will ensure that only the
				highest priority task that is attempting a read will actually
				receive bytes. */

				/* A write is performed first, to generate the clock required
				to clock the data in.  NULL is passed as the source buffer as
				there isn't actually any data to send, and NULL will just
				result in 0xff being sent.  When the data written to the FIFO
				has been transmitted an Rx interrupt will copy the received
				data into the circular buffer, and check to see if there is
				more data to be written. */

				/* Ensure the last Tx has completed. */
				xReturn = xIOUtilsGetZeroCopyWriteMutex( pxPeripheralControl, ioctlOBTAIN_WRITE_MUTEX, ioutilsDEFAULT_ZERO_COPY_TX_MUTEX_BLOCK_TIME );
				configASSERT( xReturn );

				if( xReturn == pdPASS )
				{
					/* Empty whatever is lingering in the Rx buffer (there
					shouldn't be any. */
					vIOUtilsClearRxCircularBuffer( pxPeripheralControl );

					/* Data should be received during the following write. */
					ulReceiveActive[ cPeripheralNumber ] = pdTRUE;

					/* Write to solicit received data. */
					FreeRTOS_SSP_write( pxPeripheralControl, NULL, xBytes );

					/* This macro will continuously wait on the new data mutex,
					reading bytes from the circular buffer each time it
					receives it, until the desired number of bytes have been
					read. */
					ioutilsRECEIVE_CHARS_FROM_CIRCULAR_BUFFER
						(
							pxPeripheralControl,
							SSP_IntConfig( pxSSP, sspRX_DATA_AVAILABLE_INTERRUPTS, DISABLE ), /* Disable interrupt. */
							SSP_IntConfig( pxSSP, sspRX_DATA_AVAILABLE_INTERRUPTS, ENABLE ), /* Enable interrupt. */
							( ( uint8_t * ) pvBuffer ),	/* Data destination. */
							xBytes,						/* Bytes to read. */
							xReturn						/* Number of bytes read. */
						);

					/* Not expecting any more Rx data now, so just junk anything
					that is received until the next explicit read is performed. */
					ulReceiveActive[ cPeripheralNumber ] = pdFALSE;
				}
			}
			#endif
			break;


		case ioctlUSE_CHARACTER_QUEUE_RX :

			/* The queue allows multiple tasks to attempt to read bytes,
			but ensures only the highest priority of these tasks will
			actually receive bytes.  If two tasks of equal priority attempt
			to read simultaneously, then the application must ensure mutual
			exclusion, as time slicing could result in the string being
			received being partially received by each task. */
			#if ioconfigUSE_SSP_RX_CHAR_QUEUE == 1
			{
				/* Ensure the last Tx has completed. */
				xIOUtilsWaitTxQueueEmpty( pxPeripheralControl, boardDEFAULT_READ_MUTEX_TIMEOUT );

				/* Clear any residual data - there shouldn't be any! */
				xIOUtilsClearRxCharQueue( pxPeripheralControl );

				/* Data should be received during the following write. */
				ulReceiveActive[ cPeripheralNumber ] = pdTRUE;

				/* Write to solicit received data. */
				FreeRTOS_SSP_write( pxPeripheralControl, NULL, xBytes );

				/* Read the received data placed in the Rx queue by the
				interrupt. */
				xReturn = xIOUtilsReceiveCharsFromRxQueue( pxPeripheralControl, ( uint8_t * ) pvBuffer, xBytes );

				/* Not expecting any more Rx data now, so just junk anything
				that is received until the next explicit read is performed. */
				ulReceiveActive[ cPeripheralNumber ] = pdFALSE;
			}
			#endif
			break;


		default :

			/* Other methods can be implemented here. */
			configASSERT( xReturn );

			/* Prevent compiler warnings when the configuration is set such
			that the following parameters are not used. */
			( void ) pvBuffer;
			( void ) xBytes;
			( void ) pxSSP;
			( void ) pxSSPTransferDefinition;
			( void ) cPeripheralNumber;
			break;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static size_t prvFillFifoFromBuffer( LPC_SSP_TypeDef * const pxSSP, uint8_t **ppucBuffer, const size_t xTotalBytes )
{
size_t xBytesSent;

	/* This function is only used by zero copy transmissions, so mutual
	exclusion is already taken care of by the fact that a task must first
	obtain a semaphore before initiating a zero copy transfer.  The semaphore
	is part of the zero copy structure, not part of the application.

	The buffer is passed in by reference as the incremented pointer is in
	effect returned. */
	for( xBytesSent = 0U; ( xBytesSent < sspMAX_FIFO_DEPTH ) && ( xBytesSent < xTotalBytes ); xBytesSent++ )
	{
		/* While the Tx FIFO is not full, and the Rx FIFO is not full. */
		if( ( ( pxSSP->SR & SSP_SR_TNF ) != 0 ) && ( ( pxSSP->SR & SSP_SR_RFF ) == 0 ) )
		{
			if( *ppucBuffer == NULL )
			{
				/* There is no data to send.  This transmission is probably
				just to	generate a clock so data can be received, so send out
				0xff each time. */
				pxSSP->DR = SSP_DR_BITMASK( ( ( uint16_t ) 0xffffU ) );
			}
			else
			{
				pxSSP->DR = SSP_DR_BITMASK( ( uint16_t ) ( **ppucBuffer ) );
				( *ppucBuffer )++;
			}
		}
		else
		{
			break;
		}
	}

	return xBytesSent;
}
/*-----------------------------------------------------------*/

portBASE_TYPE FreeRTOS_SSP_ioctl( Peripheral_Descriptor_t const pxPeripheral, uint32_t ulRequest, void *pvValue )
{
Peripheral_Control_t * const pxPeripheralControl = ( Peripheral_Control_t * const ) pxPeripheral;
uint32_t ulValue = ( uint32_t ) pvValue, ulInitSSP = pdFALSE;
const int8_t cPeripheralNumber = diGET_PERIPHERAL_NUMBER( ( ( Peripheral_Control_t * const ) pxPeripheral ) );
LPC_SSP_TypeDef * pxSSP = ( LPC_SSP_TypeDef * ) diGET_PERIPHERAL_BASE_ADDRESS( ( ( Peripheral_Control_t * const ) pxPeripheral ) );

	taskENTER_CRITICAL();
	{
		switch( ulRequest )
		{
			case ioctlUSE_INTERRUPTS :

				/* Sanity check the array index. */
				configASSERT( cPeripheralNumber < ( int8_t ) ( sizeof( xIRQ ) / sizeof( IRQn_Type ) ) );

				if( ulValue == pdFALSE )
				{
					NVIC_DisableIRQ( xIRQ[ cPeripheralNumber ] );
				}
				else
				{
					/* Enable the Rx interrupts only.  New data is sent if an
					Rx interrupt makes space in the FIFO, so Tx interrupts are
					not required. */
					SSP_IntConfig( LPC_SSP1, SSP_INTCFG_TX, DISABLE );
					SSP_IntConfig( pxSSP, sspALL_SSP_RX_INTERRUPTS, ENABLE );

					/* Enable the interrupt and set its priority to the minimum
					interrupt priority.  A separate command can be issued to raise
					the priority if desired. */
					NVIC_SetPriority( xIRQ[ cPeripheralNumber ], configSPI_INTERRUPT_PRIORITY );
					NVIC_EnableIRQ( xIRQ[ cPeripheralNumber ] );

					/* If the Rx is configured to use interrupts, remember the
					transfer control structure that should be used.  A reference
					to the Tx transfer control structure is taken when a write()
					operation is actually performed. */
					pxRxTransferControlStructs[ cPeripheralNumber ] = pxPeripheralControl->pxRxControl;
				}
				break;


			case ioctlSET_INTERRUPT_PRIORITY :

				/* The ISR uses ISR safe FreeRTOS API functions, so the priority
				being set must be lower than (ie numerically larger than)
				configMAX_LIBRARY_INTERRUPT_PRIORITY. */
				configASSERT( ulValue < configMAX_LIBRARY_INTERRUPT_PRIORITY );
				NVIC_SetPriority( xIRQ[ cPeripheralNumber ], ulValue );
				break;


			case ioctlSET_SPEED : /* In Hz. */

				xSSPConfigurations[ cPeripheralNumber ].ClockRate = ulValue;
				ulInitSSP = pdTRUE;
				break;


			case ioctlSET_SPI_DATA_BITS	: /* 4 to 16. */

				xSSPConfigurations[ cPeripheralNumber ].Databit = ulValue;
				ulInitSSP = pdTRUE;
				break;


			case ioctlSET_SPI_CLOCK_PHASE : /* SSP_CPHA_FIRST or SSPCPHA_SECOND */
				xSSPConfigurations[ cPeripheralNumber ].CPHA = ulValue;
				ulInitSSP = pdTRUE;
				break;


			case ioctlSET_SPI_CLOCK_POLARITY : /* SSP_CPOL_HI or SSP_CPOL_LO. */

				xSSPConfigurations[ cPeripheralNumber ].CPOL = ulValue;
				break;


			case ioctlSET_SPI_MODE : /* SSP_MASTER_MODE or SSP_SLAVE_MODE. */

				xSSPConfigurations[ cPeripheralNumber ].Mode = ulValue;
				break;


			case ioctlSET_SSP_FRAME_FORMAT : /* SSP_FRAME_SPI or SSP_FRAME_TI or SSP_FRAME_MICROWIRE. */

				xSSPConfigurations[ cPeripheralNumber ].FrameFormat = ulValue;
				break;
		}

		if( ulInitSSP == pdTRUE )
		{
			SSP_Cmd( pxSSP, DISABLE );
			SSP_DeInit( pxSSP );
			SSP_Init( pxSSP, &( xSSPConfigurations[ cPeripheralNumber ] ) );
			SSP_Cmd( pxSSP, ENABLE );
		}
	}
	taskEXIT_CRITICAL();

	return pdPASS;
}
/*-----------------------------------------------------------*/

#if ioconfigINCLUDE_SSP != 1
	/* If the SSP driver is not being used, rename the interrupt handler.  This
	will prevent it being installed in the vector table.  The linker will then
	identify it as unused code, and remove it from the binary image. */
	#define SSP1_IRQHandler Unused_SSP1_IRQHandler
#endif /* ioconfigINCLUDE_SSP */

void SSP1_IRQHandler( void )
{
uint32_t ulInterruptSource;
volatile uint32_t usJunk, ulReceived = 0UL;
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
const unsigned portBASE_TYPE uxSSPNumber = 1UL;
Transfer_Control_t *pxTxTransferStruct, *pxRxTransferStruct;

	/* Determine the interrupt source. */
	ulInterruptSource = LPC_SSP1->MIS;

	/* Clear receive overruns, and optionally assert. */
	if( ( ulInterruptSource & SSP_INTSTAT_ROR ) != 0 )
	{
		configASSERT( ( ulInterruptSource & SSP_INTSTAT_ROR ) == 0 );
		LPC_SSP1->ICR = SSP_INTCLR_ROR;
	}

	/* Clear timeouts. */
	if( ( ulInterruptSource & SSP_INTSTAT_RT ) != 0 )
	{
		LPC_SSP1->ICR = SSP_INTCLR_RT;
	}

	/* Is this a receive FIFO half full or receive timeout? */
	if( ( ulInterruptSource & sspRX_DATA_AVAILABLE_INTERRUPTS ) != 0 )
	{
		pxTxTransferStruct = pxTxTransferControlStructs[ uxSSPNumber ];
		pxRxTransferStruct = pxRxTransferControlStructs[ uxSSPNumber ];
		configASSERT( pxRxTransferStruct );
		configASSERT( pxTxTransferStruct );

		if( pxRxTransferStruct != NULL )
		{
			if( ulReceiveActive[ uxSSPNumber ] == pdFALSE )
			{
				/* The data being received is just in response to data
				being sent,	not in response to data being read, just
				just junk it. */
				while( ( LPC_SSP1->SR & SSP_SR_RNE ) != 0 )
				{
					usJunk = LPC_SSP1->DR;
					ulReceived++;
				}
			}
			else
			{
				/* Data is being received because a read is being
				performed.  Store the data using whichever
				transfer mechanism is currently configured. */
				switch( diGET_TRANSFER_TYPE_FROM_CONTROL_STRUCT( pxRxTransferStruct ) )
				{
					case ioctlUSE_CIRCULAR_BUFFER_RX :

						#if ioconfigUSE_SSP_CIRCULAR_BUFFER_RX == 1
						{
							/* This call will empty the FIFO, and give the New
							Data semaphore so a task blocked on an SSP read
							will unblock.  Note that this does not mean that
							more data will not arrive after this interrupt,
							even if there is no more data to send. */
							ioutilsRX_CHARS_INTO_CIRCULAR_BUFFER_FROM_ISR(
																		pxRxTransferStruct, 	/* The structure that contains the reference to the circular buffer. */
																		( ( LPC_SSP1->SR & SSP_SR_RNE ) != 0 ), 		/* While loop condition. */
																		( LPC_SSP1->DR ),						/* The function that returns the chars. */
																		ulReceived,
																		xHigherPriorityTaskWoken
																	);
						}
						#endif /* ioconfigUSE_SSP_CIRCULAR_BUFFER_RX */
						break;


					case ioctlUSE_CHARACTER_QUEUE_RX :

						#if ioconfigUSE_SSP_RX_CHAR_QUEUE == 1
						{
							ioutilsRX_CHARS_INTO_QUEUE_FROM_ISR( pxRxTransferStruct, ( ( LPC_SSP1->SR & SSP_SR_RNE ) != 0 ), ( LPC_SSP1->DR ), ulReceived, xHigherPriorityTaskWoken );
						}
						#endif /* ioconfigUSE_SSP_RX_CHAR_QUEUE */
						break;


					default :

						/* This must be an error.  Force an assert. */
						configASSERT( xHigherPriorityTaskWoken );
						break;
				}
			}

			/* Space has been created in the Rx FIFO, see if there is any data
			to send to the Tx FIFO. */
			switch( diGET_TRANSFER_TYPE_FROM_CONTROL_STRUCT( pxTxTransferStruct ) )
			{
				case ioctlUSE_ZERO_COPY_TX:

					#if ioconfigUSE_SSP_ZERO_COPY_TX == 1
					{
						iouitlsTX_CHARS_FROM_ZERO_COPY_BUFFER_FROM_ISR( pxTxTransferStruct, ( ( ulReceived-- ) > 0 ), ( LPC_SSP1->DR = ucChar), xHigherPriorityTaskWoken );
					}
					#endif /* ioconfigUSE_SSP_ZERO_COPY_TX */
					break;


				case ioctlUSE_CHARACTER_QUEUE_TX:

					#if ioconfigUSE_SSP_TX_CHAR_QUEUE == 1
					{
						ioutilsTX_CHARS_FROM_QUEUE_FROM_ISR( pxTxTransferStruct, ( ( ulReceived-- ) > 0 ), ( LPC_SSP1->DR = SSP_DR_BITMASK( ( uint16_t ) ucChar ) ), xHigherPriorityTaskWoken );
					}
					#endif /* ioconfigUSE_SSP_TX_CHAR_QUEUE */
					break;


				default :

					/* Should not get here.  Set the saved transfer control
					structure to NULL so the Tx interrupt will get disabled
					before this ISR is exited. */
					pxTxTransferControlStructs[ uxSSPNumber ] = NULL;

					/* This must be an error.  Force an assert. */
					configASSERT( xHigherPriorityTaskWoken );
					break;
			}
		}
	}

	/* If lHigherPriorityTaskWoken is now equal to pdTRUE, then a context
	switch should be performed before the interrupt exists.  That ensures the
	unblocked (higher priority) task is returned to immediately. */
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
