/*
 * twim_int.c
 *
 * Created: 6/30/2017
 *  Author: awells
 */ 

typedef int pedantic; // FOR PEDANTIC COMPILER WARNING

#ifdef TWIM_POLL
#include "twi.h"

volatile TWI_INFO_STRUCT *TWI_INFO;

//--------------------------------------------------------------------
// TO USE:
//	-make sure to define F_CPU
//  -DEFINE TWIM_POLL for polling lib
//--------------------------------------------------------------------

void TWI_Init_Master(void) {
	
	volatile TWI_INFO_STRUCT *TWI_INFO = malloc(sizeof(TWI_INFO_STRUCT));
	
	TWI_INFO->mode = MODE_INIT;
	TWI_INFO->status = STATUS_IDLE;
	TWI_INFO->state = STATE_REGISTER;
	TWI_INFO->busAddress = 0x00;
	TWI_INFO->registerAddress = 0x00;
	TWI_INFO->dataBuf = NULL;
	TWI_INFO->dataLength = 0x00;
	TWI_INFO->dataCount = 0x00;
	
	PR.PRPC &= ~PR_TWI_bm; // ENSURE TWI CLOCK IS ACTIVE
	
	TWIC.MASTER.BAUD = 65; // 100KHz, (Fclk/(2*Ftwi)) -5 //TODO: change this so it is not hard coded with fclk
	
	TWIC.CTRL = 0; // SDA HOLD TIME OFF
	TWIC.MASTER.CTRLA = TWI_MASTER_ENABLE_bm; // ENABLE WITH NO INTERRUPTS
	TWIC.MASTER.CTRLB = TWI_MASTER_TIMEOUT_200US_gc; // SET TIMEOUT FOR BUS TO 200US
	TWIC.MASTER.CTRLC = 0; // CLEAR COMMAND REGISTER

	
	TWIC.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc; // PUT TWI BUS INTO IDLE STATE
	
	TWI_INFO->mode = MODE_IDLE;
}

// READ DATA FROM REG
void TWI_Read_Reg(void) {
	
	TWI_INFO->mode = MODE_MASTER_READ_REG;
	TWI_Start_Write();
	
	while (!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
		// wait possible delay here
	}
	
	TWI_Write_Error_Check();
	
	// WRITE ADDRESS
	TWIC.MASTER.DATA = TWI_INFO->registerAddress;
	TWI_INFO->state = STATE_DATA;
	
	while (!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
		// wait possible delay here
	}
	
	TWI_Write_Error_Check();
	
	// SEND REPEATED START
	TWI_INFO->mode = MODE_MASTER_READ;
	TWI_INFO->state = STATE_DATA;
	TWIC.MASTER.ADDR = ((TWI_INFO->busAddress) << 1 | 0x01); // SEND REPEATED START
	
	while (!(TWIC.MASTER.STATUS & (TWI_MASTER_RIF_bm | TWI_MASTER_BUSERR_bm | TWI_MASTER_ARBLOST_bm))) {
		// wait possible delay here
	}
	
	TWI_Read_Error_Check();
	
	for (TWI_INFO->dataCount=0; TWI_INFO->dataCount < TWI_INFO->dataLength; TWI_INFO->dataCount++) {
		TWI_INFO->dataBuf[TWI_INFO->dataCount] = TWIC.MASTER.DATA;
		while (!(TWIC.MASTER.STATUS & (TWI_MASTER_RIF_bm | TWI_MASTER_BUSERR_bm | TWI_MASTER_ARBLOST_bm))) {
			// wait possible delay here
		}
		TWI_Read_Error_Check();
	}
	
	// SEND STOP
	TWI_INFO->mode = MODE_IDLE;
	TWI_INFO->status = STATUS_SUCCESS;
	TWI_INFO->status = STATE_REGISTER;
	TWIC.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
}

// READ DATA
void TWI_Read(void) {
	
	TWI_INFO->mode = MODE_MASTER_READ;
	TWI_Start_Read();
	
	while (!(TWIC.MASTER.STATUS & (TWI_MASTER_RIF_bm | TWI_MASTER_BUSERR_bm | TWI_MASTER_ARBLOST_bm))) {
		// wait possible delay here
	}
	
	TWI_Read_Error_Check();
	
	for (TWI_INFO->dataCount=0; TWI_INFO->dataCount < TWI_INFO->dataLength; TWI_INFO->dataCount++) {
		TWI_INFO->dataBuf[TWI_INFO->dataCount] = TWIC.MASTER.DATA;
		while (!(TWIC.MASTER.STATUS & (TWI_MASTER_RIF_bm | TWI_MASTER_BUSERR_bm | TWI_MASTER_ARBLOST_bm))) {
			// wait possible delay here
		}
		TWI_Read_Error_Check();
	}
	
	// SEND STOP
	TWI_INFO->mode = MODE_IDLE;
	TWI_INFO->status = STATUS_SUCCESS;
	TWI_INFO->status = STATE_REGISTER;
	TWIC.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
}

// WRITE DATA
void TWI_Write(void) {
	
	TWI_INFO->mode = MODE_MASTER_WRITE;
	TWI_Start_Write();
	
	while (!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
		// wait possible delay here
	}
	
	TWI_Write_Error_Check();
	
	// WRITE ADDRESS
	TWIC.MASTER.DATA = TWI_INFO->registerAddress;
	TWI_INFO->state = STATE_DATA;
	
	while (!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
		// wait possible delay here
	}
	
	TWI_Write_Error_Check();
	
	// WRITE DATA
	for (TWI_INFO->dataCount=0; TWI_INFO->dataCount < TWI_INFO->dataLength; TWI_INFO->dataCount++) {
		TWIC.MASTER.DATA = TWI_INFO->dataBuf[TWI_INFO->dataCount];
		while (!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm)) {
			// wait possible delay here
		}
		TWI_Write_Error_Check();
	}
	
	// SEND STOP
	TWI_INFO->mode = MODE_IDLE;
	TWI_INFO->status = STATUS_SUCCESS;
	TWI_INFO->status = STATE_REGISTER;
	TWIC.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
}
#endif //TWIM_POLL