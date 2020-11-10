// MCP9843.h

#ifndef _MCP9843_h
#define _MCP9843_h

//MCP23017 Register Names
#define CAPABILITY	0x00 //Capability register
#define CONFIG		0x01 //Configuration register (CONFIG)
#define TUPPER		0x02 //Event Temperature Upper-Boundary Trip register (TUPPER)
#define TLOWER		0x03 //Event Temperature Lower-Boundary Trip register (TLOWER)
#define TCRIT		0x04 //Critical Temperature Trip register (TCRIT)
#define TA			0x05 //Temperature register (TA)
#define MANUID		0x06 //Manufacturer ID register
#define DEVID		0x07 //Resolution register
#define RESOLUTION	0x08 //If set, compare to DEFVAL register.If clear compareto previous value.

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "WProgram.h"
#endif

class MCP9843Class
{
protected:
	uint8_t SLAVE_ADDRESS;

	String sendData(byte DATA[], byte DATALENGTH);

	byte recieveByte(byte REGISTER);
	word recieveWord(byte REGISTER);

public:
	void init(byte ADDRESS);
	float getTemp();
	bool isAlive();

};

extern MCP9843Class MCP9843;

#endif


