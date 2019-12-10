main.c // ecu 2

#include "me_init.h"

/********************************************************************
 *             Global variables and function declarations           *
 ********************************************************************/

void sendAliveSignal();
void sendErrorSignal();
void toggle_LED_P();
void sendMessage();

void sendFuelLevel();
void sendIndicators(int left, int right, int hazard);
void sendDoorsWarning();
void sendECU_2_alive();
void sendECU_2_error();

// ECU 2 received data
int carSpeed = 0;
int leftDoorOpen = 0;
int rightDoorOpen = 0;

// global variables
int timerCounter = 0;
int ECU_3_iteration = 0; // ECU2

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
  /* Timer configuration */
   PIT_ConfigureTimer(1, 100); // channel 1
   PIT_StartTimer(1);

  /* Application main loop that runs forever*/
for ( ; ; ) {

	can_receive();
   /* Operating System Delay*/
   osalThreadDelayMilliseconds(250UL);
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
			case 0x83:
			updateInputs();
			break;

			case 0x402:
				length = CAN_0.BUF[0].CS.B.LENGTH;
				if (length == 1){
					int byte0 = CAN_0.BUF[0].DATA.B[0];
					carSpeed = byte0; // GLOBAL
				}
				if (length >= 1){
					int byte0 = CAN_0.BUF[0].DATA.B[0];
					int byte1 = CAN_0.BUF[0].DATA.B[1] << 8;
					carSpeed = byte0 + byte1; // GLOBAL
				}
				break;

			case 0x102:
				if (1){
					char data = CAN_0.BUF[0].DATA.B[0];
					leftDoorOpen = (data >> 0) & 0x01; // GLOBAL
					rightDoorOpen = (data >> 1) & 0x01; // GLOBAL
				}
				break;

			case 0x404:
				ECU_3_iteration = 0;
				break;

			default :
				CAN_0.BUF[0].ID.B.STD_ID;
				break;
		}
	//CAN_0.IFRL.B.BUF5I = 1;
}

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
	    CAN_0.BUF[msg_buffer].CS.B.CODE = 0b1100; //? Transmit data frame unconditionally once. After transmission,
												//	the MB automatically returns to the INACTIVE state.
	}




	void sendFuelLevel(){ // 200ms
	    // send fuel level
	    int fuelLevel = POT;
	    sendMessage(fuelLevel, 2 /* length */, 0x301 /* msg id */, 8 /* msg buffer */);
	}


	void sendIndicators(int left, int right, int hazard){ // 200ms
	    // send indicators
	    int data = ((1 * left) + (2 * right) + (4 * hazard));
	    sendMessage(data, 1 /* length */, 0x302 /* msg id */, 9 /* msg buffer */);
	}

	void sendDoorsWarning(){ // 400ms
	    // send doors state and warning (REQUIRES RECEIVED DATA)
	    int warning = 0;
	    if (((!leftDoorOpen) || (!rightDoorOpen)) && (carSpeed > 5)){
	        warning = 1; //sendWarning(0x303);
	    }

	    int data = ((1 * warning) + (2 * leftDoorOpen) + (4 * rightDoorOpen));
	    sendMessage(data, 1 /* length */, 0x303 /* msg id */, 10 /* msg buffer */);
	}

	void sendECU_2_alive(){ // 200ms
	    // send ECU 2 alive
	    sendAliveSignal(0x304, 11); //our alive signal
	}

	void sendECU_2_error(){ // 200ms
	    // send ECU 2 error
	    if (ECU_3_iteration > 8){
	        sendErrorSignal(0x305, 12);//our error signal
	        toggle_LED_P();
	    }
	}



/********************************************************************
 *                   Interrupt Handlers for PIT Channel 1-3         *
 ********************************************************************/
void Pit_Channel_1()
{
	 // Timer 1 finished -> Interrupt // EVERY LOOP:
	        if (timerCounter == 0){
	        // every 400 ms
	        sendDoorsWarning();
	    }
	    if ((timerCounter == 0) || (timerCounter == 2)){
	        // every 200 ms
	        sendFuelLevel();
	        sendECU_2_alive();
	        sendECU_2_error();

	        if (SW1 == 1){ //SW1 on?
	            // left indicator
	            sendIndicators(0, 1, 1); // 6
	        }else if (SW2 == 1){ //SW2 on?
	            // right indicator
	            sendIndicators(1, 0, 1); // 5
	        }else if (SW3 == 1){ //SW3 on?
	            // hazard light
	            sendIndicators(1, 1, 0); // 3
	        }else{
	        	sendIndicators(1, 1, 1); // 7
	        }
	    }
	    // every 100 ms
	    if (timerCounter == 3){
	        timerCounter = 0;
	    }else{
	        timerCounter++;
	    }
	    ECU_3_iteration++;
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






