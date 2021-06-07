#ifndef _BLE_H
#define _BLE_H

#include <Arduino.h>   //for Serial
//include <Tympan_Library.h>
#include "bc127.h"
#include "Tympan.h"

class BLE : public BC127
{
public:
    BLE(Stream *sp) : BC127(sp) {}
	int begin(void);
	void setupBLE(Tympan &);  //to be called from the Arduino sketch's setup() routine.  Includes error reporting to Serial
    size_t sendByte(char c);
    size_t sendString(const String &s);
    size_t sendMessage(const String &s);
	//size_t sendMessage(const char* c_str, const int len); //use this if you need to send super long strings (more than 1797 characters)
    size_t recvMessage(String *s);
    size_t recvBLE(String *s);
	bool isAdvertising(bool printResponse = false);
    bool isConnected(bool printResponse = false);
    bool waitConnect(int time = -1);
	void updateAdvertising(unsigned long curTime_millis, unsigned long updatePeriod_millis = 3000);
};

#endif