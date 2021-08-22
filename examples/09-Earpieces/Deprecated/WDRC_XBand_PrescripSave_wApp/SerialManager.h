
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//add in the algorithm whose gains we wish to set via this SerialManager...change this if your gain algorithms class changes names!
#include "AudioEffectCompWDRC_F32.h"    //change this if you change the name of the algorithm's source code filename
typedef AudioEffectCompWDRC_F32 GainAlgorithm_t; //change this if you change the algorithm's class name

#define MAX_DATASTREAM_LENGTH   1024
#define DATASTREAM_START_CHAR   (char)0x02
#define DATASTREAM_SEPARATOR    (char)0x03
#define DATASTREAM_END_CHAR     (char)0x04
#define QUADCHAR_START_CHAR     ((char)'_')   //this is an underscore
#define QUADCHAR_CHAR__USE_PERSISTENT_MODE ((char)'_')  //this is an underscore
enum read_state_options {
  SINGLE_CHAR,
  STREAM_LENGTH,
  STREAM_DATA,
  QUAD_CHAR_1,
  QUAD_CHAR_2,
  QUAD_CHAR_3,
};


//Extern variables (from main sketch file)
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;
extern State myState;

//Extern Functions (from main sketch file)
extern int setAnalogInputSource(int);
extern int setInputAnalogVsPDM(int);
//extern void setInputMixer(int);
extern float incrementInputGain(float);

extern void incrementKnobGain(float);
extern float getChannelLinearGain_dB(int chan);
extern void printGainSettings(void);
extern void togglePrintAveSignalLevels(bool);
//extern void incrementDSLConfiguration(Stream *);
extern void setAlgorithmPreset(int);
extern void updateDSL(BTNRH_WDRC::CHA_DSL &);
float updateDSL_linearGain(int,float);
float updateDSL_compressionRatio(int,float);
float updateDSL_compressionKnee(int,float);
float updateDSL_limitter(int,float);
extern void updateGHA(BTNRH_WDRC::CHA_WDRC &);
extern void updateAFC(BTNRH_WDRC::CHA_AFC &);
extern int configureFrontRearMixer(int);
extern int setTargetRearDelay_samps(int);
extern int configureLeftRightMixer(int);
extern float incrementRearMicGain(float);
extern bool enableAFC(bool);
extern void printCompressorSettings(void);
extern void reloadCurrentAlgPresetFromSD(void);
extern void revertCurrentAlgPresetToDefault(void);


//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(int n,
          AudioControlTestAmpSweep_F32 &_ampSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester_filterbank,
          AudioEffectFeedbackCancel_Local_F32 &_feedbackCancel,
          AudioEffectFeedbackCancel_Local_F32 &_feedbackCancelR
          )
      : ampSweepTester(_ampSweepTester), 
        freqSweepTester(_freqSweepTester),
        freqSweepTester_filterbank(_freqSweepTester_filterbank),
        feedbackCanceler(_feedbackCancel),
        feedbackCancelerR(_feedbackCancelR)
        {
          N_CHAN = n;
          serial_read_state = SINGLE_CHAR;
        };
      
    void respondToByte(char c);
    void processSingleCharacter(char c);
    void processStream(void);
    void interpretQuadChar(char mode_char, char chan_char, char data_char);
    void printHelp(void);
    void incrementChannelGain(int chan, float change_dB);
    void incrementCompressionRatio(int chan, float change);
    void incrementCompressionKnee(int chan, float change_dB);
    void incrementLimitter(int chan, float change_dB);
    
    void decreaseChannelGain(int chan);
    void set_N_CHAN(int _n_chan) { N_CHAN = _n_chan; };
    void printChanUpMsg(int N_CHAN);
    void printChanDownMsg(int N_CHAN);
    void interpretStreamGHA(int idx);
    void interpretStreamDSL(int idx);
    void interpretStreamAFC(int idx);
    void sendStreamDSL(const BTNRH_WDRC::CHA_DSL &this_dsl);
    void sendStreamGHA(const BTNRH_WDRC::CHA_WDRC &this_gha);
    void sendStreamAFC(const BTNRH_WDRC::CHA_AFC &this_afc);
    int readStreamIntArray(int idx, int* arr, int len);
    int readStreamFloatArray(int idx, float* arr, int len);

    void printTympanRemoteLayout(void);
    void printCPUtoGUI(unsigned long, unsigned long);
    void setFullGUIState(void);
    void setInputConfigButtons(void);    
    void setButtonState_algPresets(void);
    void setButtonState_afc(void);
    void setButtonState_rearMic(void);
    void setButtonState_frontRearMixer(void);
    void setButtonState_inputMixer(void);
    void setButtonState_gains(void);
    void setButtonState_tunerGUI(void);
    void setSDRecordingButtons(void);  
    void setButtonState(String btnId, bool newState);
    void setButtonText(String btnId, String text);
    String channelGainAsString(int chan);

    float channelGainIncrement_dB = 2.5f;  
    float compKneeIncrement_dB = 2.5f;
    float compressionRatioIncrement = 0.1f;
    float limitterIncrement_dB = 1.0f;
    float rearMicGainIncrement_dB = 0.5f;
    int N_CHAN;
    int serial_read_state; // Are we reading one character at a time, or a stream?
    char stream_data[MAX_DATASTREAM_LENGTH];
    int stream_length;
    int stream_chars_received;
    //float FAKE_GAIN_LEVEL[8] = {0.,0.,0.,0.,0.,0.,0.,0.};

  private:
    AudioControlTestAmpSweep_F32 &ampSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester_filterbank;
    AudioEffectFeedbackCancel_Local_F32 &feedbackCanceler;
    AudioEffectFeedbackCancel_Local_F32 &feedbackCancelerR;
};

#define MAX_CHANS 8
void SerialManager::printChanUpMsg(int N_CHAN) {
  char fooChar[] = "12345678";
  myTympan.print(" ");
  for (int i=0;i<min(MAX_CHANS,N_CHAN);i++) {
    myTympan.print(fooChar[i]); 
    if (i < (N_CHAN-1)) myTympan.print(",");
  }
  myTympan.print(": Increase linear gain of given channel (1-");
  myTympan.print(N_CHAN);
  myTympan.print(") by ");
}
void SerialManager::printChanDownMsg(int N_CHAN) {
  char fooChar[] = "!@#$%^&*";
  myTympan.print(" ");
  for (int i=0;i<min(MAX_CHANS,N_CHAN);i++) {
    myTympan.print(fooChar[i]); 
    if (i < (N_CHAN-1)) myTympan.print(",");
  }
  myTympan.print(": Decrease linear gain of given channel (1-");
  myTympan.print(N_CHAN);
  myTympan.print(") by ");
}
void SerialManager::printHelp(void) {  
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println(" h: Print this help");
  myTympan.println(" g: Print the gain settings of the device.");
  myTympan.println(" c/C: Enable/disable printing of CPU and Memory");
  myTympan.println(" w/W/e/E: Inputs: Use PCB Mics, Mic on Jack, Line on Jack, PDM Earpieces");
  myTympan.println(" t/T/H: Inputs: Use Front Mic, Inverted-Delayed Mix of Mics, or Rear Mic");
  myTympan.print  (" i/I: Rear Mic: incr/decr rear gain by "); myTympan.print(rearMicGainIncrement_dB); myTympan.println(" dB");
  myTympan.print  (" y/Y: Rear Mic: incr/decr rear delay by one sample (currently "); myTympan.print(myState.targetRearDelay_samps); myTympan.println(")");
  myTympan.println(" l: Toggle printing of pre-gain per-channel signal levels (dBFS)");
  myTympan.println(" L: Toggle printing of pre-gain per-channel signal levels (dBSPL, per DSL 'maxdB')");
  myTympan.println(" A/a: Self-Generated Test: Amplitude sweep (1kHz/250Hz).  End-to-End Measurement.");
  myTympan.println(" F: Self-Generated Test: Frequency sweep.  End-to-End Measurement.");
  myTympan.println(" f: Self-Generated Test: Frequency sweep.  Measure filterbank.");
  myTympan.print(" k/K: Increase/Decrease gain of all channels (ie, knob gain) by "); myTympan.print(channelGainIncrement_dB); myTympan.println(" dB");
  myTympan.println(" q,Q: Mute or Unmute the audio.");
  myTympan.println(" s,S,B: Left, Right, or Mono-mix the audio");
  printChanUpMsg(N_CHAN);  myTympan.print(channelGainIncrement_dB); myTympan.println(" dB");
  printChanDownMsg(N_CHAN);  myTympan.print(channelGainIncrement_dB); myTympan.println(" dB");
  myTympan.println(" d,D,G: Switch to WDRC Preset A, Full-On, RTS");
  myTympan.println(" {,>,[: Preset: Save to SD / Reload from SD / Revert to Factory Default");
  //myTympan.println("   b: Print compressor settings.");
  myTympan.print(  " p,P: Enable or Disable Adaptive Feedback Cancelation.");myTympan.println();
  myTympan.print(  " m,M: Increase or Decrease AFC mu (currently "); myTympan.print(feedbackCanceler.getMu(),6) ; myTympan.println(").");
  myTympan.print(  " r,R: Increase or Decrease AFC rho (currently "); myTympan.print(feedbackCanceler.getRho(),6) ; myTympan.println(").");
  myTympan.print(  " n,N: Increase or Decrease AFC eps (currently "); myTympan.print(feedbackCanceler.getEps(),6) ; myTympan.println(").");
  //myTympan.print(" x,X: Increase or Decrease AFC filter length (currently "); myTympan.print(feedbackCanceler.getAfl()) ; myTympan.println(").");
  //myTympan.print(" u,U: Increase or Decrease Cutoff Frequency of HP Prefilter (currently "); myTympan.print(myTympan.getHPCutoff_Hz()); myTympan.println(" Hz).");
  myTympan.print(  " z,Z: Increase or Decrease AFC N_Coeff_To_Zero (currently "); myTympan.print(feedbackCanceler.getNCoeffToZero()) ; myTympan.println(").");  
  myTympan.println(" ?: Print estimated feedback impulse response.");
  //myTympan.println(" J: Print the JSON config object, for the Tympan Remote app");
  myTympan.println(" ],}: Enable/Disable printing of data to plot.");
  myTympan.println(" `,~,|: SD: begin/stop/deleteAll recording");  
  myTympan.println();
}


void SerialManager::interpretQuadChar(char mode_char, char chan_char, char data_char) {
  int chan=0;
  float float_val;
  bool is_ok = false;

  //are we in persistent mode?
  if (mode_char == QUADCHAR_CHAR__USE_PERSISTENT_MODE) mode_char = myState.GUI_tuner_persistent_mode;
  
  switch (mode_char) {
    case '|': //change the persistent mode
      Serial.print("SerialManager: interpretQuadChar: changing persistent mode to: "); Serial.println(data_char);
      myState.GUI_tuner_persistent_mode = data_char;
      setButtonState_tunerGUI();
      break;
    case 'g': // linear gain
      chan = chan_char - '0'; //Interpret character as an integer.  For some reason, I couldn't get atoi() to work
      if (chan >= 1) { //assumes counts from zero
        if (data_char == '+') { float_val = 1.0; is_ok = true; } else if (data_char == '-') { float_val = -1.0; is_ok = true; }
      }
      if (is_ok) {
          incrementChannelGain(chan-1, float_val*channelGainIncrement_dB);
          setButtonState_tunerGUI();
          sendStreamDSL(myState.wdrc_perBand);
      }
      break;
    case 'c': //compression ratio
      chan = chan_char - '0'; //Interpret character as an integer.  For some reason, I couldn't get atoi() to work
      if (chan >= 1) { //assumes counts from zero
        if (data_char == '+') { float_val = 1.0; is_ok = true; } else if (data_char == '-') { float_val = -1.0; is_ok = true; }
      }
      if (is_ok) {
          incrementCompressionRatio(chan-1, float_val*compressionRatioIncrement);
          setButtonState_tunerGUI();
          sendStreamDSL(myState.wdrc_perBand); break;
      }
      break;
    case 'k': //compression knee
      chan = chan_char - '0'; //Interpret character as an integer.  For some reason, I couldn't get atoi() to work
      if (chan >= 1) { //assumes counts from zero
        if (data_char == '+') { float_val = 1.0; is_ok = true; } else if (data_char == '-') { float_val = -1.0; is_ok = true; }
      }
      if (is_ok) {
          incrementCompressionKnee(chan-1, float_val*compKneeIncrement_dB);
          setButtonState_tunerGUI();
          sendStreamDSL(myState.wdrc_perBand); break;
      }      
    case 'b': //bolt limiter
      chan = chan_char - '0'; //Interpret character as an integer.  For some reason, I couldn't get atoi() to work
      if (chan >= 1) { //assumes counts from zero
        if (data_char == '+') { float_val = 1.0; is_ok = true; } else if (data_char == '-') { float_val = -1.0; is_ok = true; }
      }
      if (is_ok) {
          incrementLimitter(chan-1, float_val*limitterIncrement_dB);
          setButtonState_tunerGUI();
          sendStreamDSL(myState.wdrc_perBand); break;
      }
      break;           
  }
}

//switch yard to determine the desired action
char mode_char, chan_char, data_char;
void SerialManager::respondToByte(char c) {
  switch (serial_read_state) {
    case SINGLE_CHAR:
      if (c == DATASTREAM_START_CHAR) {
        Serial.println("Start data stream.");
        // Start a data stream:
        serial_read_state = STREAM_LENGTH;
        stream_chars_received = 0;
      } else if (c == QUADCHAR_START_CHAR) {
        serial_read_state = QUAD_CHAR_1;
      } else {
        if ((c != '\n') && (c != '\r')) {
          Serial.print("Processing character ");
          Serial.println(c);
          processSingleCharacter(c);
        }
      }
      break;
    case STREAM_LENGTH:
      //Serial.print("Reading stream length char: ");
      //Serial.print(c);
      //Serial.print(" = ");
      //Serial.println(c, HEX);
      if (c == DATASTREAM_SEPARATOR) {
        // Get the datastream length:
        stream_length = (int)( ((unsigned int)stream_data[0] << 0) + ((unsigned int)stream_data[1] << 8) + ((unsigned int)stream_data[2] << 16) + ((unsigned int)stream_data[3] << 24) );
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
    case QUAD_CHAR_1:
      if ((c == '\n') || (c == '\r')) { serial_read_state = SINGLE_CHAR; return; } // abort
      if (c == DATASTREAM_START_CHAR) { Serial.println("Start data stream."); serial_read_state = STREAM_LENGTH; stream_chars_received = 0; return; }//abort into stream mode
      mode_char = c; chan_char = (char)NULL; data_char = (char)NULL;
      serial_read_state = QUAD_CHAR_2;
      break;
    case QUAD_CHAR_2:
      if ((c == '\n') || (c == '\r')) { serial_read_state = SINGLE_CHAR; return; } // abort
      if (c == DATASTREAM_START_CHAR) { Serial.println("Start data stream."); serial_read_state = STREAM_LENGTH; stream_chars_received = 0; return;} //abort into stream mode
      chan_char = c;
      serial_read_state = QUAD_CHAR_3;
      break;
    case QUAD_CHAR_3:
      if ((c == '\n') || (c == '\r')) { serial_read_state = SINGLE_CHAR; return; } // abort
      if (c == DATASTREAM_START_CHAR) { Serial.println("Start data stream."); serial_read_state = STREAM_LENGTH; stream_chars_received = 0; return; }//abort into stream mode
      data_char = c;
      //Serial.print("Serial_Manager: respondToByte: Quad Char "); Serial.print(mode_char); Serial.print(", "); Serial.print(chan_char); Serial.print(", ");Serial.println(data_char);
      interpretQuadChar(mode_char, chan_char, data_char);
      serial_read_state = SINGLE_CHAR;
      break;      
  }
}

void SerialManager::processSingleCharacter(char c) {
  float old_val = 0.0, new_val = 0.0;
  switch (c) {
    case 'h':
      printHelp(); break;
    case 'g':
      printGainSettings(); break;
    case 'k':
      incrementKnobGain(channelGainIncrement_dB); 
      setButtonState_gains();
      setButtonState("inp_mute",false);
      break;
    case 'K':   //which is "shift k"
      incrementKnobGain(-channelGainIncrement_dB); 
      setButtonState_gains(); 
      setButtonState("inp_mute",false);
      break;
    case '1':
      incrementChannelGain(1-1, channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand); break;
    case '2':
      incrementChannelGain(2-1, channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand); break;
    case '3':
      incrementChannelGain(3-1, channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand); break;
    case '4':
      incrementChannelGain(4-1, channelGainIncrement_dB); setButtonState_gains();sendStreamDSL(myState.wdrc_perBand);  break;
    case '5':
      incrementChannelGain(5-1, channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand);   break;
    case '6':
      incrementChannelGain(6-1, channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand);   break;
    case '7':
      incrementChannelGain(7-1, channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand);   break;
    case '8':      
      incrementChannelGain(8-1, channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand);   break;    
    case '!':  //which is "shift 1"
      incrementChannelGain(1-1, -channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand); break;  
    case '@':  //which is "shift 2"
      incrementChannelGain(2-1, -channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand); break;  
    case '#':  //which is "shift 3"
      incrementChannelGain(3-1, -channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand); break;  
    case '$':  //which is "shift 4"
      incrementChannelGain(4-1, -channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand);  break;
    case '%':  //which is "shift 5"
      incrementChannelGain(5-1, -channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand);  break;
    case '^':  //which is "shift 6"
      incrementChannelGain(6-1, -channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand);  break;
    case '&':  //which is "shift 7"
      incrementChannelGain(7-1, -channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand); break;
    case '*':  //which is "shift 8"
      incrementChannelGain(8-1, -channelGainIncrement_dB);setButtonState_gains();sendStreamDSL(myState.wdrc_perBand); break;          
    case 'A':case 'a':
      //amplitude sweep test
      { //limit the scope of any variables that I create here
        if (c == 'a') {
          ampSweepTester.setSignalFrequency_Hz(250.f);
        } else {
          ampSweepTester.setSignalFrequency_Hz(1000.f);
        }
        float start_amp_dB = -100.0f, end_amp_dB = 0.0f, step_amp_dB = 5.0f;
        ampSweepTester.setStepPattern(start_amp_dB, end_amp_dB, step_amp_dB);
        ampSweepTester.setTargetDurPerStep_sec(1.0);
      }
      myTympan.println("Received: starting test using amplitude sweep...");
      ampSweepTester.begin();
      while (!ampSweepTester.available()) {delay(100);};
      myTympan.println("Press 'h' for help...");
      break;
    case 'c':
      Serial.println("Received: printing memory and CPU.");
      myState.flag_printCPUandMemory = true;
      myState.flag_printCPUtoGUI = true;
      setButtonState("cpuStart",true);
      break;
    case 'C':
      Serial.println("Received: stopping printing of memory and CPU.");
      myState.flag_printCPUandMemory = false;
      myState.flag_printCPUtoGUI = false;
      setButtonState("cpuStart",false);
      break;
    case ']':
      myTympan.println("Received: printing plot data.");
      myState.flag_printPlottableData = true;
      setButtonState("printStart",true);
      break;
    case '}':
      myTympan.println("Received: stopping printing plot data.");
      myState.flag_printPlottableData = false;
      setButtonState("printStart",false);
      break;        
    case 'd':
      myTympan.println("Received: changing to WDRC Preset");
      {
        int prev_Nchan = myState.getNChan();
        setAlgorithmPreset(State::ALG_PRESET_A); //This could change the number of DSL channels.  
        if (prev_Nchan == myState.getNChan()) {
          //number of DSL channels is the same, so just update the button states and text values
          setFullGUIState(); //update the App's GUI (button values and displays)
        } else {
          //the number of DSL channels has changed, so re-send the whole GUI
          printTympanRemoteLayout();
        }
        sendStreamDSL(myState.wdrc_perBand); sendStreamGHA(myState.wdrc_broadband); sendStreamAFC(myState.afc); 
      }  
      break;      
    case 'D':
      myTympan.println("Received: changing to Full-On Gain");
      {
        int prev_Nchan = myState.getNChan();
        setAlgorithmPreset(State::ALG_PRESET_B); //This could change the number of DSL channels.  
        if (prev_Nchan == myState.getNChan()) {
          //number of DSL channels is the same, so just update the button states and text values
          setFullGUIState(); //update the App's GUI (button values and displays)
        } else {
          //the number of DSL channels has changed, so re-send the whole GUI
          printTympanRemoteLayout();
        }
        sendStreamDSL(myState.wdrc_perBand); sendStreamGHA(myState.wdrc_broadband); sendStreamAFC(myState.afc); 
      }
      break;
     case 'G':
      myTympan.println("Received: changing to RTS Preset");
      {
        int prev_Nchan = myState.getNChan();
        setAlgorithmPreset(State::ALG_PRESET_C); //This could change the number of DSL channels.  
        if (prev_Nchan == myState.getNChan()) {
          //number of DSL channels is the same, so just update the button states and text values
          setFullGUIState(); //update the App's GUI (button values and displays)
        } else {
          //the number of DSL channels has changed, so re-send the whole GUI
          printTympanRemoteLayout();
        }
        sendStreamDSL(myState.wdrc_perBand); sendStreamGHA(myState.wdrc_broadband); sendStreamAFC(myState.afc); 
      }
      break;  
    case '[':
      myTympan.println("Received: save preset to SD");
      myState.saveCurrentAlgPresetToSD(true); //true says to write it to SD
      break;
    case '{':
      {
        int prev_Nchan = myState.getNChan();
        myTympan.println("Received: revert preset to factory default");
        revertCurrentAlgPresetToDefault(); //This could change the number of DSL channels.  
        if (prev_Nchan == myState.getNChan()) {
          //number of DSL channels is the same, so just update the button states and text values
          setFullGUIState(); //update the App's GUI (button values and displays)
        } else {
          //the number of DSL channels has changed, so re-send the whole GUI
          printTympanRemoteLayout();
        }
        sendStreamDSL(myState.wdrc_perBand);      
        sendStreamGHA(myState.wdrc_broadband);      
        sendStreamAFC(myState.afc); 
      }
      break;
    case '>':
      {
        int prev_Nchan = myState.getNChan();
        myTympan.println("Received: reload preset from SD");
         reloadCurrentAlgPresetFromSD();//This could change the number of DSL channels.  
        if (prev_Nchan == myState.getNChan()) {
          //number of DSL channels is the same, so just update the button states and text values
          setFullGUIState(); //update the App's GUI (button values and displays)
        } else {
          //the number of DSL channels has changed, so re-send the whole GUI
          printTympanRemoteLayout();
        }
        sendStreamDSL(myState.wdrc_perBand);      
        sendStreamGHA(myState.wdrc_broadband);      
        sendStreamAFC(myState.afc); 
      }
      break;
    case 'F':
      //frequency sweep test...end-to-end
      { //limit the scope of any variables that I create here
        freqSweepTester.setSignalAmplitude_dBFS(-70.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
        freqSweepTester.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester.setTargetDurPerStep_sec(1.0);
      }
      myTympan.println("Received: starting test using frequency sweep, end-to-end assessment...");
      freqSweepTester.begin();
      while (!freqSweepTester.available()) {delay(100);};
      myTympan.println("Press 'h' for help...");
      break; 
    case 'f':
      //frequency sweep test
      { //limit the scope of any variables that I create here
        freqSweepTester_filterbank.setSignalAmplitude_dBFS(-30.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
        freqSweepTester_filterbank.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester_filterbank.setTargetDurPerStep_sec(0.5);
      }
      myTympan.println("Received: starting test using frequency sweep.  Filterbank assessment...");
      freqSweepTester_filterbank.begin();
      while (!freqSweepTester_filterbank.available()) {delay(100);};
      myTympan.println("Press 'h' for help...");
      break; 
   case 'w':
      myTympan.println("Received: Listen to PCB mics");
      setInputAnalogVsPDM(State::INPUT_ANALOG);
      setAnalogInputSource(State::INPUT_PCBMICS);
      //setInputMixer(State::MIC_AIC0_LR);  //PCB Mics are only on the main Tympan board
      setFullGUIState();
      break;
    case 'W':
      myTympan.println("Received: Mic jack as mic");
      setInputAnalogVsPDM(State::INPUT_ANALOG);
      setAnalogInputSource(State::INPUT_MICJACK_MIC);
      //setInputMixer(myState.analog_mic_config); //could be on either board, so remember the last-used state
      setFullGUIState();
      break;
    case 'e':
      myTympan.println("Received: Mic jack as line-in");
      setInputAnalogVsPDM(State::INPUT_ANALOG);
      setAnalogInputSource(State::INPUT_MICJACK_LINEIN);
      //setInputMixer(myState.analog_mic_config);
      setFullGUIState();
      break;     
    case 'E':
      myTympan.println("Received: Listen to PDM mics"); //digital mics
      setInputAnalogVsPDM(State::INPUT_PDM);
      //setInputMixer(myState.digital_mic_config);
      setFullGUIState();
      break;  
    case 't':
      myTympan.println("Received: Front Mics (if using earpieces)"); 
      configureFrontRearMixer(State::MIC_FRONT);
      setButtonState_frontRearMixer();
      break;
    case 'T':
      myTympan.println("Received: Mix of Front Mics + Delayed Inverted Rear Mics"); 
      configureFrontRearMixer(State::MIC_BOTH_INVERTED_DELAYED);
      setButtonState_frontRearMixer();
      break;
     case 'H':
      myTympan.println("Received: Rear Mics (if using earpieces)"); 
      configureFrontRearMixer(State::MIC_REAR);
      setButtonState_frontRearMixer();
      break;
    case 'i':
      new_val = incrementRearMicGain(rearMicGainIncrement_dB);
      myTympan.print("Adjusting rear mic gain to "); myTympan.print(new_val); myTympan.println(" dB");
      setButtonState_rearMic();
      break;
    case 'I':
      new_val = incrementRearMicGain(-rearMicGainIncrement_dB);
      myTympan.print("Adjusting rear mic gain to "); myTympan.print(new_val); myTympan.println(" dB");
      setButtonState_rearMic();
      break;
    case 'y':
      { 
        int new_val = myState.targetRearDelay_samps+1;      
        myTympan.print("Received: Increase rear-mic delay by one sample to ");myTympan.println(new_val);
        setTargetRearDelay_samps(new_val);
        setButtonState_frontRearMixer();
      }
      break;
    case 'Y':
      myTympan.println("Received: Reduce rear-mic delay by one sample");
      setTargetRearDelay_samps(myState.targetRearDelay_samps-1);
      setButtonState_frontRearMixer();
      break;
    case 'q':
      configureLeftRightMixer(State::INPUTMIX_MUTE);
      myTympan.println("Received: Muting audio.");
      setButtonState_inputMixer();
      break;
    case 'Q':
      configureLeftRightMixer(myState.input_mixer_nonmute_config);
      setButtonState_inputMixer();
      break;  
    case 'B':
      myTympan.println("Received: Input: Mix L+R.");
      configureLeftRightMixer(State::INPUTMIX_MONO);
      setButtonState_inputMixer();
      break;  
    case 's':
      configureLeftRightMixer(State::INPUTMIX_BOTHLEFT);
      myTympan.println("Received: Input: Both Left.");
      setButtonState_inputMixer();
      break;
    case 'S':
      configureLeftRightMixer(State::INPUTMIX_BOTHRIGHT);
      myTympan.println("Received: Input: Both Right.");
      setButtonState_inputMixer();
      break;   
    case 'J':
    {
      printTympanRemoteLayout();
      delay(20);
      sendStreamDSL(myState.wdrc_perBand);
      sendStreamGHA(myState.wdrc_broadband);
      sendStreamAFC(myState.afc);
      setFullGUIState();
      break;
    }
    case 'l':
      myTympan.println("Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = false; togglePrintAveSignalLevels(as_dBSPL); }
      break;
    case 'L':
      myTympan.println("Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = true; togglePrintAveSignalLevels(as_dBSPL); }
      break;
    case 'p':
      myTympan.println("Received: enabling feedback cancelation.");
      //feedbackCanceler.setEnable(true);feedbackCancelerR.setEnable(true);
      enableAFC(true);
      //feedbackCanceler.resetAFCfilter();
      setButtonState_afc();
      break;
    case 'P':
      myTympan.println("Received: disabling feedback cancelation.");      
      //feedbackCanceler.setEnable(false);feedbackCancelerR.setEnable(false);
      enableAFC(false);
      setButtonState_afc();
      break;
    case '?':
      feedbackCanceler.printEstimatedFeedbackImpulseResponse();
      break;
    case 'm':
      old_val = feedbackCanceler.getMu(); new_val = old_val * 2.0;
      myTympan.print("Received: increasing AFC mu to ");
      myTympan.println(feedbackCanceler.setMu(new_val),6);
      feedbackCancelerR.setMu(new_val);
      break;
    case 'M':
      old_val = feedbackCanceler.getMu(); new_val = old_val / 2.0;
      myTympan.print("Received: decreasing AFC mu to "); 
      myTympan.println(feedbackCanceler.setMu(new_val),6);
      feedbackCancelerR.setMu(new_val);
      break;
    case 'r':
      old_val = feedbackCanceler.getRho(); new_val = 1.0-((1.0-old_val)/sqrt(2.0));
      myTympan.print("Received: increasing AFC rho to "); 
      myTympan.println(feedbackCanceler.setRho(new_val),6);
      feedbackCancelerR.setRho(new_val);
      break;
    case 'R':
      old_val = feedbackCanceler.getRho(); new_val = 1.0-((1.0-old_val)*sqrt(2.0));
      myTympan.print("Received: increasing AFC rho to "); 
      myTympan.println(feedbackCanceler.setRho(new_val),6);
      feedbackCancelerR.setRho(new_val);
      break;
    case 'n':
      old_val = feedbackCanceler.getEps(); new_val = old_val*sqrt(10.0);
      myTympan.print("Received: increasing AFC eps to "); 
      myTympan.println(feedbackCanceler.setEps(new_val),6);
      feedbackCancelerR.setEps(new_val);
      break;
    case 'N':
      old_val = feedbackCanceler.getEps(); new_val = old_val/sqrt(10.0);
      myTympan.print("Received: increasing AFC eps to ");
      myTympan.println(feedbackCanceler.setEps(new_val),6);
      feedbackCancelerR.setEps(new_val);
      break;    
//    case 'x':
//      old_val = feedbackCanceler.getAfl(); new_val = old_val + 5;
//      myTympan.print("Received: increasing AFC filter length to ");
//      myTympan.println(feedbackCanceler.setAfl(new_val));
//      feedbackCancelerR.setAfl(new_val);
//      break;    
//    case 'X':
//      old_val = feedbackCanceler.getAfl(); new_val = old_val - 5;
//      myTympan.print("Received: decreasing AFC filter length to ");
//      myTympan.println(feedbackCanceler.setAfl(new_val));
//      feedbackCanceler.setAfl(new_val);
//      break;            
    case 'z':
      old_val = feedbackCanceler.getNCoeffToZero(); new_val = old_val + 5;
      myTympan.print("Received: increasing AFC N_Coeff_To_Zero to "); myTympan.println(feedbackCanceler.setNCoeffToZero(new_val));
      break;
    case 'Z':
      old_val = feedbackCanceler.getNCoeffToZero(); new_val = old_val - 5;
      myTympan.print("Received: decreasing AFC N_Coeff_To_Zero to "); myTympan.println(feedbackCanceler.setNCoeffToZero(new_val));      
      break;
    case 'u':
    {
      old_val = myTympan.getHPCutoff_Hz(); new_val = min(old_val*sqrt(2.0), 8000.0); //half-octave steps up
      float fs_Hz = myTympan.getSampleRate_Hz();
      myTympan.setHPFonADC(true,new_val,fs_Hz);
      myTympan.print("Received: Increasing ADC HP Cutoff to "); myTympan.print(myTympan.getHPCutoff_Hz());myTympan.println(" Hz");
    }
      break;
    case 'U':
    {
      old_val = myTympan.getHPCutoff_Hz(); new_val = max(old_val/sqrt(2.0), 5.0); //half-octave steps down
      float fs_Hz = myTympan.getSampleRate_Hz();
      myTympan.setHPFonADC(true,new_val,fs_Hz);
      myTympan.print("Received: Decreasing ADC HP Cutoff to "); myTympan.print(myTympan.getHPCutoff_Hz());myTympan.println(" Hz");   
      break;
    }
    case '`':
      myTympan.println("Received: begin SD recording");
      audioSDWriter.startRecording();
      setSDRecordingButtons();
      break;
    case '~':
      myTympan.println("Received: stop SD recording");
      audioSDWriter.stopRecording();
      setSDRecordingButtons();
      break;
    case '|':
      myTympan.println("Recieved: delete all SD recordings.");
      audioSDWriter.stopRecording();
      audioSDWriter.deleteAllRecordings();
      setSDRecordingButtons();
      myTympan.println("Delete all SD recordings complete.");

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

   // Print the layout for the Tympan Remote app, in a JSON-ish string 
  // (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).  
  char jsonConfig[] = "JSON={"
    "'pages':["
      "{'title':'Main Controls','cards':["
          //"{'name':'Overall Audio','buttons':[{'label': 'Stereo','cmd': 'Q','id':'inp_stereo','width':'6'},{'label': 'Mono','cmd': 's','id':'inp_mono','width':'6'},{'label': 'Mute','cmd': 'q','id':'inp_mute','width':'12'}]},"
          //"{'name':'Overall Audio','buttons':[{'label': 'Active','cmd': 'Q','id':'inp_stereo','width':'6'},{'label': 'Mute','cmd': 'q','id':'inp_mute','width':'6'}]},"
          //"{'name':'Select Earpiece','buttons':[{'label': 'Mute','cmd': 'q','id':'inp_mute','width':'6'},{'label': 'Mix L+R','cmd': 'Q','id':'inp_mono','width':'6'},{'label': 'Left','cmd': 's','id':'inp_micL','width':'6'},{'label': 'Right','cmd': 'S','id':'inp_micR','width':'6'}]},"          
          "{'name':'Overall Volume', 'buttons':[{'label': 'Less', 'cmd' :'QK'},{'label': 'More', 'cmd': 'Qk'},{'label': 'Mute','cmd': 'q','id':'inp_mute','width':'12'}]},"
          "{'name':'Algorithm Presets','buttons':[{'label': 'Compression (WDRC)', 'cmd': 'd', 'id': 'alg_preset0'},{'label': 'Full-On Gain', 'cmd': 'D', 'id': 'alg_preset1'},{'label': 'RTS Gain', 'cmd': 'G', 'id': 'alg_preset2'}]},"
          "{'name':'Feedback Cancellation','buttons':[{'label': 'On', 'cmd': 'p', 'id': 'afc_on'},{'label': 'Off', 'cmd': 'P', 'id': 'afc_off'}]}"
          //"{'name':'Overwrite Current Preset','buttons':[{'label': 'Save', 'cmd': '[', 'id': 'savePreset'},{'label': 'Restore', 'cmd': '{', 'id': 'restorePreset'}]}"
      "]},"   //include comma if NOT the last one

      "{'title':'WDRC Tuner','cards':["
          //"{'name':'Overall Volume',   'buttons':[{'label': 'Less',     'cmd' :'K'},{'label': 'More', 'cmd': 'k'}]},"
          "{'name':'Parameter to Change','buttons':[{'id':'modeGain','label':'Linear Gain (dB)','cmd':'_|0g','width':'12'},"
                                                   "{'id':'modeKnee','label':'Comp Knee (dB SPL)','cmd':'_|0k','width':'12'},"
                                                   "{'id':'modeCR','label':'Comp Ratio','cmd':'_|0c','width':'12'},"
                                                   "{'id':'modeLim','label':'Limitter (dB SPL)','cmd':'_|0b','width':'12'}"
                                                   "]},"
          "{'name':'', 'buttons':["
                      "{'label':'LOW','width':'3'},{'label': '-', 'cmd': '__1-','width':'3'},{'id':'tune1', 'label': '', 'width':'3'},{'label': '+', 'cmd': '__1+','width':'3'},"
                      "{'label':'Band2','width':'3'},{'label': '-', 'cmd': '__2-','width':'3'},{'id':'tune2', 'label': '', 'width':'3'},{'label': '+', 'cmd': '__2+','width':'3'},"
                      "{'label':'Band3','width':'3'},{'label': '-', 'cmd': '__3-','width':'3'},{'id':'tune3', 'label': '', 'width':'3'},{'label': '+', 'cmd': '__3+','width':'3'},"
                      "{'label':'Band4','width':'3'},{'label': '-', 'cmd': '__4-','width':'3'},{'id':'tune4', 'label': '', 'width':'3'},{'label': '+', 'cmd': '__4+','width':'3'},"
                      "{'label':'Band5','width':'3'},{'label': '-', 'cmd': '__5-','width':'3'},{'id':'tune5', 'label': '', 'width':'3'},{'label': '+', 'cmd': '__5+','width':'3'},"
                      "{'label':'HIGH','width':'3'},{'label': '-', 'cmd': '__6-','width':'3'},{'id':'tune6', 'label': '', 'width':'3'},{'label': '+', 'cmd': '__6+','width':'3'}"  // //no comma if the last one, which this one is in tis button group
                  "]},"        //no comma if the last one or, yes, comma if not the last one
          "{'name':'','buttons':[{'label': 'Save As Preset', 'cmd': '[', 'id': 'savePreset','width':'12'},{'label': 'Reload', 'cmd': '>', 'id': 'reloadPreset','width':'6'},{'label': 'Factory', 'cmd': '{', 'id': 'restorePreset','width':'6'},"
                                                    "{'label': '', 'id':'presetFname','width':'12'}]}"                  
      "]}," //include comma if NOT the last one  

      "{'title':'Digital Earpieces','cards':["
          "{'name':'Select Earpiece','buttons':[{'label': 'Left','cmd': 's','id':'inp_micL','width':'6'},{'label': 'Right','cmd': 'S','id':'inp_micR','width':'6'},{'label': 'Mix L+R','cmd': 'B','id':'inp_mono','width':'6'},{'label': 'Mute','cmd': 'q','id':'inp_mute','width':'6'}]},"          
          "{'name':'Front or Rear Mics', 'buttons':["
                     "{'label':'Front','id':'frontMic','cmd':'t','width':'4'},{'label':'Mix(F-R)','id':'frontRearMix','cmd':'T','width':'4'},{'label':'Rear','id':'rearMic','cmd':'H','width':'4'}" 
           "]},"  //include trailing comma if NOT the last one
           "{'name':'Rear Mic Gain (dB)', 'buttons':["
                    "{'label':'-','cmd':'I','width':'4'},{'label':'','id':'rearGain','width':'4'},{'label':'+','cmd':'i','width':'4'}" 
           "]},"
           "{'name':'Rear Mic Delay (samples)', 'buttons':["
                     "{'label':'-','cmd':'Y','width':'4'},{'label':'','id':'delaySamps','width':'4'},{'label':'+','cmd':'y','width':'4'}" 
           "]}"  //exclude trailing comma if it IS the last one
      "]}," //include comma if NOT the last one      
//      "{'title':'Input Select','cards':["
//          "{'name':'Audio Source', 'buttons':["
//                                             "{'label':'Digital: Earpieces', 'cmd': 'E', 'id':'configPDMMic', 'width':'12'},"
//                                             "{'label':'Analog: PCB Mics',  'cmd': 'w', 'id':'configPCBMic',  'width':'12'},"
//                                             "{'label':'Analog: Mic Jack (Mic)',  'cmd': 'W', 'id':'configMicJack', 'width':'12'},"
//                                             "{'label':'Analog: Mic Jack (Line)',  'cmd': 'e', 'id':'configLineJack', 'width':'12'}" //add a comma if you also add the line below
//                                             //"{'label':'Analog: BT Audio', 'cmd': 'E', 'id':'configLineSE',  'width':'12'}" //don't have a trailing comma on this last one
//                                            "]}" //don't have a trailing comma on this last one
//     "]}," //include comma if NOT the last one
      "{'title':'Globals','cards':["
          "{'name':'CPU Usage (%)', 'buttons':[{'label': 'Start', 'cmd' :'c','id':'cpuStart','width':'4'},{'id':'cpuValue', 'label': '', 'width':'4'},{'label': 'Stop', 'cmd': 'C','width':'4'}]},"  //add comma if you add any lines below before this line's closing quote
          "{'name':'Record Mics to SD Card','buttons':[{'label': 'Start', 'cmd': '`', 'id':'recordStart','width':'6'},{'label': 'Stop', 'cmd': '~','width':'6'},"
                                                    "{'label': '', 'id':'sdFname','width':'12'}]}"
         "]}" //no comma if last one
    "],"
    "'prescription':{'type':'BoysTown','pages':['serialMonitor','multiband','broadband','afc','plot']}"
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
    interpretStreamGHA(idx);
  } else if (streamType == "dsl") {
    myTympan.println("Stream is of type 'dsl'.");
    interpretStreamDSL(idx);
  } else if (streamType == "afc") {
    myTympan.println("Stream is of type 'afc'.");
    interpretStreamAFC(idx);
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

void SerialManager::interpretStreamGHA(int idx) {
  //float attack, release, sampRate, maxdB, compRatioLow, kneepoint, compStartGain, compStartKnee, compRatioHigh, threshold;

  //interpret the values
  BTNRH_WDRC::CHA_WDRC gha;
  gha.attack        = *((float*)(stream_data+idx)); idx=idx+4; 
  gha.release       = *((float*)(stream_data+idx)); idx=idx+4;
  gha.maxdB         = *((float*)(stream_data+idx)); idx=idx+4;
  gha.exp_cr        =  1.0;  Serial.print("SerialManager: interpretStreamGHA: no Expansion Ratio.  Assuming "); Serial.println(gha.exp_cr);
  gha.exp_end_knee  =  -100.0;  Serial.print("SerialManager: interpretStreamGHA: no Exansion Knee.  Assuming "); Serial.println(gha.exp_end_knee);
  gha.tkgain        = *((float*)(stream_data+idx)); idx=idx+4;
  gha.tk            = *((float*)(stream_data+idx)); idx=idx+4;
  gha.cr            = *((float*)(stream_data+idx)); idx=idx+4;
  gha.bolt          = *((float*)(stream_data+idx)); idx=idx+4;

  //print out values for debugging
  State::printBroadbandSettings("SerialManager::interpretStreamGHA: received broadband settings",gha);

  // send to the tympan
  updateGHA(gha);
  setFullGUIState(); //update the App's GUI
  sendStreamGHA(myState.wdrc_broadband); //send the GHA back to ensure the GUI shows the right thing
  //myTympan.println("SUCCESS.");    
}

void SerialManager::interpretStreamDSL(int idx) {
  const int maxChan = DSL_MXCH; //"DSL_MXCH" is in BTNRH_WDRC_Types.h via AudioEffectCompWDRC_F32.h
 
  BTNRH_WDRC::CHA_DSL this_dsl = myState.wdrc_perBand; //default to what we already have?

  this_dsl.attack        = *((float*)(stream_data+idx)); idx=idx+4;
  this_dsl.release       = *((float*)(stream_data+idx)); idx=idx+4;
  this_dsl.nchannel      = *((int*)(stream_data+idx)); idx=idx+4;
  this_dsl.maxdB         = *((float*)(stream_data+idx)); idx=idx+4;
  this_dsl.ear = 0; //hardwired for now

  if (this_dsl.nchannel > maxChan) {
    Serial.println("SerialManager::interpretStreamDSL: *** ERROR ***");
    Serial.print  ("  : App says numChannels = "); Serial.print(this_dsl.nchannel); Serial.print(", which is larger than "); Serial.println(maxChan);
    Serial.println("  : Ignoring DSL given by the App.  Returning.");
    return;
  }

  //interpret the rest of the values in the stream
  int nchan = this_dsl.nchannel;
  idx = readStreamFloatArray(idx, this_dsl.cross_freq, nchan); 
  idx = readStreamFloatArray(idx, this_dsl.exp_cr, nchan);
  idx = readStreamFloatArray(idx, this_dsl.exp_end_knee, nchan);
  idx = readStreamFloatArray(idx, this_dsl.tkgain, nchan);
  idx = readStreamFloatArray(idx, this_dsl.tk, nchan); //from the App, for DSL, tk comes before cr
  idx = readStreamFloatArray(idx, this_dsl.cr, nchan);  //from the App, for DSL, tk comes before cr
  idx = readStreamFloatArray(idx, this_dsl.bolt, nchan);    
  
  //print to serial for debugging
  State::printPerBandSettings("interpretStreamDSL: printing new dsl:",this_dsl); //for debugging

  //send this new data to the rest of the system
  int prev_Nchan = myState.getNChan();
  updateDSL(this_dsl);    //send it to the Tympan for setting the algorithms.  This could change the number of DSL channels.
  if (prev_Nchan == myState.getNChan()) {
    //number of DSL channels is the same, so just update the button states and text values
    setFullGUIState(); //update the App's GUI (button values and displays)
  } else {
    //the number of DSL channels has changed, so re-send the whole GUI
    printTympanRemoteLayout();
  }
  sendStreamDSL(myState.wdrc_perBand); //send the DSL back to ensure the GUI shows the right thing
  //myTympan.println("SUCCESS.");      
}

void SerialManager::interpretStreamAFC(int idx) {
  int default_to_active; //enable AFC at startup?  1=active. 0=disabled.
  int afl;  //length (samples) of adaptive filter for modeling feedback path.
  float mu; //mu, scale factor for how fast the adaptive filter adapts (bigger is faster)
  float rho;  //rho, smoothing factor for estimating audio envelope (bigger is a longer average)
  float eps;  //eps, when est the audio envelope, this is the min allowed level (avoids divide-by-zero)

  default_to_active = *((int*)(stream_data+idx)); idx=idx+4;
  afl               = *((int*)(stream_data+idx)); idx=idx+4;
  mu                = *((float*)(stream_data+idx)); idx=idx+4;
  rho               = *((float*)(stream_data+idx)); idx=idx+4;
  eps               = *((float*)(stream_data+idx)); idx=idx+4;

  BTNRH_WDRC::CHA_AFC afc = {default_to_active, afl, mu, rho, eps};

  //print to serial for debugging
  State::printAFCSettings("interpretStreamAFC: printing new afc:",afc); //for debugging

  //now update the actual values being used
  updateAFC(afc);
  sendStreamAFC(myState.afc); //send the DSL back to ensure the GUI shows the right thing 
  //myTympan.println("SUCCESS.");
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

void SerialManager::sendStreamDSL(const BTNRH_WDRC::CHA_DSL &this_dsl) {
  int i;
  int num_digits = 4; // How many spots past the decimal point are sent.  Only used for floats

  myTympan.print("PRESC=DSL:");
  myTympan.print(DSL_MXCH);
  myTympan.print(":");

  myTympan.print(this_dsl.attack,num_digits); myTympan.print(",");
  myTympan.print(this_dsl.release,num_digits); myTympan.print(",");
  myTympan.print(this_dsl.maxdB,num_digits); myTympan.print(",");
  myTympan.print(this_dsl.ear); myTympan.print(","); // int
  myTympan.print(this_dsl.nchannel); myTympan.print(","); // int
  for (i=0; i<this_dsl.nchannel; i++) { myTympan.print(this_dsl.cross_freq[i], num_digits); myTympan.print(","); }
  for (i=0; i<this_dsl.nchannel; i++) { myTympan.print(this_dsl.exp_cr[i], num_digits); myTympan.print(","); }
  for (i=0; i<this_dsl.nchannel; i++) { myTympan.print(this_dsl.exp_end_knee[i], num_digits); myTympan.print(","); }
  for (i=0; i<this_dsl.nchannel; i++) { myTympan.print(this_dsl.tkgain[i], num_digits); myTympan.print(","); }
  for (i=0; i<this_dsl.nchannel; i++) { myTympan.print(this_dsl.tk[i], num_digits); myTympan.print(","); }
  for (i=0; i<this_dsl.nchannel; i++) { myTympan.print(this_dsl.cr[i], num_digits); myTympan.print(","); }
  for (i=0; i<this_dsl.nchannel; i++) { myTympan.print(this_dsl.bolt[i], num_digits); myTympan.print(","); }

  myTympan.println(DSL_MXCH); // We can double-check this on the other end to confirm we got the right thing
}

void SerialManager::sendStreamGHA(const BTNRH_WDRC::CHA_WDRC &this_gha) {
  int num_digits = 4; // How many spots past the decimal point are sent.  Only used for floats
  int checkVal = 11; // Arbitrary number, just to make sure we get the same number at the beginning and the end (as a double-check on the string parsing)

  myTympan.print("PRESC=GHA:");
  myTympan.print(checkVal);
  myTympan.print(":");
  myTympan.print(this_gha.attack,        num_digits); myTympan.print(",");
  myTympan.print(this_gha.release,       num_digits); myTympan.print(",");
  myTympan.print(this_gha.fs,            num_digits); myTympan.print(",");
  myTympan.print(this_gha.maxdB,         num_digits); myTympan.print(",");
  myTympan.print(this_gha.exp_cr,        num_digits); myTympan.print(",");
  myTympan.print(this_gha.exp_end_knee,  num_digits); myTympan.print(",");
  myTympan.print(this_gha.tkgain,        num_digits); myTympan.print(",");
  myTympan.print(this_gha.tk,            num_digits); myTympan.print(",");
  myTympan.print(this_gha.cr,            num_digits); myTympan.print(",");
  myTympan.print(this_gha.bolt,          num_digits); myTympan.print(",");
  myTympan.println(checkVal);
};

void SerialManager::sendStreamAFC(const BTNRH_WDRC::CHA_AFC &this_afc) {
  int num_digits = 4; // How many spots past the decimal point are sent.  Only used for floats
  int checkVal = 11; // Arbitrary number, just to make sure we get the same number at the beginning and the end (as a double-check on the string parsing)

  myTympan.print("PRESC=AFC:");
  myTympan.print(checkVal);
  myTympan.print(":");

  myTympan.print(this_afc.default_to_active); myTympan.print(",");
  myTympan.print(this_afc.afl); myTympan.print(",");
  myTympan.print(this_afc.mu, num_digits); myTympan.print(",");
  myTympan.print(this_afc.rho, num_digits); myTympan.print(",");
  myTympan.print(this_afc.eps, num_digits); myTympan.print(",");

  myTympan.println(checkVal);
};

void SerialManager::incrementChannelGain(int chan, float change_dB) { //chan counts from zero
  if ((chan >= 0) && (chan < N_CHAN)) {
    myState.wdrc_perBand.tkgain[chan] += change_dB;
    updateDSL_linearGain(chan,myState.wdrc_perBand.tkgain[chan]);
    //printGainSettings();  //in main sketch file
  }
}
void SerialManager::incrementCompressionRatio(int chan, float change) { //chan counts from zero
  if ((chan >= 0) && (chan < N_CHAN)) {
    myState.wdrc_perBand.cr[chan] += change;
    updateDSL_compressionRatio(chan,myState.wdrc_perBand.cr[chan]);
  }
}
void SerialManager::incrementCompressionKnee(int chan, float change_dB) { //chan counts from zero
  if ((chan >= 0) && (chan < N_CHAN)) {
    myState.wdrc_perBand.tk[chan] += change_dB;
    updateDSL_compressionKnee(chan,myState.wdrc_perBand.tk[chan]);
  }
}
void SerialManager::incrementLimitter(int chan, float change_dB) { //chan counts from zero
  if ((chan >= 0) && (chan < N_CHAN)) {
    myState.wdrc_perBand.bolt[chan] += change_dB;
    updateDSL_limitter(chan,myState.wdrc_perBand.bolt[chan]);
  }
}


String SerialManager::channelGainAsString(int chan) {  //channels start from zero
  //return String(FAKE_GAIN_LEVEL[chan],1);
  return String(getChannelLinearGain_dB(chan),1);
}

void SerialManager::printCPUtoGUI(unsigned long curTime_millis = 0, unsigned long updatePeriod_millis = 0) {
  static unsigned long lastUpdate_millis = 0;
  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    setButtonText("cpuValue", String(myState.getCPUUsage(),1));
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

void SerialManager::setFullGUIState(void) {
  setButtonState_algPresets();
  setButtonState_afc();
  setButtonState_rearMic();
  setButtonState_frontRearMixer();
  setButtonState_inputMixer();
  setInputConfigButtons();

  setButtonState_gains();
  setButtonState_tunerGUI();
  setSDRecordingButtons();  
  
    
  //add something here to send prescription values to the remote device

  //update buttons related to the "print" states
  if (myState.flag_printCPUandMemory) {
    setButtonState("cpuStart",true);
  } else {
    setButtonState("cpuStart",false);
  }
  if (myState.flag_printPlottableData) {
    setButtonState("plotStart",true);
  } else {
    setButtonState("plotStart",false);
  }

}

void SerialManager::setButtonState_algPresets(void) {
  setButtonState("alg_preset0",false);  delay(3);
  setButtonState("alg_preset1",false); delay(3);
  setButtonState("alg_preset2",false); delay(3);
  switch (myState.current_alg_config) {
    case (State::ALG_PRESET_A):
      setButtonState("alg_preset0",true);  delay(3); break;
    case (State::ALG_PRESET_B):
      setButtonState("alg_preset1",true); delay(3); break;
    case (State::ALG_PRESET_C):
      setButtonState("alg_preset2",true); delay(3); break;      
  }
  setButtonText("presetFname",myState.preset_fnames[myState.current_alg_config]);
}

void SerialManager::setButtonState_rearMic(void) {
  setButtonText("rearGain",String(myState.rearMicGain_dB,1));
}
void SerialManager::setButtonState_frontRearMixer(void) {
  setButtonState("frontMic",false);
  setButtonState("frontRearMix",false);
  setButtonState("rearMic",false);

  switch (myState.input_frontrear_config) {
    case State::MIC_FRONT:
      setButtonState("frontMic",true);
      break;
    case State::MIC_BOTH_INVERTED_DELAYED:
      setButtonState("frontRearMix",true);
      break;
    case State::MIC_REAR:
      setButtonState("rearMic",true);
      break;    
  }
  Serial.print("setButtonState_frontRearMixer: setting delaySamps field to ");
  Serial.println(String(myState.currentRearDelay_samps));
  setButtonText("delaySamps",String(myState.currentRearDelay_samps));
}
void SerialManager::setButtonState_inputMixer(void) {
   setButtonState("inp_mute",false);
   setButtonState("inp_mono",false); 
   setButtonState("inp_micL",false);
   setButtonState("inp_micR",false);
   //setButtonState("inp_stereo",flae);  delay(3); 
  
        
  switch (myState.input_mixer_config) {
    case (State::INPUTMIX_STEREO):
      setButtonState("inp_stereo",true);  delay(3); 
      break;
    case (State::INPUTMIX_MONO):
      setButtonState("inp_mono",true); delay(3);
      break;
    case (State::INPUTMIX_MUTE):
      setButtonState("inp_mute",true);  delay(3); 
      break;
    case (State::INPUTMIX_BOTHLEFT):
      setButtonState("inp_micL",true);  delay(3); 
      break;
    case (State::INPUTMIX_BOTHRIGHT):
      setButtonState("inp_micR",true);  delay(3); 
      break;
  }
}

void SerialManager::setInputConfigButtons(void) {
  //clear out previous state of buttons
  setButtonState("configPDMMic",false);  delay(3);
  setButtonState("configPCBMic",false);  delay(3);
  setButtonState("configMicJack",false); delay(3);
  setButtonState("configLineJack",false);delay(3);
  setButtonState("configLineSE",false);  delay(3);

  //set the new state of the buttons
  switch (myState.input_analogVsPDM) {
    case (State::INPUT_PDM):
      setButtonState("configPDMMic",true);  delay(3); break;
    default:
      switch (myState.input_analog_config) {
        //case (State::INPUT_PDMMICS):
        //  setButtonState("configPDMMic",true);  delay(3); break;
        case (State::INPUT_PCBMICS):
          setButtonState("configPCBMic",true);  delay(3); break;
        case (State::INPUT_MICJACK_MIC): 
          setButtonState("configMicJack",true); delay(3); break;
        case (State::INPUT_LINEIN_SE): 
          setButtonState("configLineSE",true);  delay(3); break;
        case (State::INPUT_MICJACK_LINEIN): 
          setButtonState("configLineJack",true);delay(3); break;
      }  
      break;
  }
}

void SerialManager::setButtonState_afc(void) {
  if (myState.afc.default_to_active) {
    setButtonState("afc_on",true);
    setButtonState("afc_off",false);
  } else {
    setButtonState("afc_on",false);
    setButtonState("afc_off",true);
  }   
}

void SerialManager::setButtonState_gains(void) {
  
//  setButtonText("gain1",channelGainAsString(1-1));
//  setButtonText("gain2",channelGainAsString(2-1));
//  setButtonText("gain3",channelGainAsString(3-1));
//  setButtonText("gain4",channelGainAsString(4-1));
//  setButtonText("gain5",channelGainAsString(5-1));
//  setButtonText("gain6",channelGainAsString(6-1));
}

void SerialManager::setButtonState_tunerGUI(void) {
  float *vals = NULL;

  setButtonState("modeGain",false);
  setButtonState("modeKnee",false);
  setButtonState("modeCR",false);
  setButtonState("modeLim",false);
  
  switch (myState.GUI_tuner_persistent_mode) {
    case 'g':
      //setButtonText("tunerMode","Linear Gain (dB)");
      vals = myState.wdrc_perBand.tkgain;
      setButtonState("modeGain",true);
      break;
    case 'c':
      //setButtonText("tunerMode","Comp Ratio");
      vals = myState.wdrc_perBand.cr;
      setButtonState("modeCR",true);
      break;
    case 'k':
      //setButtonText("tunerMode","Comp Knee (dB SPL)");
      vals = myState.wdrc_perBand.tk;
      setButtonState("modeKnee",true);
      break;
    case 'b':
      //setButtonText("tunerMode","Limitter Knee (dB SPL)");
      vals = myState.wdrc_perBand.bolt;
      setButtonState("modeLim",true);
      break;
  }

  if (vals != NULL) {
    setButtonText("tune1",String(vals[1-1],1));
    setButtonText("tune2",String(vals[2-1],1));
    setButtonText("tune3",String(vals[3-1],1));
    setButtonText("tune4",String(vals[4-1],1));
    setButtonText("tune5",String(vals[5-1],1));
    setButtonText("tune6",String(vals[6-1],1));    
  }
}

void SerialManager::setSDRecordingButtons(void) {
  if (audioSDWriter.getState() == AudioSDWriter_F32::STATE::RECORDING) {
    setButtonState("recordStart",true);
  } else {
    setButtonState("recordStart",false);
  }
  setButtonText("sdFname",audioSDWriter.getCurrentFilename());
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
