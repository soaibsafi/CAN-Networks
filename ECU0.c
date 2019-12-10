main.c // ecu 0

#include "me_init.h"


void sendAliveSignal();
void sendErrorSignal();
void toggle_LED_P();
void sendMessage();

void sendGear();
void sendDoor();
void sendFuelLevelWarning();
void sendEngineCoolingWarning();
void sendECU_0_alive();
void sendECU_0_error();

// ECU 0 received data
int coolantTemperature = 0;
int fuelLevel = 0;

// global variables
int timerCounter = 0;
int ECU_1_iteration = 0; // ECU0

int btn5pressed = 0;
int btn6pressed = 0;
int gear = 1; // 0=Reverse, 1=Neutral, 2=1th gear, 3=2nd gear, 4=3rd gear, 5=4th gear, 6=5th gear, 7=6th gear
int leftDoor = 1; // closed
int rightDoor = 1; // closed

/********************************************************************
 *             Global variables and function declarations           *
 ********************************************************************/



/* Switches and buttons variable to be used to receive signals from board */
int SW1 = 0;
int SW2 = 0;
int SW3 = 0;
int SW4= 0;
int BT5= 0;
int BT6= 0;





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
for ( ; ; ) {

	can_receive();
	P = 1;
   /* Operating System Delay*/
   osalThreadDelayMilliseconds(10UL);
 }
}



/********************************************************************
 *                   Can Reception Function                         *
 ********************************************************************/
void can_receive() {
	if(CAN_0.IFRL.B.BUF5I == 1)
	{
		int length = 0;
		switch(CAN_0.BUF[0].ID.B.STD_ID)
		{

		case 0x81:
			updateInputs();
			break;

		case 0x301:
			// fuel level

			length = CAN_0.BUF[0].CS.B.LENGTH;
			if (length == 1){
				int byte0 = CAN_0.BUF[0].DATA.B[0];

				fuelLevel = byte0; // GLOBAL
			}
			if (length >= 1){
				int byte0 = CAN_0.BUF[0].DATA.B[0];
				int byte1 = CAN_0.BUF[0].DATA.B[1] << 8;

				fuelLevel = byte0 + byte1; // GLOBAL
			}

			break;

		case 0x401:
			// coolant temperature

			length = CAN_0.BUF[0].CS.B.LENGTH;
			if (1){
				int byte0 = CAN_0.BUF[0].DATA.B[0];
				int byte1 = CAN_0.BUF[0].DATA.B[1] << 8;

				coolantTemperature = byte0 + byte1; // GLOBAL
			}

			break;

		case 0x203:
			ECU_1_iteration = 0;
			break;

		default :
			CAN_0.BUF[0].ID.B.STD_ID;
			break;
		}

	}
	//CAN_0.IFRL.B.BUF5I = 1;
}




void sendAliveSignal(int msg_id, int msg_buffer){
    //CAN_0.BUF[msg_buffer].DATA.B[0] = 0x00;
    CAN_0.BUF[msg_buffer].CS.B.LENGTH = 0; // 1 byte

    CAN_0.BUF[msg_buffer].MSG_ID.B.STD_ID = msg_id;
    CAN_0.BUF[msg_buffer].CS.B.CODE = 0b1100; //? Transmit data frame unconditionally once. After transmission, the MB automatically returns to the INACTIVE state.
}

void sendErrorSignal(int msg_id, int msg_buffer){
    //CAN_0.BUF[msg_buffer].DATA.B[0] = 0x00;
    CAN_0.BUF[msg_buffer].CS.B.LENGTH = 0; // 1 byte

    CAN_0.BUF[msg_buffer].MSG_ID.B.STD_ID = msg_id;
    CAN_0.BUF[msg_buffer].CS.B.CODE = 0b1100; //? Transmit data frame unconditionally once. After transmission, the MB automatically returns to the INACTIVE state.
}

void toggle_LED_P(){
    // toggle LED P
    P = ~P;
}

void sendMessage(int data, int length, int msg_id, int msg_buffer){
    if (length == 2){
        // 2 bytes

        char firstBits = (data >> 8) & 0xFF;
        char lastBits = data & 0xFF;

        CAN_0.BUF[msg_buffer].DATA.B[0] = lastBits;
        CAN_0.BUF[msg_buffer].DATA.B[1] = firstBits;
        CAN_0.BUF[msg_buffer].CS.B.LENGTH = 2; // 2 byte
    }else{
        // 1 byte

        char bits = data & 0xFF;

        CAN_0.BUF[msg_buffer].DATA.B[0] = bits;
        CAN_0.BUF[msg_buffer].CS.B.LENGTH = 1; // 1 byte
    }

    CAN_0.BUF[msg_buffer].MSG_ID.B.STD_ID = msg_id;
    CAN_0.BUF[msg_buffer].CS.B.CODE = 0b1100; //? Transmit data frame unconditionally once. After transmission, the MB automatically returns to the INACTIVE state.
}





void sendGear(){ // 200ms
    // send gear

    int data = 2;

    if (gear == 0) {data = 1; }
    if (gear == 1) {data = 2; }
    if (gear == 2) {data = 4; }
    if (gear == 3) {data = 8; }
    if (gear == 4) {data = 16; }
    if (gear == 5) {data = 32; }
    if (gear == 6) {data = 64; }
    if (gear == 7) {data = 128; }

    // int data = 1 << gear;

    // send message
    sendMessage(data, 1 /* length */, 0x101 /* msg id */, 8 /* msg buffer */);
}


void sendDoor(){ // 200ms
    // send door

    int data = ((1 * leftDoor) + (2 * rightDoor));

    sendMessage(data, 1 /* length */, 0x102 /* msg id */, 9 /* msg buffer */);
}


void sendFuelLevelWarning(){ // 400ms
    // send fuel level warning (REQUIRES RECEIVED DATA)

    int fuelLevelWarning = 1; // off
    if (fuelLevel < 102){ // low fuel
        fuelLevelWarning = 0; // on
    }

    sendMessage(fuelLevelWarning, 1 /* length */, 0x103 /* msg id */, 12 /* msg buffer */);
}

void sendEngineCoolingWarning(){ // 400ms
    // send engine coolant temperature warning (REQUIRES RECEIVED DATA)

    int engineCoolingWarning = 1; // off
    if (coolantTemperature > 921){ // engine overheat
        engineCoolingWarning = 0; // on
    }

    sendMessage(engineCoolingWarning, 1 /* length */, 0x104 /* msg id */, 13 /* msg buffer */);
}

void sendECU_0_alive(){ // 200ms
    // send ECU 0 alive

    sendAliveSignal(0x105, 10);
}

void sendECU_0_error(){ // 200ms
    // send ECU 0 error

    if (ECU_1_iteration > 8){
        sendErrorSignal(0x106, 11);
        toggle_LED_P();
    }
}





/********************************************************************
 *                   Interrupt Handlers for PIT Channel 1-3         *
 ********************************************************************/
void Pit_Channel_1()
{
	// Timer 1 finished -> Interrupt


	if (BT5 == 1){ // BT5 pressed?
		btn5pressed = 1;
	}else{
		if (btn5pressed){ // BT5 released
			// shift up
			if (gear != 7){
				gear++;
			}

			btn5pressed = 0;
		}
	}

	if (BT6 == 1){ // BT6 pressed?
		btn6pressed = 1;
	}else{
		if (btn6pressed){ // BT6 released
			// shift down
			if (gear != 0){
				gear--;
			}

			btn6pressed = 0;
		}
	}

	if (SW1 == 1){ //SW1 on?
		leftDoor = 0;
		U1 = ~U1;
	}else{
		leftDoor = 1;
	}

	if (SW2 == 1){ //SW2 on?
		rightDoor = 0;
	}else{
		rightDoor = 1;
	}


	// EVERY LOOP:

	if (timerCounter == 0){
		// every 400 ms

		sendFuelLevelWarning();
		sendEngineCoolingWarning();
	}
	if ((timerCounter == 0) || (timerCounter == 2)){
		// every 200 ms

		sendGear();
		sendDoor();
		sendECU_0_alive();
		sendECU_0_error();

	}
	// every 100 ms


	if (timerCounter == 3){
		timerCounter = 0;
	}else{
		timerCounter++;
	}

	ECU_1_iteration++;



	PIT.CHANNEL[1].TFLG.R = 1;
}

void Pit_Channel_2()
{
PIT.CHANNEL[2].TFLG.R = 1;
}

void Pit_Channel_3()
{
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
	CAN_0.IFRL.B.BUF8I  = 1;
	CAN_0.IFRL.B.BUF9I  = 1;
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
void updateInputs()
{
	SW1 = CAN_0.BUF[0].DATA.B[0];
	SW2 = CAN_0.BUF[0].DATA.B[1];
	SW3 = CAN_0.BUF[0].DATA.B[2];
	SW4 = CAN_0.BUF[0].DATA.B[3];
	BT5 = CAN_0.BUF[0].DATA.B[4];
	BT6 = CAN_0.BUF[0].DATA.B[5];

}






