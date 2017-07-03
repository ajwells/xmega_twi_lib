/*
 * twim_int.c
 *
 * Created: 6/30/2017
 *  Author: awells
 */ 

typedef int pedantic; // FOR PEDANTIC COMPILER WARNING

#ifdef TWIM_POLL
#include "twi.h"
#include <util/atomic.h>

volatile TWI_INFO_STRUCT *TWI_INFO;

//--------------------------------------------------------------------
// TODO: CHECK IF OUT OF BOUNDS ON DATA BUFFER
// TODO: ASSUMES REGISTER ADDRESSES ARE ONLY 1 BYTE
// TODO: CALCULATE BUAD WITH F_CPU
// TODO: ERROR CHECKING
// TODO: CHECK RXACK REG IN ERROR CHECKING
// TODO: CHECK WIF WHEN SENDING NACK IN MASTER READ
// TODO: CHECK WHAT NEEDS TO BE ATOMIC
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// TO USE:
//	-make sure to define F_CPU
//  -DEFINE TWIM_POLL for polling lib
//--------------------------------------------------------------------

void TWI_InitMaster_Poll(void) {
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		PR.PRPC &= ~PR_TWI_bm; // ENSURE TWI CLOCK IS ACTIVE
	
		TWIC.MASTER.BAUD = 65; // 100KHz, (Fclk/(2*Ftwi)) -5 //TODO: change this so it is not hard coded with fclk
	
		TWIC.CTRL = 0; // SDA HOLD TIME OFF
		TWIC.MASTER.CTRLA = TWI_MASTER_ENABLE_bm; // ENABLE WITH NO INTERRUPTS
		TWIC.MASTER.CTRLB = TWI_MASTER_TIMEOUT_200US_gc; // SET TIMEOUT FOR BUS TO 200US
		TWIC.MASTER.CTRLC = 0; // CLEAR COMMAND REGISTER

	
		TWIC.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc; // PUT TWI BUS INTO IDLE STATE
	
		TWI_INFO->mode = MODE_IDLE;
	}
}

void TWI_WriteWaitAndCheck(void) {
	
	while (!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
		// wait possible delay here
	}
	
	TWI_WriteErrorCheck();
}

void TWI_ReadWaitAndCheck(void) {
	
	while (!(TWIC.MASTER.STATUS & (TWI_MASTER_RIF_bm | TWI_MASTER_BUSERR_bm | TWI_MASTER_ARBLOST_bm))) {
		// wait possible delay here
	}
	
	TWI_ReadErrorCheck();
}

// READ DATA FROM REG
void TWI_ReadReg_Poll(void) {
	
	TWI_INFO->mode = MODE_MASTER_READ_REG;
	TWI_StartWrite();
	
	TWI_WriteWaitAndCheck();
	
	// WRITE ADDRESS
	TWIC.MASTER.DATA = TWI_INFO->registerAddress;
	TWI_INFO->state = STATE_DATA;
	
	TWI_WriteWaitAndCheck();
	
	// SEND REPEATED START
	TWI_INFO->mode = MODE_MASTER_READ;
	TWI_INFO->state = STATE_DATA;
	TWIC.MASTER.ADDR = ((TWI_INFO->busAddress) << 1 | 0x01); // SEND REPEATED START
	
	TWI_ReadWaitAndCheck();
	
	for (TWI_INFO->dataCount=0; TWI_INFO->dataCount < TWI_INFO->dataLength; TWI_INFO->dataCount++) {
		TWI_INFO->dataBuf[TWI_INFO->dataCount] = TWIC.MASTER.DATA;
		TWI_ReadWaitAndCheck();
	}
	
	// SEND STOP
	TWI_INFO->mode = MODE_IDLE;
	TWI_INFO->status = STATUS_SUCCESS;
	TWI_INFO->status = STATE_REGISTER;
	TWIC.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
}

// READ DATA
void TWI_Read_Poll(void) {
	
	TWI_INFO->mode = MODE_MASTER_READ;
	TWI_StartRead();
	
	TWI_ReadWaitAndCheck();
	
	for (TWI_INFO->dataCount=0; TWI_INFO->dataCount < TWI_INFO->dataLength; TWI_INFO->dataCount++) {
		TWI_INFO->dataBuf[TWI_INFO->dataCount] = TWIC.MASTER.DATA;
		TWI_ReadWaitAndCheck();
	}
	
	// SEND STOP
	TWI_INFO->mode = MODE_IDLE;
	TWI_INFO->status = STATUS_SUCCESS;
	TWI_INFO->status = STATE_REGISTER;
	TWIC.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
}

// WRITE DATA
void TWI_Write_Poll(void) {
	
	TWI_INFO->mode = MODE_MASTER_WRITE;
	TWI_StartWrite();
	
	TWI_WriteWaitAndCheck();
	
	// WRITE ADDRESS
	TWIC.MASTER.DATA = TWI_INFO->registerAddress;
	TWI_INFO->state = STATE_DATA;
	
	TWI_WriteWaitAndCheck();
	
	// WRITE DATA
	for (TWI_INFO->dataCount=0; TWI_INFO->dataCount < TWI_INFO->dataLength; TWI_INFO->dataCount++) {
		TWIC.MASTER.DATA = TWI_INFO->dataBuf[TWI_INFO->dataCount];
		TWI_WriteWaitAndCheck();
	}
	
	// SEND STOP
	TWI_INFO->mode = MODE_IDLE;
	TWI_INFO->status = STATUS_SUCCESS;
	TWI_INFO->status = STATE_REGISTER;
	TWIC.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
}
#endif //TWIM_POLL
