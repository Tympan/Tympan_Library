/*
	Core header file for BC127 module support

	Created: Ira Ray Jenkins, Creare
	Purpose: Wrap required bluetooth functionality of Tympan's integrated bc127
             module.

             Based on: SparkFun BC127 Bluetooth Module Arduino Library
             https://github.com/sparkfun/SparkFun_BC127_Bluetooth_Module_Arduino_Library

	License: MIT License.  Use at your own risk.
*/
#ifndef _BC127_H_
#define _BC127_H_

#include <Arduino.h>

class BC127
{
public:
    // data type for function results
    enum opResult
    {
        TIMEOUT_ERROR = -1, // took too long
        MODULE_ERROR,       // generic error
        SUCCESS             // everything is good
    };

    BC127(Stream *sp, unsigned long timeout = 4000) : _serialPort(sp), _timeout(timeout)
    {
        _cmdResponse = String("");
    };

    int available();                                 // returns number of bytes available on internal stream
    int read();                                      // read a byte from internal stream
    size_t print(char c);                            // print a byte to internal stream
    String readString(size_t max = 120);             // read a string from internal stream
    size_t printString(const String &s);             // print a string to internal stream
    void setTimeout(unsigned long timeout);          // set read timeout
    opResult advertise(bool mode = true);            // starts | stops BLE advertising
    opResult close(int linkid = -1);                 // closes Bluetooth connection
    opResult discoverable(bool mode = true);         // puts device in discoverable mode
    opResult enterDataMode();                        // enters Data mode, does not work in BLE
    opResult exitDataMode(int guardDelay);           // exits Data mode
    opResult getConfig(String config = "");          // returns all or specific config options
    String getCmdResponse();                         // returns the command response string
    Stream *getSerial();                             // returns _serialPort
    opResult help();                                 // prints the help
    opResult power(bool mode = true);                // power cycles the device
    opResult recv(String *msg);                      // grab serial response, by line
    opResult reset();                                // resets the device
    opResult restore();                              // restores the device to factory defaults
    opResult send(String str);                       // sends a string over the connection profile
    opResult status();                               // returns the connections status
    opResult stdCmd(String cmd);                     // executes a standard command option
    opResult setConfig(String config, String param); // sets a configuration register
    opResult version();                              // returns the version info in _cmdResponse
    opResult writeConfig();                          // writes the current config to non-volatile memory

protected:
    // end-of-line delimiter
    const String EOC = String("\r");
    // end-of-command delimiter
    const String EOL = String("\n\r");

    Stream *_serialPort;    // port to talk on
    String _cmdResponse;    // response from commands
    unsigned long _timeout; // timeout for command wait

private:
    opResult knownStart();                   // baseline starting function
    opResult waitResponse(int timeout = -1); // wait for a full response
};

#endif // _BC127_H_