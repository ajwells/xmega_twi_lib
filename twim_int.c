/*
 * twim_int.c
 *
 * Created: 6/21/2017
 *  Author: awells
 */ 

#include "twim.h"
#ifdef TWIM_INT
#include <avr/interrupt.h>
#include <util/atomic.h>

// SETUP TWI
void TWI_InitMaster_Int(TWI_MASTER_INTLVL_t twi_master_intlv) {
	
	#ifdef USE_TWIC
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		PR.PRPC &= ~PR_TWI_bm; // ENSURE TWI CLOCK IS ACTIVE
	
		TWIC.MASTER.BAUD = (F_CPU/(2*TWI_FREQ)) - 5;
	
		TWIC.CTRL = 0; // SDA HOLD TIME OFF
		TWIC.MASTER.CTRLA = twi_master_intlv | TWI_MASTER_RIEN_bm | TWI_MASTER_WIEN_bm | TWI_MASTER_ENABLE_bm; // SET INTERRUPT LV | ENABLE READ INTERRUPT | ENABLE WRITE INTERRUPT | ENABLE TWI
		TWIC.MASTER.CTRLB = TWI_MASTER_TIMEOUT_200US_gc; // SET TIMEOUT FOR BUS TO 200US
		TWIC.MASTER.CTRLC = 0; // CLEAR COMMAND REGISTER
	
		TWIC.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc; // PUT TWI BUS INTO IDLE STATE
	}
	#endif //USE_TWIC
	#ifdef USE_TWIE
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		PR.PRPC &= ~PR_TWI_bm; // ENSURE TWI CLOCK IS ACTIVE
		
		TWIE.MASTER.BAUD = (F_CPU/(2*TWI_FREQ)) - 5;
		
		TWIE.CTRL = 0; // SDA HOLD TIME OFF
		TWIE.MASTER.CTRLA = twi_master_intlv | TWI_MASTER_RIEN_bm | TWI_MASTER_WIEN_bm | TWI_MASTER_ENABLE_bm; // SET INTERRUPT LV | ENABLE READ INTERRUPT | ENABLE WRITE INTERRUPT | ENABLE TWI
		TWIE.MASTER.CTRLB = TWI_MASTER_TIMEOUT_200US_gc; // SET TIMEOUT FOR BUS TO 200US
		TWIE.MASTER.CTRLC = 0; // CLEAR COMMAND REGISTER
		
		TWIE.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc; // PUT TWI BUS INTO IDLE STATE
	}
	#endif //USE_TWIE
}

// REGISTER TWI STRUCT FOR ISR's
void TWI_RegisterStruct_Int(volatile TWI_INFO_STRUCT *TWI_INFO) {
		
	TWI_INFO->mode = MODE_IDLE;
	
	if (TWI_INFO->port == &(PORT_TWIC)) {
		TWIC_INFO = TWI_INFO;
	} else if (TWI_INFO->port == &(PORT_TWIE)) {
		TWIE_INFO = TWI_INFO;
	} else {
		exit(EXIT_FAILURE); // NOT A VALID TWI PORT
	}
}

// READ DATA FROM REG
void TWI_ReadReg_Int(volatile TWI_INFO_STRUCT *TWI_INFO) {
	TWI_INFO->status = STATUS_BUSY;
	TWI_INFO->mode = MODE_MASTER_READ_REG;
	TWI_StartWrite(TWI_INFO);
}

// READ DATA
void TWI_Read_Int(volatile TWI_INFO_STRUCT *TWI_INFO) {
	TWI_INFO->status = STATUS_BUSY;
	TWI_INFO->mode = MODE_MASTER_READ;
	TWI_StartRead(TWI_INFO);
}

// WRITE DATA
void TWI_Write_Int(volatile TWI_INFO_STRUCT *TWI_INFO) {
	TWI_INFO->status = STATUS_BUSY;
	TWI_INFO->mode = MODE_MASTER_WRITE;
	TWI_StartWrite(TWI_INFO);
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
						TWIC_INFO->state = STATE_REGISTER;
						TWIC.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
					}
					break;
				default:
					exit(EXIT_FAILURE); // NOT IN CORRECT STATE
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
							TWIC_INFO->state = STATE_REGISTER;
							TWIC.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
						}
					}
					break;
				default:
					exit(EXIT_FAILURE); // NOT IN CORRECT STATE
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
					exit(EXIT_FAILURE); // NOT IN CORRECT STATE
					break;
			}
			break;
		default:
			exit(EXIT_FAILURE); // NOT IN CORRECT MODE
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
						TWIE_INFO->state = STATE_REGISTER;
						TWIE.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
					}
					break;
				default:
					exit(EXIT_FAILURE); // NOT IN CORRECT STATE
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
							TWIE_INFO->state = STATE_REGISTER;
							TWIE.MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
						}
					}
					break;
				default:
					exit(EXIT_FAILURE); // NOT IN CORRECT STATE
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
					exit(EXIT_FAILURE); // NOT IN CORRECT STATE
					break;
			}
			break;
		default:
			exit(EXIT_FAILURE); // NOT IN CORRECT MODE
			break;
	}
}
#endif //USE_TWIE
#endif //TWIM_INT