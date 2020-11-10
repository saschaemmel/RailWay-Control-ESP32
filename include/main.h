#pragma once

#ifndef _main_h
#define _main_h

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "WProgram.h"
#endif

enum QOS_LEVEL { QOS_SEND_ONE, QOS_AT_LEAST_ONE, QOS_EXACTLY_ONE };	//Quality of Service Level for MQTT
enum ERR_CLASS { INFO, WARN, ERROR, FATAL };						//Diferent Err-Classes for Logging
enum DATE_TIME_FORMAT { FULL, SHORT, DATE, TIME };					//Sets which Date/Time Format will be Used
enum SIGNAL_STATES { DRIVE, STOP, SLOW, CAUTION, OFF, ALL};				//Sets possible Signal States


void portInit();
bool i2cInit();
void serialInit(int BAUDRATE);
void wifiInit();

bool connectMqtt();
bool sendMQTT(String MESSAGE, String TOPIC, QOS_LEVEL QOS) ;
void receiveMQTT(String &topic, String &payload) ;
bool sendStatus() ;

void setStatusLED(long SIGNALSTRENGTH) ;
void setSignalLED(SIGNAL_STATES STATE, bool FRONT, bool BACK);
void setSignalLED(SIGNAL_STATES STATE, bool FRONT, bool BACK, bool SENDSTATUS);

String getJSON(String* MESSAGE, String* CATEGORY, int ARRAYSIZE);
void parseJSON(String MESSAGE) ;

void isrEnable();
void isrDisable();
void isrSensor1();
void isrSensor2();

void calcSpeed();

void saveSettings() ;
void loadSettings() ;

void setDateTime();
String getDateTime(DATE_TIME_FORMAT FORMAT);

void errLog(String MESSAGE, ERR_CLASS LEVEL, bool MQTT);

float getBatVoltage();
String getStringState(SIGNAL_STATES STATE);

void readSerialData();

void scanI2C();

#endif