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
		   
	Compliant "UI" elements:  The bulk of this base class is aimed at implementing a
	   range of common communication methods from the App to the Tympan.  You get the
	   benefit of these pre-written routines if your own Audio classes are able to
	   tell this SerialManagerBase that they exist and that they can respond appropriately.
	   This means that:
	   
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
			 
	License: MIT License.  Use at your own risk.  Have fun!

*/


#ifndef SerialManagerBase_h
#define SerialManagerBase_h

#include <Arduino.h> //for Serial and String
#include "BLE/ble.h"
//#include "SerialManager_UI.h"
class SerialManager_UI;  //forward declare.  Assume SerialManager_UI.h will be included elsewhere

#define MAX_DATASTREAM_LENGTH 1024
#define DATASTREAM_START_CHAR (char)0x02
#define DATASTREAM_SEPARATOR (char)0x03
#define DATASTREAM_END_CHAR (char)0x04
#define QUADCHAR_START_CHAR     ((char)'_')   //this is an underscore
#define QUADCHAR_CHAR__USE_PERSISTENT_MODE ((char)'_')  //this is an underscore

#define SERIALMANAGERBASE_MAX_UI_ELEMENTS 30
class SerialManagerBase {
  public:
    SerialManagerBase(void) {};
    SerialManagerBase(BLE *_ble) :
      ble(_ble) {};

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
	
	SerialManager_UI* add_UI_element(SerialManager_UI *);

  protected:
    virtual bool processCharacter(char c);
    virtual void processStream(void);
    virtual bool interpretQuadChar(char mode_char, char chan_char, char data_char); 
    virtual int readStreamIntArray(int idx, int* arr, int len);
    virtual int readStreamFloatArray(int idx, float* arr, int len);
    
    int serial_read_state; // Are we reading one character at a time, or a stream?
    char stream_data[MAX_DATASTREAM_LENGTH];
    int stream_length;
    int stream_chars_received;
    BLE *ble;
    char GUI_persistent_mode = 'g';
    String TX_string;
    char mode_char, chan_char, data_char; //for quad_char processing
	SerialManager_UI* UI_element_ptr[SERIALMANAGERBASE_MAX_UI_ELEMENTS];
	int next_UI_element_ind = 0;
	const int max_UI_elements = SERIALMANAGERBASE_MAX_UI_ELEMENTS;
};



#endif