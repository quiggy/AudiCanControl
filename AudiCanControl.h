#ifndef AUDI_CAN_CONTROL
#define AUDI_CAN_CONTROL

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


#include <avr/io.h>
#include <inttypes.h>
#include "SPI.h"
#include <MCP2515.h>

class AudiCanControl
{ 	
  public:
    AudiCanControl();

    /**
     * Initializes the library.
     * This method must be called in your sketch's setup() function.
     * You should *not* call begin on the serial port that your
     * iPod is connected to: the library will do that here since it
     * knows what baud rate it wants to talk at.
     */
    void setup();

    /**
     * Checks for data coming in from the iPod and processes it if there is any.
     * The library handles partial messages coming in from the iPod so it
     * will buffer those chunks until it gets a complete message. Once a complete
     * message (that the library understands) has been received it will process
     * that message in here and call any callbacks that are applicable to that
     * message and that have been configured.
     */
    void loop();
	
	void printFISln1(char pData[]);
	void printFISln2(char pData[]);
	
	/**
	 * Converts a Char to Bytecode the Audi Fis needs to display the symbol.
	 */
	byte convertCharToByte(char pChar);
	
	/**
	 * Sends the Header "CarPC   " to the FIS
	 */
	void callback_scrollFis();
	
	
	// Setting up intervalls
	unsigned long IdleInterval;
	unsigned long ScrollInterval;
	unsigned long FISInterval;
	unsigned long BTConnectInterval;
	
    // Setting up Timers
	unsigned long waitUntilIdle; 
	unsigned long waitUntilFISIdle;
	unsigned long waitUntilFISScroll;
	unsigned long waitUntilBTConnect;	

	int getHeadUnitMode();
	
	
  private:
	int RadioMode;
	int led2;
	int led3;
	int cmdPin;
	int led13;
	int ledPower;
	int ledActive;
	int Fis2Position;
	CANMSG msg;
	MCP2515 can;
	String readString;
  
  protected: // methods	
	void processData();
	void printMessage();
	void sendFisLine1(byte d0, byte d1, byte d2, byte d3, byte d4, byte d5, byte d6, byte d7);
	void sendFisLine2(byte d0, byte d1, byte d2, byte d3, byte d4, byte d5, byte d6, byte d7);
	void sendFisMiddle(int length, byte pData[]);
	void sendFisTEST();
	void enableTvMode();
};

#endif // AUDI_CAN_CONTROL