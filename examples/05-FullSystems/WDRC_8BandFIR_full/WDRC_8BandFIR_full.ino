/*
  WDRC_8BandFIR_full

  Created: Chip Audette (OpenAudio), August 2021
    Primarly built upon CHAPRO "Generic Hearing Aid" from
    Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro

  Purpose: Implements 8-band WDRC compressor.
  
  Features: 
    * 8-Band FIR Filterbank
    * Each channel has its own WDRC compressor
    * Ends with a broadband WDRC compressor (used as a limiter)
    * Can write raw and processed audio to SD card
    * Can control via TympanRemote App or via USB Serial
    * Mono (pushed to both ears)
    * Does not use Tympan digital earpieces
    * Does not include feedback cancellation
    * Does not include prescription saving
    * It can switch between two presets, both of which you can change
        * a NORMAL preset ("DSL_GHA_Preset0.h")
		    * a FULL-ON GAIN preset ("DSL_GHA_Preset1.h")
     
  Hardware Controls:
    Potentiometer on Tympan controls the broadband gain.

  Changing Number of Channels:
    As written, you can use 8 channels or fewer.  Simply change N_CHAN.  However, if you want
    *more* channels than the 8 shown here, simply go to DSL_GHA_Preset0.h and DSL_GHA_Preset1.h, 
    and add entries in each row beyond the 8 entries that are already there.  Then, come back to
    this file and change the value of N_CHAN.  Using those DSL_GHA files, you can configure up
    to 16 channels.  If you want more than 16 channels, that'll take a bit more effort. Ask the
    question in the Tympan forum!

  MIT License.  use at your own risk.
*/

// Include all the of the needed libraries
#include <Tympan_Library.h>


// Define the audio settings
const float sample_rate_Hz = 24000.0f ; //24000 or 32000 or 44100 (or other frequencies in the table in AudioOutputI2S_F32
const int audio_block_samples = 16;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32   audio_settings(sample_rate_Hz, audio_block_samples);

// Define the number of channels! Make sure DSL_GHA_Preset0.h and DSL_GHA_Preset1.h have enough values
// (it needs N_CHAN or more values) defined for each compressor parameter.  If not, it'll bomb at run time!
const int N_CHAN = 8;    

// Create audio classes and make audio connections
Tympan    myTympan(TympanRev::E, audio_settings);  //choose TympanRev::D or TympanRev::E
#include "AudioConnections.h"                      //let's put them in their own file for clarity
                         // number of frequency bands (channels)

// Create classes for controlling the system
#include      "SerialManager.h"
#include      "State.h"                            //must be after N_CHAN is defined
BLE           ble(&Serial1);                       //create bluetooth BLE
SerialManager serialManager(&ble);                 //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI


//configure the parameters of the audio-processing classes (must be after myState is created)
#include "ConfigureAlgorithms.h" //let's put all of these functions in their own file for clarity


//function to setup the hardware
void setupTympanHardware(void) {

  //activate the Tympan's audio interface
  myTympan.enable();
  connectClassesToOverallState();

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble

  //Choose the desired audio input on the Typman
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board micropphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //set volumes
  setOutputGain_dB(0.f);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  float default_mic_input_gain_dB = 15.0f; //gain on the microphone
  setInputGain_dB(default_mic_input_gain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps
  setDigitalGain_dB(myState.digital_gain_dB); // set gain low
}


void connectClassesToOverallState(void) {
  myState.filterbank = &filterbank.state;
  myState.compbank = &compbank.state;
}

//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here
  serialManager.add_UI_element(&myState);
  serialManager.add_UI_element(&filterbank);
  serialManager.add_UI_element(&compbank);
  serialManager.add_UI_element(&compBroadband);
  serialManager.add_UI_element(&audioSDWriter);
}

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
int USE_VOLUME_KNOB = 1;  //set to 1 to use volume knob to override the default vol_knob_gain_dB set a few lines below
void setup() {
  myTympan.beginBothSerial();
  if (Serial) Serial.print(CrashReport);
  Serial.println("WDRC_FIRbank_Compbank: setup():...");
  Serial.print("Sample Rate (Hz): "); Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("Audio Block Size (samples): "); Serial.println(audio_settings.audio_block_samples);

  // Audio connections require memory
  AudioMemory_F32(80,audio_settings);  //allocate Float32 audio data blocks (primary memory used for audio processing)

  // Enable the audio shield, select input, and enable output
  setupTympanHardware();  //see code earlier in this file

  //setup filters and mixers
  setupAudioProcessing(); //see function in ConfigureAlgorithms.h

  //setup BLE
  delay(500); ble.setupBLE(myTympan.getBTFirmwareRev()); delay(500); //Assumes the default Bluetooth firmware. You can override!
  
  //setup the serial manager
  setupSerialManager();

  //Set the state of the LEDs
  myTympan.setRedLED(HIGH);  myTympan.setAmberLED(LOW);

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(2);     //can record 2 or 4 channels
  Serial.println("Setup: SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

  //update the potentiometer settings
	//if (USE_VOLUME_KNOB) servicePotentiometer(millis());  //see code later in this file
  
  //End of setup
  Serial.println("Setup complete.");
  serialManager.printHelp();

  filterbank.enable(true);

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB...respondToByte is in SerialManagerBase...it then calls SerialManager.processCharacter(c)

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);  //respondToByte is in SerialManagerBase...it then calls SerialManager.processCharacter(c)
  }

  //service the BLE advertising state
  ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)

  //service the SD recording
  serviceSD();

  //service the LEDs
  serviceLEDs(millis());

  //service the potentiometer...if enough time has passed
  if (USE_VOLUME_KNOB) servicePotentiometer(millis());
  
  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)

  //print info about the signal processing
  updateAveSignalLevels(millis());
  if (myState.enable_printAveSignalLevels) myState.printAveSignalLevels(millis(),3000);
 

} //end loop()


// ///////////////// Servicing routines

void serviceLEDs(unsigned long curTime_millis) {
  static unsigned long lastUpdate_millis = 0;
  if (lastUpdate_millis > curTime_millis) { lastUpdate_millis = 0; } //account for possible wrap-around
  unsigned long dT_millis = curTime_millis - lastUpdate_millis;
  
  if (audioSDWriter.getState() == AudioSDWriter::STATE::UNPREPARED) {
    if (dT_millis > 1000) {  //slow toggle
      toggleLEDs(true,true); //blink both
      lastUpdate_millis = curTime_millis;
    }
  } else if (audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING) {
    if (dT_millis > 50) {  //fast toggle
      toggleLEDs(true,true); //blink both
      lastUpdate_millis = curTime_millis;
    }
  } else {
    if (dT_millis > 1000) {  //slow toggle
      toggleLEDs(true,true); //blink both
      lastUpdate_millis = curTime_millis;
    }
  }
}

void toggleLEDs(const bool &useAmber, const bool &useRed) {
  static bool LED = false;
  LED = !LED;
  if (LED) {
    if (useAmber) myTympan.setAmberLED(true);
    if (useRed) myTympan.setRedLED(false);
  } else {
    if (useAmber) myTympan.setAmberLED(false);
    if (useRed) myTympan.setRedLED(true);
  }
  if (!useAmber) myTympan.setAmberLED(false);
  if (!useRed) myTympan.setRedLED(false);
}


#define PRINT_OVERRUN_WARNING 1   //set to 1 to print a warning that the there's been a hiccup in the writing to the SD.
void serviceSD(void) {
  static int max_max_bytes_written = 0; //for timing diagnotstics
  static int max_bytes_written = 0; //for timing diagnotstics
  static int max_dT_micros = 0; //for timing diagnotstics
  static int max_max_dT_micros = 0; //for timing diagnotstics

  unsigned long dT_micros = micros();  //for timing diagnotstics
  int bytes_written = audioSDWriter.serviceSD();
  dT_micros = micros() - dT_micros;  //timing calculation

  if ( bytes_written > 0 ) {
    
    max_bytes_written = max(max_bytes_written, bytes_written);
    max_dT_micros = max((int)max_dT_micros, (int)dT_micros);
   
    if (dT_micros > 10000) {  //if the write took a while, print some diagnostic info
      max_max_bytes_written = max(max_bytes_written,max_max_bytes_written);
      max_max_dT_micros = max(max_dT_micros, max_max_dT_micros);
      
      Serial.print("serviceSD: bytes written = ");
      Serial.print(bytes_written); Serial.print(", ");
      Serial.print(max_bytes_written); Serial.print(", ");
      Serial.print(max_max_bytes_written); Serial.print(", ");
      Serial.print("dT millis = "); 
      Serial.print((float)dT_micros/1000.0,1); Serial.print(", ");
      Serial.print((float)max_dT_micros/1000.0,1); Serial.print(", "); 
      Serial.print((float)max_max_dT_micros/1000.0,1);Serial.print(", ");      
      Serial.println();
      max_bytes_written = 0;
      max_dT_micros = 0;     
    }
      
    //print a warning if there has been an SD writing hiccup
    if (PRINT_OVERRUN_WARNING) {
      //if (audioSDWriter.getQueueOverrun() || i2s_in.get_isOutOfMemory()) {
      if (i2s_in.get_isOutOfMemory()) {
        float approx_time_sec = ((float)(millis()-audioSDWriter.getStartTimeMillis()))/1000.0;
        if (approx_time_sec > 0.1) {
          Serial.print("SD Write Warning: there was a hiccup in the writing. ");//  Approx Time (sec): ");
          Serial.println(approx_time_sec);
        }
      }
    }
    i2s_in.clear_isOutOfMemory();
  }
}


//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    //float scaled_val = val / 3.0; scaled_val = scaled_val * scaled_val;
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around
      setDigitalGain_dB(val*45.0f - 25.0f,false);
      Serial.print("servicePotentiometer: digital gain (dB) = ");Serial.println(myState.digital_gain_dB);
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();



void updateAveSignalLevels(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how often to perform the averaging
  static unsigned long lastUpdate_millis = 0;
  float update_coeff = 0.2; //Smoothing filter.  Choose 1.0 (for no smoothing) down to 0.0 (which would never update)

  //is it time to update the calculations
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    for (int i=0; i<N_CHAN; i++) { //loop over each band
      myState.aveSignalLevels_dBFS[i] = (1.0f-update_coeff)*myState.aveSignalLevels_dBFS[i] + update_coeff*compbank.compressors[i].getCurrentLevel_dB(); //running average
    }
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

// ////////////////////////////// Functions to set the parameters and maintain the state

// Control the System Gains
float setInputGain_dB(float gain_dB) { return myState.input_gain_dB = myTympan.setInputGain_dB(gain_dB); }
float setOutputGain_dB(float gain_dB) {  return myState.output_gain_dB = myTympan.volume_dB(gain_dB); }
float setDigitalGain_dB(float gain_dB) { return setDigitalGain_dB(gain_dB, true); }
float setDigitalGain_dB(float gain_dB, bool printToUSBSerial) {  
  myState.digital_gain_dB = broadbandGain.setGain_dB(gain_dB); 
  serialManager.updateGainDisplay();
  if (printToUSBSerial) printGainSettings();
  return myState.digital_gain_dB;
}
float incrementDigitalGain(float increment_dB) { return setDigitalGain_dB(myState.digital_gain_dB + increment_dB); }

void printGainSettings(void) {
  Serial.print("Gain (dB): ");
  Serial.print("Input PGA = "); Serial.print(myState.input_gain_dB,1);
  Serial.print(", Per-Channel = ");
  for (int i=0; i<N_CHAN; i++) {
    Serial.print(compbank.getLinearGain_dB(i),1); //gets the linear gain setting
    Serial.print(", ");
  }
  Serial.print("Knob = "); Serial.print(myState.digital_gain_dB,1);
  Serial.println();
}

// Control Printing of Signal Levels
void togglePrintAveSignalLevels(bool as_dBSPL) { 
  myState.enable_printAveSignalLevels = !myState.enable_printAveSignalLevels; 
  myState.printAveSignalLevels_as_dBSPL = as_dBSPL;
};
