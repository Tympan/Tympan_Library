/*
   SDWriter_wApp_Log
   
   Created: Chip Audette, OpenAudio, Sept 2021
   Purpose: Write audio to SD based on serial commands
      via USB serial or via Bluetooth (BLE).
      
      Also writes a text log to the SD card whenever you change
      the input gain via the App.

   For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
   For Tympan Rev E, program in Arduino IDE as a Teensy 4.1.

   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>
#include <SD.h>  //this is needed because of our addition of the text logging to the SD card

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ; //24000 or 44117 or 96000 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// /////////// Define audio objects...they will be configured later

//create audio library objects for handling the audio
Tympan   myTympan(TympanRev::E);   //do TympanRev::D or TympanRev::E
SDClass  sdx;  // explicitly create SD card, which we will pass to AudioSDWriter *and* which we will use for text logging
String   log_filename;

AudioInputI2S_F32             i2s_in(audio_settings);   //Digital audio input from the ADC
AudioSDWriter_F32_UI          audioSDWriter(&(sdx.sdfs), audio_settings); //this is stereo by default
AudioOutputI2S_F32            i2s_out(audio_settings);  //Digital audio output to the DAC.  Should always be last.

//Connect to outputs
AudioConnection_F32           patchcord1(i2s_in, 0, i2s_out, 0);    //Left input to left output
AudioConnection_F32           patchcord2(i2s_in, 1, i2s_out, 1);    //Right input to right output

//Connect to SD logging
AudioConnection_F32           patchcord3(i2s_in, 0, audioSDWriter, 0);   //connect Raw audio to left channel of SD writer
AudioConnection_F32           patchcord4(i2s_in, 1, audioSDWriter, 1);   //connect Raw audio to right channel of SD writer


// /////////// Create classes for controlling the system, espcially via USB Serial and via the App
#include      "SerialManager.h"
#include      "State.h"                
BLE_UI        ble(&Serial1);           //create bluetooth BLE class
SerialManager serialManager(&ble);     //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI


//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here
  serialManager.add_UI_element(&myState);
  serialManager.add_UI_element(&ble);
  serialManager.add_UI_element(&audioSDWriter);
}



// /////////// Functions for configuring the system

float setInputGain_dB(float val_dB) {
  return myState.input_gain_dB = myTympan.setInputGain_dB(val_dB);
}

void setConfiguration(int config) {
  myState.input_source = config;
  const float default_mic_gain_dB = 10.0f;

  switch (config) {
    case State::INPUT_PCBMICS:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      setInputGain_dB(default_mic_gain_dB);
      break;

    case State::INPUT_JACK_MIC:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      setInputGain_dB(default_mic_gain_dB);
      break;
      
    case State::INPUT_JACK_LINE:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      setInputGain_dB(0.0);
      break;
  }
}

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBothSerial(); delay(1000);
  Serial.println("Tympan: SDWriter_wApp: setup():...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //activate the Tympan audio hardware
  myTympan.enable();        // activate the flow of audio
  myTympan.volume_dB(0.0);  // headphone amplifier

  //Select the input that we will use 
  setConfiguration(myState.input_source);      //use the default that is written in State.h 

  //setup BLE
  delay(500); ble.setupBLE(myTympan.getBTFirmwareRev()); delay(500); //Assumes the default Bluetooth firmware. You can override!
  
  //setup the serial manager
  setupSerialManager();

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);         //the library will print any error info to this serial stream (note that myTympan is also a serial stream)
  audioSDWriter.setNumWriteChannels(2);       //this is also the built-in defaullt, but you could change it to 4 (maybe?), if you wanted 4 channels.
  Serial.println("Setup: SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

  //normally audioSDWriter starts the SD card when it's needed, but because we're doing the text logging
  //to the SD card, let's manually begin so that we know it's ready
  if (!sdx.sdfs.begin(SdioConfig(FIFO_SDIO))) sdx.sdfs.errorHalt(&Serial, "setup: SD begin failed!");  //adapted from Tympan_Library SDWriter.init()
  log_filename = findAndSetNextFilenameForLog();  Serial.println("setup: log filename is: " + log_filename);
  
  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();

} //end setup()

// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
  }

  //service the BLE advertising state
  ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info

  //add a log entry if we detect the starting or stopping fo the recording
  static int prev_SD_state = (int)audioSDWriter.getState(); //this is executed the first time through this function and then it persists
  if ((int)audioSDWriter.getState() != prev_SD_state) writeTextToSD(String(millis()) + String(", audioSDWriter_State = ") + String((int)audioSDWriter.getState()));
  prev_SD_state = (int)audioSDWriter.getState();  //prepare for the next time through this function
  

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)


} //end loop()


// //////////////////////////////////// Control the audio processing from the SerialManager

//here's a function to change the volume settings.   We'll also invoke it from our serialManager
float incrementInputGain(float increment_dB) {
  return setInputGain_dB(myState.input_gain_dB + increment_dB);
}

// //////////////////////////////////// here are some functions for writing text logs to the SD card

//get the next available filename for the log...to be called in setup()
String findAndSetNextFilenameForLog(void) { 
  static int recording_count = 0; 
  String out_fname = String();

  //this code was largely copied from the Tympan_Library AudioSDWriter::startRecording()
  bool done = false;
  while ((!done) && (recording_count < 998)) {
    
    recording_count++;
    if (recording_count < 1000) {
      //make file name
      char fname[] = "LOGxxx.TXT";
      int hundreds = recording_count / 100;
      fname[3] = hundreds + '0';  //stupid way to convert the number to a character
      int tens = (recording_count - (hundreds*100)) / 10;  //truncates
      fname[4] = tens + '0';  //stupid way to convert the number to a character
      int ones = recording_count - (tens * 10) - (hundreds*100);
      fname[5] = ones + '0';  //stupid way to convert the number to a character

      //does the file exist?
      if (sdx.sdfs.exists(fname)) {
        //loop around again
        done = false;
      } else {
        //this is our winner!
        out_fname = String(fname);
        done = true;
      }
    } else {
      Serial.println(F("findAndSetNextFilenameForLog: Cannot do more than 999 files."));
      done = true; //don't loop again
    } //close if (recording_count)
      
  } //close the while loop
  return out_fname;
}

//write text to SD card...to be called in SerialManager (or wherever!)
void writeTextToSD(String myString) {

  //adapted from Teensy SD example: SdFat_Usage.ino
  Serial.println("writeTextToSD: opening " + String(log_filename));
  FsFile myfile = sdx.sdfs.open(log_filename, O_WRITE | O_CREAT | O_APPEND);  //will append to the end of the file
  Serial.println("writeTextToSD: writing: " + myString);
  myfile.println(myString); //write the text myfile.close();
  Serial.println("writeTextToSD: closing file.");
  myfile.close(); 
}
