    
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
extern State myState;

//Extern Functions
extern void setInputSource(int);
extern void setInputMixer(int);
extern float incrementInputGain(float);
extern float incrementKnobGain(float);


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

    void printTympanRemoteLayout(void);
    void setFullGUIState(void);
    void setInputConfigButtons(void);    
    void setMicConfigButtons(bool disableAll= false);
    void setInputGainButtons(void);
    void setOutputGainButtons(void);
    void setSDRecordingButtons(void);
    void setButtonState(String btnId, bool newState);
    void setButtonText(String btnId, String text);

    int serial_read_state; // Are we reading one character at a time, or a stream?
    char stream_data[MAX_DATASTREAM_LENGTH];
    int stream_length;
    int stream_chars_received;

};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("  h: Print this help");
  myTympan.println("  g: Print the gain settings of the device.");
  myTympan.println("  c/C: Enablel/Disable printing of CPU and Memory usage");
  myTympan.println("  w: Inputs: Use PCB Mics");
  myTympan.println("  d: Inputs: Use PDM Mics as input");
  myTympan.println("  W: Inputs: Use Mic on Mic Jack");
  myTympan.println("  e: Inputs: Use LineIn on Mic Jack");
  myTympan.println("  E: Inputs: Use BTAudio as input");
  myTympan.println("  9/(: Mic Mix: Use front or rear mic");
  myTympan.println("  0/): Mic Mix: Use both, inphase or inverted");
  myTympan.print  ("  i/I: Increase/Decrease Input Gain by "); myTympan.print(INCREMENT_INPUTGAIN_DB); myTympan.println(" dB");
  myTympan.print  ("  k/K: Increase/Decrease Headphone Volume by "); myTympan.print(INCREMENT_HEADPHONE_GAIN_DB); myTympan.println(" dB");
  myTympan.println("  r,s,|: SD: begin/stop/deleteAll recording");
  myTympan.println();
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
   case 'g': case 'G':
      myState.printGainSettings(); break;
    case 'c':
      Serial.println("Recvd: printing memory and CPU.");
      myState.flag_printCPUandMemory = true;
      setButtonState("cpuStart",myState.flag_printCPUandMemory);
      break;
    case 'C':
      Serial.println("Recvd: stopping printing memory and CPU.");
      myState.flag_printCPUandMemory = false;
      setButtonState("cpuStart",myState.flag_printCPUandMemory);
      break;
    case 'i':
      incrementInputGain(INCREMENT_INPUTGAIN_DB);
      setInputGainButtons();
      myState.printGainSettings();
      break;
    case 'I':  
      incrementInputGain(-INCREMENT_INPUTGAIN_DB);
      setInputGainButtons();
      myState.printGainSettings();  
      break;
    case 'k':
      incrementKnobGain(INCREMENT_HEADPHONE_GAIN_DB);
      //setOutputGainButtons(); //button state is already being updated by setOutputVolume_dB()
      myState.printGainSettings();
      break;
    case 'K':   
      incrementKnobGain(-INCREMENT_HEADPHONE_GAIN_DB);
      //setOutputGainButtons();  //button state is already being updated by setOutputVolume_dB()
      myState.printGainSettings();  
      break;
    case 'w':
      myTympan.println("Received: Listen to PCB mics");
      setInputSource(State::INPUT_PCBMICS);
      setInputMixer(State::MIC_AIC0_LR);  //PCB Mics are only on the main Tympan board
      setFullGUIState();
      break;
    case 'E':
      myTympan.println("Received: Listen to BTAudio");
      setInputSource(State::INPUT_LINEIN_SE);
      setInputMixer(State::MIC_AIC0_LR); //BT Audio is only on the main Tympan board
      setFullGUIState();
      break;
    case 'd':
      myTympan.println("Received: Listen to PDM mics"); //digital mics
      setInputSource(State::INPUT_PDMMICS);
      setInputMixer(myState.digital_mic_config);
      setFullGUIState();
      break;
    case 'W':
      myTympan.println("Received: Mic jack as mic");
      setInputSource(State::INPUT_MICJACK_MIC);
      setInputMixer(myState.analog_mic_config); //could be on either board, so remember the last-used state
      setFullGUIState();
      break;
    case 'e':
      myTympan.println("Received: Mic jack as line-in");
      setInputSource(State::INPUT_MICJACK_LINEIN);
      setInputMixer(myState.analog_mic_config);
      setFullGUIState();
      break;
    case '9':
      myTympan.println("Received: Use front mics");
      setInputMixer(State::MIC_FRONT);
      setMicConfigButtons();
      break;
    case '(':
      myTympan.println("Received: Use rear mics");
      setInputMixer(State::MIC_REAR);
      setMicConfigButtons();
      break;
    case '0':
      myTympan.println("Received: Use both mics (in-phase)");
      setInputMixer(State::MIC_BOTH_INPHASE);
      setMicConfigButtons();
      break;
    case ')':
      myTympan.println("Received: Use both mics (inverted)");
      setInputMixer(State::MIC_BOTH_INVERTED);
      setMicConfigButtons();
      break;
     case '-':
      myTympan.println("Received: Use Tympan Board");
      setInputMixer(State::MIC_AIC0_LR);
      setMicConfigButtons();
      break;
    case '_':
      myTympan.println("Received: Use Earpiece Board");
      setInputMixer(State::MIC_AIC1_LR);
      setMicConfigButtons();
      break;
    case '=':
      myTympan.println("Received: Mix Both Boards");
      setInputMixer(State::MIC_BOTHAIC_LR);
      setMicConfigButtons();
      break;
    case 'r':
      myTympan.println("Received: begin SD recording");
      audioSDWriter.startRecording();
      setSDRecordingButtons();
      break;
    case 's':
      myTympan.println("Received: stop SD recording");
      audioSDWriter.stopRecording();
      setSDRecordingButtons();
      break;
    case '|':
      myTympan.println("Recieved: delete all SD recordings.");
      audioSDWriter.deleteAllRecordings();
      myTympan.println("Delete all SD recordings complete.");
      break;
    case 'J': case 'j':
      printTympanRemoteLayout();
      delay(100);
      setFullGUIState();
      break;
  }
}

// Print the layout for the Tympan Remote app, in a JSON-ish string
// (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).
// If you don't give a button an id, then it will be assigned the id 'default'.  IDs don't have to be unique; if two buttons have the same id,
// then you always control them together and they'll always have the same state. 
// Please don't put commas or colons in your ID strings!
// The 'icon' is how the device appears in the app - could be an icon, could be a pic of the device.  Put the
// image in the TympanRemote app in /src/assets/devIcon/ and set 'icon' to the filename.
void SerialManager::printTympanRemoteLayout(void) {
  char jsonConfig[] = "JSON={"
    "'icon':'creare.png',"
    "'pages':["
      "{'title':'Input Select','cards':["
        "{'name':'Audio Source', 'buttons':["
                                           "{'label':'Digital: Earpieces', 'cmd': 'd', 'id':'configPDMMic', 'width':'12'},"
                                           "{'label':'Analog: PCB Mics',  'cmd': 'w', 'id':'configPCBMic',  'width':'12'},"
                                           "{'label':'Analog: Mic Jack (Mic)',  'cmd': 'W', 'id':'configMicJack', 'width':'12'},"
                                           "{'label':'Analog: Mic Jack (Line)',  'cmd': 'e', 'id':'configLineJack', 'width':'12'},"
                                           "{'label':'Analog: BT Audio', 'cmd': 'E', 'id':'configLineSE',  'width':'12'}" //don't have a trailing comma on this last one
                                          "]},"
        "{'name':'Digital and Analog Options','buttons':[{'label':'Swipe Right'}]}"  //don't have a trailing comma on this last one
      "]},"
      "{'title':'Digital Input Settings','cards':["
        "{'name':'Digital Earpiece Mics','buttons':[{'label': 'Front', 'cmd': '9', 'id':'micsFront', 'width':'6'},{'label': 'Rear', 'cmd': '(', 'id':'micsRear', 'width':'6'}, {'label': 'Both','cmd':'0', 'id':'micsBoth', 'width':'12'}, {'label': 'Both (Inverted)','cmd':')','id':'micsBothInv', 'width':'12'}]},"
        "{'name':'Output Gain',    'buttons':[{'label':'-', 'cmd':'K', 'width':'4'},{'id':'outGain', 'label':'0', 'width':'4'},{'label':'+', 'cmd':'k', 'width':'4'}]}"//don't have a trailing comma on this last one
      "]},"
      "{'title':'Analog Input Settings','cards':["
        "{'name':'Analog Input Source','buttons':[{'label': 'Tympan Board', 'cmd': '-', 'id':'micsAIC0', 'width':'12'},{'label': 'Earpiece Shield', 'cmd': '_', 'id':'micsAIC1', 'width':'12'}, {'label': 'Mix Both','cmd':'=', 'id':'micsBothAIC', 'width':'12'}]},"
        "{'name':'Analog Input Gain', 'buttons':[{'label':'-', 'cmd':'I', 'width':'4'},{'id':'inGain',  'label':'0', 'width':'4'},{'label':'+', 'cmd':'i', 'width':'4'}]},"  //whichever line is the last line, NO TRAILING COMMA!
        "{'name':'Output Gain',    'buttons':[{'label':'-', 'cmd':'K', 'width':'4'},{'id':'outGain', 'label':'0', 'width':'4'},{'label':'+', 'cmd':'k', 'width':'4'}]}"//don't have a trailing comma on this last one
      "]},"
      "{'title':'Global','cards':["
        "{'name':'Record to SD Card','buttons':[{'label':'Start', 'cmd':'r', 'id':'recordStart'},{'label':'Stop', 'cmd':'s'}]},"
        "{'name':'CPU Reporting',    'buttons':[{'label':'Start', 'cmd':'c', 'id':'cpuStart'}   ,{'label':'Stop', 'cmd':'C'}]}" //don't have a trailing comma on this last one
      "]}"   //don't have a trailing comma on this last one.                         
    "]"
  "}";
  myTympan.println(jsonConfig);
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


int SerialManager::readStreamIntArray(int idx, int* arr, int len) {
  int i;
  for (i=0; i<len; i++) {
    arr[i] = *((int*)(stream_data+idx)); 
    idx=idx+4;
  }
  return idx;
}

int SerialManager::readStreamFloatArray(int idx, float* arr, int len) {
  int i;
  for (i=0; i<len; i++) {
    arr[i] = *((float*)(stream_data+idx)); 
    idx=idx+4;
  }
  return idx;
}


void SerialManager::setFullGUIState(void) {
  setInputConfigButtons();
  setMicConfigButtons();
  setInputGainButtons();
  setOutputGainButtons();
  
  setButtonState("cpuStart",myState.flag_printCPUandMemory);
  setSDRecordingButtons();
}

void SerialManager::setSDRecordingButtons(void) {
   if (audioSDWriter.getState() == AudioSDWriter_F32::STATE::RECORDING) {
    setButtonState("recordStart",true);
  } else {
    setButtonState("recordStart",false);
  } 
}

void SerialManager::setMicConfigButtons(bool disableAll) {

  //clear any previous state of the buttons
  setButtonState("micsFront",false); delay(10);
  setButtonState("micsRear",false); delay(10);
  setButtonState("micsBoth",false); delay(10);
  setButtonState("micsBothInv",false); delay(10);
  setButtonState("micsAIC0",false); delay(10);
  setButtonState("micsAIC1",false); delay(10);
  setButtonState("micsBothAIC",false); delay(10);

  //now, set the one button that should be active
  int mic_config = myState.analog_mic_config;   //assume that we're in our analog input configuration
  if (myState.input_source == State::INPUT_PDMMICS) mic_config = myState.digital_mic_config; //but check to see if we're in digital input configuration
  if (disableAll == false) {
    switch (mic_config) {
      case myState.MIC_FRONT:
        setButtonState("micsFront",true);
        break;
      case myState.MIC_REAR:
        setButtonState("micsRear",true);
        break;
      case myState.MIC_BOTH_INPHASE:
        setButtonState("micsBoth",true);
        break;
      case myState.MIC_BOTH_INVERTED:
        setButtonState("micsBothInv",true);
        break;
      case myState.MIC_AIC0_LR:
        setButtonState("micsAIC0",true);
        break;
     case myState.MIC_AIC1_LR:
        setButtonState("micsAIC1",true);
        break;
     case myState.MIC_BOTHAIC_LR:
        setButtonState("micsBothAIC",true);
        break;
    }
  }
}


void SerialManager::setInputConfigButtons(void) {
  //clear out previous state of buttons
  setButtonState("configPDMMic",false);  delay(10);
  setButtonState("configPCBMic",false);  delay(10);
  setButtonState("configMicJack",false); delay(10);
  setButtonState("configLineJack",false);delay(10);
  setButtonState("configLineSE",false);  delay(10);

  //set the new state of the buttons
  switch (myState.input_source) {
    case (State::INPUT_PDMMICS):
      setButtonState("configPDMMic",true);  delay(10); break;
    case (State::INPUT_PCBMICS):
      setButtonState("configPCBMic",true);  delay(10); break;
    case (State::INPUT_MICJACK_MIC): 
      setButtonState("configMicJack",true); delay(10); break;
    case (State::INPUT_LINEIN_SE): 
      setButtonState("configLineSE",true);  delay(10); break;
    case (State::INPUT_MICJACK_LINEIN): 
      setButtonState("configLineJack",true);delay(10); break;
  }  
}
void SerialManager::setInputGainButtons(void) {
  setButtonText("inGain",String(myState.inputGain_dB,1));
}
void SerialManager::setOutputGainButtons(void) {
  setButtonText("outGain",String(myState.volKnobGain_dB,1));
}

void SerialManager::setButtonState(String btnId, bool newState) {
  if (newState) {
    myTympan.println("STATE=BTN:" + btnId + ":1");
  } else {
    myTympan.println("STATE=BTN:" + btnId + ":0");
  }
}

void SerialManager::setButtonText(String btnId, String text) {
  myTympan.println("TEXT=BTN:" + btnId + ":"+text);
}



#endif
