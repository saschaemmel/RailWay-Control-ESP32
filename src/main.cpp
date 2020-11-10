/**@file RailWay-Signal-ESP32.ino */

/*
 Name:		RailWay_Signal_MKR1000.ino
 Created:	15.07.2018 16:49:46
 Author:	Sascha Patrick Emmel - VESCON Automation GmbH

 Version-Log:
 Version		Date			Editor				Comment
	0.0.1		15.07.2018		Sascha P. Emmel		Init
	0.0.2		08.11.2018		Sascha P. Emmel		Moved project to ESP32 CPU
	0.0.3		09.11.2018		Sascha P. Emmel		Added Libary for MCP9843 / MCP79410
	0.0.4		11.01.2019		Sascha P. Emmel		Added JSON Parser, print errLog on MQTT
 */

//		NAME								                  DEZ		  BIT			HEX		STRING
#define PIN_I2C_SDA							            21
#define PIN_I2C_SCL							            22
#define PIN_UART_TX							            0
#define PIN_UART_RX							            0
#define PIN_BATVOLTAGE					            32
#define PIN_PWM_SIGNAL					            0
#define PIN_SENSOR1							            34
#define PIN_SENSOR2							            35
#define PIN_LED								              2
#define PIN_PortExpander_Reset	            4
#define PIN_PortExpander_INTA		            17
#define PIN_PortExpander_INTB		            16
#define PIN_Expansion_1					            12
#define PIN_Expansion_3					            14
#define PIN_Expansion_5					            27
#define PIN_Expansion_7					            26
#define PIN_Expansion_9					            25
#define ADRESS_I2C_PORTEXPANDER	                    0b00100000
#define ADRESS_I2C_TEMP								              0b00011000
#define ADRESS_I2C_RTC								              0b01101111
#define ADRESS_I2C_RTC_EEPROM					              0b01010111
#define SPEED_I2C_CLOCK					            100000
#define PIN_PORTEXPANDER_PORTA_FRONT_RED1			      0b00000010
#define PIN_PORTEXPANDER_PORTA_FRONT_RED2			      0b00000100
#define PIN_PORTEXPANDER_PORTA_FRONT_YELLOW			    0b00001000
#define PIN_PORTEXPANDER_PORTA_FRONT_GREEN			    0b00000001
#define PIN_PORTEXPANDER_PORTA_BACK_RED1			      0b00100000
#define PIN_PORTEXPANDER_PORTA_BACK_RED2			      0b01000000
#define PIN_PORTEXPANDER_PORTA_BACK_YELLOW			    0b10000000
#define PIN_PORTEXPANDER_PORTA_BACK_GREEN			      0b00010000
#define PIN_PORTEXPANDER_PORTB_WIFI_GREEN			      0b10000000
#define PIN_PORTEXPANDER_PORTB_WIFI_YELLOW			    0b01000000
#define PIN_PORTEXPANDER_PORTB_WIFI_RED				      0b00100000
#define PIN_PORTEXPANDER_PORTB_CON_WIFI				      0b00010000
#define PIN_PORTEXPANDER_PORTB_CON_MQTT				      0b00001000

// Import Header Files
//#include <esp_task_wdt.h>
#include <main.h>
#include <MCP23017.h> //IO-Expander
#include <MCP9843.h> //Temp
#include <MCP79410.h> //RTC
#include <Wire.h>
#include "WiFi.h"
#include <WiFiUdp.h>
#include <Time.h>
//#include <TimeLib.h>
#include <MQTT.h>
#include <EasyNTPClient.h>


struct LEDSignal
{
	SIGNAL_STATES State;
	byte Brigthness;
};

LEDSignal LEDFront;
LEDSignal LEDBack;


String VERSION = "0.0.4";

String MQTT_TOPIC_SYSTEM_ID = "RailWay-Control/FT-TRAIN";
String MQTT_TOPIC_SIGNAL_ID = "SIGNAL001";
String MQTT_TOPIC_STATUS = "STATUS";
String MQTT_TOPIC_LIFE = "LIFE";
String MQTT_TOPIC_ERROR = "ERROR";
String MQTT_TOPIC_COMMAND = "SET";
char* MQTT_BROKER_ADDRESS = "192.168.178.43";

String WIFI_HOSTNAME = "Signal01";

char WIFI_AP_SSID[] = "Springfield";
char WIFI_AP_PASSWORD[] = "32688070998039391711";
String WIFI_IP = "";

char* NTP_SERVER = "pool.ntp.org";
//char* NTP_SERVER = "192.168.1.1";
int	TIME_OFFSET = 0;

int MQTT_BROKER_PORT = 1883;
int SERIAL_BAUDRATE = 9600;

float sensorDif = 100; //Distance between the two Sensor in milimeters

unsigned long t_100ms = 0;
unsigned long t_500ms = 0; 
unsigned long t_1000ms = 0; //1second
unsigned long t_2000ms = 0; //2seconds
unsigned long t_5000ms = 0; //5seconds
unsigned long t_10000ms = 0; //10seconds
unsigned long t_300000ms = 0; //5minutes

unsigned long runtime[20];

unsigned long t1 = 0;
unsigned long t2 = 0;

//Array for JSON Parsing
//jsonBuf[Name, Value][Element]
String jsonBuf[2][10];

bool RunLoop = false;
bool LifeBit = false; //LifeBit for Sending to Host
bool DebugMode = true; //If true, sending debugmessages to serial-interface
bool I2C_PortExpander_Connection = false;
bool Wifi_Connection = false;
bool MQTT_Connection = false;
bool Serial_Connection = false;

bool newValue = false;
bool newT1 = false;
bool newT2 = false;

WiFiClient wifiClient;
MQTTClient mqttClient(256);
WiFiUDP ntpUDP;
EasyNTPClient ntpClient(ntpUDP, NTP_SERVER);// , NTP_SERVER, 1);

/// <summary> Setup-Routine, initialize Hardware before running main functions </summary>
void setup() {
	delay(500);

	// Init Serial Interface
	serialInit(SERIAL_BAUDRATE);
	
	delay(500);

	//Print Software Info
	String sBuf = "Version: " + VERSION + " by Sascha Patrick Emmel";
	errLog(sBuf, INFO, false);

	// Ini Port Config
	errLog("PortInit", INFO, false);
	portInit();

	digitalWrite(PIN_LED, HIGH);
	digitalWrite(PIN_PortExpander_Reset, HIGH);
	
	// Init I2C Interface
	errLog("I2CInit", INFO, false);
	if (i2cInit() == true) {
		errLog("I2C SUCCESS", INFO, false);

	}// if
	else {
		errLog("I2C FAILED!", ERROR, false);
	}// else

	//Scan for I2C devices on bus and print it to console
	if (DebugMode == true) {
		scanI2C();
	}
	
	if (I2C_PortExpander_Connection == true) {
		//Config Ports as Output
		MCP23017.configDirection(0x00, 0x00);
	}// if

	// Init Signal LED's with STOP!
	errLog("SignalLED Init", INFO, false);
	setSignalLED(STOP, true, true);

	// Load Settings from EEPROM
	loadSettings();

	// Init Wifi Connection
	errLog("WiFiInit", INFO, false);
	wifiInit();

	// Init MQTT Connection
	errLog("MQTTInit", INFO, false);
	if (connectMqtt() == true) {
		errLog("MQTT SUCCESS", INFO, false);
	}// if
	else {
		errLog("MQTT FAILED!", ERROR, false);
	}// else

	errLog("Get NTP Time", INFO, false);
	setDateTime();

	//Enabled Watchdog-Timer
	//esp_task_wdt_init(2, false);
	//esp_task_wdt_add(NULL);

	// Init Interrupt
	isrEnable();
}// setup

 /// <summary> Main Loop </summary>
void loop() {
	//Do Every loop
	mqttClient.loop();	

	//set runtime array
	for (int i = 0; i<=18; i++){
		runtime[i] = runtime[i + 1];
	}
	runtime[19] = micros();
	
	//Reset Watchdog
	//esp_task_wdt_reset();

	//Do Every 100ms (0,1s)
	if (millis() >= t_100ms) {
		//Calc Speed on new Values
		if (newT1 == true && newT2 == true) {
			calcSpeed();
		}// if

		//Checks the Serial Interface
		if (Serial.available()) {
			readSerialData();
		}

		t_100ms = millis() + 100;
	}// if
	//Do Every 500ms (0,5s)
	if (millis() >= t_500ms) {

		//Check WiFi Connection
		if (WiFi.status() != WL_CONNECTED) {

			errLog("Lost WiFi Connection", WARN, true);

			Wifi_Connection = false;
			wifiInit();

			MQTT_Connection = false;
			connectMqtt();
		}// if

		// Check MQTT Connection
		if (mqttClient.connected() != true) {

			errLog("Lost Mqtt Connection", WARN, true);

		//	MQTT_Connection = false;
			connectMqtt();
		}// if

		else {
			// Blink Status LED
			if (digitalRead(PIN_LED) == HIGH) digitalWrite(PIN_LED, LOW);
			else digitalWrite(PIN_LED, HIGH);
		}// else

		//Blink Yellow LED on Caution
		if (LEDFront.State == CAUTION){
			static bool last_front_state;

			if (last_front_state){
				setSignalLED(STOP, true, false, false);
				LEDFront.State = CAUTION;
				last_front_state = !last_front_state;
			}// if
			else{
				setSignalLED(CAUTION, true, false, false);
				last_front_state = !last_front_state;
			}// else
		}// if

		if (LEDBack.State == CAUTION){
			static bool last_back_state;

			if (last_back_state){
				setSignalLED(STOP, false, true, false);
				LEDBack.State = CAUTION;
				last_back_state = !last_back_state;
			}// if
			else{
				setSignalLED(CAUTION, false, true, false);
				last_back_state = !last_back_state;
			}// else
		}// if

		t_500ms = millis() + 500;
	}// if

	//Do Every 1000ms (1s)
	if (millis() >= t_1000ms) {
		//Send Life Telegram
		//sendLife();
		//actTime++;

		t_1000ms = millis() + 1000;
	}// if

	//Do Every 2000ms (2s)
	if (millis() >= t_2000ms) {
		//Set WiFi-Signal Strength LED
		setStatusLED(WiFi.RSSI());
		////check if Portexpander is still reachable
		//if (MCP23017.isAlive() == false) {
		//	//TODO - Generate ERROR-MESSAGE
		//	//I2C_PortExpander_Connection = false;
		//	errLog("Error on MCP23017 I2C-Connection", ERROR);
		//}// if
		//else {
		//	//errLog("MCP23017-Connected", INFO);
		//	I2C_PortExpander_Connection = true;
		//}// else

		t_2000ms = millis() + 2000;
	}// if

	//Do Every 5000ms (5s)
	if (millis() >= t_5000ms) {

		//calc runtime
		unsigned long diftime[10];
		//calc diference of timevalues
		for (int i = 0; i <= 18; i++){
			diftime[i] = runtime[i + 1] - runtime[i];
		}

		unsigned long tmptime = 0;
		//calc mean
		for (int i = 0; i <= 18; i++){
			
			tmptime = tmptime + diftime[i];
		}
	
		if (tmptime != 0){
			tmptime = tmptime / 19;
		}
		
		errLog("Taktzeit: " + String(tmptime) + "µs", INFO, true);
		t_5000ms = millis() + 5000;
	}// if

	//Do Every 10000ms (10s)
	if (millis() >= t_10000ms) {
		//Send Status Telegram
		sendStatus();

		t_10000ms = millis() + 10000;
	}// if

	 //Do Every t_300000ms (5m)
	if (millis() >= t_300000ms) {
		//Get Time from NTP-Server
		errLog("Get NTP Time", INFO, true);
		setDateTime();

		t_300000ms = millis() + 300000;
	}// if
	
}// loop

///<summary> Config Pins as In/Output and sets the PullUp resistors </summary>
void portInit() {
	errLog("->portInit", INFO, false);
	pinMode(PIN_SENSOR1, INPUT);
	pinMode(PIN_SENSOR2, INPUT);
	pinMode(PIN_BATVOLTAGE, INPUT);
	pinMode(PIN_PWM_SIGNAL, OUTPUT);
	pinMode(PIN_LED, OUTPUT);
	pinMode(PIN_PortExpander_Reset, OUTPUT);
}// portInit
							
 /// <summary>Initialize I2C Interface</summary>
 /// <returns>true init OK, false init NOK</returns>
bool i2cInit() {
	errLog("->i2cInit", INFO, false);
	bool NoError = true;

	if (DebugMode == true) {
		errLog("Initialize I2C Interface", INFO, false);
	}// if

	Wire.setClock(SPEED_I2C_CLOCK);
	Wire.begin(); // Connect the \B5C as master on I2C-bus
	MCP23017.init(ADRESS_I2C_PORTEXPANDER); // Initialize Connection to Portexpander
	MCP9843.init(ADRESS_I2C_TEMP);
	MCP79410.init(ADRESS_I2C_RTC, ADRESS_I2C_RTC_EEPROM);


	if (MCP23017.isAlive() == false) {
		NoError = false;
		errLog("Error on MCP23017 I2C-Connection", ERROR, true);
	}// if
	else {
		errLog("MCP23017-Connected", INFO, true);
		I2C_PortExpander_Connection = true;
	}// else

	if (MCP9843.isAlive() == false) {
		NoError = false;
		errLog("Error on MCP9843 I2C-Connection", ERROR, true);
	}// if
	else {
		errLog("MCP9843-Connected", INFO, true);
	}// else

	if (MCP79410.isAlive() == false) {
		NoError = false;
		errLog("Error on MCP79410 I2C-Connection", ERROR, true);
	}// if
	else {
		errLog("MCP79410-Connected", INFO, true);
	}// else

	return NoError;
}// i2cInit

 /// <summary>Initialize Serial Interface</summary>
void serialInit(int BAUDRATE) {
	Serial.begin(BAUDRATE);
	Serial_Connection = true;
}// serialInit

 /// <summary> Connects to the WiFi Network </summary>
 void wifiInit() {
	errLog("->wifiInit", INFO, false);
	String sBuf;

	sBuf = "Trying to connect to " + String(WIFI_AP_SSID);
	errLog(sBuf, INFO, false);

	WiFi.begin(WIFI_AP_SSID, WIFI_AP_PASSWORD);

	//Stop watchdog for connection
	//esp_task_wdt_delete(NULL);
	int maxwaittime = 0;

	while (WiFi.status() != WL_CONNECTED) {
		delay(250);
		Serial.print(".");
		maxwaittime++;

		//Exit after a wait time...
		if (maxwaittime >= 20){
			break;
		}// if
	}// while
 
	if (WiFi.status() == WL_CONNECTED) {
		errLog("SUCCESS", INFO, false);
		Wifi_Connection = true;

		// print the SSID of the network you're attached to:
		sBuf = "SSID: " + String(WiFi.SSID());
		errLog(sBuf, INFO, false);
	
		// print your WiFi shield's IP address:
		IPAddress ip = WiFi.localIP();
		WIFI_IP = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
		sBuf = "IP Address: " + WIFI_IP;
		errLog(sBuf, INFO, false);
	}// if
	else {
		errLog("FAILED", ERROR, false);
		Wifi_Connection = false;
	}// else
  }// wifiInit

   /// <summary> Connects to the MQTT-Broker </summary>
   /// <returns> true connecton = OK, false connection = NOK</returns>
 bool connectMqtt() {
     bool NoError = false;
 	 errLog("->connectMqtt", INFO, false);
	 
	 if (Wifi_Connection == true) {
		 String cBuf = "";
		 cBuf = "Connect to Broker: " + String(MQTT_BROKER_ADDRESS);
		 errLog(cBuf, INFO, false);

		 //Stop watchdog for connection
		 //esp_task_wdt_delete(NULL);

		 mqttClient.begin(MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT, wifiClient);

		 //esp_task_wdt_add(NULL);

		 mqttClient.onMessage(receiveMQTT);

		 if (mqttClient.connect("TestClient") == true) {
			 MQTT_Connection = true;
			 errLog("Connected to MQTT Broker", INFO, true);
			 mqttClient.subscribe(MQTT_TOPIC_SYSTEM_ID + "/" + MQTT_TOPIC_COMMAND  + "/" + MQTT_TOPIC_SIGNAL_ID);
			 NoError = true;
		 }// if

		 else {
			 errLog("Error on connect to MQTT", INFO, false);
		 }// else
	 }// if
	 return NoError;
 }// connectMQTT

  /// <summary> Send Message to MQTT-Broker </summary>
  /// <param name="MESSAGE"> Messages to be Send to Broker </param>
  /// <param name="TOPIC"> Topic where to Send Message in </param>
  /// <param name="QOS"> Quality of Service Level </param>
  /// <returns> true = sending OK, false = sending NOK</returns>
  /// <example> mqttSend( "MESSAGE", "TOPIC", QOS_AT_LEAST_ONE) </example>
 bool sendMQTT(String MESSAGE, String TOPIC, QOS_LEVEL QOS) {
	bool NoError = false;
	 if (MQTT_Connection == true) {
		 mqttClient.publish(TOPIC, MESSAGE);
	 }// if

	 return NoError;
}// sendMQTT

 /// <summary> Called on incoming Message from Broker </summary>
 /// <param name="topic"> Holds the topic of the incoming Message </param>
 /// <param name="payload"> Holds the payload of the incoming message </param>
 void receiveMQTT(String &topic, String &payload) {
	errLog("incoming: " + topic + " - " + payload, INFO, false);
	String rcvTopic = topic;
	String rcvPayload = payload;

	parseJSON(rcvPayload);
	
	//SIGNAL_STATES { DRIVE, STOP, SLOW, CAUTION, OFF, FULL}
	bool enFront = false;
	bool enBack = false;
	SIGNAL_STATES sigFront = STOP;
	SIGNAL_STATES sigBack = STOP;

	//Go through buffer and search for commands
	for (int y = 0; y < 10; y++) {
		if (jsonBuf[0][y] == "LED-FRONT") {
			enFront = true;
			//errLog("rcv Front: " + jsonBuf[1][y], INFO);
			if (jsonBuf[1][y] == "DRIVE") {
				sigFront = DRIVE;
			}//if
			else if (jsonBuf[1][y] == "STOP") {
				sigFront = STOP;
			}//else if
			else if (jsonBuf[1][y] == "SLOW") {
				sigFront = SLOW;
			}//else if
			else if (jsonBuf[1][y] == "CAUTION") {
				sigFront = CAUTION;
			}//else if
			else if (jsonBuf[1][y] == "OFF") {
				sigFront = OFF;
			}//else if
			else if (jsonBuf[1][y] == "ALL") {
				sigFront = ALL;
			}//else if
			else {
				errLog("Received SET Signal with wrong Syntax", WARN, true);
			}//else
		}//if
		else if (jsonBuf[0][y] == "LED-BACK") {
			enBack = true;

			if (jsonBuf[1][y] == "DRIVE") {
				sigBack = DRIVE;
			}//if
			else if (jsonBuf[1][y] == "STOP") {
				sigBack = STOP;
			}//else if
			else if (jsonBuf[1][y] == "SLOW") {
				sigBack = SLOW;
			}//else if
			else if (jsonBuf[1][y] == "CAUTION") {
				sigBack = CAUTION;
			}//else if
			else if (jsonBuf[1][y] == "OFF") {
				sigBack = OFF;
			}//else if
			else if (jsonBuf[1][y] == "ALL") {
				sigBack = ALL;
			}//else if
			else {
				errLog("Received SET Signal with wrong Syntax", WARN, true);
			}//else
		}//else if
	}
	
		//errLog("Front: " + String(enFront), INFO);
		//errLog("Sig: " + String(sigFront), INFO);
		//errLog("Back: " + String(enBack), INFO);
		//errLog("Sig: " + String(sigBack), INFO);
		setSignalLED(sigFront, enFront, false);
		setSignalLED(sigBack, false, enBack);
 }

 ///// <summary>Send LifeTelegram to Host</summary>
 ///// <returns>true = sending OK, false = sending NOK</returns>
 //bool sendLife() {

	// // errLog("->sendLife", INFO);
	// bool NoError = false;

	// //Inverts LifeBit
	// if (LifeBit == true) {
	//	 LifeBit = false;
	// }// if
	// else {
	//	 LifeBit = true;
	// }// else

	// if (MQTT_Connection == true) {

	//	 String Message[1];
	//	 String Category[1];

	//	 if (LifeBit == true) {
	//		 Message[0] = "1";
	//	 }// if
	//	 else {
	//		 Message[0] = "0";
	//	 }// else

	//	 Category[0] = "LIFE";

	//	 String Topic =  MQTT_TOPIC_SYSTEM_ID + "/" + MQTT_TOPIC_SIGNAL_ID + "/" + MQTT_TOPIC_LIFE;

	//	 NoError = sendMQTT(getJSON(Message, Category, 1), Topic, QOS_SEND_ONE);
	// }// if
	// return NoError;
 //}// sendLife

  /// <summary>Send LifeTelegram to Host</summary>
  /// <returns>true = sending OK, false = sending NOK</returns>
 bool sendStatus() {
	 errLog("->sendStatus", INFO, false);
	 //errLog("->sendStatus", INFO);
	 bool NoError = false;

	 if (MQTT_Connection == true) {

		 String Message[6];
		 String Category[6];

		 //String sBuf;

		 long rssi = WiFi.RSSI();

		 Category[0] = "IP";
		 Message[0] = WIFI_IP;

		 Category[1] = "LED-FRONT";
		 Message[1] = getStringState(LEDFront.State);

		 Category[2] = "LED-BACK";
		 Message[2] = getStringState(LEDBack.State);

		 Category[3] = "WIFI-LEVEL";
		 Message[3] = String(rssi);

		 Category[4] = "U-BAT";
		 Message[4] = String(getBatVoltage());

		 Category[5] = "TEMP";
		 Message[5] = String(MCP9843.getTemp());

		 String Topic =  MQTT_TOPIC_SYSTEM_ID + "/" + MQTT_TOPIC_STATUS + "/" + MQTT_TOPIC_SIGNAL_ID;

		 NoError = sendMQTT(getJSON(Message, Category, 6), Topic, QOS_AT_LEAST_ONE);
	 }// if
	 return NoError;
 }// sendLife


#pragma region LEDs
 /// <summary> Sets Wifi-Strength LEDs </summary>
 /// <param name="SIGNALSTRENGTH"> Signal Strength in dBm</param>
 /// <example> setStatusLED(-60) </example>
 void setStatusLED(long SIGNALSTRENGTH) {
	 //errLog("->setStatusLED", INFO);

	 if (I2C_PortExpander_Connection == true) {
		 if (SIGNALSTRENGTH > -60) {	 //green -> more than -30dBm
			 MCP23017.setOutput(0x00, PIN_PORTEXPANDER_PORTB_WIFI_GREEN, 0x00, 0xE0);
		 }// if
		 else if (SIGNALSTRENGTH > -67) {	//green + yellow -> more than -67dBm
											// MCP23017.setOutput(PIN_PORTEXPANDER_PORTB_WIFI_GREEN | PIN_PORTEXPANDER_PORTB_WIFI_YELLOW, 0x00, 0xE0, 0x00);
			 MCP23017.setOutput(0x00, PIN_PORTEXPANDER_PORTB_WIFI_GREEN | PIN_PORTEXPANDER_PORTB_WIFI_YELLOW, 0x00, 0xE0);
		 }// if
		 else if (SIGNALSTRENGTH > -70) {	//yellow -> more than -70dBm
			 MCP23017.setOutput(0x00, PIN_PORTEXPANDER_PORTB_WIFI_YELLOW, 0x00, 0xE0);
		 }// if
		 else if (SIGNALSTRENGTH > -80) {	//yellow + red -> more than -80dBm
			 MCP23017.setOutput(0x00, PIN_PORTEXPANDER_PORTB_WIFI_YELLOW | PIN_PORTEXPANDER_PORTB_WIFI_RED, 0x00, 0xE0);
		 }// if
		 else {							//red -> more than -90dbM
			 MCP23017.setOutput(0x00, PIN_PORTEXPANDER_PORTB_WIFI_RED, 0x00, 0xE0);
		 }// else
	 

		if (wifiClient.connected() == true) {
			MCP23017.setOutput(0x00, PIN_PORTEXPANDER_PORTB_CON_WIFI, 0x00, PIN_PORTEXPANDER_PORTB_CON_WIFI);
		}
	
		if (mqttClient.connected() == true) {
			MCP23017.setOutput(0x00, PIN_PORTEXPANDER_PORTB_CON_MQTT, 0x00, PIN_PORTEXPANDER_PORTB_CON_MQTT);
		}
	 }// if
 }// setStatusLED

 void setSignalLED(SIGNAL_STATES STATE, bool FRONT, bool BACK) {
	 setSignalLED(STATE, FRONT, BACK, true);
 }


  /// <summary> Sets Signal LEDs </summary>
  /// <param name="STATE"> Signal State to be set </param>
  /// <param name="FRONT"> true = Front will be set to the given state </param>
  /// <param name="BACK"> back = Front will be set to the given state </param>
  /// <param name="BRIGTHNESS"> Brigthness of the LED's </param>
  /// <param name="BLINK"> true = LED's will blink </param>
  /// <example> setSignalLED(DRIVE, true, true, 255, false) </example>
 void setSignalLED(SIGNAL_STATES STATE, bool FRONT, bool BACK, bool SENDSTATUS) {

	 if (FRONT == true) {
		 LEDFront.State = STATE;

		 switch (STATE) {
		 case DRIVE:
			 MCP23017.setOutput(PIN_PORTEXPANDER_PORTA_FRONT_GREEN, 0x00, 0x0F, 0x00);
			 break;
		 case STOP:
			 MCP23017.setOutput(PIN_PORTEXPANDER_PORTA_FRONT_RED1 | PIN_PORTEXPANDER_PORTA_FRONT_RED2, 0x00, 0x0F, 0x00);
			 break;
		 case SLOW:
			 MCP23017.setOutput(PIN_PORTEXPANDER_PORTA_FRONT_GREEN | PIN_PORTEXPANDER_PORTA_FRONT_YELLOW, 0x00, 0x0F, 0x00);
			 break;
		 case CAUTION:
			 MCP23017.setOutput(PIN_PORTEXPANDER_PORTA_FRONT_RED1 | PIN_PORTEXPANDER_PORTA_FRONT_RED2 | PIN_PORTEXPANDER_PORTA_FRONT_YELLOW, 0x00, 0x0F, 0x00);
			 break;
		 case OFF:
			 MCP23017.setOutput(0x00, 0x00, 0x0F, 0x00);
			 break;
		 case ALL:
			 MCP23017.setOutput(PIN_PORTEXPANDER_PORTA_FRONT_GREEN | PIN_PORTEXPANDER_PORTA_FRONT_RED1 | PIN_PORTEXPANDER_PORTA_FRONT_RED2 | PIN_PORTEXPANDER_PORTA_FRONT_YELLOW, 0x00, 0x0F, 0x00);
			 break;
		 }// switch
	 }// if

	 if (BACK == true) {
		 LEDBack.State = STATE;

		 switch (STATE) {
		 case DRIVE:
			 MCP23017.setOutput(PIN_PORTEXPANDER_PORTA_BACK_GREEN, 0x00, 0xF0, 0x00);
			 break;
		 case STOP:
			 MCP23017.setOutput(PIN_PORTEXPANDER_PORTA_BACK_RED1 | PIN_PORTEXPANDER_PORTA_BACK_RED2, 0x00, 0xF0, 0x00);
			 break;
		 case SLOW:
			 MCP23017.setOutput(PIN_PORTEXPANDER_PORTA_BACK_GREEN | PIN_PORTEXPANDER_PORTA_BACK_YELLOW, 0x00, 0xF0, 0x00);
			 break;
		 case CAUTION:
			 MCP23017.setOutput(PIN_PORTEXPANDER_PORTA_BACK_RED1 | PIN_PORTEXPANDER_PORTA_BACK_RED2 | PIN_PORTEXPANDER_PORTA_BACK_YELLOW, 0x00, 0xF0, 0x00);
			 break;
		 case OFF:
			 MCP23017.setOutput(0x00, 0x00, 0xF0, 0x00);
			 break;
		 case ALL:
			 MCP23017.setOutput(PIN_PORTEXPANDER_PORTA_BACK_GREEN | PIN_PORTEXPANDER_PORTA_BACK_RED1 | PIN_PORTEXPANDER_PORTA_BACK_RED2 | PIN_PORTEXPANDER_PORTA_BACK_YELLOW, 0x00, 0xF0, 0x00);
			 break;
		 }// switch
	 }// if

	 //send Status Message
	 if (SENDSTATUS){
	 	sendStatus();
	 }
}// setSignalLED

 /// <summary> Convert Data to JSON Telegram </summary>
 /// <param name="MESSAGE"> Messages to be Send to Host </param>
 /// <param name="CATEGORY"> Categorys where to Send Message in </param>
 /// <param name="ARRAYSIZE"> Size of the Message/Category Array </param>
 /// <returns> true sending OK, false sending NOK</returns>
 /// <example> getJSON(MESSAGE, CATEGORY, 2) </example>
 String getJSON(String* MESSAGE, String* CATEGORY, int ARRAYSIZE) {
	 //Serial.println("getJSON");
	String DateTime = getDateTime(SHORT);

	String OutMessage = "{\n\"UUID\" : \"" + MQTT_TOPIC_SIGNAL_ID + "\",\n";
	 
	OutMessage = OutMessage + "\"DATETIME\" : \"" + DateTime + "\",\n";
	
	for (int i = 0; i < ARRAYSIZE; i++) {
		if (i == ARRAYSIZE - 1) { //Last element need no ","
			OutMessage = OutMessage + "\"" + CATEGORY[i] + "\" : \"" + MESSAGE[i] + "\"\n";
		}// if
		else {
			OutMessage = OutMessage + "\"" + CATEGORY[i] + "\" : \"" + MESSAGE[i] + "\",\n";
		}// else
	}// for

	OutMessage = OutMessage + "}";

	return OutMessage;
 }// getJSON
 
  /// <summary> Converts JSON Telegram to Signal Commands </summary>
  /// <param name="MESSAGE"> JSON Input Message </param>
 void parseJSON(String MESSAGE) {
	 //jsonBuf[Name, Value][Element]
	 //First clean Array
	 for (int x = 0; x < 2; x++) {
		for (int y = 0; y < 10; y++){
			jsonBuf[x][y] = "";
		}//for
	 }//for

	 //Go trough Message and Add to Array
	 unsigned int index = 0;
	 int x = 0;
	 int y = 0;

	 while (index < MESSAGE.length()) {
		 int firstIndex = MESSAGE.indexOf('"', index + 1);
		 int nextIndex = MESSAGE.indexOf('"', firstIndex + 1);

		 //break when nothing found
		 if (nextIndex < 1) {
			 break;
		 }
		 else if (firstIndex < 1)
		 {
			 break;
		 }

		 //get Parameters from Payload String
		 jsonBuf[x][y] = MESSAGE.substring(firstIndex + 1, nextIndex);
		 x++;
		 if (x >= 2) {
			 y++;
			 x = 0;
		 }//if

		 //exit function if buffer is full
		 if (y >= 10) {
			 break;
		 }//if

		 index = nextIndex;
	 }//while
 }// parseJSON

 /// <summary> Enables Interrupts on Sensor Pins </summary>
 void isrEnable() {
	 attachInterrupt(PIN_SENSOR1, isrSensor1, RISING);
	 attachInterrupt(PIN_SENSOR2, isrSensor2, RISING);
 }// isrEnable

 /// <summary> Disables Interrupts on Sensor Pins </summary>
 void isrDisable() {
	 detachInterrupt(PIN_SENSOR1);
	 detachInterrupt(PIN_SENSOR2);
 }// isrDisable

 /// <summary> Will be called on Interrupt on Sensor1 </summary>
 void isrSensor1() {
	 t1 = millis();
	 newT1 = true;
 }// isrSensor1

 /// <summary> Will be called on Interrupt on Sensor2 </summary>
 void isrSensor2() {
	 t2 = millis();
	 newT2 = true;
 }// isrSensor2

 /// <summary> Calculates Speed and send it on MQTT </summary>
 void calcSpeed() {
	 	 errLog("-> calcSpeed", INFO, false);

		 String cMESSAGE[2];
		 String cCATEGORY[2];
		 String cTOPIC =  MQTT_TOPIC_SYSTEM_ID + "/" + "SPEED" + "/" + MQTT_TOPIC_SIGNAL_ID;

		 cCATEGORY[0] = "DIRECION";
		 cCATEGORY[1] = "SPEED [km/h]";
		 cMESSAGE[0] = "";
		 cMESSAGE[1] = "";

		 //calc Time difference
		 unsigned long diffTime = 0;
		 if (t1 >= t2) {
			 diffTime = t1 - t2;
			 cMESSAGE[0] = "FWD";
		 }// if
		 else {
				 diffTime = t2 - t1;
				 cMESSAGE[0] = "BWD";
		 }// else

		 //calc the Speed Value in m/s^2
		 float SpeedMS = sensorDif / diffTime;

		 float SpeedKMH = SpeedMS * 3.6;

		 cMESSAGE[1] = String(SpeedKMH);

		 //errLog("Speed: " + String(SpeedMS) + " m/s", INFO);
		 //errLog("Speed: " + String(SpeedKMH) + " km/h", INFO);

		 sendMQTT(getJSON(cMESSAGE, cCATEGORY, 2), cTOPIC, QOS_AT_LEAST_ONE);

		 // Reset Values
		 t1 = 0;
		 t2 = 0;
		 newT1 = 0;
		 newT2 = 0;
 }// calcSpeed

 void saveSettings() {
	/*
	MemoryMap
		0x00 -	0x01	16Zeichen	MQTT_TOPIC_SYSTEM_ID = "MANNHEIM001"
		0x02			4Byte	MQTT_BROKER_ADDRESS = "192.168.0.1"
		0x03 -	0x0A	64Zeichen	WIFI_AP_SSID[] = "RailWay"
		0x0B -	0x12	64Zeichen	WIFI_AP_PASSWORD[] = "RailWayPW"
		0x13			4Byte	NTP_SERVER = "192.168.0.1"
		0x14			4Byte	TIME_OFFSET = 0
		0x15			4Byte	MQTT_BROKER_PORT = 1883
		0x16			4Byte	SERIAL_BAUDRATE = 115200
		0x17			4Byte	sensorDif = 100
		0x18			1Bit	DebugMode = true
	*/
 }

 void loadSettings() {

 }

 /// <summary> Returns actual Date Time </summary>
 void setDateTime() {
	 ntpClient.setTimeOffset(TIME_OFFSET);
	 setTime(ntpClient.getUnixTime());

	 String result = MCP79410.setTime(second(), minute(), hour(), day(), month(), year() - 2000);
	 if (result != "") {
		 errLog(result, ERROR, false);
	 }
 }// setDateTime

 /// <summary> Returns actual Date Time </summary>
 /// <param name="FORMAT"> Output Date/Time Format </param>
 /// <returns> actual Date Time like "2018-06-26 20:19:30:1551</returns>
 /// <example> String DateTime = getDateTime(FULL) </example>
String getDateTime(DATE_TIME_FORMAT FORMAT) {
	//Get time from RTC
	MCP79410.getTime();
	setTime(MCP79410.hour, MCP79410.minute, MCP79410.second, MCP79410.day, MCP79410.month, MCP79410.year + 2000);
	
	String DateTime = "";
	String Year = String(year());
	String Month = String(month());
	if (month() < 10) {
		Month = "0" + Month;
	}
	String Day = String(day());
	if (day() < 10) {
		Day = "0" + Day;
	}
	String Hours = String(hour() + 1);
	if (hour() < 10) {
		Hours = "0" + Hours;
	}
	String Minutes = String(minute());
	if (minute() < 10) {
		Minutes = "0" + Minutes;
	}
	String Seconds = String(second());
	if (second() < 10) {
		Seconds = "0" + Seconds;
	}

	String sBuf = String(millis());
	char last = sBuf.charAt(sBuf.length() - 1);
	char first = sBuf.charAt(sBuf.length() - 2);

	String MILISECONDS = String(first) + String(last);
	
	String textbuf = "";

	switch (FORMAT)
	{
	case FULL:
		textbuf = Year + "/" + Month + "/" + Day + " " + Hours + ":" + Minutes + ":" + Seconds + "." + MILISECONDS;
		DateTime = textbuf;
		break;
	case SHORT:
		textbuf = Year + "/" + Month + "/" + Day + " " + Hours + ":" + Minutes + ":" + Seconds;
		DateTime = textbuf;
		break;
	case DATE:
		textbuf = Year + "/" + Month + "/" + Day;
		DateTime = textbuf;
		break;
	case TIME:
		textbuf =  Hours + ":" + Minutes + ":" + Seconds + "." + MILISECONDS;
		DateTime = textbuf;
		break;
	default:
		break;
	}//switch
	return DateTime;
}// getDateTime

 /// <summary> Sends errors to serial interface </summary>
 /// <param name="MESSAGE"> Errormessage </param>
 /// <param name="LEVEL"> Errorlevel </param>
 /// <param name="LEVEL"> true = Also send on MQTT </param>
 /// <example> errLog("Error on Connection", WARN) </example>
void errLog(String MESSAGE, ERR_CLASS LEVEL, bool MQTT) {
	boolean send = false;
	String sLevel;

	switch (LEVEL) {
	case INFO:
		if (DebugMode == true) {
			send = true;
		}// if
		sLevel = "INFO";
		break;
	case WARN:
		if (DebugMode == true) {
			send = true;
		}// if
		sLevel = "WARN";
		break;
	case ERROR:
		send = true;
		sLevel = "ERROR";
		break;
	case FATAL:
		send = true;
		sLevel = "FATAL";
		break;
	}// switch

	String DateTime = getDateTime(FULL);
	String MessageBuf = DateTime + " - " + sLevel + ": " + MESSAGE + "\n";

	if (Serial_Connection == true && send == true) {
		Serial.println(MessageBuf);
	}// if

	if (mqttClient.connected() == true && send == true && MQTT == true) {
		String Message[2];
		String Category[2];

		//String sBuf;

		long rssi = WiFi.RSSI();

		Category[0] = "ERR-CLASS";
		Message[0] = sLevel;

		Category[1] = "ERR-MSG";
		Message[1] = MESSAGE;

		String Topic = MQTT_TOPIC_SYSTEM_ID + "/" + MQTT_TOPIC_SIGNAL_ID + "/" + MQTT_TOPIC_ERROR;

		sendMQTT(getJSON(Message, Category, 2), Topic, QOS_SEND_ONE);
	}
}// errLog

float getBatVoltage() {
	float BatVoltage = 0;

	analogReadResolution(10); //Sets the Resolution of ADC in bits

	unsigned int aVal = analogRead(PIN_BATVOLTAGE);//PIN_BATVOLTAGE);

	//Convert value to inputVoltage
	BatVoltage = 0.0377 * aVal + 1.369;

	return BatVoltage;
}// getBatVoltage

String getStringState(SIGNAL_STATES STATE){
	String returnString = "";

	switch (STATE){
		case DRIVE: returnString = "DRIVE"; break;
		case STOP: returnString = "STOP"; break;
		case SLOW: returnString = "SLOW"; break;
		case CAUTION: returnString = "CAUTION"; break;
		case OFF: returnString = "OFF"; break;
		case ALL: returnString = "ALL"; break;
	}// switch
	return returnString;
}// getStringState

void readSerialData() {
	bool Error = false;

	String strBuf = Serial.readString();

	if (strBuf.indexOf("config") != -1) {
		//Search for Command
		int cmdPos = strBuf.indexOf("-");
		if (cmdPos == -1) {
			Error = true;
			goto ON_ERROR;
		}


		//get command
		char command = strBuf.charAt(cmdPos + 1);

		//Search for Value
		int valPos = strBuf.indexOf("""", cmdPos + 1);
		int valPosEnd = strBuf.indexOf("""", valPos + 1);
		if (valPos == -1 || valPosEnd == -1) {
			Error = true;
			goto ON_ERROR;
		}
		//get value
		String value = strBuf.substring(valPos + 1, valPosEnd - 1);
		String nothing = "";
		String sBuf = "";

		switch (command) {
		case 'a': //Set Accesspoint Name
			//value.toCharArray(WIFI_AP_SSID, value.length);
			sBuf = "SET AP Name to " + value;
			errLog(sBuf, INFO, true);
			break;
		case 'p': //Set Accesspoint Password
			//value.toCharArray(WIFI_AP_PASSWORD, value.length);
			sBuf = "SET AP Password to " + value;
			errLog(sBuf, INFO, true);
			break;
		case 'm': //Set MQTT-Broker IP
			//value.toCharArray(MQTT_BROKER_ADDRESS, value.length);
			sBuf = "SET Broker IP to " + value;
			errLog(sBuf, INFO, true);
			break;
		case 'o': //Set MQTT-Broker Port
			MQTT_BROKER_PORT = value.toInt();
			sBuf = "SET Broker Port to " + value;
			errLog(sBuf, INFO, true);
			break;
		case 's': //Save Settings in EEPROM
			saveSettings();
			sBuf = "Saved Settings to ROM";
			errLog(sBuf, INFO, true);
			break;
		case 'r': //Reset Settings
			nothing.toCharArray(WIFI_AP_SSID, 0);
			nothing.toCharArray(WIFI_AP_PASSWORD, 0);
			MQTT_BROKER_ADDRESS = "";
			MQTT_BROKER_PORT = 1883;
			sBuf = "Reset everything to default" + value;
			errLog(sBuf, INFO, true);
			break;
		case 'd': //Switch Debug Mode
			if (DebugMode == true) {
				errLog("DebugMode Off", INFO, true);
				DebugMode = false;
			}
			else {
				DebugMode = true;
				errLog("DebugMode On", INFO, true);
			}
			break;
		default: Error = true;
		}
	}// if
	else {
		Error = true;
	}// else

	ON_ERROR:
	if (Error == true) {
		errLog("Can't read input!", ERROR, false);
	}// if
}// readSerialData

void scanI2C() {
	for (int i = 0; i < 0b01111111; i++) {
		Wire.beginTransmission(i);
		byte Array[1] = { 0x10 };
		Wire.write(Array, 1);
		if (Wire.endTransmission(true) != 2) {
			String sBuf = "Found Device on I2C: " + String(i, BIN);
			errLog(sBuf, INFO, true);
		}// if
	}// for
}// scanI2C