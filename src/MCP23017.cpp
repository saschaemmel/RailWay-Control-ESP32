/*
Name:			MCP23017.ccp

Description:	

Libary for MCP23017

Summary from Manufactorer

The MCP23017/MCP23S17 (MCP23X17) device family provides 16-bit,
general purpose parallel I/O expansion for I2C bus or SPI applications.
The two devices differ only in the serial interface:
ï¿½ MCP23017 ï¿½ I2C interface
ï¿½ MCP23S17 ï¿½ SPI interface
The MCP23X17 consists of multiple 8-bit configuration registers for input,
output and polarity selection. The system master can enable the I/Os as either inputs
or outputs by writing the I/O configuration bits (IODIRA/B). The data for each input
or output is kept in the corresponding input or output register. The polarity of the
Input Port register can be inverted with the Polarity Inversion register.
All registers can be read by the system master.

The 16-bit I/O port functionally consists of two 8-bit ports (PORTA and PORTB).
The MCP23X17 can be configured to operate in the 8-bit or 16-bit modes via IOCON.BANK.
There are two interrupt pins, INTA and INTB, that can be associated with their
respective ports, or can be logically ORï¿½ed together so that both pins will activate
if either port causes an interrupt. The interrupt output can be configured to activate
under two conditions (mutually exclusive):
1. When any input state differs from its corresponding Input Port register state.
This is used to indicate to the system master that an input state has changed.
2. When an input state differs from a preconfigured register value (DEFVAL register).

The Interrupt Capture register captures port values at the time of the interrupt,
thereby saving the condition that caused the interrupt. The Power-on Reset (POR)
sets the registers to their default values and initializes the device state machine.
The hardware address pins are used to determine the device address.

Created:		26.06.2018 19:51:43

Author:			Sascha Patrick Emmel - VESCON Automation GmbH

Contact:		saschapatrick.emmel@vescon.com
*/ 

#include "MCP23017.h"
#include <Wire.h>


#pragma region System-Functions

///<summary>Init the Device with the slave-adress</summary>
///<param name="ADDRESS">The deviceaddress in format "0 1 0 0 0 A2 A1 A0"</param>
///<example>MCP23017.init(0b01000111)</example>
void MCP23017Class::init(byte ADDRESS)
{
	SLAVE_ADDRESS = ADDRESS;
}// init

 ///<summary>Returns a "true" if the Device is reachable</summary>
 /// <returns>true init OK, false init NOK</returns>
bool MCP23017Class::isAlive()
{
	bool returnValue = false;
	byte Array[1] = { INTCONA };
	if (sendData(Array,1) == "") returnValue = true;
	return returnValue;
}// isAlive

///<summary>sends data to the MCP23017</summary>
///<param name="DATA">the data to be send</param>
///<param name="DATALENGTH">size of data array</param>
///<returns>Error as clear String</returns>
///<example>MCP23017.sendData(DATA[2], 2)</example>
String MCP23017Class::sendData(byte DATA[], byte DATALENGTH)
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

///<summary>gets data from the MCP23017</summary>
///<param name="REGISTER">the register to be readed</param>
///<returns>the result as byte</returns>
///<example>MCP23017.recieveData(GPIOA)</example>
byte MCP23017Class::recieveData(byte REGISTER)
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
#pragma endregion

#pragma region Config
///<summary>Sets direction of Port</summary>
///<param name="PORTAval">Direction of Port A (HIGH=INPUT) like 0b00000000"</param>
///<param name="PORTBval">Direction of Port B (HIGH=INPUT) like 0b00000000"</param>
///<returns>Error as clear String</returns>
///<example>MCP23017.configDirection(0b00000000, 0b00000000)</example>
String MCP23017Class::configDirection(byte PORTAval, byte PORTBval)
{
	byte TransferData[3] = { IODIRA, PORTAval, PORTBval };
	String ReturnVal = sendData(TransferData, 3);
	return ReturnVal;
}// configDirection

///<summary>Enable PullUps on Port</summary>
///<param name="PORTAval">Enable PullUp on Port A (HIGH=Enabled) like 0b00000000"</param>
///<param name="PORTBval">Enable PullUp on Port B (HIGH=Enabled) like 0b00000000"</param>
///<returns>Error as clear String</returns>
///<example>MCP23017.configPullUp(0b00000000, 0b00000000)</example>
String MCP23017Class::configPullUp(byte PORTAval, byte PORTBval)
{
	byte TransferData[3] = { GPPUA, PORTAval, PORTBval };
	String ReturnVal = sendData(TransferData, 3);
	return ReturnVal;
}// configPullUp
#pragma endregion

#pragma region Set
///<summary>Sets selected Pins to given level</summary>
///<param name="PORTAval">Port A Value (1=HIGH) like 0b00000000"</param>
///<param name="PORTBval">Port B Value (1=HIGH) like 0b00000000"</param>
///<param name="PORTAsel">Port A Selection (1=Select) like 0b00000000"</param>
///<param name="PORTBsel">Port B Selection (1=Select) like 0b00000000"</param>
///<returns>Error as clear String</returns>
///<example>MCP23017.setOutput(0b00000000, 0b00000000,0b00000000, 0b00000000)</example>
String MCP23017Class::setOutput(byte PORTAval, byte PORTBval, byte PORTAsel, byte PORTBsel)
{
	String ReturnVal = "";

	//Checks if both Registers need to be written
	if (PORTAsel != 0 & PORTBsel == 0) {
		byte TransA = (PORTAsel & PORTAval) | (PORTA_STATUS & ~PORTAsel);
		byte TransferData[2] = { GPIOA, TransA };
		ReturnVal = sendData(TransferData, 2);
		if (ReturnVal == 0) PORTA_STATUS = TransA; //if sended without Errors, set Portstatus
	}// if
	if (PORTAsel == 0 & PORTBsel != 0) {
		byte TransB = (PORTBsel & PORTBval) | (PORTB_STATUS & ~PORTBsel);
		byte TransferData[2] = { GPIOB, TransB };
		ReturnVal = sendData(TransferData, 2);
		if (ReturnVal == 0) PORTB_STATUS = TransB; //if sended without Errors, set Portstatus
	}// if
	else {
		byte TransA = (PORTAsel & PORTAval) | (PORTA_STATUS & ~PORTAsel);
		byte TransB = (PORTBsel & PORTBval) | (PORTB_STATUS & ~PORTBsel);
		byte TransferData[3] = { GPIOA, TransA, TransB };
		ReturnVal = sendData(TransferData, 3);
		if (ReturnVal == 0) {
			PORTA_STATUS = TransA;
			PORTB_STATUS = TransB;
		}// if
	}// if
	return ReturnVal;
}// setOutput
#pragma endregion

#pragma region Get
///<summary>Reads level of specified port</summary>
///<param name="PORT">Reads from 0=A, 1=B</param>
///<returns>portlevel</returns>
///<example>MCP23017.readInput(0)</example>
byte MCP23017Class::readInput(byte PORTsel)
{
	byte Port = 0;
	if (PORTsel == 0) Port = GPIOA;
	else Port = GPIOB;
	byte returnValue = readInput(Port);
	return returnValue;
}// readInput

 ///<summary>gets the levels which the port has been setted</summary>
 ///<returns>portlevel</returns>
 ///<example>MCP23017.getPortAStatus()</example>
byte MCP23017Class::getPortAStatus()
{
	return PORTA_STATUS;
}// getPortAStatus

 ///<summary>gets the levels which the port has been setted</summary>
 ///<returns>portlevel</returns>
 ///<example>MCP23017.getPortBStatus()</example>
byte MCP23017Class::getPortBStatus()
{
	return PORTB_STATUS;
}// getPortBStatus
#pragma endregion

MCP23017Class MCP23017;


