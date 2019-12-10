main.c  // ecu 3


#include "me_init.h"

/********************************************************************
 *             Global variables and function declarations           *
 ********************************************************************/




void sendAliveSignal(int, int);
void sendErrorSignal(int, int);
void toggle_LED_P(void);
void sendMessage(int, int,int, int);

void sendTemperature(void);
void sendHeadlights(int highBeam, int lowBeam, int parkingLight);
void sendSpeed(void);
void sendECU_3_alive(void);
void sendECU_3_error(void);

// ECU 3 received data
int rpm = 0;
int gearRatio = 0;
int coolanttemperature = 0;
// global variables
int timerCounter = 0;
int ECU_0_iteration = 0; // ECU3


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

	coolanttemperature = POT ;
   /* Operating System Delay*/
   osalThreadDelayMilliseconds(10UL);
 }
}



/********************************************************************
 *                   Can Reception Function                         *
 ********************************************************************/
void can_receive() {

	int length = 0;
	int data = 0x00 ;
	int byte0 = 0x00;
	int byte1 = 0x00;
	if(CAN_0.IFRL.B.BUF5I == 1)
	{
		//int can_id = CAN_0.BUF[0].ID.B.STD_ID;
		switch(CAN_0.BUF[0].ID.B.STD_ID)
		{
			case 0x101:
				//gear value
	            length = CAN_0.BUF[0].CS.B.LENGTH;
	            if (1){
	                data = CAN_0.BUF[0].DATA.B[0];

	                gearRatio = 0; // GLOBAL

	                if (data == 1) {gearRatio = 7842;} // Reverse
	                if (data == 2) {gearRatio = 0;} // Neutral
	                if (data == 4) {gearRatio = 7842;} // 1
	                if (data == 8) {gearRatio = 13112;} // 2
	                if (data == 16) {gearRatio = 19861;} // 3
	                if (data == 32) {gearRatio = 27038;} // 4
	                if (data == 64) {gearRatio = 33149;} // 5
	                if (data == 128) {gearRatio = 40035;} // 6

	            }
	            break;
			case 0x202:
				//rpm
	            length = CAN_0.BUF[0].CS.B.LENGTH;
	            if (length == 1){
					byte0 = CAN_0.BUF[0].DATA.B[0];

					rpm = byte0; // GLOBAL

				}
	            if (length >= 1){
	                byte0 = CAN_0.BUF[0].DATA.B[0];
	                byte1 = CAN_0.BUF[0].DATA.B[1] << 8;

	                rpm = byte0 + byte1; // GLOBAL

	            }
	            break;
			case 0x105:
	            // ECU 0 alive

	            ECU_0_iteration = 0;
	            break;
			case 0x84:
				updateInputs();
				break;

			default :
				//CAN_0.BUF[0].ID.B.STD_ID;
				break;
		}
	}
	//CAN_0.IFRL.B.BUF5I = 1;
}




void sendAliveSignal(int msg_id, int msg_buffer)
{
    //CAN_0.BUF[msg_buffer].DATA.B[0] = 0x00;
    CAN_0.BUF[msg_buffer].CS.B.LENGTH = 0x01; // 1 byte
    CAN_0.BUF[msg_buffer].DATA.B[0] = 0x00;
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





void sendTemperature()
{ // 200ms
    // send coolant temperature



    sendMessage(coolanttemperature, 2 /* length */, 0x401 /* msg id */, 8 /* msg buffer */);
}

void sendHeadlights(int highBeam, int lowBeam, int parkingLight)
{ // 200ms
    // send headlights

    int data = ((1 * highBeam) + (2 * lowBeam) + (4 * parkingLight));

    sendMessage(data, 1 /* length */, 0x403 /* msg id */, 9 /* msg buffer */);
}

void sendSpeed(){ // 100ms
    // send calculated speed (REQUIRES RECEIVED DATA)

    int speed = (gearRatio * rpm) / 100000;

    sendMessage(speed, 2 /* length */, 0x402 /* msg id */, 10 /* msg buffer */);
}

void sendECU_3_alive(){ // 200ms
    // send ECU 3 alive

    sendAliveSignal(0x404, 11);
}

void sendECU_3_error(){ // 200ms
    // send ECU 3 error

    if (ECU_0_iteration > 8){
        sendErrorSignal(0x405, 12);
        toggle_LED_P();
    }

}






/********************************************************************
 *                   Interrupt Handlers for PIT Channel 1-3         *
 ********************************************************************/
void Pit_Channel_1()
{
	  // Timer 1 finished -> Interrupt

	    // EVERY LOOP:


	    if ((timerCounter == 0) || (timerCounter == 2)){
	        // every 200 ms
	        sendTemperature();

	        int lowBeam = 1;
	        int highBeam = 1;
	        int parkingLight = 1;

	        if (SW1 == 1){ //SW1 on?
	            // low beam (Abblendlicht)
	            lowBeam = 0;
	        }
	        if (SW2 == 1){ //SW2 on?
	            // high beam (Fernlicht)
	            highBeam = 0;
	        }
	        if (SW3 == 1){ //SW3 on?
	            // hazard light
	            parkingLight = 0;
	        }

	        sendHeadlights(highBeam, lowBeam, parkingLight);

	        sendECU_3_alive();
	        sendECU_3_error();
	    }
	    // every 100 ms
	    sendSpeed();

	    if (timerCounter == 3){
	        timerCounter = 0;
	    }else{
	        timerCounter++;
	    }

	    ECU_0_iteration++;


	    // Clear Interrupt Flag
	    //PIT.CHANNEL[1].TFLG.B.TIF = 1;
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






