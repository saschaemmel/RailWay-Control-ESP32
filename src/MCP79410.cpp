/*
Name:			MCP79410.ccp

Description:

Libary for MCP79410

Summary from Manufactorer

The MCP7941X series of low-power Real-Time Clocks
(RTC) uses digital timing compensation for an accurate
clock/calendar, a programmable output control for
versatility, a power sense circuit that automatically
switches to the backup supply, and nonvolatile memory
for data storage. Using a low-cost 32.768 kHz crystal,
it tracks time using several internal registers. For
communication, the MCP7941X uses the I2C™ bus.
The clock/calendar automatically adjusts for months
with fewer than 31 days, including corrections for
leap years. The clock operates in either the 24-hour
or 12-hour format with an AM/PM indicator and
settable alarm(s) to the second, minute, hour, day of
the week, date or month. Using the programmable
CLKOUT, frequencies of 32.768, 8.192 and 4.096
kHz and 1 Hz can be generated from the external
crystal.
Along with the on-board Serial EEPROM and batterybacked
SRAM memory, a 64-bit protected space is
available for a unique ID or MAC address to be
programmed at the factory or by the end user.
The device is fully accessible through the serial
interface while VCC is between 1.8V and 5.5V, but can
operate down to 1.3V for timekeeping and SRAM
retention only.
The RTC series of devices are available in the standard
8-lead SOIC, TSSOP, MSOP and 2x3 TDFN packages.

Created:		09.11.2018 09:30:00

Author:			Sascha Patrick Emmel

Contact:		saschaemmel@gmail.com
*/


#include "MCP79410.h"
#include <Wire.h>

#pragma region System-Functions

///<summary>Init the Device with the slave-adress</summary>
///<param name="ADDRESS">The deviceaddress in format "0 1 0 0 0 A2 A1 A0"</param>
///<example>MCP79410.init(0b01000111)</example>
void MCP79410Class::init(byte ADDRESS, byte ADDRESS_EEPROM)
{
	SLAVE_ADDRESS = ADDRESS;
	SLAVE_ADDRESS_EEPROM = ADDRESS_EEPROM;

	//byte TransferData[3] = { RESOLUTION, 0b00000011 };
	//sendData(TransferData, 2);
}// init

 ///<summary>Returns a "true" if the Device is reachable</summary>
 /// <returns>true init OK, false init NOK</returns>
bool MCP79410Class::isAlive()
{
	bool returnValue = false;
	byte Array[1] = { 0x10 };
	if (sendData(Array, 1) == "") returnValue = true;
	return returnValue;
}// isAlive

 ///<summary>sends data to the MCP79410</summary>
 ///<param name="DATA">the data to be send</param>
 ///<param name="DATALENGTH">size of data array</param>
 ///<returns>Error as clear String</returns>
 ///<example>MCP79410.sendData(DATA[2], 2)</example>
String MCP79410Class::sendData(byte DATA[], byte DATALENGTH)
{
	byte Status = 0;
	String ERRVal = "";

	Wire.beginTransmission(SLAVE_ADDRESS);
	Wire.write(DATA, DATALENGTH);
	Status = Wire.endTransmission(true);

	//true = stop, false = restart
	//  0 : success
	//	1 : data too long to fit in transmit buffer
	//	2 : received NACK on transmit of address
	//	3 : received NACK on transmit of data
	//	4 : other error

	switch (Status)
	{
	case 0:
		break;
	case 1:
		ERRVal = "I2C data too long to fit in transmit buffer";
		break;
	case 2:
		ERRVal = "I2C received NACK on transmit of address";
		break;
	case 3:
		ERRVal = "I2C received NACK on transmit of data";
		break;
	default:
		ERRVal = "I2C other error";
		break;
	}// switch
	return ERRVal;
}

///<summary>gets data from the MCP79410</summary>
///<param name="REGISTER">the register to be readed</param>
///<returns>the result as byte</returns>
///<example>MCP79410.recieveData(GPIOA)</example>
byte MCP79410Class::recieveByte(byte REGISTER)
{
	byte readValue = 0;
	//Send the Pointing Register Address
	byte ReadRegister[1] = { REGISTER };
	sendData(ReadRegister, 1);

	//Recives Data
	int Address = SLAVE_ADDRESS; //Workaround, function is not clearly definied
	Wire.requestFrom(Address, 1, true);	// request 1 bytes from slave device


	if (Wire.available() != 0) // slave may send less than requested

	{
		readValue = Wire.read(); // receive a byte as character
	}// if
	return byte();
}// recieveData

 ///<summary>gets data from the MCP79410</summary>
 ///<param name="REGISTER">the register to be readed</param>
 ///<returns>the result as word</returns>
 ///<example>MCP79410.recieveData(GPIOA)</example>
word MCP79410Class::recieveWord(byte REGISTER)
{
	word readValue = 0;
	//Send the Pointing Register Address
	byte ReadRegister[1] = { REGISTER };
	sendData(ReadRegister, 1);

	//Recives Data
	int Address = SLAVE_ADDRESS; //Workaround, function is not clearly definied
	Wire.requestFrom(Address, 2, true);	// request 2 bytes from slave device


	if (Wire.available() != 0) // slave may send less than requested
	{
		readValue = Wire.read() << 8;
		readValue = readValue + Wire.read();
	}// if
	return readValue;
}// recieveData

 ///<summary>gets data from the MCP79410 and write it to rcvBuf[10]</summary>
 ///<param name="REGISTER">the register to be readed</param>
 ///<param name="COUNT">size be readed</param>
 ///<returns>true = reading ok</returns>
 ///<example></example>
bool MCP79410Class::recieveToBuf(byte REGISTER, byte COUNT)
{
	bool returnValue = true;
	//Send the Pointing Register Address
	byte ReadRegister[1] = { REGISTER };
	sendData(ReadRegister, 1);

	//Recives Data
	int Address = SLAVE_ADDRESS; //Workaround, function is not clearly definied
	Wire.requestFrom(Address, COUNT, true);	// request 2 bytes from slave device

	if (Wire.available() != 0) {
		for (byte i = 0; i < COUNT; i++) {
			rcvBuf[i] = Wire.read();
		}
	}
	else {
		returnValue = false;
	}
	return returnValue;
}// recieveData

#pragma endregion

#pragma region Config
#pragma endregion

#pragma region Set
 ///<summary>Reads the temperature from sensor</summary>
 ///<returns>temperature</returns>
 ///<example>MCP79410.getTemp()</example>
String MCP79410Class::setTime(byte SETSECONDS, byte SETMINUTES, byte SETHOURS, byte SETDAY, byte SETMONTH, byte SETYEAR){
	String returnValue = "";

	byte sendBytes[8];

	sendBytes[0] = SECONDS;
	
	//Convert Seconds to BCD
	byte hSeconds = SETSECONDS / 10;
	byte lSeconds = SETSECONDS % 10;
	sendBytes[1] = (0x1 << 7) + (hSeconds << 4) + lSeconds;

	//Convert Minutes to BCD
	byte hMinutes = SETMINUTES / 10;
	byte lMinutes = SETMINUTES % 10;
	sendBytes[2] = (hMinutes << 4) + lMinutes;

	//Convert Hours to BCD
	byte hHours = SETHOURS / 10;
	byte lHours = SETHOURS % 10;
	sendBytes[3] = (hHours << 4) + lHours;

	//TODO WEEKDAY
	sendBytes[4] = 0x01;

	//Convert Day to BCD
	byte hDay = SETDAY / 10;
	byte lDay = SETDAY % 10;
	sendBytes[5] = (hDay << 4) + lDay;

	//Convert Month to BCD
	byte hMonth = SETMONTH / 10;
	byte lMonth = SETMONTH % 10;
	sendBytes[6] = (hMonth << 4) + lMonth;

	//Convert Year to BCD
	byte hYear = SETYEAR / 10;
	byte lYear = SETYEAR % 10;
	sendBytes[7] = (hYear << 4) + lYear;

	//returns true when sending ok
	returnValue = sendData(sendBytes, 8);

	return returnValue;
}
#pragma endregion

#pragma region Get
 ///<summary>Reads the temperature from sensor</summary>
 ///<returns>temperature</returns>
 ///<example>MCP79410.getTemp()</example>
void MCP79410Class::getTime()
{
	//Reads Values from RTC and store in Message Buffer
	recieveToBuf(SECONDS, 7);
	year =		(rcvBuf[6] & 0x0F) + ((rcvBuf[6] >> 4) * 10);
	month =		(rcvBuf[5] & 0x0F) + (((rcvBuf[5] & 0x10) >> 4) * 10);
	day =		(rcvBuf[4] & 0x0F) + (((rcvBuf[4] & 0x30) >> 4) * 10);
	hour =		(rcvBuf[2] & 0x0F) + (((rcvBuf[2] & 0x30) >> 4) * 10);
	minute =	(rcvBuf[1] & 0x0F) + (((rcvBuf[1] & 0x70) >> 4) * 10);
	second =	(rcvBuf[0] & 0x0F) + (((rcvBuf[0] & 0x7F) >> 4) * 10);

}// readInput
#pragma endregion

MCP79410Class MCP79410;