/*
 * twi_lib.h
 *
 * Created: 6/21/2017
 *  Author: awells
 */ 

#ifndef _TWI_LIB_
#define _TWI_LIB_

#include <stdlib.h>
#include <avr/io.h>

// DEFINES FOR USAGE
#define USE_TWIC
#define USE_TWIE
#define TWIM_POLL
#define TWIM_INT

// VALUES
#define PORT_TWIC TWIC
#define PORT_TWIE TWIE
typedef TWI_t TWI_PORT;

// ENUMS
typedef enum TWI_MODE{
	MODE_IDLE,
	MODE_INIT,
	MODE_MASTER_WRITE,
	MODE_MASTER_READ,
	MODE_MASTER_READ_REG
} TWI_MODE;

typedef enum TWI_STATUS{
	STATUS_IDLE,
	STATUS_BUSY,
	STATUS_SUCCESS,
	STATUS_FAILURE
} TWI_STATUS;

typedef enum TWI_STATE{
	STATE_REGISTER,
	STATE_DATA,
	STATE_REPEAT_START
} TWI_STATE;

/*typedef enum TWI_PORT{
	PORT_TWIC,
	PORT_TWIE
} TWI_PORT;*/

//STRUCTS
typedef struct TWI_INFO_STRUCT{
	TWI_PORT *port;
	volatile TWI_MODE mode;
	volatile TWI_STATUS status;
	volatile TWI_STATE state;
	volatile uint8_t busAddress;
	volatile uint8_t registerAddress;
	volatile uint8_t *dataBuf;
	volatile uint8_t dataLength;
	volatile uint8_t dataCount;
}TWI_INFO_STRUCT;

// VARIABLES
volatile TWI_INFO_STRUCT *TWIC_INFO;
volatile TWI_INFO_STRUCT *TWIE_INFO;

// FUNCTIONS
#ifdef TWIM_INT
void TWI_InitMaster_Int(TWI_MASTER_INTLVL_t twi_master_intlv);
void TWI_RegisterStruct_Int(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_ReadReg_Int(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_Read_Int(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_Write_Int(volatile TWI_INFO_STRUCT *TWI_INFO);
#endif
#ifdef TWIM_POLL
void TWI_InitMaster_Poll(void);
void TWI_RegisterStruct_Poll(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_ReadReg_Poll(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_Read_Poll(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_Write_Poll(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_WriteWaitAndCheck(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_ReadWaitAndCheck(volatile TWI_INFO_STRUCT *TWI_INFO);
#endif
void TWI_InitStruct(volatile TWI_INFO_STRUCT *TWI_INFO, TWI_PORT *port, uint8_t busAddress, uint8_t registerAddress, volatile uint8_t *dataBuff, uint8_t dataLength);
void TWI_UpdateStruct(volatile TWI_INFO_STRUCT *TWI_INFO, uint8_t busAddress, uint8_t registerAddress, volatile uint8_t *dataBuff, uint8_t dataLength);
void TWI_IdleBus(TWI_PORT *port);
void TWI_WriteErrorCheck(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_ReadErrorCheck(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_StartRead(volatile TWI_INFO_STRUCT *TWI_INFO);
void TWI_StartWrite(volatile TWI_INFO_STRUCT *TWI_INFO);

#endif //_TWI_LIB_