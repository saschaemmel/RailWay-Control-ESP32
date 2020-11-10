// MCP79410.h

#ifndef _MCP79410_h
#define _MCP79410_h

//MCP79410 Register Names
#define SECONDS		0x00 //Seconds 00-59
#define MINUTES		0x01 //Minutes 00-59
#define HOURS		0x02 //Hours 00-23
#define WEEKDAY		0x03 //Day 1-7
#define DAY			0x04 //Date 01-31
#define MONTH		0x05 //Month 01-12
#define YEAR		0x06 //Year 00-99
#define RTCCONFIG	0x07 //Control Reg.
#define CALIBRATION	0x08 //Calibration
#define UNLOCKID	0x09 //Unlock ID
#define A0SECONDS	0x0A //Alarm0 Seconds
#define A0MINUTES	0x0B //Alarm0 Minutes
#define A0HOURS		0x0C //Alarm0 Hours
#define A0DAY		0x0D //Alarm0 Day
#define A0DATE		0x0E //Alarm0 Date
#define A0MONTH		0x0F //Alarm0 Month
#define	A1SECONDS	0x11 //Alarm1 Seconds
#define A1MINUTES	0x12 //Alarm1 Minutes
#define A1HOURS		0x13 //Alarm1 Hours
#define A1DAY		0x14 //Alarm1 Day
#define A1DATE		0x15 //Alarm1 Date
#define	A1MONTH		0x16 //Alarm1 Month
#define	T1MINUTES	0x18 //Timesaver1 Minutes
#define T1HOURS		0x19 //Timesaver1 Hours
#define T1DATE		0x1A //Timesaver1 Date
#define T1MONTH		0x1B //Timesaver1 Month
#define T2MINUTES	0x1C //Timesaver2 Minutes
#define T2HOURS		0x1D //Timesaver2 Hours
#define T2DATE		0x1E //Timesaver2 Date
#define	T2MONTH		0x1F //Timesaver2 Month

#if defined(ARDUINO) && ARDUINO >= 100
	#include <Arduino.h>
#else
	#include "WProgram.h"
#endif

class MCP79410Class
{
protected:
	uint8_t SLAVE_ADDRESS;
	uint8_t SLAVE_ADDRESS_EEPROM;

	String sendData(byte DATA[], byte DATALENGTH);

	byte recieveByte(byte REGISTER);
	word recieveWord(byte REGISTER);
	bool recieveToBuf(byte REGISTER, byte COUNT);

	byte rcvBuf[10];

public:
	void init(byte ADDRESS, byte ADDRESS_EEPROM);

	String setTime(byte SETSECONDS, byte SETMINUTES, byte SETHOURS, byte SETDAY, byte SETMONTH, byte SETYEAR);
	void getTime();

	void writeEEPROM();
	void readEEPROM();
	
	bool isAlive();

	byte year;
	byte month;
	byte day;
	byte hour;
	byte minute;
	byte second;

	//Allocate 128*8Byte EEPROM Space
	byte EEPROM[1024];
};

extern MCP79410Class MCP79410;

#endif

