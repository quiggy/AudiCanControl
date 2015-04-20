#include "AudiCanControl.h"

// FIS Telefonmodul zu FIS
// ------------------------------------------
// 667;8;42;54;1C;1C;1C;1C;1C;1C	BT
// 66B;8;41;4B;54;49;56;1C;1C;1C	AKTIV

// FIS TVModul zu FIS
// 0x363	Zeile1
// 0x365	Zeile2


int actPosition = 0;
int RadioMode = 0;
int ignitionStatus = 1;
long kmStand = 0;
String FgNummer_temp = "";
String FgNummer_comp = "";

char completeFisLine1[] = " iPhone ";
char completeFisLine2[] = "RNS-E BT Dies ist ein TEst";
char actualFisLine2[8] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

AudiCanControl::AudiCanControl()
{
}

void AudiCanControl::setup() {  
  // Setup the intervals...
  IdleInterval = 300;
  FISInterval = 10;
  ScrollInterval = 1000;
  BTConnectInterval = 4000;
  StatusInformationInterval = 5000;
  
  Fis2Position = 0;

  // Setup the digital outputs
  led2  =  7;
  led3  =  8;
  led13 = 13;
  
  // Setup the Statusleds
  ledPower = 11;
  ledActive = 9;
  
  // Setup cmd pin for RN-52
  cmdPin = 12; 
  
  // Setup the LED and Control Lines
  pinMode(cmdPin, OUTPUT);      // sets the digital pin as output 
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led13, OUTPUT);
  pinMode(ledPower, OUTPUT);
  pinMode(ledActive, OUTPUT);
  
  digitalWrite(ledPower, HIGH); // sets the LED on
  digitalWrite(ledActive, LOW);
  digitalWrite(cmdPin, HIGH);   // Turns CMD-Mode of RN-52 off
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);
  digitalWrite(led13, HIGH);
 
  Serial1.begin(115200); 
 
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // Initialize Can Controller
  if(can.initCAN(CAN_BAUD_100K) == 0)
  {
    Serial.println("initCAN() failed");
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);
    digitalWrite(led13, LOW);
    while(1);
  }

  if(can.setCANNormalMode(LOW) == 0) //normal mode non single shot
  {
    Serial.println("setCANNormalMode() failed");
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);
    digitalWrite(led13, LOW);
    while(1);
  }
  
  digitalWrite(cmdPin, LOW);
  Serial.println("Can-Control successfully initialized!");
  //sendFisTEST();
}

void AudiCanControl::callback_scrollFis(){

}

void AudiCanControl::processData(){
    printMessage();
    
    // Verarbeite CAN-ID Statusmeldung (KM-Stand)
    if (msg.adrsValue == 0x65D) {
	  byte d1 = msg.data[1]; 
      byte d2 = msg.data[2]; 
      byte d3 = msg.data[3];
      d3 = d3 & 0x0F;
	  
      byte in[3] = {d3, d2, d1};
	  
      long value = 0;
      for (int i = 0; i < sizeof(in); i++)
      {
         value = (value << 8) + (in[i] & 0xff);
      }	  
	  
	  kmStand = value;
    }
	
	// Verarbeite CAN-ID Statusmeldung (Fahrgestellnummer)
    if (msg.adrsValue == 0x65F) {
      switch (msg.data[0]) {
          case 0x00:
            // Part 1
            FgNummer_temp = String((char)msg.data[5]) + String((char)msg.data[6]) + String((char)msg.data[7]);
            break;
          case 0x01:
            // Part 2
            FgNummer_temp = FgNummer_temp + String((char)msg.data[1]) + String((char)msg.data[2]) + String((char)msg.data[3]) + String((char)msg.data[4]) + String((char)msg.data[5]) +String((char)msg.data[6]) + String((char)msg.data[7]);
            break;
		   case 0x02:
            // Part 3 - FGN Komplett
            FgNummer_temp = FgNummer_temp + String((char)msg.data[1]) + String((char)msg.data[2]) + String((char)msg.data[3]) + String((char)msg.data[4]) + String((char)msg.data[5]) +String((char)msg.data[6]) + String((char)msg.data[7]);
			FgNummer_comp = FgNummer_temp;
            break;
      }
    }

    // Verarbeite CAN-ID Statusmeldung (Modus)
    if (msg.adrsValue == 0x661) {
   
      // Datenbits für Status werden Überprüft (Modus)
      if ((msg.data[0] == 0xA3) && (msg.data[1]==0x01) && (msg.data[2]==0x12)){
        
        // Modus wird ausgelesen
         switch (msg.data[3]) {
          case 0xA4:
            // Modus: MP3
            if (RadioMode != 1){RadioMode = 1;}
            break;
          case 0xA5:
            // Modus: CD
            if (RadioMode != 2){RadioMode = 2;}
            break;
          }
      }
      
       // Datenbits für Status werden Überprüft (Modus)
      if ((msg.data[0] == 0x83) && (msg.data[1]==0x01) && (msg.data[2]==0x12)){
        
        // Modus wird ausgelesen
         switch (msg.data[3]) {
          case 0xA0:
            // Modus: AM/FM
            if (RadioMode != 3){RadioMode = 3;}
            break;
          case 0xA4:
            // Modus: AUX
            if (RadioMode != 4){RadioMode = 4;}
            break;
          case 0x2F:
            // Modus: SAT
            if (RadioMode != 5){RadioMode = 5;}
            break;
          case 0x37:
            // Modus: TV
            if (RadioMode != 6){RadioMode = 6;}
            break;
          }
      }
    }
	
	// Verarbeite Tastendrücke im TV Modus
    if (msg.adrsValue == 0x461) {
        // Datenbits für Status werden Überprüft (Modus)
		// 461;6;37;30;1;2;0;0 Taste: Zurück _Down
		// 461;6;37;30;1;1;0;0 Taste: Vor _Down
		
		// Datenbits werden überprüft
		if ((msg.data[0] == 0x37) && (msg.data[1]==0x30) && (msg.data[2]==0x1)){
			switch (msg.data[3]) {
				case 0x2:
					// Taste: Zurück _Down
					//digitalWrite(cmdPin, LOW);   // Turns CMD-Mode of RN-52 on.
					//delay(3);
					Serial1.println("AT+");		 // Tells RN-52 so send next track to BT-Device.
					//delay(3);
					//digitalWrite(cmdPin, HIGH);	 // Turns CMD-Mode of RN-52 off.
					CarO();
					break;
				case 0x1:
					// Taste: Vor _Down
					//digitalWrite(cmdPin, LOW);   // Turns CMD-Mode of RN-52 on.
					//delay(3);
					Serial1.println("AT-");		 // Tells RN-52 so send next track to BT-Device.
					//delay(3);
					//digitalWrite(cmdPin, HIGH);	 // Turns CMD-Mode of RN-52 off.
					
					break;
			}
		}
    }
	
	// Verarbeite CAN-ID Statusmeldung (Zündung)
    if (msg.adrsValue == 0x2c3) {
		// Status wird ausgelesen
         switch (msg.data[0]) {
          case 0x10:
            // Status: Kein Schlüssel
            if (ignitionStatus != 1){ignitionStatus = 1;}
            break;
          case 0x11:
            // Status: Schlüssel steckt
            if (ignitionStatus != 2){ignitionStatus = 2;}
            break;
		  case 0x01:
            // Status: Lenkradschloss entriegelt
            if (ignitionStatus != 3){ignitionStatus = 3;}
            break;
		  case 0x05:
            // Status: Display an
            if (ignitionStatus != 4){ignitionStatus = 4;}
            break;
		  case 0x07:
            // Status: Zündung ein
            if (ignitionStatus != 5){ignitionStatus = 5;}
            break;
		  case 0x0B:
            // Status: Zündung ein und Anlasser
            if (ignitionStatus != 6){ignitionStatus = 6;}
            break;
          }
		
	}
  }
  
void AudiCanControl::loop() {
		digitalWrite(ledActive, LOW);
		
		
		//ignitionStatus = 4;
		// Can Messages nur senden wenn Zündung an
		if (ignitionStatus >= 4) {
			// Turns StatusLed On.
			digitalWrite(ledActive, HIGH);
			
			// Setting up Timer Events
			// TV Freischaltung alle 500ms
		    if ((unsigned long)(millis() - waitUntilIdle) >= IdleInterval) {
				waitUntilIdle = millis(); // Increase our wait for another 500ms.
				enableTvMode();		
			}
			
			// BT Connection Status alle 2000ms
		    if ((unsigned long)(millis() - waitUntilBTConnect) >= BTConnectInterval) {
				waitUntilBTConnect = millis(); // Increase our wait for another 2000ms.
				Serial.println("Getting BT Connectionstatus:");
				Serial1.println("Q");
			}
			
			// Scroll für das FIS alle 2000ms
			if ((unsigned long)(millis() - waitUntilFISScroll) >= ScrollInterval) {
				waitUntilFISScroll = millis(); // Increase our wait for another 500ms.
				
				int length = strlen(completeFisLine2);
				
				for (int i = 0; i <= 7; i++){
					actualFisLine2[i] = ' ';
				}
				
				if (length <=8) {
					for (int i = 0; i <= 7; i++){
					actualFisLine2[i] = completeFisLine2[i];
					}
				}
				
				if (length > 8) {
					if (Fis2Position <= length - 9) {
						for (int i = 0; i <= 7; i++){
							actualFisLine2[i] = completeFisLine2[i+Fis2Position];
						}
						Fis2Position = Fis2Position+1;
					} else {
						for (int i = 0; i <= 7; i++){
							actualFisLine2[i] = completeFisLine2[i+Fis2Position];
						}
						Fis2Position = 0;
					}
				}
			}			
			
			// Show Information on FIS only in TV Mode
			if (RadioMode == 6) {
				// Refresh FIS every 500ms
				if ((unsigned long)(millis() - waitUntilFISIdle) >= FISInterval) {
					waitUntilFISIdle = millis(); // Increase our wait for another 500ms.
					
					printFISln1(completeFisLine1);
					printFISln2(actualFisLine2);
								
				}
			}
			
			// Statusinformationen an PC alle 5 Sekunden
		    if ((unsigned long)(millis() - waitUntilStatusInformation) >= StatusInformationInterval) {
				waitUntilStatusInformation = millis(); // Increase our wait for another 500ms.
				Serial.print("Radio Mode: ");	
				Serial.print(RadioMode);
				Serial.print(" Ignition Status: ");	
				Serial.print(ignitionStatus);
				Serial.print(" KM: ");	
				Serial.print(kmStand);	
				Serial.print(" FGN: ");	
				Serial.print(FgNummer_comp);
				Serial.println("!");
			}
		}
		 
        // Daten Empfangen von CAN
		int i = can.receiveCANMessage(&msg, 1000);
		if(i)
		{
			processData();
		}
		
		// Daten empfangen von Bluetooth (RN-52)
		readString = "";
  
		while (Serial1.available()) {  
			//delay(3);  //delay to allow buffer to fill 
			if (Serial1.available() >0) {
			  char c = Serial1.read();  //gets one byte from serial buffer
			  readString += c; //makes the string readString
			} 
		}
			
		if (readString.length() >0) {
				
			if (sizeof(readString)==7) {
					if (readString == "0001\r\n"){
					Serial.println("Try to reconnect!");
					Serial1.println("B,04");
					}
			} else {
				Serial.println(readString);
			}
			
		}
		
		while (Serial.available()) {  
			//delay(3);  //delay to allow buffer to fill 
			if (Serial.available() >0) {
			  char c = Serial.read();  //gets one byte from serial buffer
			  readString += c; //makes the string readString
			
			  if (readString == "C") {
					Serial.println("Sending RN-52 to CMD-Mode:");
					digitalWrite(cmdPin, LOW);   // Turns CMD-Mode of RN-52 on.
					//Serial1.println("Q");
			  }
			  if (readString == "B") {
					Serial.println("Sending B to RN-52:");
					Serial1.println("B");
			  }
			  if (readString == "Q") {
					Serial.println("Sending Q to RN-52:");
					Serial1.println("Q");
			  }
			} 
		}
  }
  
 // Returns the actual HeadUnitMode (1 = MP3, 2 = CD, 3 = AM/FM, 4 = AUX, 5 = SAT, 6=TV)
 int AudiCanControl::getHeadUnitMode() {
			return RadioMode;
  }
  
  // Prints the last message the Class received for debugging purposes...
void  AudiCanControl::printMessage(){	
	// Send CanMessage to the Computer
		
		Serial.print(msg.adrsValue, HEX);
		Serial.print(";");
		Serial.print(msg.dataLength, HEX);
		Serial.print(";");
		
        // Send Data
         for (int i = 0; i <= msg.dataLength-1; i++){
             
			 if (i ==  msg.dataLength-1) {
				 Serial.println(msg.data[i], HEX);
			 }
			 else 
			 {
				 Serial.print(msg.data[i], HEX);
				 Serial.print(";");
			 }
         }     	
}

// Sends Command to enable TV-Mode to the HeadUnit
void  AudiCanControl::enableTvMode(){
	msg.adrsValue = 0x602;
	msg.isExtendedAdrs = false;
	msg.rtr = false;
	msg.dataLength = 8;
	msg.data[0] = 0x81;
	msg.data[1] = 0x12;
	msg.data[2] = 0x30;
	msg.data[3] = 0x3A;
	msg.data[4] = 0x20;
	msg.data[5] = 0x41;
	msg.data[6] = 0x46;
	msg.data[7] = 0x20;
	can.transmitCANMessage(msg, 1000);
}

// Sends text to Fis Line 1
void  AudiCanControl::sendFisLine1(byte d0, byte d1, byte d2, byte d3, byte d4, byte d5, byte d6, byte d7){
	msg.adrsValue = 0x363;
	//msg.adrsValue = 0x66B;
	msg.isExtendedAdrs = false;
	msg.rtr = false;
	msg.dataLength = 8;
	msg.data[0] = d0;
	msg.data[1] = d1;
	msg.data[2] = d2;
	msg.data[3] = d3;
	msg.data[4] = d4;
	msg.data[5] = d5;
	msg.data[6] = d6;
	msg.data[7] = d7;
	can.transmitCANMessage(msg, 1000);
}

// Sends text to Fis Line 2
void  AudiCanControl::sendFisLine2(byte d0, byte d1, byte d2, byte d3, byte d4, byte d5, byte d6, byte d7){
	msg.adrsValue = 0x365;
	//msg.adrsValue = 0x667;
	msg.isExtendedAdrs = false;
	msg.rtr = false;
	msg.dataLength = 8;
	msg.data[0] = d0;
	msg.data[1] = d1;
	msg.data[2] = d2;
	msg.data[3] = d3;
	msg.data[4] = d4;
	msg.data[5] = d5;
	msg.data[6] = d6;
	msg.data[7] = d7;
	can.transmitCANMessage(msg, 1000);
}

// DO NOT USE ITS NOT WORKING!!!
void  AudiCanControl::sendFisTEST(){
	/*
	6C0	8	2A	57	13	26	0	29	53	43				SC
	6C0	8	2B	48	57	41	4E	48	49	4C				Zähler,HWANHIL
	6C0	8	2C	44	45	4E	48	60	48	45				Zähler,DENHÖHE
	6C0	8	2D	52	5	2	0	1B	36	28
	6C0	1	A3							
	6C0	8	2E	57	4	B	24	17	3A	57
	6C0	8	2F	4	B	24	1E	3A	57	4
	6C0	5	10	B	16	9	11			
	6C0	8	21	57	4	B	22	9	13	57
	6C0	8	23	B	1C	10	1F	57	4	B
	6C0	8	24	10	10	1D	57	4	B	16
	6C0	8	25	10	1E	57	4	B	22	10
	6C0	8	26	20	57	4	B	28	10	21
	6C0	7	17	57	4	B	E	1E	61	*/
	
	byte data1[] = {0x2A,	0x57, 0x13,	0x26, 0x0,  0x29, 0x53, 0x43};
	sendFisMiddle(8, data1);
	
	byte data2[] = {0x2B,	0x48, 0x57,	0x57, 0x57,	0x57, 0x57,	0x57};
	sendFisMiddle(8, data2);
	
	byte data3[] = {0x2C,	0x44, 0x45,	0x4E, 0x48,	0x60, 0x48,	0x45};
	sendFisMiddle(8, data3);
	
	byte data4[] = {0x2D,	0x52, 0x5,	0x2,  0x0,	0x1B, 0x36,	0x28};
	sendFisMiddle(8, data4);
	
	byte data5[] = {0xA3};
	sendFisMiddle(1, data5);
	
	byte data6[] = {0x2E,	0x57,	0x4,	0xB,	0x24,	0x17,	0x3A,	0x57};
	sendFisMiddle(1, data6);
	
	byte data7[] = {0x2F,	0x4,	0xB,	0x24,	0x1E,	0x3A,	0x57,	0x4};
	sendFisMiddle(1, data7);
	
	byte data8[] = {0x10,	0xB,	0x16,	0x9,	0x11};
	sendFisMiddle(1, data8);
	
	byte data9[] = {0x21,	0x57,	0x4,	0xB,	0x22,	0x9,	0x13,	0x57};
	sendFisMiddle(1, data9);
	
	byte data10[] = {0x23,	0xB,	0x1C,	0x10,	0x1F,	0x57,	0x4,	0xB};
	sendFisMiddle(1, data10);
	
	byte data11[] = {0x24,	0x10,	0x10,	0x1D,	0x57,	0x4,	0xB,	0x16};
	sendFisMiddle(1, data11);
	
	byte data12[] = {0x25,	0x10,	0x1E,	0x57,	0x4,	0xB,	0x22,	0x10};
	sendFisMiddle(1, data12);
	
	byte data13[] = {0x26,	0x20,	0x57,	0x4,	0xB,	0x28,	0x10,	0x21};
	sendFisMiddle(1, data13);
	
	byte data14[] = {0x17,	0x57,	0x4,	0xB,	0xE,	0x1E,	0x61};
	sendFisMiddle(1, data14);
}

void  AudiCanControl::sendFisMiddle(int length, byte pData[]){
	
	msg.adrsValue = 0x6C0;
	msg.isExtendedAdrs = false;
	msg.rtr = false;
	msg.dataLength = length;
	
	for (int i = 0; i <= length-1; i++){
		msg.data[i] = pData[i];
	}
	
	can.transmitCANMessage(msg, 1000);
}

void AudiCanControl::printFISln1(char pData[]) {
  int length = strlen(pData);
  byte buffer[8];
  
  for (int i=0;i<=length-1;i++) {
    buffer[i] = convertCharToByte(pData[i]); 
  }
     
	byte d1 = buffer[0];
    byte d2 = buffer[1];
    byte d3 = buffer[2];
    byte d4 = buffer[3];
    byte d5 = buffer[4];
    byte d6 = buffer[5];
    byte d7 = buffer[6];
    byte d8 = buffer[7];	 
	 
  sendFisLine1(d1,d2,d3,d4,d5,d6,d7,d8);
}

void AudiCanControl::printFISln2(char pData[]) {
  int length = strlen(pData);
  byte buffer[8];
  
  for (int i=0;i<=length-1;i++) {
    buffer[i] = convertCharToByte(pData[i]); 
  }
     
	byte d1 = buffer[0];
    byte d2 = buffer[1];
    byte d3 = buffer[2];
    byte d4 = buffer[3];
    byte d5 = buffer[4];
    byte d6 = buffer[5];
    byte d7 = buffer[6];
    byte d8 = buffer[7];	 
	 
  sendFisLine2(d1,d2,d3,d4,d5,d6,d7,d8);
}

// Sends Command to open the car.
void  AudiCanControl::OpenCar(){
	msg.adrsValue = 0x291;
	msg.isExtendedAdrs = false;
	msg.rtr = false;
	msg.dataLength = 5;
	msg.data[0] = 0x89;
	msg.data[1] = 0x55;
	msg.data[2] = 0x01;
	msg.data[3] = 0x00;
	msg.data[4] = 0x00;
	can.transmitCANMessage(msg, 1000);
}

// Sends Command to open the car.
void  AudiCanControl::CloseCar(){
	msg.adrsValue = 0x291;
	msg.isExtendedAdrs = false;
	msg.rtr = false;
	msg.dataLength = 5;
	msg.data[0] = 0x49;
	msg.data[1] = 0xAA;
	msg.data[2] = 0x02;
	msg.data[3] = 0x00;
	msg.data[4] = 0x00;
	can.transmitCANMessage(msg, 1000);
}

byte AudiCanControl::convertCharToByte(char pChar) { 
  switch (pChar) {
    // ---------------------------------
    // -------- Kleinbuchstaben --------
    // ---------------------------------
    case 'a':
      return 0x01;
      break;
    case 'b':
      return 0x02;
      break;
    case 'c':
      return 0x03;
      break;
    case 'd':
      return 0x04;
      break;
    case 'e':
      return 0x05;
      break;
    case 'f':
      return 0x06;
      break;
    case 'g':
      return 0x07;
      break;
    case 'h':
      return 0x08;
      break;
    case 'i':
      return 0x09;
      break;
    case 'j':
      return 0x0A;
      break;
    case 'k':
      return 0x0B;
      break;
    case 'l':
      return 0x0C;
      break;
    case 'm':
      return 0x0D;
      break;
    case 'n':
      return 0x0E;
      break;
    case 'o':
      return 0x0F;
      break;
    case 'p':
      return 0x10;
      break;
    case 'q':
      return 0x71;
      break;
    case 'r':
      return 0x72;
      break;
    case 's':
      return 0x73;
      break;
    case 't':
      return 0x74;
      break;
    case 'u':
      return 0x75;
      break;
    case 'v':
      return 0x76;
      break;
    case 'w':
      return 0x77;
      break;
    case 'x':
      return 0x78;
      break;
    case 'y':
      return 0x79;
      break;
    case 'z':
      return 0x7A;
      break;
    // ---------------------------------
    // -------- Großbuchstaben ---------
    // ---------------------------------
    case 'A':
      return 0x41;
      break;
    case 'B':
      return 0x42;
      break;
    case 'C':
      return 0x43;
      break;
    case 'D':
      return 0x44;
      break;
    case 'E':
      return 0x45;
      break;
    case 'F':
      return 0x46;
      break;
    case 'G':
      return 0x47;
      break;
    case 'H':
      return 0x48;
      break;
    case 'I':
      return 0x49;
      break;
    case 'J':
      return 0x4A;
      break;
    case 'K':
      return 0x4B;
      break;
    case 'L':
      return 0x4C;
      break;
    case 'M':
      return 0x4D;
      break;
    case 'N':
      return 0x4E;
      break;
    case 'O':
      return 0x4F;
      break;
    case 'P':
      return 0x50;
      break;
    case 'Q':
      return 0x51;
      break;
    case 'R':
      return 0x52;
      break;
    case 'S':
      return 0x53;
      break;
    case 'T':
      return 0x54;
      break;
    case 'U':
      return 0x55;
      break;
    case 'V':
      return 0x56;
      break;
    case 'W':
      return 0x57;
      break;
    case 'X':
      return 0x58;
      break;
    case 'Y':
      return 0x59;
      break;
    case 'Z':
      return 0x5A;
      break;
	case 'Ö':
      return 0x60;
      break;
    // ---------------------------------
    // ------------ Zahlen -------------
    // ---------------------------------
    case 0:
      return 0x30;
      break;
    case 1:
      return 0x31;
      break;
    case 2:
      return 0x32;
      break;
    case 3:
      return 0x33;
      break;
    case 4:
      return 0x34;
      break;
    case 5:
      return 0x35;
      break;
    case 6:
      return 0x36;
      break;
    case 7:
      return 0x37;
      break;
    case 8:
      return 0x38;
      break;
    case 9:
      return 0x39;
      break;
    // ---------------------------------
    // -------- Sonderzeichen ----------
    // ---------------------------------
    case '_':
      return 0x12;
      break; 
    case '!':
      return 0x21;
      break; 
    case '"':
      return 0x22;
      break; 
    case '#':
      return 0x23;
      break; 
    case '$':
      return 0x24;
      break; 
    case '%':
      return 0x25;
      break; 
    case '&':
      return 0x26;
      break; 
    case '(':
      return 0x28;
      break; 
    case ')':
      return 0x29;
      break; 
    case '*':
      return 0x2A;
      break; 
    case '+':
      return 0x2B;
      break;
    case ',':
      return 0x2C;
      break;
    case '-':
      return 0x2D;
      break;
    case '.':
      return 0x2E;
      break;
    case '/':
      return 0x2F;
      break;
    case ':':
      return 0x3A;
      break;
    case ';':
      return 0x3B;
      break;
    case '<':
      return 0x3C;
      break;
    case '=':
      return 0x3D;
      break;
    case '>':
      return 0x3E;
      break;
    case '?':
      return 0x3F;
      break;
    case '§':
      return 0xCF;
      break;
    case ' ':
      return 0x20;
      break;
  }
}

long AudiCanControl::hexToDec(String hexString) {
	long decValue = 0;
	int nextInt;
	for (int i = 0; i < hexString.length(); i++) {
		nextInt = int(hexString.charAt(i));
		if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
		if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
		if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
		nextInt = constrain(nextInt, 0, 15);
		decValue = (decValue * 16) + nextInt;
	}
	return decValue;
}