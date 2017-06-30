/*
 * twim_int.c
 *
 * Created: 6/21/2017 3:53:31 PM
 *  Author: awells
 */ 

#include "twi.h"
#include <avr/interrupt.h>

volatile TWI_INFO_STRUCT *TWI_INFO;

//--------------------------------------------------------------------
// TODO: CHECK IF OUT OF BOUNDS ON DATA BUFFER
// TODO: ASSUMES REGISTER ADDRESSES ARE ONLY 1 BYTE
// TODO: CALCULATE BUAD WITH F_CPU
// TODO: ERROR CHECKING
// TODO: CHECK RXACK REG IN ERROR CHECKING
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// TO USE:
//	-make sure to enable global interrupts (sei)
//	-make sure to enable correct interrupt levels (PMIC.CTRL)
//	-make sure to define F_CPU
//--------------------------------------------------------------------

//--------------------------
// TWI FUNCTIONS
//--------------------------

// SETUP TWI
void TWI_Init_Master(TWI_MASTER_INTLVL_t twi_master_intlv) {
	
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
	TWIC.MASTER.CTRLA = twi_master_intlv | TWI_MASTER_RIEN_bm | TWI_MASTER_WIEN_bm | TWI_MASTER_ENABLE_bm; // SET INTERRUPT LV | ENABLE READ INTERRUPT | ENABLE WRITE INTERRUPT | ENABLE TWI
	TWIC.MASTER.CTRLB = TWI_MASTER_TIMEOUT_200US_gc; // SET TIMEOUT FOR BUS TO 200US
	TWIC.MASTER.CTRLC = 0;
	
	TWIC.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc; // PUT TWI BUS INTO IDLE STATE
	
	TWI_INFO->mode = MODE_IDLE;
}

// WAIT FOR TWI BUS TO BE IDLE
void TWI_Idle_Bus(void) {
	while(!(TWIC.MASTER.STATUS & TWI_MASTER_BUSSTATE_IDLE_gc)) {
		//do nothing
		//TODO: Possibly delay here
	}
}

// CHECK FOR BUS ERROR WHEN WRITING
void TWI_Write_Bus_Error_Check(void) {
	if (!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm) || TWIC.MASTER.STATUS & (TWI_MASTER_BUSERR_bm || TWI_MASTER_ARBLOST_bm)) { // WRITE FLAG NOT WRITTEN || (BUSERROR || ARBITRATION LOST)
		// something went wrong
		//TODO: Error handling
	}
}

// CHECK FOR BUS ERROR WHEN READING
void TWI_Read_Bus_Error_Check(void) {
	if (!(TWIC.MASTER.STATUS & TWI_MASTER_RIEN_bm) || TWIC.MASTER.STATUS & (TWI_MASTER_BUSERR_bm || TWI_MASTER_ARBLOST_bm)) { // READ FLAG NOT WRITTEN || (BUSERROR || ARBITRATION LOST)
		// something went wrong
		//TODO: Error handling
	}
}

// READ DATA FROM REG
void TWI_Read_Reg(void) {
	TWI_INFO->mode = MODE_MASTER_READ_REG;
	TWI_Start_Write();
}

// READ DATA
void TWI_Read(void) {
	TWI_INFO->mode = MODE_MASTER_READ;
	TWI_Start_Read();
}

// WRITE DATA
void TWI_Write(void) {
	TWI_INFO->mode = MODE_MASTER_WRITE;
	TWI_Start_Write();
}

// INITIATE WRITE TO TWI_INFO->bus_address
void TWI_Start_Write(void) {
	
	//RESET VARIABLES
	TWI_INFO->dataCount = 0x00;
	
	TWI_Idle_Bus();
	TWI_INFO->status = STATUS_BUSY;
	
	// SEND START AND BUS ADDRESS WITH WRITE BIT (0)
	TWIC.MASTER.ADDR = (TWI_INFO->busAddress << 1);
}

// INITIATE READ TO TWI_INFO->bus_address
void TWI_Start_Read(void) {
	
	//RESET VARIABLES
	TWI_INFO->dataCount = 0x00;
	
	TWI_Idle_Bus();
	TWI_INFO->status = STATUS_BUSY;
	
	// SEND START AND BUS ADDRESS WITH READ BIT (1)
	TWIC.MASTER.ADDR = ((TWI_INFO->busAddress) << 1 || 0x01);
}

// MASTER ISR
ISR(TWIC_TWIM_vect) {
	
	switch(TWI_INFO->mode) {

		case MODE_MASTER_WRITE:
			TWI_Write_Bus_Error_Check();
			
			switch(TWI_INFO->state) {
				
				case STATE_REGISTER:
					TWIC.MASTER.DATA = TWI_INFO->registerAddress;
					TWI_INFO->state = STATE_DATA;
					break;
				case STATE_DATA:
					if (TWI_INFO->dataLength > TWI_INFO->dataCount) { // MORE DATA TO WRITE
						TWIC.MASTER.DATA = TWI_INFO->dataBuf[TWI_INFO->dataCount++];
					} else { // SEND STOP
						TWI_INFO->mode = MODE_IDLE;
						TWI_INFO->status = STATUS_SUCCESS;
						TWI_INFO->status = STATE_REGISTER;
						TWIC.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
					}
					break;
				default:
					//TODO: ERROR HANDLING
					break;
			}
			break;
		case MODE_MASTER_READ:
			TWI_Read_Bus_Error_Check();
			
			switch(TWI_INFO->state) {
				
				case STATE_DATA:
					if (TWI_INFO->dataLength > TWI_INFO->dataCount) { // MORE DATA TO READ
						TWI_INFO->dataBuf[TWI_INFO->dataCount++] = TWIC.MASTER.DATA;
						if (TWI_INFO->dataLength > TWI_INFO->dataCount) {
							TWIC.MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
						} else { // SEND STOP
							TWI_INFO->mode = MODE_IDLE;
							TWI_INFO->status = STATUS_SUCCESS;
							TWI_INFO->status = STATE_REGISTER;
							TWIC.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
						}
					}
					break;
				default:
					//TODO: ERROR HANDLING
					break;
			}
			break;
		case MODE_MASTER_READ_REG:
			TWI_Write_Bus_Error_Check();
			
			switch(TWI_INFO->state) {
				
				case STATE_REGISTER:
					TWI_INFO->state = STATE_REPEAT_START;
					TWIC.MASTER.DATA = TWI_INFO->registerAddress;
					break;
				case STATE_REPEAT_START:
					TWI_INFO->mode = MODE_MASTER_READ;
					TWI_INFO->state = STATE_DATA;
					TWIC.MASTER.ADDR = ((TWI_INFO->busAddress) << 1 | 0x01); // SEND REPEATED START
					break;
				default:
					//TODO: ERROR HANDLING
					break;
			}
			break;
		default:
			break;
	}
}
