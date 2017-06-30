/*
 * twi_lib.h
 *
 * Created: 6/21/2017 2:43:31 PM
 *  Author: awells
 */ 

#ifndef _TWI_LIB_
#define _TWI_LIB_

#include <stdlib.h>
#include <avr/io.h>


// VALUES
#define TWI_BUFFER_SIZE 4

// ENUMS
typedef enum TWI_MODE{
	MODE_IDLE,
	MODE_INIT,
	MODE_MASTER_WRITE,
	MODE_MASTER_READ,
	MODE_MASTER_READ_REG,
	//MODE_SLAVE_WRITE,
	//MODE_SLAVE_READ
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

//STRUCTS
typedef struct TWI_INFO_STRUCT{
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
extern volatile TWI_INFO_STRUCT *TWI_INFO;

// FUNCTIONS
#ifdef TWIM_INT
void TWI_Init_Master(TWI_MASTER_INTLVL_t twi_master_intlv);
#endif
#ifdef TWIM_POLL
void TWI_Init_Master(void);
#endif
void TWI_Idle_Bus(void);
void TWI_Write_Error_Check(void);
void TWI_Read_Error_Check(void);
void TWI_Start_Read(void);
void TWI_Start_Write(void);

void TWI_Read_Reg(void);
void TWI_Read(void);
void TWI_Write(void);


#endif // _TWI_LIB_