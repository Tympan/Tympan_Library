#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

#define MAX_DATASTREAM_LENGTH 1024
#define DATASTREAM_START_CHAR (char)0x02
#define DATASTREAM_SEPARATOR (char)0x03
#define DATASTREAM_END_CHAR (char)0x04

enum read_state_options {
  SINGLE_CHAR,
  STREAM_LENGTH,
  STREAM_DATA
};


//Extern variables
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;

//Extern Functions
extern void setInputSource(Mic_Input);
extern void setInputMixer(Mic_Channels, int);
extern void incrementInputGain(float);
extern void incrementKnobGain(float);


//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(void) {  };
      
    void respondToByte(char c);
    void processSingleCharacter(char c);
    void processStream(void);
    int readStreamIntArray(int idx, int* arr, int len);
    int readStreamFloatArray(int idx, float* arr, int len);
    
    void printHelp(void); 
    const float INCREMENT_INPUTGAIN_DB = 2.5f;  
    const float INCREMENT_HEADPHONE_GAIN_DB = 2.5f;  
    
    int serial_read_state; // Are we reading one character at a time, or a stream?
    char stream_data[MAX_DATASTREAM_LENGTH];
    int stream_length;
    int stream_chars_received;
};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("  h: Print this help");
  myTympan.println("  w: Inputs: Use PCB Mics");
  myTympan.println("  d: Inputs: Use PDM Mics as input");
  myTympan.println("  W: Inputs: Use Mic on Mic Jack");
  myTympan.println("  e: Inputs: Use LineIn on Mic Jack");
  myTympan.print  ("  i/I: Increase/Decrease Input Gain by "); myTympan.print(INCREMENT_INPUTGAIN_DB); myTympan.println(" dB");
  myTympan.print  ("  k/K: Increase/Decrease Headphone Volume by "); myTympan.print(INCREMENT_HEADPHONE_GAIN_DB); myTympan.println(" dB");
  myTympan.println("  r,s2|: SD: begin/stop/deleteAll recording");
  myTympan.println("  !/1: Enable/Disable Left-Front Earpiece Mic");
  myTympan.println("  @/2: Enable/Disable Left-Rear Earpiece Mic");
  myTympan.println("  #/3: Enable/Disable Right-Front Earpiece Mic");
  myTympan.println("  $/4: Enable/Disable Right-Rear Earpiece Mic");
  myTympan.println("  %/5: Enable/Disable All Earpiece Mics");
}


//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  switch (serial_read_state) {
    case SINGLE_CHAR:
      if (c == DATASTREAM_START_CHAR) {
        Serial.println("Start data stream.");
        // Start a data stream:
        serial_read_state = STREAM_LENGTH;
        stream_chars_received = 0;
      } else {
        Serial.print("Processing character ");
        Serial.println(c);
        processSingleCharacter(c);
      }
      break;
    case STREAM_LENGTH:
      //Serial.print("Reading stream length char: ");
      //Serial.print(c);
      //Serial.print(" = ");
      //Serial.println(c, HEX);
      if (c == DATASTREAM_SEPARATOR) {
        // Get the datastream length:
        stream_length = *((int*)(stream_data));
        serial_read_state = STREAM_DATA;
        stream_chars_received = 0;
        Serial.print("Stream length = ");
        Serial.println(stream_length);
      } else {
        //Serial.println("End of stream length message");
        stream_data[stream_chars_received] = c;
        stream_chars_received++;
      }
      break;
    case STREAM_DATA:
      if (stream_chars_received < stream_length) {
        stream_data[stream_chars_received] = c;
        stream_chars_received++;        
      } else {
        if (c == DATASTREAM_END_CHAR) {
          // successfully read the stream
          Serial.println("Time to process stream!");
          processStream();
        } else {
          myTympan.print("ERROR: Expected string terminator ");
          myTympan.print(DATASTREAM_END_CHAR, HEX);
          myTympan.print("; found ");
          myTympan.print(c,HEX);
          myTympan.println(" instead.");
        }
        serial_read_state = SINGLE_CHAR;
        stream_chars_received = 0;
      }
      break;
  }
}



//switch yard to determine the desired action
void SerialManager::processSingleCharacter(char c) {
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'i':
      incrementInputGain(INCREMENT_INPUTGAIN_DB);
      break;
    case 'I':  
      incrementInputGain(-INCREMENT_INPUTGAIN_DB);
      break;
    case 'k':
      incrementKnobGain(INCREMENT_HEADPHONE_GAIN_DB);
      break;
    case 'K':   
      incrementKnobGain(-INCREMENT_HEADPHONE_GAIN_DB);
      break;
    case 'w':
      myTympan.println("Received: Listen to PCB mics");
      setInputSource(INPUT_PCBMICS);
      setInputMixer(ALL_MICS, 0.5);
      break;
    case 'd':
      myTympan.println("Received: Listen to PDM mics"); //digital mics
      setInputSource(INPUT_PDMMICS);
      setInputMixer(ALL_MICS, 0.5);
      break;
    case 'W':
      myTympan.println("Received: Listen to Mic jack as mic");
      setInputSource(INPUT_MICJACK_MIC);
      setInputMixer(ALL_MICS, 0.5);
      break;
    case 'e':
      myTympan.println("Received: Listen to Mic jack as line-in");
      setInputSource(INPUT_MICJACK_LINEIN);
      setInputMixer(ALL_MICS, 0.5);
      break;
    case 'r':
      myTympan.println("Received: begin SD recording");
      audioSDWriter.startRecording();
      break;
    case 's':
      myTympan.println("Received: stop SD recording");
      audioSDWriter.stopRecording();
      break;
    case '|':
      myTympan.println("Recieved: delete all SD recordings.");
      audioSDWriter.deleteAllRecordings();
      myTympan.println("Delete all SD recordings complete.");
      break;
    case '!':
      myTympan.println("Received: DISABLE Left-Front earpiece mic");
      setInputMixer(MIC_FRONT_LEFT, 0.0);
      break;
    case '1':
      myTympan.println("Received: ENABLE Left-Front earpiece mic");
      setInputMixer(MIC_FRONT_LEFT, 1.0);
      break;
    case '@':
      myTympan.println("Received: DISABLE Left-Rear earpiece mic");
      setInputMixer(MIC_REAR_LEFT, 0.0);
      break;
    case '2':
      myTympan.println("Received: ENABLE Left-Rear earpiece mic");
      setInputMixer(MIC_REAR_LEFT, 1.0);
      break;
    case '#':
      myTympan.println("Received: DISABLE Right-Front earpiece mic");
      setInputMixer(MIC_FRONT_RIGHT, 0.0);
      break;
    case '3':
      myTympan.println("Received: Use Right-Frontearpiece mic");
      setInputMixer(MIC_FRONT_RIGHT, 1.0);
      break;
    case '$':
      myTympan.println("Received: DISABLE Right-Rear earpiece mic");
      setInputMixer(MIC_REAR_RIGHT, 0.0);
      break;
    case '4':
      myTympan.println("Received: Use Right-Rear earpiece mic");
      setInputMixer(MIC_REAR_RIGHT, 1.0);
      break;
    case '%':
      myTympan.println("Received: DISABLE All earpiece mics");
      setInputMixer(ALL_MICS, 0.0);
      break;
    case '5':
      myTympan.println("Received: ENABLE All earpiece mics");
      setInputMixer(ALL_MICS, 1.0);
      break;
  }
}



void SerialManager::processStream(void) {
  int idx = 0;
  String streamType;
  int tmpInt;
  float tmpFloat;
  
  while (stream_data[idx] != DATASTREAM_SEPARATOR) {
    streamType.append(stream_data[idx]);
    idx++;
  }
  idx++; // move past the datastream separator

  //myTympan.print("Received stream: ");
  //myTympan.print(stream_data);

  if (streamType == "gha") {    
    myTympan.println("Stream is of type 'gha'.");
    //interpretStreamGHA(idx);
  } else if (streamType == "dsl") {
    myTympan.println("Stream is of type 'dsl'.");
    //interpretStreamDSL(idx);
  } else if (streamType == "afc") {
    myTympan.println("Stream is of type 'afc'.");
    //interpretStreamAFC(idx);
  } else if (streamType == "test") {    
    myTympan.println("Stream is of type 'test'.");
    tmpInt = *((int*)(stream_data+idx)); idx = idx+4;
    myTympan.print("int is "); myTympan.println(tmpInt);
    tmpFloat = *((float*)(stream_data+idx)); idx = idx+4;
    myTympan.print("float is "); myTympan.println(tmpFloat);
  } else {
    myTympan.print("Unknown stream type: ");
    myTympan.println(streamType);
  }
}


#endif
