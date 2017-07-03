/*
 * twim_int.c
 *
 * Created: 6/21/2017
 *  Author: awells
 */ 

typedef int pedantic; // FOR PEDANTIC COMPILER WARNING

#ifdef TWIM_INT
#include "twi.h"
#include <avr/interrupt.h>
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
// TODO: ADD TWIE ISR
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// TO USE:
//	-make sure to enable global interrupts (sei)
//	-make sure to enable correct interrupt levels (PMIC.CTRL)
//	-make sure to define F_CPU
// 	-DEFINE TWIM_INT for interrupt lib
//--------------------------------------------------------------------

//--------------------------
// TWI FUNCTIONS
//--------------------------

// SETUP TWI
void TWI_InitMasterInt(TWI_INFO_STRUCT *TWI_INFO, TWI_MASTER_INTLVL_t twi_master_intlv) {
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		PR.PRPC &= ~PR_TWI_bm; // ENSURE TWI CLOCK IS ACTIVE
	
		TWIC.MASTER.BAUD = 65; // 100KHz, (Fclk/(2*Ftwi)) -5 //TODO: change this so it is not hard coded with fclk
	
		TWIC.CTRL = 0; // SDA HOLD TIME OFF
		TWIC.MASTER.CTRLA = twi_master_intlv | TWI_MASTER_RIEN_bm | TWI_MASTER_WIEN_bm | TWI_MASTER_ENABLE_bm; // SET INTERRUPT LV | ENABLE READ INTERRUPT | ENABLE WRITE INTERRUPT | ENABLE TWI
		TWIC.MASTER.CTRLB = TWI_MASTER_TIMEOUT_200US_gc; // SET TIMEOUT FOR BUS TO 200US
		TWIC.MASTER.CTRLC = 0; // CLEAR COMMAND REGISTER
	
		TWIC.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc; // PUT TWI BUS INTO IDLE STATE
	
		TWI_INFO->mode = MODE_IDLE;
	}
}

// REGISTER TWI STRUCT FOR ISR's
void TWI_RegisterStruct(TWI_INFO_STRUCT *TWI_INFO) {
	if (TWI_INFO->port == PORT_TWIC) {
		TWIC_INFO = TWI_INFO;
	} else if (TWI_INFO->port == PORT_TWIE) {
		TWIE_INFO = TWI_INFO;
	} else {
		// not a valid port
	}
}

// READ DATA FROM REG
void TWI_ReadReg_Int(void) {
	TWI_INFO->mode = MODE_MASTER_READ_REG;
	TWI_StartWrite();
}

// READ DATA
void TWI_Read_Int(void) {
	TWI_INFO->mode = MODE_MASTER_READ;
	TWI_StartRead();
}

// WRITE DATA
void TWI_Write_Int(void) {
	TWI_INFO->mode = MODE_MASTER_WRITE;
	TWI_StartWrite();
}

#ifdef USE_TWIC
// MASTER ISR
ISR(TWIC_TWIM_vect) {
	
	switch(TWIC_INFO->mode) {

		case MODE_MASTER_WRITE:
			TWI_WriteErrorCheck(TWIC_INFO);
			
			switch(TWIC_INFO->state) {
				
				case STATE_REGISTER:
					TWIC.MASTER.DATA = TWIC_INFO->registerAddress;
					TWIC_INFO->state = STATE_DATA;
					break;
				case STATE_DATA:
					if (TWIC_INFO->dataLength > TWIC_INFO->dataCount) { // MORE DATA TO WRITE
						TWIC.MASTER.DATA = TWIC_INFO->dataBuf[TWIC_INFO->dataCount++];
					} else { // SEND STOP
						TWIC_INFO->mode = MODE_IDLE;
						TWIC_INFO->status = STATUS_SUCCESS;
						TWIC_INFO->status = STATE_REGISTER;
						TWIC.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
					}
					break;
				default:
					//TODO: ERROR HANDLING
					break;
			}
			break;
		case MODE_MASTER_READ:
			TWI_ReadErrorCheck(TWIC_INFO);
			
			switch(TWIC_INFO->state) {
				
				case STATE_DATA:
					if (TWIC_INFO->dataLength > TWIC_INFO->dataCount) { // MORE DATA TO READ
						TWIC_INFO->dataBuf[TWIC_INFO->dataCount++] = TWIC.MASTER.DATA;
						if (TWIC_INFO->dataLength > TWIC_INFO->dataCount) {
							TWIC.MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
						} else { // SEND STOP
							TWIC_INFO->mode = MODE_IDLE;
							TWIC_INFO->status = STATUS_SUCCESS;
							TWIC_INFO->status = STATE_REGISTER;
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
			TWI_WriteErrorCheck(TWIC_INFO);
			
			switch(TWIC_INFO->state) {
				
				case STATE_REGISTER:
					TWIC_INFO->state = STATE_REPEAT_START;
					TWIC.MASTER.DATA = TWIC_INFO->registerAddress;
					break;
				case STATE_REPEAT_START:
					TWIC_INFO->mode = MODE_MASTER_READ;
					TWIC_INFO->state = STATE_DATA;
					TWIC.MASTER.ADDR = ((TWIC_INFO->busAddress) << 1 | 0x01); // SEND REPEATED START
					break;
				default:
					//TODO: ERROR HANDLING
					break;
			}
			break;
		default:
			// error if here
			break;
	}
}
#endif //USE_TWIC
#ifdef USE_TWIE
// MASTER ISR
ISR(TWIE_TWIM_vect) {
	
	switch(TWIE_INFO->mode) {

		case MODE_MASTER_WRITE:
			TWI_WriteErrorCheck(TWIE_INFO);
		
			switch(TWIE_INFO->state) {
			
				case STATE_REGISTER:
					TWIE.MASTER.DATA = TWIE_INFO->registerAddress;
					TWIE_INFO->state = STATE_DATA;
					break;
					case STATE_DATA:
					if (TWIE_INFO->dataLength > TWIE_INFO->dataCount) { // MORE DATA TO WRITE
						TWIE.MASTER.DATA = TWIE_INFO->dataBuf[TWIE_INFO->dataCount++];
						} else { // SEND STOP
						TWIE_INFO->mode = MODE_IDLE;
						TWIE_INFO->status = STATUS_SUCCESS;
						TWIE_INFO->status = STATE_REGISTER;
						TWIE.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
					}
					break;
				default:
					//TODO: ERROR HANDLING
					break;
			}
			break;
		case MODE_MASTER_READ:
			TWI_ReadErrorCheck(TWIE_INFO);
		
			switch(TWIE_INFO->state) {
			
				case STATE_DATA:
					if (TWIE_INFO->dataLength > TWIE_INFO->dataCount) { // MORE DATA TO READ
						TWIE_INFO->dataBuf[TWIE_INFO->dataCount++] = TWIE.MASTER.DATA;
						if (TWIE_INFO->dataLength > TWIE_INFO->dataCount) {
							TWIE.MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
							} else { // SEND STOP
							TWIE_INFO->mode = MODE_IDLE;
							TWIE_INFO->status = STATUS_SUCCESS;
							TWIE_INFO->status = STATE_REGISTER;
							TWIE.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
						}
					}
					break;
				default:
					//TODO: ERROR HANDLING
					break;
			}
			break;
		case MODE_MASTER_READ_REG:
			TWI_WriteErrorCheck(TWIE_INFO);
		
			switch(TWIE_INFO->state) {
			
				case STATE_REGISTER:
					TWIE_INFO->state = STATE_REPEAT_START;
					TWIE.MASTER.DATA = TWIE_INFO->registerAddress;
					break;
				case STATE_REPEAT_START:
					TWIE_INFO->mode = MODE_MASTER_READ;
					TWIE_INFO->state = STATE_DATA;
					TWIE.MASTER.ADDR = ((TWIE_INFO->busAddress) << 1 | 0x01); // SEND REPEATED START
					break;
				default:
					//TODO: ERROR HANDLING
					break;
			}
			break;
		default:
			// error if here
			break;
	}
}
#endif //USE_TWIE
#endif //TWIM_INT
