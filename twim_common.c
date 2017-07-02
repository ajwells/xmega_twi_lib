/*
 * twim_common.c
 *
 * Created: 6/30/2017
 *  Author: awells
 */ 

#include "twi.h"

// WAIT FOR TWI BUS TO BE IDLE
void TWI_IdleBus(void) {
	while(!(TWIC.MASTER.STATUS & TWI_MASTER_BUSSTATE_IDLE_gc)) {
		//do nothing
		//TODO: Possibly delay here
	}
}

// CHECK FOR BUS ERROR WHEN WRITING
void TWI_WriteErrorCheck(void) {
	if (!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm) || TWIC.MASTER.STATUS & (TWI_MASTER_BUSERR_bm || TWI_MASTER_ARBLOST_bm)) { // WRITE FLAG NOT WRITTEN || (BUSERROR || ARBITRATION LOST)
		// something went wrong
		//TODO: Error handling
	}
}

// CHECK FOR BUS ERROR WHEN READING
void TWI_ReadErrorCheck(void) {
	if (!(TWIC.MASTER.STATUS & TWI_MASTER_RIF_bm) || TWIC.MASTER.STATUS & (TWI_MASTER_BUSERR_bm || TWI_MASTER_ARBLOST_bm)) { // READ FLAG NOT WRITTEN || (BUSERROR || ARBITRATION LOST)
		// something went wrong
		//TODO: Error handling
	}
}

// INITIATE WRITE TO TWI_INFO->bus_address
void TWI_StartWrite(void) {
	
	//RESET VARIABLES
	TWI_INFO->dataCount = 0x00;
	
	TWI_IdleBus();
	TWI_INFO->status = STATUS_BUSY;
	
	// SEND START AND BUS ADDRESS WITH WRITE BIT (0)
	TWIC.MASTER.ADDR = (TWI_INFO->busAddress << 1);
}

// INITIATE READ TO TWI_INFO->bus_address
void TWI_StartRead(void) {
	
	//RESET VARIABLES
	TWI_INFO->dataCount = 0x00;
	
	TWI_IdleBus();
	TWI_INFO->status = STATUS_BUSY;
	
	// SEND START AND BUS ADDRESS WITH READ BIT (1)
	TWIC.MASTER.ADDR = ((TWI_INFO->busAddress) << 1 || 0x01);
}
