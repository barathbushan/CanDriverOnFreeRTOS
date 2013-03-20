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
#include "FreeRTOS_i2c.h"

/* Hardware setup peripheral driver includes.  The includes for the I2C itself
is already included from FreeRTOS_IO_BSP.h. */
#include "lpc17xx_pinsel.h"

typedef enum
{
	i2cWriting = 0,
	i2cReading = 1,
	i2cIdle
} eDataDirection;

/*-----------------------------------------------------------*/

/* Stores the transfer control structures that are currently in use by the
supported I2C ports. */
static Transfer_Control_t *pxTxTransferControlStructs[ boardNUM_I2CS ] = { NULL };
static Transfer_Control_t *pxRxTransferControlStructs[ boardNUM_I2CS ] = { NULL };

/* Stores the IRQ numbers of the supported I2C ports. */
static const IRQn_Type xIRQ[ boardNUM_I2CS ] = { I2C0_IRQn, I2C1_IRQn, I2C2_IRQn };

/* Stores the slave address currently being used by each supported I2C port. */
static uint8_t ucSlaveAddresses[ boardNUM_I2CS ] = { 0U };

/* Stores the current direction of data transfer (writing/reading) for each
supported I2C port. */
static eDataDirection xDataDirection[ boardNUM_I2CS ];

/* Stores the number of bytes currently outstanding for any transfers that
are in progress on each supported I2C port. */
static size_t xBytesOutstanding[ boardNUM_I2CS ] = { 0UL };


/*-----------------------------------------------------------*/

portBASE_TYPE FreeRTOS_I2C_open( Peripheral_Control_t * const pxPeripheralControl )
{
PINSEL_CFG_Type xPinConfig;
LPC_I2C_TypeDef * const pxI2C = ( LPC_I2C_TypeDef * const ) diGET_PERIPHERAL_BASE_ADDRESS( pxPeripheralControl );
portBASE_TYPE xReturn = pdFAIL;
const uint8_t cPeripheralNumber = diGET_PERIPHERAL_NUMBER( pxPeripheralControl );
I2C_M_SETUP_Type *pxI2CTxTransferDefinition = NULL, *pxI2CRxTransferDefinition = NULL;

	/* Sanity check the peripheral number. */
	if( cPeripheralNumber < boardNUM_I2CS )
	{
		/* Polled mode is used by default.  This can be changed using an
		ioctl() call.  Create the structures used to transfer I2C data in
		polled mode. */
		pxI2CTxTransferDefinition = ( I2C_M_SETUP_Type * ) pvPortMalloc( sizeof( I2C_M_SETUP_Type ) );
		pxI2CRxTransferDefinition = ( I2C_M_SETUP_Type * ) pvPortMalloc( sizeof( I2C_M_SETUP_Type ) );

		if( ( pxI2CTxTransferDefinition != NULL ) && ( pxI2CRxTransferDefinition != NULL ) )
		{
			/* Create the transfer control structures in which references to
			pxI2CNxTransferDefinitions will be stored. */
			vIOUtilsCreateTransferControlStructure( &( pxPeripheralControl->pxTxControl ) );
			vIOUtilsCreateTransferControlStructure( &( pxPeripheralControl->pxRxControl ) );

			if( ( pxPeripheralControl->pxTxControl != NULL ) && ( pxPeripheralControl->pxRxControl != NULL ) )
			{
				pxPeripheralControl->read = FreeRTOS_I2C_read;
				pxPeripheralControl->write = FreeRTOS_I2C_write;
				pxPeripheralControl->ioctl = FreeRTOS_I2C_ioctl;
				pxPeripheralControl->pxTxControl->pvTransferState = pxI2CTxTransferDefinition;
				pxPeripheralControl->pxTxControl->ucType = ioctlUSE_POLLED_TX;
				pxPeripheralControl->pxRxControl->pvTransferState = pxI2CRxTransferDefinition;
				pxPeripheralControl->pxRxControl->ucType = ioctlUSE_POLLED_RX;

				taskENTER_CRITICAL();
				{
					/* Setup the pins for the I2C being used. */
					boardCONFIGURE_I2C_PINS( cPeripheralNumber, xPinConfig );

					/* Set up the default I2C configuration. */
					I2C_Init( pxI2C, boardDEFAULT_I2C_SPEED );
					I2C_Cmd( pxI2C, ENABLE );
				}
				taskEXIT_CRITICAL();

				xReturn = pdPASS;
			}
		}

		if( xReturn != pdPASS )
		{
			/* The open operation could not complete because something could
			not be created.  Delete anything that was created. */
			if( pxI2CTxTransferDefinition != NULL )
			{
				vPortFree( pxI2CTxTransferDefinition );
			}

			if( pxI2CRxTransferDefinition != NULL )
			{
				vPortFree( pxI2CRxTransferDefinition );
			}

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
		}
	}

	/* Just to prevent compiler warnings when FreeRTIOSIOConfig.h is configured
	such that the variable is not used. */
	( void ) pxRxTransferControlStructs;

	return xReturn;
}
/*-----------------------------------------------------------*/

size_t FreeRTOS_I2C_write( Peripheral_Descriptor_t const pxPeripheral, const void *pvBuffer, const size_t xBytes )
{
Peripheral_Control_t * const pxPeripheralControl = ( Peripheral_Control_t * const ) pxPeripheral;
size_t xReturn = 0U;
LPC_I2C_TypeDef * const pxI2C = ( LPC_I2C_TypeDef * const ) diGET_PERIPHERAL_BASE_ADDRESS( ( ( Peripheral_Control_t * const ) pxPeripheral ) );
I2C_M_SETUP_Type *pxI2CTransferDefinition;
const int8_t cPeripheralNumber = diGET_PERIPHERAL_NUMBER( ( ( Peripheral_Control_t * const ) pxPeripheral ) );

	/* Remember which transfer control structure is being used, so if
	an interrupt is being used, it can continue the same transfer until
	all data has been transmitted. */
	pxTxTransferControlStructs[ cPeripheralNumber ] = diGET_TX_TRANSFER_STRUCT( pxPeripheralControl );
	xDataDirection[ cPeripheralNumber ] = i2cWriting;

	switch( diGET_TX_TRANSFER_TYPE( pxPeripheralControl ) )
	{
		case ioctlUSE_POLLED_TX :

			#if ioconfigUSE_I2C_POLLED_TX == 1
			{
				/* Configure the transfer data.  No semaphore or queue is used
				here, so the application must ensure only one task attempts to
				make a polling write at a time. */
				pxI2CTransferDefinition = ( I2C_M_SETUP_Type * ) diGET_TX_TRANSFER_STATE( pxPeripheralControl );
				configASSERT( pxI2CTransferDefinition );
				pxI2CTransferDefinition->sl_addr7bit = ucSlaveAddresses[ cPeripheralNumber ];
				pxI2CTransferDefinition->tx_data = ( uint8_t * ) pvBuffer;
				pxI2CTransferDefinition->tx_length = xBytes;
				pxI2CTransferDefinition->rx_data = NULL;
				pxI2CTransferDefinition->rx_length = 0;
				pxI2CTransferDefinition->retransmissions_max = boardI2C_MAX_RETRANSMISSIONS;

				if( I2C_MasterTransferData( pxI2C, pxI2CTransferDefinition, I2C_TRANSFER_POLLING ) == SUCCESS )
				{
					xReturn = xBytes;
				}
			}
			#endif /* ioconfigUSE_I2C_POLLED_TX_RX */

			/* The transfer struct is set back to NULL as the Tx is complete
			before the above call to I2C_ReadWrite() completes. */
			pxTxTransferControlStructs[ cPeripheralNumber ] = NULL;
			break;


		case ioctlUSE_ZERO_COPY_TX :

			#if ioconfigUSE_I2C_ZERO_COPY_TX == 1
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
						I2C_IntCmd( pxI2C, DISABLE ),	/* Disable interrupt.  Not really necessary in this case as it should not be enabled anyway. */
						( void ) 0, 					/* As the start condition has not been sent, the interrupt is not enabled yet. */
						0,  							/* In this case no write is attempted and all that should happen is the buffer gets set up ready. */
						pvBuffer, 						/* Data source. */
						xReturn							/* Number of bytes to be written.  This will get set to zero if the write mutex is not held. */
					);

				/* Ensure there are not already interrupt pending. */
				pxI2C->I2CONCLR = ( I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC | I2C_I2CONCLR_AAC | I2C_I2CONCLR_STAC );

				/* Set start flag. */
				pxI2C->I2CONSET = I2C_I2CONSET_STA;

				/* Now enable interrupts. */
				I2C_IntCmd( pxI2C, ENABLE );
			}
			#endif /* ioconfigUSE_I2C_ZERO_COPY_TX */
			break;


		case ioctlUSE_CHARACTER_QUEUE_TX :

			/* Not (yet?) implemented for I2C. */
			configASSERT( xReturn );
			break;


		default :

			/* Other methods can be implemented here.  For now, set the stored
			Tx structure back to NULL as nothing is being sent. */
			pxTxTransferControlStructs[ cPeripheralNumber ] = NULL;
			configASSERT( xReturn );

			/* Prevent compiler warnings when the configuration is set such
			that the following parameters are not used. */
			( void ) pvBuffer;
			( void ) xBytes;
			( void ) pxI2C;
			( void ) pxI2CTransferDefinition;
			break;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

size_t FreeRTOS_I2C_read( Peripheral_Descriptor_t const pxPeripheral, void * const pvBuffer, const size_t xBytes )
{
Peripheral_Control_t * const pxPeripheralControl = ( Peripheral_Control_t * const ) pxPeripheral;
size_t xReturn = 0U;
LPC_I2C_TypeDef * const pxI2C = ( LPC_I2C_TypeDef * const ) diGET_PERIPHERAL_BASE_ADDRESS( ( ( Peripheral_Control_t * const ) pxPeripheral ) );
I2C_M_SETUP_Type *pxI2CTransferDefinition;
const int8_t cPeripheralNumber = diGET_PERIPHERAL_NUMBER( ( ( Peripheral_Control_t * const ) pxPeripheral ) );

	/* Sanity check the array index. */
	configASSERT( cPeripheralNumber < ( int8_t ) ( sizeof( xIRQ ) / sizeof( IRQn_Type ) ) );
	configASSERT( xBytes > 0U );

	/* Remove compiler warnings when configASSERT() is not defined. */
	( void ) xBytes;

	switch( diGET_RX_TRANSFER_TYPE( pxPeripheralControl ) )
	{
		case ioctlUSE_POLLED_RX :

			#if ioconfigUSE_I2C_POLLED_RX == 1
			{
				/* Configure the transfer data.  No semaphore or queue is used
				here, so the application must ensure only one task attempts to
				make a polling write at a time. */
				pxI2CTransferDefinition = ( I2C_M_SETUP_Type * ) diGET_RX_TRANSFER_STATE( pxPeripheralControl );
				configASSERT( pxI2CTransferDefinition );
				pxI2CTransferDefinition->sl_addr7bit = ucSlaveAddresses[ cPeripheralNumber ];
				pxI2CTransferDefinition->tx_data = NULL;
				pxI2CTransferDefinition->tx_length = 0;
				pxI2CTransferDefinition->rx_data = ( uint8_t * ) pvBuffer;;
				pxI2CTransferDefinition->rx_length = xBytes;
				pxI2CTransferDefinition->retransmissions_max = boardI2C_MAX_RETRANSMISSIONS;

				if( I2C_MasterTransferData( pxI2C, pxI2CTransferDefinition, I2C_TRANSFER_POLLING ) == SUCCESS )
				{
					xReturn = xBytes;
				}
			}
			#endif /* ioconfigUSE_I2C_POLLED_RX */

			break;


		case ioctlUSE_CIRCULAR_BUFFER_RX :

			#if ioconfigUSE_I2C_CIRCULAR_BUFFER_RX == 1
			{
				/* There is nothing to prevent multiple tasks attempting to
				read the circular buffer at any one time.  The implementation
				of the circular buffer uses a semaphore to indicate when new
				data is available, and the semaphore will ensure that only the
				highest priority task that is attempting a read will actually
				receive bytes. */

				if( xDataDirection[ cPeripheralNumber ] == i2cIdle )
				{
					/* This is the first time read() has been called for this
					transfer.  Start the transfer by setting the start
					bit, then mark the read transfer as in progress. */
					pxI2C->I2CONSET = I2C_I2CONSET_STA;
					xDataDirection[ cPeripheralNumber ] = i2cReading;
					xBytesOutstanding[ cPeripheralNumber ] = xBytes;
					I2C_IntCmd( pxI2C, ENABLE );
				}

				ioutilsRECEIVE_CHARS_FROM_CIRCULAR_BUFFER
					(
						pxPeripheralControl,
						I2C_IntCmd( pxI2C, DISABLE ),	/* Disable peripheral. */
						I2C_IntCmd( pxI2C, ENABLE ), 	/* Enable peripheral. */
						( ( uint8_t * ) pvBuffer ),		/* Data destination. */
						xBytes,							/* Bytes to read. */
						xReturn							/* Number of bytes read. */
					);
			}
			#endif /* ioconfigUSE_I2C_CIRCULAR_BUFFER_RX */
			break;


		case ioctlUSE_CHARACTER_QUEUE_RX :
			/* Not (yet?) implemented for I2C. */
			configASSERT( xReturn );
			break;


		default :

			/* Other methods can be implemented here. */
			configASSERT( xReturn );

			/* Prevent compiler warnings when the configuration is set such
			that the following parameters are not used. */
			( void ) pvBuffer;
			( void ) pxI2C;
			( void ) pxI2CTransferDefinition;
			( void ) cPeripheralNumber;
			break;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

portBASE_TYPE FreeRTOS_I2C_ioctl( Peripheral_Descriptor_t const pxPeripheral, uint32_t ulRequest, void *pvValue )
{
Peripheral_Control_t * const pxPeripheralControl = ( Peripheral_Control_t * const ) pxPeripheral;
uint32_t ulValue = ( uint32_t ) pvValue;
const int8_t cPeripheralNumber = diGET_PERIPHERAL_NUMBER( ( ( Peripheral_Control_t * const ) pxPeripheral ) );
portBASE_TYPE xReturn = pdPASS;
LPC_I2C_TypeDef * pxI2C = ( LPC_I2C_TypeDef * ) diGET_PERIPHERAL_BASE_ADDRESS( ( ( Peripheral_Control_t * const ) pxPeripheral ) );

	/* Sanity check the array index. */
	configASSERT( cPeripheralNumber < ( int8_t ) ( sizeof( xIRQ ) / sizeof( IRQn_Type ) ) );

	taskENTER_CRITICAL();
	{
		switch( ulRequest )
		{
			case ioctlSET_I2C_SLAVE_ADDRESS :

				ucSlaveAddresses[ cPeripheralNumber ] = ( uint8_t ) ulValue;
				break;


			case ioctlUSE_INTERRUPTS :

				if( ulValue == pdFALSE )
				{
					NVIC_DisableIRQ( xIRQ[ cPeripheralNumber ] );
				}
				else
				{
					/* Ensure the interrupt is not already enabled. */
					I2C_IntCmd( pxI2C, DISABLE );

					/* Clear any pending interrupts in the peripheral and
					NVIC. */
					pxI2C->I2CONCLR = I2C_I2CONCLR_SIC;
					NVIC_ClearPendingIRQ( xIRQ[ cPeripheralNumber ] );

					/* Set the priority of the interrupt to the minimum
					interrupt priority.  A separate command can be issued to
					raise the priority if desired.  The interrupt is only
					enabled when a transfer is started. */
					NVIC_SetPriority( xIRQ[ cPeripheralNumber ], configMIN_LIBRARY_INTERRUPT_PRIORITY );

					/* Remember the	transfer control structure that should
					be used for Rx.  A reference to the Tx transfer control
					structure is taken when a write() operation is actually
					performed. */
					pxRxTransferControlStructs[ cPeripheralNumber ] = pxPeripheralControl->pxRxControl;
				}
				break;


			case ioctlSET_SPEED :

				/* Set up the default I2C configuration. */
				I2C_SetClock( pxI2C, ulValue );
				break;


			case ioctlSET_INTERRUPT_PRIORITY :

				/* The ISR uses ISR safe FreeRTOS API functions, so the priority
				being set must be lower than or equal to (ie numerically larger
				than or equal to) configMAX_LIBRARY_INTERRUPT_PRIORITY. */
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

#if ioconfigINCLUDE_I2C != 1
	#if ( ioconfigUSE_I2C_ZERO_COPY_TX != 1 ) && ( ioconfigUSE_I2C_CIRCULAR_BUFFER_RX != 1 )
		/* If the I2C driver is not being used, rename the interrupt handler.  This
		will prevent it being installed in the vector table.  The linker will then
		identify it as unused code, and remove it from the binary image. */
		#define I2C2_IRQHandler Unused_I2C2_IRQHandler
	#endif /* ( ioconfigUSE_I2C_ZERO_COPY_TX != 1 ) && ( ioconfigUSE_I2C_CIRCULAR_BUFFER_RX != 1 ) */
#endif /* ioconfigINCLUDE_I2C */

void I2C2_IRQHandler( void )
{
uint32_t ulI2CStatus, ulChar, ulReceived = 0UL;
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
const unsigned portBASE_TYPE uxI2CNumber = 2UL;
Transfer_Control_t *pxTransferStruct;

	/* Determine the event that caused the interrupt. */
	ulI2CStatus = ( LPC_I2C2->I2STAT & I2C_STAT_CODE_BITMASK );

	/* States that are valid for both Rx and Tx, or are not dependent on either
	of the Rx or Tx state transfer structures being populated. */
	switch( ulI2CStatus )
	{
		case I2C_I2STAT_NO_INF	:

			/* There is no I2C status information to act upon.  Clear the
			interrupt. */
			LPC_I2C2->I2CONCLR = I2C_I2CONCLR_SIC;
			break;


		case I2C_I2STAT_M_RX_START	:
		case I2C_I2STAT_M_RX_RESTART	:

			/* A start or restart.	Send the slave address and 'W'rite
			bits.  This could be during an Rx or a Tx, hence it is outside of
			the Rx and Tx state machines. */
			LPC_I2C2->I2DAT = ( ucSlaveAddresses[ uxI2CNumber ] << 1U ) | ( uint8_t ) xDataDirection[ uxI2CNumber ];

			/* Clear the interrupt and the start bit. */
			LPC_I2C2->I2CONCLR = I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC;
			break;

		case I2C_I2STAT_M_TX_SLAW_NACK: /* SLA+W has been transmitted, and a NACK was received. */
		case I2C_I2STAT_M_TX_DAT_NACK:  /* Data has been transmitted, and a NACK was received. */
		case I2C_I2STAT_M_TX_ARB_LOST:  /* Arbitration lost.  Could be Rx or Tx. */
		case I2C_I2STAT_M_RX_SLAR_NACK: /* SLA+W has been transmitted, and a NACK was received. */
			/* Clear the interrupt. */
			LPC_I2C2->I2CONCLR = I2C_I2CONCLR_SIC;

			/* force an assert. */
			configASSERT( xHigherPriorityTaskWoken );
			break;
	}

	/* Transmit state machine. */
	pxTransferStruct = pxTxTransferControlStructs[ uxI2CNumber ];
	if( pxTransferStruct != NULL )
	{
		switch( ulI2CStatus )
		{
			case I2C_I2STAT_M_TX_SLAW_ACK:
			case I2C_I2STAT_M_TX_DAT_ACK:

				/* An Ack has been received after either the slave address or
				data was transmitted.  Is there more data to send?  */
				iouitlsTX_SINGLE_CHAR_FROM_ZERO_COPY_BUFFER_FROM_ISR( pxTransferStruct, ( LPC_I2C2->I2DAT = ucChar ), ulChar );

				if( ulChar == pdFALSE )
				{
					/* There was no more data to send, so send the stop bit and
					disable interrupts. */
					I2C_IntCmd( LPC_I2C2, DISABLE );
					I2C_Stop( LPC_I2C2 );
					pxTxTransferControlStructs[ uxI2CNumber ] = NULL;
					xDataDirection[ uxI2CNumber ] = i2cIdle;
					ioutilsGIVE_ZERO_COPY_MUTEX( pxTransferStruct, xHigherPriorityTaskWoken );
				}
				else
				{
					/* Clear the interrupt. */
					LPC_I2C2->I2CONCLR = I2C_I2CONCLR_SIC;
				}
				break;


			default:
				/* Error and naks can be implemented by extending this switch
				statement. */
				break;
		}
	}

	/* Receive state machine. */
	pxTransferStruct = pxRxTransferControlStructs[ uxI2CNumber ];
	if( pxTransferStruct != NULL )
	{
		switch( ulI2CStatus )
		{
			case I2C_I2STAT_M_RX_SLAR_ACK:
				/* The slave address has been acknowledged. */
				if( xBytesOutstanding[ uxI2CNumber ] > 1U )
				{
					/* Expecting more than one more byte, keep ACKing. */
					LPC_I2C2->I2CONSET = I2C_I2CONSET_AA;
				}
				else
				{
					/* End the reception after the next byte. */
					LPC_I2C2->I2CONCLR = I2C_I2CONSET_AA;
				}
				LPC_I2C2->I2CONCLR = I2C_I2CONCLR_SIC;
				break;


			case I2C_I2STAT_M_RX_DAT_ACK:
			case I2C_I2STAT_M_RX_DAT_NACK:

				/* Data was received.  The strange ( ulChar++ == 0UL )
				parameter is used to ensure only a single character is
				received. */
				ulChar = 0UL;
				ioutilsRX_CHARS_INTO_CIRCULAR_BUFFER_FROM_ISR(
															pxTransferStruct, 		/* The structure that contains the reference to the circular buffer. */
															( ulChar++ == 0UL ), 	/* While loop condition. */
															LPC_I2C2->I2DAT,		/* Register holding the received character. */
															ulReceived,
															xHigherPriorityTaskWoken
														);

				configASSERT( xBytesOutstanding[ uxI2CNumber ] );
				( xBytesOutstanding[ uxI2CNumber ] )--;

				if( ulI2CStatus == I2C_I2STAT_M_RX_DAT_ACK )
				{
					if( xBytesOutstanding[ uxI2CNumber ] > 1U )
					{
						/* Expecting more than one more byte, keep ACKing. */
						LPC_I2C2->I2CONSET = I2C_I2CONSET_AA;
					}
					else
					{
						/* End the reception after the next byte. */
						LPC_I2C2->I2CONCLR = I2C_I2CONSET_AA;
					}
					LPC_I2C2->I2CONCLR = I2C_I2CONCLR_SIC;
				}
				else
				{
					/* This is the last data item. */
					configASSERT( xBytesOutstanding[ uxI2CNumber ] == 0U );
					I2C_IntCmd( LPC_I2C2, DISABLE );
					I2C_Stop( LPC_I2C2 );
					xDataDirection[ uxI2CNumber ] = i2cIdle;
				}
				break;


			default:
				/* Error and naks can be implemented by extending this switch
				statement.  */
				break;
		}
	}


	/* If lHigherPriorityTaskWoken is now equal to pdTRUE, then a context
	switch should be performed before the interrupt exists.  That ensures the
	unblocked (higher priority) task is returned to immediately. */
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}



