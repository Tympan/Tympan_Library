/* 
	SerialManagerBase.h
	
	Created: Chip Audette, OpenAudio, 2021
	
	Purpose: Be the base class for implementing a SerialManager in your Tympan sketch.
		This base class encapsulates much of the functionality that is likely helpful
		to everyone's SerialManager implementation.  Why recreate the wheel every time?
		This base class already implements the most commonly needed portions of that wheel!
	   
	BLE Communication: By using this base class, you can re-use the BLE communication
		methods without having to implement them yourself.  They're not complicated, but
		why go through the effort.  In particular, this base class provides:
		   * setButtonState
		   * setButtonText
		This is a pretty minor help to you, but it'll save you a little bit of work.
		   
	Compliant "UI" elements:  The most important part of this class is that it helps
		automate a range of common communication methods from the App to the Tympan.  The
		different methods are briefly introduced later in this comment block.  The important
		part is that this base class automates the processing of these different modes if
		you make your own classes (ie, your Audio processing classes who want to be controlled
		from the App or from the SerialMonitor) compatible with how this SerialManagerBase
		is designed to work.  If your classes are compatible, you get access to all the automated
		processing, which will save you a lot of time and effort.  

		You get the benefit of the pre-written routines in this Base class if your own classes
		are able to tell this SerialManagerBase that they exist and that they can respond
		appropriately.  To do this, you must:
	   
		* Your audio processing class (or whatever) must inherit from SerialManager_UI
			and implement the methods required by that SerialManager_UI.  These are 
			methods like printHelp() and processCharacterTriple() and setFullGUIState().
			Several of the Tympan_Library's most important classes already have these 
			SerialManager_UI methods implemented, so you don't have to do anything!
		* In your sketch (in your program), you register each instance of your class 
			with your SerialManager via the SerialManagerBase method add_UI_element().
			This tells the SerialManager that your class instances exist and that they
			are ready to interact!
		* In your sketch (in your program), you create a Tympan Remote GUI, and that
			the command characters that you set for each GUI element agree with the
			command characters that you wrote when you implemented your class's 
			processCharacterTriple().  Or, if you're using one of the Tympan_Library classes
			that alrady implement SerialManager_UI, it probably already has an App GUI that
			has been pre-written for you!  You can simply invoke those pre-written elements!
			 
	Communication Methods: At its core, the App communicates one or more text characters
		back to the Tympan.  Given that it must be characters, you can implement any protocol
		that you would like.  But, if you want to build upon the existing example programs
		and if you want to build upon the automated routines in this base class, the primary
		communication approaches are:
		
		* Single Character Commands:  The most basic approach is for you to create a
			button in the App whose command is a single character.  For example, you might
			create a button that sends a 'c' back to to the Tympan.  Typically, these single-
			character commands are interpretted in code that you write in your own
			implementation of SerialManager.  This single character can also come from a user
			typing it into the SerialMonitor.  All of the Tympan_Library examples treat characters
			coming in from the USB Serial link as the same as characters coming in from the 
			Bluetooth connection.
		* Four-Character Commands:  Most of the automated communcation takes place using
			a four-character command.  Each button in the App GUI would be told to transmit
			a four characters instead of just one.  The first character is a special character
			that identifies it as a four-character command. By default, this special character
			is an underscore ('_').  If this SerialManagerBase.respondToByte() sees an underscore
			it will assume that the next three characters form the rest of the command (with
			exceptions...see the code of respondToByte()).  The three characters are sent to
			each of the classes that have been registered with the SerialManagerBase via the
			processCharacterTriple.  Typically, the first of three characters identifies the
			command with a particular instance of your class.  Then, the second and third
			characters can be interpretted by the class however you'd like.
		* DataStreams: Besides the single-character and four-character modes, there are also
			data streaming modes to support specialized communication.  These special modes
			are not inteded to be invoked by a user's GUI, so they can be ignored.  To avoid
			inadvertently invoking these modes, never send characters such as 0x02, 0x03, 0x04.
			In fact, you should generally avoid any non-printable character or you risk seeing
			unexpected behavior.

			Datastreams expect the following message protocol.  Note that the message length and
			payload are sent as little endian (LSB, MSB):
			 	1.	DATASTREAM_START_CHAR 	(0x02)
				2.	Message Length (int32): number of bytes including parts-4 thru part-6
				3.	DATASTREAM_SEPARATOR 	(0x03)
				4.	Message Type (char): Avoid using the special characters 0x03 or 0x04.  (if set
							to ‘test’, it will print out the payload as an int, then a float)
				5.	DATASTREAM_SEPARATOR 	(0x03)
				6.	Payload
				7.	DATASTREAM_END_CHAR 	(0x04)

			Use RealTerm to send a 'test' message: 
			0x02 0x0D 0x00 0x00 0x00 0x03 0x74 0x65 0x73 0x74 0x03 0xD2 0x02 0x96 0x49 0xD2 0x02 0x96 0x49 0x04
				1. DATASTREAM_START_CHAR 	(0x02)
				2.	Message Length (int32): (0x000D) = 13 
				3.	DATASTREAM_SEPARATOR 	(0x03)
				4.	Message Type (char): 	(0x74657374) = 'test'
				5.	DATASTREAM_SEPARATOR 	(0x03)
				6.	Payload					(0x499602D2, 0x499602D2) = [1234567890, 1234567890]
				7.	DATASTREAM_END_CHAR 	(0x04)

			Expected output
				- SerialManagerBase: RespondToByte: Start data stream.
				- SerialManagerBase: RespondToByte: Stream length = 13
				- SerialManagerBase: RespondToByte: Time to process stream!
					- Stream is of type 'test'.
						- int is 1234567890
						- float is 1228890.25

			To register a callback when a datastream message is received, use setDataStreamCallback()
			and set a unique message type (i.e. not 'gha', 'dsl', afc', or 'test'):
 			- `serialManager.setDataStreamCallback(&dataStreamCallback);`
			- `void dataStreamCallback(char* payload_p, String *msgType_p, int numBytes)`

	License: MIT License.  Use at your own risk.  Have fun!
*/


#ifndef SerialManagerBase_h
#define SerialManagerBase_h

#include <Arduino.h> //for Serial and String
#include "BLE/ble.h"
#include "BLE/BleTypes.h"
#include <vector>
//#include "SerialManager_UI.h"
class SerialManager_UI;  //forward declare.  Assume SerialManager_UI.h will be included elsewhere

#define MAX_DATASTREAM_LENGTH 1024
#define MAX_BLE_PAYLOAD (MAX_DATASTREAM_LENGTH) //for safety
#define DATASTREAM_START_CHAR (char)0x02
#define DATASTREAM_SEPARATOR (char)0x03
#define DATASTREAM_END_CHAR (char)0x04
#define QUADCHAR_START_CHAR     ((char)'_')   //this is an underscore
#define QUADCHAR_CHAR__USE_PERSISTENT_MODE ((char)'_')  //this is an underscore

#define SERIALMANAGERBASE_MAX_UI_ELEMENTS 30

typedef void(*callback_t)(char* payload_p, String* msgType_p, int numBytes);	//typedef for datastream callback pointer 

class SerialManagerBase {
  public:
    SerialManagerBase(void) {common_setup(); };
    SerialManagerBase(BLE *_ble) :
      ble(_ble) { common_setup(); };
			
		void common_setup(void) {
			bleDataMessageQueue.setMaxAllowedSize(16); //maximum number of BLE messages that can be queued
		}

    virtual BLE *setBLE(BLE *_ble) { return ble = _ble; };
    virtual BLE *getBLE(void) { return ble; };
    
		virtual void printHelp(void);
    virtual void respondToByte(char c);
    virtual void setButtonState(String btnId, bool newState, bool sendNow = true);
    virtual void setButtonText(String btnId, String text);
    virtual void sendTxBuffer(void);
		virtual void setFullGUIState(bool activeButtonsOnly=false); 

		enum read_state_options {
			SINGLE_CHAR,	  STREAM_LENGTH,	  STREAM_DATA,	  QUAD_CHAR_1,	  QUAD_CHAR_2,	  QUAD_CHAR_3
		};
		
		void setDataStreamCallback(callback_t callBackFunc_p);

		//handle BLE messages
		SerialManager_UI* add_UI_element(SerialManager_UI *);
		int isBleDataMessageAvailable(void) { return bleDataMessageQueue.size(); }
		BleDataMessage getBleDataMessage(void);
		uint32_t setMaxAllowedBleMessageQueueSize(uint32_t size) { return bleDataMessageQueue.getMaxAllowedSize(); }


  protected:
    virtual bool processCharacter(char c);
    virtual void processStream(void);
    virtual bool interpretQuadChar(char mode_char, char chan_char, char data_char); 
		virtual int interpretBleData(int idx);
    virtual int readStreamIntArray(int idx, int* arr, int len);
    virtual int readStreamFloatArray(int idx, float* arr, int len);
		virtual void printStreamData(int idx);
    
    int serial_read_state; // Are we reading one character at a time, or a stream?
    char stream_data[MAX_DATASTREAM_LENGTH];
		//size_t ble_n_bytes = 0;
		//uint8_t ble_payload[MAX_BLE_PAYLOAD];
    int stream_length = 0;
    int stream_chars_received;
    BLE *ble;
    char GUI_persistent_mode = 'g';  //is this used?  I don't think so.
    String TX_string;
    char mode_char, chan_char, data_char; //for quad_char processing
		callback_t _datastreamCallback_p = NULL;

		std::vector<SerialManager_UI *> UI_element_ptr;
		BleDataMessageQueue bleDataMessageQueue; // Like a std::queue, but limits the number of elements.  See BLE/BleTypes.h

};



#endif