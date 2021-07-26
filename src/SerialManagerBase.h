
#ifndef SerialManagerBase_h
#define SerialManagerBase_h

#include <Arduino.h> //for Serial and String
//include <Tympan_Library.h> // for BLE library
#include "BLE/ble.h"
//#include "SerialManager_UI.h"
class SerialManager_UI;  //forward declare

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