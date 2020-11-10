// MCP23017.h

#ifndef _MCP23017_h
#define _MCP23017_h
//MCP23017 Register Names
#define IODIRA		0x00 //Controls the direction of the data I/O, HIGH = INPUT, LOW = OUTPUT
#define IODIRB		0x01 //Controls the direction of the data I/O, HIGH = INPUT, LOW = OUTPUT
#define IPOLA		0x02 //Negates Polarity - 0 = HIGH, 1 = LOW.
#define IPOLB		0x03 //Negates Polarity - 0 = HIGH, 1 = LOW.
#define GPINTENA	0x04 //Controls the interrupt on change feature for each pin. (Need also DEFVAL and INTCON)
#define GPINTENB	0x05 //Controls the interrupt on change feature for each pin. (Need also DEFVAL and INTCON)
#define DEFVALA		0x06 //Controls if an opposite value on the associated pin will cause an interrupt to occur.
#define DEFVALB		0x07 //Controls if an opposite value on the associated pin will cause an interrupt to occur.
#define INTCONA		0x08 //If set, compare to DEFVAL register.If clear compareto previous value.
#define INTCONB		0x09 //If set, compare to DEFVAL register.If clear compareto previous value.
#define IOCONA		0x0A //Config Register - See Datasheet
#define IOCONB		0x0B //Config Register - See Datasheet
#define GPPUA		0x0C //Controls the pull-up, HIGH = 100kOHM Pullup if IODIR = Input
#define GPPUB		0x0D //Controls the pull-up, HIGH = 100kOHM Pullup if IODIR = Input
#define INTFA		0x0E //If HIGH Port causes Interrupt
#define INTFB		0x0F //If HIGH Port causes Interrupt
#define INTCAPA		0x10 //captures the GPIO port value at the time the interrupt occurred, READ-ONLY!
#define INTCAPB		0x11 //captures the GPIO port value at the time the interrupt occurred, READ-ONLY!
#define GPIOA		0x12 //Reading reads the port. Writing modifies the Output Latch(OLAT) register.
#define GPIOB		0x13 //Reading reads the port. Writing modifies the Output Latch(OLAT) register.
#define OLATA		0x14 //A read results in a read of the OLAT and not the port itself.A write modifies the pins
#define OLATB		0x15 //A read results in a read of the OLAT and not the port itself.A write modifies the pins

#if defined(ARDUINO) && ARDUINO >= 100
	#include <Arduino.h>
#else
	#include "WProgram.h"
#endif

class MCP23017Class
{
 protected:
	 uint8_t SLAVE_ADDRESS;
	 
	 String sendData(byte DATA[], byte DATALENGTH);
	 
	 byte recieveData(byte REGISTER);
	
	 byte PORTA_STATUS = 0;
	 byte PORTB_STATUS = 0;
	 
 public:
	void init(byte ADDRESS);
	bool isAlive();
	
	String configDirection(byte PORTAval, byte PORTBval);
	String configPullUp(byte PORTAval, byte PORTBval);
	
	String setOutput(byte PORTAval, byte PORTBval, byte PORTAsel, byte PORTBsel);
	
	byte readInput(byte PORTsel);
	byte getPortAStatus();
	byte getPortBStatus();
	};

extern MCP23017Class MCP23017;

#endif


