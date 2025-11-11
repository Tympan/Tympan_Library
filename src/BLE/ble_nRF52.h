#ifndef _BLE_NRF52_H
#define _BLE_NRF52_H

/* //////////////////////////////////////////////////////////////
*
* BLE_nRF52
*
* Created: Chip Audette, OpenAudio
*
* Purpose: Class for implementing BLE communcation functions for the nRF52840 BLE module
*
* MIT License, Use at your own risk.
*
////////////////////////////////////////////////////////////// */

#include <Arduino.h>   //for Serial
#include <Tympan.h>
#include <SerialManager_UI.h>
#include "BLE/ble.h"


class BLE_nRF52: virtual public BLE
{
public:
	BLE_nRF52(HardwareSerial *sp) : serialToBLE(sp), serialFromBLE(sp) {};
	BLE_nRF52(TympanBase *_tympan) : serialToBLE(_tympan->BT_Serial), serialFromBLE(_tympan->BT_Serial) {};
	BLE_nRF52(void) {} //uses default serial ports set down in the protected section

	//setup the BLE module and start it
	virtual void setupBLE(void) { setupBLE(10); } 
	void setupBLE(int BT_firmware) override { setupBLE(BT_firmware,false); };     //to be called from the Arduino sketch's setup() routine.  Includes factory reset.
	void setupBLE(int BT_firmware, bool printDebug) override { setupBLE(BT_firmware, printDebug, false); };            //to be called from the Arduino sketch's setup() routine.  Includes factory reset.
	virtual void setupBLE(int BT_firmware, bool printDebug, int doFactoryReset);  //to be called from the Arduino sketch's setup() routine
	int begin(void) override {  return begin(false); }                            //normal begin()
	virtual int begin_basic(void);                                                //only use if you're really sure that you know what you're doing
	virtual int begin(int doFactoryReset);

	//low-level methods to receive bytes from the BLE module via the serial link to the BLE module
	int available(void) override { return serialFromBLE->available(); }
	virtual int read(void) override { return serialFromBLE->read(); };
	virtual int peek(void) override { return serialFromBLE->peek(); }
	//virtual size_t recvMessage(String *s_ptr);

	//generic methods to commands to the BLE module
	virtual size_t sendCommand(const String &cmd,const String &data);
	virtual size_t sendCommand(const String &cmd, const char* data, size_t data_len);
	virtual size_t sendCommand(const String &cmd, const uint8_t* data, size_t data_len);

	//helper methods for simplfying using SET or GET commands with the BLE module
	virtual int sendSetForIntegerValue(const String &parameter_str, const int value);
	virtual int sendGetForIntegerValue(const String &parameter_str);

	//methods for doing specific actions to the BLE module or asking for specific information from the module
	virtual int setBleName(const String &s);
	virtual int getBleName(String *reply_ptr);
	virtual int setBleMac(const String &s);
	//virtual int getBleMac(const String &s);
	int version(String *reply_ptr) override;
	virtual int setLedMode(int val) { return sendSetForIntegerValue("LEDMODE", val); };
	virtual int getLedMode(void) { return sendGetForIntegerValue("LEDMODE"); };
	
	//check connection and advertising status
	int isConnected(void) override { return isConnected(GET_AUTO); }
	virtual int isConnected(int method);
	int isAdvertising(void) override;
	int enableAdvertising(bool);
	int PIN_IS_CONNECTED = 39;  //Tympan RevF
	enum GET_TYPE { GET_AUTO=0, GET_VIA_SOFTWARE, GET_VIA_GPIO};

	// These do nothing but are needed for compatibility with ble.h
	void updateAdvertising(unsigned long curTime_millis, unsigned long updatePeriod_millis = 5000) override { updateAdvertising(curTime_millis, updatePeriod_millis, false); }
	void updateAdvertising(unsigned long curTime_millis, unsigned long updatePeriod_millis, bool printDebugMsgs) override {}; //do nothing, already auto-advertises after disconnect
	bool setUseFasterBaudRateUponBegin(bool enable = true) override { return enable; };
	//end do-nothing methods

	//bool simulated_Serial_to_nRF = true;


	// /////////////////////////////////// Methods for UART-style comms over BLE to the remote device
	
	//send strings
	virtual size_t send(const String &str);  //the main way to send a string 
	virtual size_t send(const char c) { return send(String(c)); }
	size_t sendString(const String &s, bool print_debug) override;
	virtual size_t sendString(const String &s) { return sendString(s,false);}

	//send strings, but add formatting for TympanRemote App
	size_t sendMessage(const String &s) override { return transmitMessage(s, false); }; //the "false" is so that it uses SEND, not QUEUE
	virtual size_t sendMessage(char c) { return sendMessage(String(c)); }

	//Define the "queue" methods, similar to the collection of "send" methods.
	//The "send" methods send the packets immediately, and will block for a set period of time to allow them to go.
	//The "queue" methods put the messages in the BLE module's internal queue and the BLE module handles the pacing
	//of the messages so that the Tympan doesn't have to wait, like it does while "send" does its blocking.
	virtual size_t queue(const String &str);  //the main way to send a string 
	virtual size_t queue(const char c) { return queue(String(c)); }
	virtual size_t queueString(const String &s, bool print_debug);
	virtual size_t queueString(const String &s) { return queueString(s,false);}
	virtual size_t queueMessage(const String &s) { return transmitMessage(s, true); }; //the "true" is so that it uses QUEUE, not SEND
	virtual size_t queueMessage(const char c) { return queueMessage(String(c)); }
		
	//receive reply from BLE unit (formatted with "OK" or "FAIL")
	unsigned long rx_timeout_millis = 2000UL;
	virtual size_t recvReply(String *s_ptr, unsigned long timeout_mills);
	virtual size_t recvReply(String *s_ptr) { return recvReply(s_ptr, rx_timeout_millis); }
	static bool doesStartWithOK(const String &s);

	//The BLE module has its own TX queue (invoked when using the queueMessage() command (or other queue command).
	//These methods help manage how the queue is used.  Again, this is about a queue that's on the BLE
	//module, not a queue here on the Tympan
	virtual int setTxQueueDelay_msec(const int delay_msec);  //a non-zero return value is a failed communication
	virtual int getTxQueueDelay_msec(void) { return sendGetForIntegerValue("DELAY_MSEC"); }         //a negative return value is a failed communication
	virtual int getTxQueueFreeBytes(void)  { return sendGetForIntegerValue("TX_QUEUE_FREEBYTES"); };//a negative return value is a vailed communication
	virtual int getTxQueueNumMsgs(void)    { return sendGetForIntegerValue("TX_QUEUE_N_MSGS"); };   //a negative return value is a failed communication
	virtual int clearTxQueue(void) { String reply_str; return clearTxQueue(&reply_str); }
	virtual int clearTxQueue(String *reply_ptr);  //as an important side effect, this stops the BLE module from transmitting any of these queued messages
	
	
	// ////////////////////////////////////// Methods for BLE-type comms (not UART-style messages) to the remote device
		
	//receive methods required by BLE interface
	virtual int recv(String *s_ptr);                      // grab serial response, by line
	virtual size_t recvBLE(String *s_ptr) { return recvBLE(s_ptr, false); };
  virtual size_t recvBLE(String *s_ptr, bool printResponse);
	
	//preset BLE services`
	enum PRESET_BLE_SERVICES {BLESVC_DFU=0, 
													BLESVC_DIS=1, 
													BLESVC_UART_TYMPAN=2, 
													BLESVC_UART_ADAFRUIT=3,
													BLESVC_BATT=4,
													BLESVC_LEDBUTTON=5,
													BLESVC_LEDBUTTON_4BYTE=6,
													BLESVC_GENERIC_1=7,
													BLESVC_GENERIC_2=8};
	virtual int enableServiceByID(int service_id, bool enable);
	virtual int enableAdvertiseServiceByID(int service_id);
	virtual int notifyBle(int service_id, int char_id, float32_t val);
	virtual int writeBle (int service_id, int char_id, float32_t val);
	virtual int notifyBle(int service_id, int char_id, int32_t val);
	virtual int writeBle (int service_id, int char_id, int32_t val);	
	virtual int notifyBle(int service_id, int char_id, uint32_t val);
	virtual int writeBle (int service_id, int char_id, uint32_t val);	
	virtual int notifyBle(int service_id, int char_id, uint8_t val)   { return notifyBle(service_id, char_id, &val, 1); };
	virtual int writeBle (int service_id, int char_id, uint8_t val)   { return writeBle (service_id, char_id, &val, 1); };
	virtual int notifyBle(int service_id, int char_id, char val)      { return notifyBle(service_id, char_id, (uint8_t)val); };
	virtual int writeBle (int service_id, int char_id, char val)      { return writeBle (service_id, char_id, (uint8_t)val); };		
	virtual int notifyBle(int service_id, int char_id, const uint8_t vals[], size_t n_bytes);
	virtual int writeBle (int service_id, int char_id, const uint8_t vals[], size_t n_bytes);
	
	
	
protected:
	//virtual bool useFasterBaudRateUponBegin = false; //default as to whether to use faster baud rate or not
	//virtual void setSerialBaudRate(int new_baud);
	//virtual int hardwareFactoryReset(bool printDebug = false);
	//virtual void switchToNewBaudRate(int new_baudrate);

	const String EOC = String('\r');
	#if defined(KINETISK)   //for Teensy 3.x only!  which should never happen for RevF
	  HardwareSerial *serialToBLE = &Serial1;  		//dummy value.  This branch should never happen.  This value is here just to avoid a compiler warning
	  HardwareSerial *serialFromBLE = &Serial1;   //dummy value.  This branch should never happen.  This value is here just to avoid a compiler warning
	#else
	  HardwareSerial *serialToBLE = &Serial7;    //Tympan design uses Teensy's Serial1 to connect to BLE
	  HardwareSerial *serialFromBLE = &Serial7;  //Tympan design uses Teensy's Serial1 to connect from BLE
	#endif
	int isConnected_getViaSoftware(void);
	int isConnected_getViaGPIO(void);
	
	//method holding common activities for actions relating ot SEND and QUEUE 
	virtual size_t transmitMessage(const String &s, bool flag_useQueue);

	
	unsigned long _timeout = 2000; // timeout for recv
};

//class BLE_UI : public BLE, public SerialManager_UI
class BLE_nRF52_UI : public BLE_nRF52, virtual public BLE_UI 
{
	public:
		BLE_nRF52_UI(TympanBase *_tympan) : BLE_UI(), BLE_nRF52(_tympan) {}
		BLE_nRF52_UI(HardwareSerial *sp) : BLE_UI(), BLE_nRF52(sp) {}

		// ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void);
		//virtual bool processCharacter(char c); //not used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
		virtual void setFullGUIState(bool activeButtonsOnly = false) {};
		// ///////// end of required methods
		virtual bool processSingleCharacter(char data_char);

};

class BLE_nR52_CustomService {
	public:
		BLE_nR52_CustomService(BLE_nRF52 *foo_nRF52, int _svc_id) {
			this_nRF52 = foo_nRF52;
			service_id = _svc_id;
		}
		virtual ~BLE_nR52_CustomService(void) {}
		int setServiceUUID(const String &uuid);
		int setServiceName(const String &name);
		int addCharacteristic(const String &uuid); 
		int setCharName(const int char_id, const String &name);
		int setCharProperties(const int char_id, const String &char_props);
		int setCharNBytes(const int char_id, const int n_bytes);
		
	protected:
		BLE_nRF52 *this_nRF52 = nullptr;
		int service_id = 7; //will get overwritten in the constructor
		
	
};

#endif