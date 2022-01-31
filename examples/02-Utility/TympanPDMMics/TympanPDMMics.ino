// TympanPDMMics
//
// Created: Eric, Sept 2019
//
// This example code is in the public domain.

// Functionality
//  -Select from PCB mic, line in, or digital PDM mic 
//  -Pass thru audio
//  -SD recording
//  -Control heaphone output volume 

// Setup: Breakout board for the PDM mic (S2GO MEMSMIC IM69D) wired to a Tympan RevC 

// Wiring from PDM breakout board to Tympan: 
//     GND: Tympan Ground (J6.4)
//     3.3V: Wire lead tacked onto Tympan supply voltage 
//     Data: Tympan "SCLK/MFP3" (J6.3)
//     CLK: Tympan "MISO/MFP4"  (J6.2)

// Ref: PDM Mic Breakout Board: https://www.infineon.com/dgdl/Infineon-WhitePaper_Shield2Go_boards_and_My_IoT_adapter-WP-v01_01-EN.pdf?fileId=5546d46267c74c9a016831aff3ef626d


#include <Tympan_Library.h>
#include "SerialManager.h"

//definitions for SD writing
#define PRINT_OVERRUN_WARNING 1   //set to 1 to print a warning that the there's been a hiccup in the writing to the SD.

//set the sample rate and block size
const float sample_rate_Hz    = 24000.0f ;  //Allowed values: 8000, 11025, 16000, 22050, 24000, 32000, 44100, 44118, or 48000 Hz
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//setup the  Tympan using the default settings
Tympan                    myTympan(TympanRev::E);     //do TympanRev::E or TympanRev::D

// Define audio objects
AudioInputI2S_F32             i2s_in(audio_settings);       //This is the Teensy Audio library's built-in 4-channel I2S class
AudioMixer4_F32               mixerIn(audio_settings);
AudioSDWriter_F32             audioSDWriter(audio_settings); //this is stereo by default
AudioOutputI2S_F32            i2s_out(audio_settings);      //This is the Teensy Audio library's built-in 4-channel I2S class

//AUDIO CONNECTIONS
//Connect Left & Right Input Channel to Left and Right SD card queue
AudioConnection_F32           patchcord3(i2s_in, 0, audioSDWriter, 0);   //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord4(i2s_in, 1, audioSDWriter, 1);  //connect Raw audio to queue (to enable SD writing)

//Connect Left & Right Input to Audio Output Left and Right
AudioConnection_F32           patchcord5(i2s_in, 0, i2s_out, 0);    //echo audio to output
AudioConnection_F32           patchcord6(i2s_in, 1, i2s_out, 1);    //echo audio to output


String overall_name = String("Tympan: PDM Mic");

//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) {  enable_printCPUandMemory = !enable_printCPUandMemory; }; 
void setPrintMemoryAndCPU(bool state) { enable_printCPUandMemory = state;};
bool enable_printAveSignalLevels = false, printAveSignalLevels_as_dBSPL = false;
void togglePrintAveSignalLevels(bool as_dBSPL) {
  enable_printAveSignalLevels = !enable_printAveSignalLevels;
  printAveSignalLevels_as_dBSPL = as_dBSPL;
};
SerialManager serialManager(mixerIn);

//set the recording configuration
const int config_pcb_mics = 20;
const int config_mic_jack = 21;
const int config_line_in_SE  = 22;
const int config_line_in_diff = 23;
const int config_pdm_mic = 24;

int current_config = 0;

void setConfiguration(int config) { 
  switch (config) {
    case config_pcb_mics:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones

      //Disable Digital Mic (enable analog inputs)
      myTympan.enableDigitalMicInputs(false);

      //Set mixer gain (right channel only; left is already sent to SD)
      mixerIn.gain(0,0.0); 
      mixerIn.gain(1,1.0);  //
      
      //Set input gain to 0dB
      input_gain_dB = 0.0;
      myTympan.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = config_pcb_mics;
      break;
      
    case config_mic_jack:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack

      //Disable Digital Mic (enable analog inputs)
      myTympan.enableDigitalMicInputs(false);
      
      //Set mixer gain
      mixerIn.gain(0,1.0); 
      mixerIn.gain(1,1.0);  

      //Set input gain to 0dB
      input_gain_dB = 0.0;
      myTympan.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = config_mic_jack;
      break;
      
    case config_line_in_SE:      
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes

      //Disable Digital Mic (enable analog inputs)
      myTympan.enableDigitalMicInputs(false);
      
      //Set mixer gain
      mixerIn.gain(0,1.0); 
      mixerIn.gain(1,1.0);

      //Set input gain to 0dB
      input_gain_dB = 0.0;
      myTympan.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = config_line_in_SE;
      break;
      
    case config_pdm_mic:
      //Set the AIC's ADC to digital mic mode. Assign MFP4 to output a clock for the PDM, and MFP3 as the input to the PDM data line
      myTympan.enableDigitalMicInputs(true);
      
      //Set mixer gain
      mixerIn.gain(0,1.0); 
      mixerIn.gain(1,1.0);

      //Set input gain to 0dB
      input_gain_dB = 0.0;
      myTympan.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = config_pdm_mic;
      break;
  }
}


float input_gain_dB = 0.0f;        //gain on the microphone
float vol_knob_gain_dB = 0.0;      //speaker output gain

// ///////////////// Main setup() and loop() as required for all Arduino programs
void setup() {
  myTympan.beginBothSerial(); delay(2000);
  myTympan.print(overall_name); myTympan.println(": setup():...");
  myTympan.print("Sample Rate (Hz): "); myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("Audio Block Size (samples): "); myTympan.println(audio_settings.audio_block_samples);

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(50,audio_settings); //I can only seem to allocate 400 blocks
  Serial.println("Setup: memory allocated.");

  //activate the Tympan audio hardware
  myTympan.enable(); // activate AIC

  //set input to digital PDM mics
  serialManager.respondToByte('d'); 

  //set volumes
  myTympan.volume_dB(vol_knob_gain_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.

  //Set the state of the LEDs
  myTympan.setRedLED(HIGH);
  myTympan.setAmberLED(LOW);

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);         //the library will print any error info to this serial stream (note that myTympan is also a serial stream)
  audioSDWriter.setNumWriteChannels(2);       //this is also the built-in defaullt, but you could change it to 4 (maybe?), if you wanted 4 channels.
  int ret_val = audioSDWriter.setWriteDataType(AudioSDWriter::WriteDataType::INT16);  //this is the built-in default, but here you could change it to FLOAT32
  Serial.print("setup: setWriteDataType() yielded return code: ");  Serial.print(ret_val); 
  if (ret_val < 0) { Serial.println(": ERROR!"); } else {  Serial.println(": OK");  }

  //End of setup
  myTympan.println("Setup: complete."); 
  serialManager.printHelp();

}

void loop() {

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial
  while (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //BT Serial

  //update the memory and CPU usage...if enough time has passed
  if (enable_printCPUandMemory) myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

  //service the SD recording
  serviceSD();
}



// ///////////////// Servicing routines

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
          myTympan.print("SD Write Warning: there was a hiccup in the writing.");//  Approx Time (sec): ");
          myTympan.println(approx_time_sec );
        }
      }
    }
    i2s_in.clear_isOutOfMemory();
  }
}

// ////////////////////////////////// Change settings based on user commands
//here's a function to change the volume settings.   We'll also invoke it from our serialManager
void incrementInputGain(float increment_dB) {
  input_gain_dB += increment_dB;
  myTympan.setInputGain_dB(input_gain_dB);
}

//Increment Headphone Output Volume
void incrementHeadphoneVol(float increment_dB) {
  vol_knob_gain_dB += increment_dB;
  myTympan.volume_dB(vol_knob_gain_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
}
