#ifndef _BLE_H
#define _BLE_H

#include <Arduino.h>

#include <Tympan_Library.h>

class BLE : public BC127
{
public:
    BLE(Stream *sp);
	int begin(void);
    size_t sendByte(char c);
    size_t sendString(const String &s);
    size_t sendMessage(const String &s);
    size_t recvMessage(String *s);
    size_t recvBLE(String *s);
    bool isConnected(bool printResponse = false);
    bool waitConnect(int time = -1);
};

#endif