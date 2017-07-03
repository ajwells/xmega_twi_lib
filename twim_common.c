/*
 * twim_common.c
 *
 * Created: 6/30/2017
 *  Author: awells
 */ 

#include "twi.h"

// SETUP TWI STRUCT
void TWI_InitStruct(volatile TWI_INFO_STRUCT *TWI_INFO, TWI_PORT *port, uint8_t busAddress, uint8_t registerAddress, volatile uint8_t *dataBuff, uint8_t dataLength) {
	
	TWI_INFO->port = port;
	TWI_INFO->mode = MODE_INIT;
	TWI_INFO->status = STATUS_IDLE;
	TWI_INFO->state = STATE_REGISTER;
	TWI_INFO->busAddress = busAddress;
	TWI_INFO->registerAddress = registerAddress;
	TWI_INFO->dataBuf = dataBuff;
	TWI_INFO->dataLength = dataLength;
	TWI_INFO->dataCount = 0x00;
}

// UPDATE TWI STRUCT WITH NEW ADDRESSES
void TWI_UpdateStruct(volatile TWI_INFO_STRUCT *TWI_INFO, uint8_t busAddress, uint8_t registerAddress, volatile uint8_t *dataBuff, uint8_t dataLength) {
	
	TWI_INFO->busAddress = busAddress;
	TWI_INFO->registerAddress = registerAddress;
	TWI_INFO->dataBuf = dataBuff;
	TWI_INFO->dataLength = dataLength;
	TWI_INFO->dataCount = 0x00;
}

// WAIT FOR TWI BUS TO BE IDLE
void TWI_IdleBus(TWI_PORT *port) {
	while(!(port->MASTER.STATUS & TWI_MASTER_BUSSTATE_IDLE_gc)) {
		//do nothing
		//TODO: Possibly delay here
	}
}

// CHECK FOR BUS ERROR WHEN WRITING
void TWI_WriteErrorCheck(volatile TWI_INFO_STRUCT *TWI_INFO) {
	if (!(TWI_INFO->port->MASTER.STATUS & TWI_MASTER_WIF_bm) || TWI_INFO->port->MASTER.STATUS & (TWI_MASTER_BUSERR_bm || TWI_MASTER_ARBLOST_bm)) { // WRITE FLAG NOT WRITTEN || (BUSERROR || ARBITRATION LOST)
		// something went wrong
		//TODO: Error handling
	}
}

// CHECK FOR BUS ERROR WHEN READING
void TWI_ReadErrorCheck(volatile TWI_INFO_STRUCT *TWI_INFO) {
	if (!(TWI_INFO->port->MASTER.STATUS & TWI_MASTER_RIF_bm) || TWI_INFO->port->MASTER.STATUS & (TWI_MASTER_BUSERR_bm || TWI_MASTER_ARBLOST_bm)) { // READ FLAG NOT WRITTEN || (BUSERROR || ARBITRATION LOST)
		// something went wrong
		//TODO: Error handling
	}
}

// INITIATE WRITE TO TWI_INFO->bus_address
void TWI_StartWrite(volatile TWI_INFO_STRUCT *TWI_INFO) {
	
	//RESET VARIABLES
	TWI_INFO->dataCount = 0x00;
	
	TWI_IdleBus(TWI_INFO->port);
	TWI_INFO->status = STATUS_BUSY;
	
	// SEND START AND BUS ADDRESS WITH WRITE BIT (0)
	TWIC.MASTER.ADDR = (TWI_INFO->busAddress << 1);
}

// INITIATE READ TO TWI_INFO->bus_address
void TWI_StartRead(volatile TWI_INFO_STRUCT *TWI_INFO) {
	
	//RESET VARIABLES
	TWI_INFO->dataCount = 0x00;
	
	TWI_IdleBus(TWI_INFO->port);
	TWI_INFO->status = STATUS_BUSY;
	
	// SEND START AND BUS ADDRESS WITH READ BIT (1)
	TWIC.MASTER.ADDR = ((TWI_INFO->busAddress) << 1 || 0x01);
}
