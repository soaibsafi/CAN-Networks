Main.c  // ecu 1

#include "me_init.h"

/********************************************************************
 *             Global variables and function declarations           *
 ********************************************************************/

void sendAliveSignal(int, int);
void sendErrorSignal(int, int);
void toggle_LED_P();
void sendMessage(int, int, int, int);

void sendRPM();
void sendECU_1_alive();
void sendECU_1_error();
void sendLights();

// ECU 1 received data
int left = 0;
int right = 0;
int hazard = 0;
int high = 0;
int low = 0;
int parking = 0;
int automatic = 0;

// global variables
int timerCounter = 0;
int ECU_2_iteration = 0; // ECU1

/* Switches and buttons variable to be used to receive signals from board */
int SW1 = 0;
int SW2 = 0;
int SW3 = 0;
int SW4 = 0;
int BT5 = 0;
int BT6 = 0;

/********************************************************************
 *                         Application entry point                  *
 ********************************************************************/
int main(void) {

	/* Board and modules initialization */
	ME_Init();

	/* Configure and start timmer channels */
	PIT_ConfigureTimer(1, 100); // channel 1
	PIT_StartTimer(1);

	/* Application main loop that runs forever*/
	for (;;) {

		/* Operating System Delay*/
		osalThreadDelayMilliseconds(10UL);
	}
}

/********************************************************************
 *                   Can Reception Function                         *
 ********************************************************************/
void can_receive() {
	if (CAN_0.IFRL.B.BUF5I == 1) {
		int length = 0;
		switch (CAN_0.BUF[0].ID.B.STD_ID) {

		case 0x82:
			updateInputs();
			break;

		case 0x302: {
			// indicators

			length = CAN_0.BUF[0].CS.B.LENGTH;
			if (1) {
				char data = CAN_0.BUF[0].DATA.B[0];

				left = (data >> 0) & 0x01; // GLOBAL
				right = (data >> 1) & 0x01; // GLOBAL
				hazard = (data >> 2) & 0x01; // GLOBAL
			}
		}
			break;
		case 0x403: { // only in manual mode
					  // headlights

			length = CAN_0.BUF[0].CS.B.LENGTH;
			if (1) {
				char data = CAN_0.BUF[0].DATA.B[0];

				high = (data >> 0) & 0x01; // GLOBAL
				low = (data >> 1) & 0x01; // GLOBAL
				parking = (data >> 2) & 0x01; // GLOBAL
			}
		}
			break;

		case 0x304: {
			// ECU 2 alive

			ECU_2_iteration = 0;
		}
			break;

		default:
			CAN_0.BUF[0].ID.B.STD_ID;
			break;
		}
	}
	//CAN_0.IFRL.B.BUF5I = 1;
}

void sendAliveSignal(int msg_id, int msg_buffer) {
	//CAN_0.BUF[msg_buffer].DATA.B[0] = 0x00;
	CAN_0.BUF[msg_buffer].CS.B.LENGTH = 0; // 1 byte

	CAN_0.BUF[msg_buffer].MSG_ID.B.STD_ID = msg_id;
	CAN_0.BUF[msg_buffer].CS.B.CODE = 0b1100; //? Transmit data frame unconditionally once. After transmission, the MB automatically returns to the INACTIVE state.
}

void sendErrorSignal(int msg_id, int msg_buffer) {
	//CAN_0.BUF[msg_buffer].DATA.B[0] = 0x00;
	CAN_0.BUF[msg_buffer].CS.B.LENGTH = 0; // 1 byte

	CAN_0.BUF[msg_buffer].MSG_ID.B.STD_ID = msg_id;
	CAN_0.BUF[msg_buffer].CS.B.CODE = 0b1100; //? Transmit data frame unconditionally once. After transmission, the MB automatically returns to the INACTIVE state.
}

void toggle_LED_P() {
	// toggle LED P
	P= ~P;
}

void sendMessage(int data, int length, int msg_id, int msg_buffer) {
	if (length == 2) {
		// 2 bytes

		char firstBits = (data >> 8) & 0xFF;
		char lastBits = data & 0xFF;

		CAN_0.BUF[msg_buffer].DATA.B[0] = lastBits;
		CAN_0.BUF[msg_buffer].DATA.B[1] = firstBits;
		CAN_0.BUF[msg_buffer].CS.B.LENGTH = 2; // 2 byte
	} else {
		// 1 byte

		char bits = data & 0xFF;

		CAN_0.BUF[msg_buffer].DATA.B[0] = bits;
		CAN_0.BUF[msg_buffer].CS.B.LENGTH = 1; // 1 byte
	}

	CAN_0.BUF[msg_buffer].MSG_ID.B.STD_ID = msg_id;
	CAN_0.BUF[msg_buffer].CS.B.CODE = 0b1100; //? Transmit data frame unconditionally once. After transmission, the MB automatically returns to the INACTIVE state.
}

void sendRPM() { // 100ms
// send RPM

	int rpm = POT;

	sendMessage(rpm, 2 /* length */, 0x202 /* msg id */, 8 /* msg buffer */);
}

void sendECU_1_alive() { // 200ms
// send ECU 1 alive

	sendAliveSignal(0x203, 10);
}

void sendECU_1_error() { // 200ms
// send ECU 1 error

	if (ECU_2_iteration > 8) {
		sendErrorSignal(0x204, 11);
		toggle_LED_P();
	}
}

void sendLights() { // 400ms
// TODO send lights (indicators and headlights) (REQUIRES RECEIVED DATA)

	// Received Data: int left = 0; int right = 0; int hazard = 0; int high = 0; int low = 0; int parking = 0;

	int data = ((1 * left) + (2 * right) + (4 * hazard) + (8 * high)
			+ (16 * low) + (32 * parking)); // manual

	// check, if automatic
	if (automatic) {
		if (LDR < 512) { // value of light sensor below 512
			// turn on (0) low beam and parking light
			data = ((1 * left) + (2 * right) + (4 * hazard) + (8 * high)
					+ (16 * 0) + (32 * 0));
		} else {
			// turn off (1) low beam and parking light
			data = ((1 * left) + (2 * right) + (4 * hazard) + (8 * high)
					+ (16 * 1) + (32 * 1));
		}

	}

	sendMessage(data, 1 /* length */, 0x201 /* msg id */, 9 /* msg buffer */);
}

/********************************************************************
 *                   Interrupt Handlers for PIT Channel 1-3         *
 ********************************************************************/
void Pit_Channel_1() {
	// Timer 1 finished -> Interrupt

	automatic = 0; // GLOBAL
	if (SW1 == 1) { //SW1 on?
		automatic = 0; // automatic light
	} else {
		automatic = 1; // manual light
	}

	// EVERY LOOP:

	if (timerCounter == 0) {
		// every 400 ms
		sendLights();
	}
	if ((timerCounter == 0) || (timerCounter == 2)) {
		// every 200 ms
		sendECU_1_alive();
		sendECU_1_error();
	}
	// every 100 ms
	sendRPM();

	if (timerCounter == 3) {
		timerCounter = 0;
	} else {
		timerCounter++;
	}

	ECU_2_iteration++;

	PIT.CHANNEL[1].TFLG.R = 1;
}

void Pit_Channel_2() {
	PIT.CHANNEL[2].TFLG.R = 1;
}

void Pit_Channel_3() {
	PIT.CHANNEL[3].TFLG.R = 1;
}

/*************************************************************************
 *                    	DONT CHANGE ANYTHING BELOW 	!!!                  *
 *                                                                        *
 **************************************************************************/

/*************************************************************************
 *                   Interrupt Handlers for CAN Message Buffer            *
 *                 Receive Interrupt for buffer 08-15 flag cleared        *
 **************************************************************************/
IRQ_HANDLER(SPC5_FLEXCAN0_BUF_08_11_HANDLER) {
	CAN_0.IFRL.B.BUF8I = 1;
	CAN_0.IFRL.B.BUF9I = 1;
	CAN_0.IFRL.B.BUF10I = 1;
	CAN_0.IFRL.B.BUF11I = 1;
}
IRQ_HANDLER(SPC5_FLEXCAN0_BUF_12_15_HANDLER) {
	CAN_0.IFRL.B.BUF12I = 1;
	CAN_0.IFRL.B.BUF13I = 1;
	CAN_0.IFRL.B.BUF14I = 1;
	CAN_0.IFRL.B.BUF15I = 1;
}

/*************************************************************************
 *                  Function to receive the data of the display           *
 **************************************************************************/
void updateInputs() {
	SW1 = CAN_0.BUF[0].DATA.B[0];
	SW2 = CAN_0.BUF[0].DATA.B[1];
	SW3 = CAN_0.BUF[0].DATA.B[2];
	SW4 = CAN_0.BUF[0].DATA.B[3];
	BT5 = CAN_0.BUF[0].DATA.B[4];
	BT6 = CAN_0.BUF[0].DATA.B[5];

}

