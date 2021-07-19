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
extern void setOutputAIC(int);
extern float incrementHPCutoffFreq_Hz(float);
extern float incrementExpKnee_dBSPL(float);
extern float incrementLinearGain_dB(float);
extern float incrementCompKnee_dBSPL(float);
extern float incrementCompRatio(float);
extern float incrementLimiterKnee_dBSPL(float);
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
    float gainIncrement_dB = 2.5f;    
    float freqIncrementFactor = powf(2.0,1.0/3.0); //move 1/3 octave with each step    
    float compRatioIncrement = 0.25;
    
    int serial_read_state; // Are we reading one character at a time, or a stream?
    char stream_data[MAX_DATASTREAM_LENGTH];
    int stream_length;
    int stream_chars_received;

    void printTympanRemoteLayout(void);
    void setFullGUIState(void);
    void setInputConfigButtons(void);    
    void setMicConfigButtons(bool disableAll= false);
    void setInputGainButtons(void);
    void setOutputGainButtons(void);
    void setSDRecordingButtons(void);    
    void setOutputConfigButtons(void);
    void setButtonState(String btnId, bool newState);
    void setButtonText(String btnId, String text);
    void setButtonText(String btnId, float val, int nplaces);
};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("  h: Print this help");
  myTympan.println("  c/C: Enablel/Disable printing of CPU and Memory usage");  
  myTympan.println("  w: Inputs: PCB Mics");
  myTympan.println("  d: Inputs: Digital PDM Mics");
  //myTympan.println("  W: Inputs: Mic on Mic Jack");
  //myTympan.println("  e: Inputs: LineIn on Mic Jack");
//  myTympan.println("  E: Inputs: BT Audio");

  myTympan.println("  -/9/(: Mic Mix: Mute, use front, or use rear mic");
  myTympan.println("  0/): Mic Mix: Use both, inphase or inverted"); 

//  myTympan.print  ("  i/I: Increase/Decrease Input Gain by "); myTympan.print(INCREMENT_INPUTGAIN_DB); myTympan.println(" dB");
//  myTympan.print  ("  k/K: Increase/Decrease Headphone Volume by "); myTympan.print(INCREMENT_HEADPHONE_GAIN_DB); myTympan.println(" dB");

//  myTympan.print  ("  f/F: Raise/Lower the highpass filter cutoff frequency by "); myTympan.print((freqIncrementFactor-1.0)*100.0,0); myTympan.println("%");  
//  myTympan.print  ("  x,X: Comp: Raise/Lower Exp Knee by "); myTympan.print(gainIncrement_dB); myTympan.println(" dB");
//  myTympan.print  ("  a,A: Comp: Incr/Decr Linear Gain by ");  myTympan.print(gainIncrement_dB); myTympan.println(" dB");
//  myTympan.print  ("  z,Z: Comp: Raise/Lower Comp Knee by ");  myTympan.print(gainIncrement_dB); myTympan.println(" dB");
//  myTympan.print  ("  v,V: Comp: Raise/Lower Comp Ratio by ");  myTympan.print(compRatioIncrement); myTympan.println();
//  myTympan.print  ("  l,L: Comp: Raise/Lower Lim Knee by "); myTympan.print(gainIncrement_dB); myTympan.println(" dB");
  myTympan.println("  u,U,o,O: Output AIC: Both, Top, Bottom, None");
  myTympan.println("  ]: Toggle sending of data to plot");
  myTympan.println("  r,s,|: SD: begin/stop/deleteAll recording");
    
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
  float val=0.0;
  
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
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

  case '-':
      myTympan.println("Received: Mute mics");
      setInputMixer(State::MIC_MUTE);
      setMicConfigButtons();
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

   case 'f':
      val=incrementHPCutoffFreq_Hz(freqIncrementFactor);
      setButtonText("hp_Hz",val,0);
      break;
    case 'F':
      val=incrementHPCutoffFreq_Hz(1.0/freqIncrementFactor);
      setButtonText("hp_Hz",val,0);
      break;
    case 'x':
      val = incrementExpKnee_dBSPL(gainIncrement_dB);
      setButtonText("V_expKnee",val),1;
      //printCompSettings();
      break;
    case 'X': 
      val = incrementExpKnee_dBSPL(-gainIncrement_dB);
      setButtonText("V_expKnee",val,1);
      //printCompSettings();   
      break;      
    case 'a':
      val = incrementLinearGain_dB(gainIncrement_dB);
      setButtonText("V_linGain",val,1);
      break;
    case 'A':
      val = incrementLinearGain_dB(-gainIncrement_dB);
      setButtonText("V_linGain",val,1);
      break;      
    case 'z':
      val = incrementCompKnee_dBSPL(gainIncrement_dB);
      setButtonText("V_compKnee",val,1);
      //printCompSettings();
      break;
    case 'V': 
      val = incrementCompRatio(-compRatioIncrement);
      setButtonText("V_compRatio",val,1);
      //printCompSettings();
      break;    
    case 'v':
      val = incrementCompRatio(compRatioIncrement);
      setButtonText("V_compRatio",val,1);
      //printCompSettings();
      break;
    case 'Z': 
      val = incrementCompKnee_dBSPL(-gainIncrement_dB);
      setButtonText("V_compKnee",val,1);
      //printCompSettings();
      break;           
    case 'l':
      val = incrementLimiterKnee_dBSPL(gainIncrement_dB);
      setButtonText("V_limKnee",val,1);
      //printCompSettings();
      break;
    case 'L': 
      val = incrementLimiterKnee_dBSPL(-gainIncrement_dB);
      setButtonText("V_limKnee",val,1);
      //printCompSettings();
      break;       
    case 'u':
      setOutputAIC(State::OUT_BOTH);
      setOutputConfigButtons();
      break;
    case 'U':
      setOutputAIC(State::OUT_AIC1);
      setOutputConfigButtons();
      break;
    case 'o':
      setOutputAIC(State::OUT_AIC0);
      setOutputConfigButtons();
      break;
    case 'O':
      setOutputAIC(State::OUT_NONE);
      setOutputConfigButtons();
      break;
    case ']':
      myState.flag_sendPlottingData = !(myState.flag_sendPlottingData);
      if (myState.flag_sendPlottingData) myState.flag_sendPlotLegend = true;
      break;      
    case 'J': case 'j':
      printTympanRemoteLayout();
      delay(20);
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
      //"{'title':'Input Select','cards':["
        //"{'name':'Audio Source', 'buttons':["
        //                                   "{'label':'Digital: Earpieces', 'cmd': 'd', 'id':'configPDMMic', 'width':'12'},"
        //                                   "{'label':'Analog: PCB Mics',  'cmd': 'w', 'id':'configPCBMic',  'width':'12'},"
        //                                   "{'label':'Analog: Mic Jack (Mic)',  'cmd': 'W', 'id':'configMicJack', 'width':'12'},"
        //                                   "{'label':'Analog: Mic Jack (Line)',  'cmd': 'e', 'id':'configLineJack', 'width':'12'},"
        //                                   "{'label':'Analog: BT Audio', 'cmd': 'E', 'id':'configLineSE',  'width':'12'}" //don't have a trailing comma on this last one
        //                                  "]},"
      "{'title':'Input / Output Select','cards':["
        "{'name':'Choose Earpiece Mics','buttons':[{'label': 'Front', 'cmd': '9', 'id':'micsFront', 'width':'6'},{'label': 'Rear', 'cmd': '(', 'id':'micsRear', 'width':'6'}, {'label': 'Both (F+R)','cmd':'0', 'id':'micsBoth', 'width':'6'}, {'label': 'Both (F-R)','cmd':')','id':'micsBothInv', 'width':'6'},{'label': 'Mute','cmd':'-','id':'micsMute', 'width':'12'}]},"
        "{'name':'Choose Output Board','buttons':[{'label': 'Both', 'cmd': 'u', 'id':'outBoth', 'width':'6'},{'label': 'None', 'cmd': 'O', 'id':'outNone', 'width':'6'}, {'label': 'Top','cmd':'U', 'id':'outTop', 'width':'6'}, {'label': 'Bottom','cmd':'o','id':'outBot', 'width':'6'}]},"
        "{'name':'More Options','buttons':[{'label':'Swipe Right'}]}"  //don't have a trailing comma on this last one
      "]},"
      "{'title':'Audio Processing','cards':["
        "{'name':'Highpass Cutoff (Hz)','buttons':[{'label': 'Lower', 'width':'4', 'cmd': 'F'},{'label': '', 'width':'4', 'id':'hp_Hz'},{'label': 'Higher', 'width':'4', 'cmd': 'f'}]},"
        "{'name':'Linear Gain (dB)',     'buttons':[{'label': 'Lower', 'cmd' :'A', 'width':'4'},{'label': '', 'width':'4', 'id':'V_linGain'},{'label': 'Higher', 'cmd': 'a', 'width':'4'}]},"
        "{'name':'Compressor Settings','buttons':[{'label':'Swipe Right'}]}"  //don't have a trailing comma on this last one
      "]},"      
      "{'title':'WDRC Settings','cards':["
        //"{'name':'Exp Knee (dB SPL)',     'buttons':[{'label': 'Lower', 'cmd' :'X', 'width':'4'},{'label': '', 'width':'4', 'id':'V_expKnee'},{'label': 'Higher', 'cmd': 'x', 'width':'4'}]},"
        "{'name':'Comp Knee (dB SPL)',    'buttons':[{'label': 'Lower', 'cmd' :'Z', 'width':'4'},{'label': '', 'width':'4', 'id':'V_compKnee'},{'label': 'Higher', 'cmd': 'z', 'width':'4'}]},"
        "{'name':'Compression Ratio',     'buttons':[{'label': 'Lower', 'cmd' :'V', 'width':'4'},{'label': '', 'width':'4', 'id':'V_compRatio'},{'label': 'Higher', 'cmd': 'v', 'width':'4'}]},"
        "{'name':'Limiter Knee (dB SPL)', 'buttons':[{'label': 'Lower', 'cmd' :'L', 'width':'4'},{'label': '', 'width':'4', 'id':'V_limKnee'},{'label': 'Higher', 'cmd': 'l', 'width':'4'}]},"//no trailing comma on last one
        "{'name':'System Settings','buttons':[{'label':'Swipe Right'}]}"  //don't have a trailing comma on this last one
      "]},"      
      "{'title':'System Settings','cards':["
        "{'name':'Record to SD Card','buttons':[{'label':'Start', 'cmd':'r', 'id':'recordStart'},{'label':'Stop', 'cmd':'s'}]},"
        "{'name':'CPU Reporting',    'buttons':[{'label':'Start', 'cmd':'c', 'id':'cpuStart'}   ,{'label':'Stop', 'cmd':'C'}]}" //don't have a trailing comma on this last one
      "]}"   //don't have a trailing comma on this last one.                         
    "],"
    "'prescription':{'type':'BoysTown','pages':['serialPlotter']}"
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

void SerialManager::setFullGUIState(void) {
  setInputConfigButtons();
  setMicConfigButtons();
  //setInputGainButtons();
  //setOutputGainButtons();

  setOutputConfigButtons();

  setButtonText("hp_Hz",incrementHPCutoffFreq_Hz(1.0),0); delay(2);
  setButtonText("V_expKnee",incrementExpKnee_dBSPL(0.0),1);delay(2);
  setButtonText("V_linGain",incrementLinearGain_dB(0.0),1);delay(2);
  setButtonText("V_compKnee",incrementCompKnee_dBSPL(0.0),1);delay(2);
  setButtonText("V_compRatio",incrementCompRatio(0.0),1);delay(2);
  setButtonText("V_limKnee",incrementLimiterKnee_dBSPL(0.0),1);delay(2);
  
  setButtonState("cpuStart",myState.flag_printCPUandMemory);
  setSDRecordingButtons();
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
      setButtonState("configPDMMic",true);  delay(3); break;
    case (State::INPUT_PCBMICS):
      setButtonState("configPCBMic",true);  delay(3); break;
    case (State::INPUT_MICJACK_MIC): 
      setButtonState("configMicJack",true); delay(3); break;
    case (State::INPUT_LINEIN_SE): 
      setButtonState("configLineSE",true);  delay(3); break;
    case (State::INPUT_MICJACK_LINEIN): 
      setButtonState("configLineJack",true);delay(3); break;
  }  
}

void SerialManager::setMicConfigButtons(bool disableAll) {

  //clear any previous state of the buttons
  setButtonState("micsFront",false); delay(3);
  setButtonState("micsRear",false); delay(3);
  setButtonState("micsBoth",false); delay(3);
  setButtonState("micsBothInv",false); delay(3);
  setButtonState("micsMute",false); delay(3);
  //setButtonState("micsAIC0",false); delay(3);
  //setButtonState("micsAIC1",false); delay(3);
  //setButtonState("micsBothAIC",false); delay(3);

  //now, set the one button that should be active
  int mic_config = myState.analog_mic_config;   //assume that we're in our analog input configuration
  if (myState.input_source == State::INPUT_PDMMICS) mic_config = myState.digital_mic_config; //but check to see if we're in digital input configuration
  if (disableAll == false) {
    switch (mic_config) {
       case myState.MIC_MUTE:
        setButtonState("micsMute",true);
        break;
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
void SerialManager::setInputGainButtons(void);
void SerialManager::setOutputGainButtons(void);


void SerialManager::setOutputConfigButtons(void) {
  //clear out previous state of buttons
  setButtonState("outBoth",false);  delay(3);
  setButtonState("outTop",false);  delay(3);
  setButtonState("outBot",false); delay(3);
  setButtonState("outNone",false);delay(3);
  

  //set the new state of the buttons
  switch (myState.output_aic) {
    case (State::OUT_BOTH):
      setButtonState("outBoth",true);  delay(3); break;
    case (State::OUT_AIC1):
      setButtonState("outTop",true);  delay(3); break;
    case (State::OUT_AIC0): 
      setButtonState("outBot",true); delay(3); break;
    case (State::OUT_NONE): 
      setButtonState("outNone",true);  delay(3); break;
  }  
}

void SerialManager::setSDRecordingButtons(void) {
   if (audioSDWriter.getState() == AudioSDWriter_F32::STATE::RECORDING) {
    setButtonState("recordStart",true);
  } else {
    setButtonState("recordStart",false);
  } 
}

void SerialManager::setButtonState(String btnId, bool newState) {
  if (newState) {
    myTympan.println("STATE=BTN:" + btnId + ":1");
  } else {
    myTympan.println("STATE=BTN:" + btnId + ":0");
  }
}

void SerialManager::setButtonText(String btnId, float val,int nplaces=2) {
  myTympan.print("TEXT=BTN:" + btnId + ":");
  myTympan.println(val,nplaces);
}
void SerialManager::setButtonText(String btnId, String text) {
  myTympan.println("TEXT=BTN:" + btnId + ":"+text);
}


#endif
