/*
Name:			MCP9843.ccp

Description:

Libary for MCP9843

Summary from Manufactorer

Microchip Technology Inc.’s MCP9843/98243 digital
temperature sensors convert temperature from -40°C
and +125°C to a digital word. These sensors meet
JEDEC Specification JC42.4-TSE3000B3 and
JC42.4-TSE2002B3 Memory Module Thermal Sensor
Component. It provides an accuracy of ±0.2°C/±1°C
(typical/maximum) from +75°C to +95°C. In addition,
MCP98243 has an internal 256 Byte EEPROM which
can be used to store memory module and vendor
information.
The MCP9843/98243 digital temperature sensor
comes with user-programmable registers that provide
flexibility for DIMM temperature-sensing applications.
The registers allow user-selectable settings such as
Shutdown or Low-Power modes and the specification
of temperature Event boundaries. When the
temperature changes beyond the specified Event
boundary limits, the MCP9843/98243 outputs an Alert
signal at the Event pin. The user has the option of
setting the temperature Event output signal polarity as
either an active-low or active-high comparator output
for thermostat operation, or as a temperature Event
interrupt output for microprocessor-based systems.
The MCP98243 EEPROM is designed specifically for
DRAM DIMMs (Dual In-line Memory Modules) Serial
Presence Detect (SPD). The lower 128 Bytes (address
0x00 to 0x7F) can be Permanent Write Protected
(PWP) or Software Reversible Write Protected (SWP).
This allows DRAM vendor and product information to
be stored and write protected. The upper 128 bytes
(address 0x80 to 0xFF) can be used for general
purpose data storage. These addresses are not write
protected.
This sensor has an industry standard 2-wire,
I2C compatible serial interface, allowing up to

Created:		08.11.2018 17:56:43

Author:			Sascha Patrick Emmel

Contact:		saschaemmel@gmail.com
*/

#include "MCP9843.h"
#include <Wire.h>


#pragma region System-Functions

///<summary>Init the Device with the slave-adress</summary>
///<param name="ADDRESS">The deviceaddress in format "0 1 0 0 0 A2 A1 A0"</param>
///<example>MCP9843.init(0b01000111)</example>
void MCP9843Class::init(byte ADDRESS)
{
	SLAVE_ADDRESS = ADDRESS;

	byte TransferData[3] = { RESOLUTION, 0b00000011 };
	sendData(TransferData, 2);
}// init

 ///<summary>Returns a "true" if the Device is reachable</summary>
 /// <returns>true init OK, false init NOK</returns>
bool MCP9843Class::isAlive()
{
	bool returnValue = false;
	byte Array[1] = { MANUID };
	if (sendData(Array, 1) == "") returnValue = true;
	return returnValue;
}// isAlive

 ///<summary>sends data to the MCP9843</summary>
 ///<param name="DATA">the data to be send</param>
 ///<param name="DATALENGTH">size of data array</param>
 ///<returns>Error as clear String</returns>
 ///<example>MCP9843.sendData(DATA[2], 2)</example>
String MCP9843Class::sendData(byte DATA[], byte DATALENGTH)
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

///<summary>gets data from the MCP9843</summary>
///<param name="REGISTER">the register to be readed</param>
///<returns>the result as byte</returns>
///<example>MCP9843.recieveData(GPIOA)</example>
byte MCP9843Class::recieveByte(byte REGISTER)
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

 ///<summary>gets data from the MCP9843</summary>
 ///<param name="REGISTER">the register to be readed</param>
 ///<returns>the result as word</returns>
 ///<example>MCP9843.recieveData(GPIOA)</example>
word MCP9843Class::recieveWord(byte REGISTER)
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
			readValue = Wire.read()<<8;
			readValue = readValue + Wire.read();
	}// if
	return readValue;
}// recieveData
#pragma endregion

#pragma region Config
#pragma endregion

#pragma region Set

#pragma endregion

#pragma region Get
 ///<summary>Reads the temperature from sensor</summary>
 ///<returns>temperature</returns>
 ///<example>MCP9843.getTemp()</example>
float MCP9843Class::getTemp()
{
	float returnValue = 0;

	//reads 2 Bytes from the Sensor
	//0-3:		Mantisse
	//4-11:		Value
	//12:		Sign
	//13-15:	Compare Bits
	word data = recieveWord(TA);

	//MODYFIED CODE EXAMPLE FROM MCP
	byte UpperByte = data >> 8; 
	byte LowerByte = data & 0x00FF; 

	//Calc decimal places
	float places = 0;
	if ((LowerByte & 0b00001000) != 0){
		places = places + 0.5;
	}
	if ((LowerByte & 0b00000100) != 0) {
		places = places + 0.25;
	}
	if ((LowerByte & 0b00000010) != 0) {
		places = places + 0.125;
	}
	if ((LowerByte & 0b00000001) != 0) {
		places = places + 0.0625;
	}

	UpperByte = UpperByte & 0x1F; //Clear flag bits
	if ((UpperByte & 0x10) == 0x10) { //TA < 0°C
		UpperByte = UpperByte & 0x0F; //Clear SIGN
		returnValue = 256 - (UpperByte * 16 + LowerByte / 16 + places);
	}
	else //TA ≥ 0°C
		returnValue = (UpperByte * 16 + LowerByte / 16 + places);
	//Temperature = Ambient Temperature (°C)

	return returnValue;
}// readInput
#pragma endregion

MCP9843Class MCP9843;


