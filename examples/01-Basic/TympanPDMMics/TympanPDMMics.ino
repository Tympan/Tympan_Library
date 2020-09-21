// TmpanPDMMics
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

#include "SerialManager.h"
#include <Tympan_Library.h>
#include "SDAudioWRiter_SdFat.h"

//definitions for SD writing
#define PRINT_OVERRUN_WARNING 1   //set to 1 to print a warning that the there's been a hiccup in the writing to the SD.
#define PRINT_FULL_SD_TIMING 0    //set to 1 to print timing information of *every* write operation.  Great for logging to file.  Bad for real-time human reading.
#define MAX_F32_BLOCKS (256)      //Can't seem to use more than 192, so you could set it to 192.  Won't run at all if much above 400.  


//set the sample rate and block size
const float sample_rate_Hz    = 48000.0f ;  //Allowed values: 8000, 11025, 16000, 22050, 24000, 32000, 44100, 44118, or 48000 Hz
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


//setup the  Tympan using the default settings
Tympan                    audioHardware(TympanRev::D);     //do TympanRev::C or TympanRev::D

// Define audio objects
AudioInputI2S_F32             i2s_in(audio_settings);       //This is the Teensy Audio library's built-in 4-channel I2S class
AudioMixer4_F32               mixerIn(audio_settings);
AudioRecordQueue_F32          queueLeft(audio_settings);    //gives access to audio data (will use for SD card)
AudioRecordQueue_F32          queueRight(audio_settings);   //gives access to audio data (will use for SD card)
AudioOutputI2S_F32            i2s_out(audio_settings);      //This is the Teensy Audio library's built-in 4-channel I2S class

//AUDIO CONNECTIONS
//Connect Left & Right Input Channel to Left and Right SD card queue
AudioConnection_F32           patchcord3(i2s_in, 0, queueLeft, 0);   //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord4(i2s_in, 1, queueRight, 0);  //connect Raw audio to queue (to enable SD writing)

//Connect Left & Right Input to Audio Output Left and Right
AudioConnection_F32           patchcord5(i2s_in, 0, i2s_out, 0);    //echo audio to output
AudioConnection_F32           patchcord6(i2s_in, 1, i2s_out, 1);    //echo audio to output

// Create variables to decide how long to record to SD
SDAudioWriter_SdFat my_SD_writer;


String overall_name = String("Tympan: PDM Mic");

//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) {
  enable_printCPUandMemory = !enable_printCPUandMemory;
}; //"extern" let's be it accessible outside
bool enable_printAveSignalLevels = false, printAveSignalLevels_as_dBSPL = false;
void togglePrintAveSignalLevels(bool as_dBSPL) {
  enable_printAveSignalLevels = !enable_printAveSignalLevels;
  printAveSignalLevels_as_dBSPL = as_dBSPL;
};
SerialManager serialManager(audioHardware, mixerIn);

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
      audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones

      //Disable Digital Mic (enable analog inputs)
      audioHardware.enableDigitalMicInputs(false);

      //Set mixer gain (right channel only; left is already sent to SD)
      mixerIn.gain(0,0.0); 
      mixerIn.gain(1,1.0);  //
      
      //Set input gain to 0dB
      input_gain_dB = 0.0;
      audioHardware.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = config_pcb_mics;
      break;
      
    case config_mic_jack:
      //Select Input
      audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack

      //Disable Digital Mic (enable analog inputs)
      audioHardware.enableDigitalMicInputs(false);
      
      //Set mixer gain
      mixerIn.gain(0,1.0); 
      mixerIn.gain(1,1.0);  

      //Set input gain to 0dB
      input_gain_dB = 0.0;
      audioHardware.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = config_mic_jack;
      break;
      
    case config_line_in_SE:      
      //Select Input
      audioHardware.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes

      //Disable Digital Mic (enable analog inputs)
      audioHardware.enableDigitalMicInputs(false);
      
      //Set mixer gain
      mixerIn.gain(0,1.0); 
      mixerIn.gain(1,1.0);

      //Set input gain to 0dB
      input_gain_dB = 0.0;
      audioHardware.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = config_line_in_SE;
      break;
      
    case config_pdm_mic:
      //Set the AIC's ADC to digital mic mode. Assign MFP4 to output a clock for the PDM, and MFP3 as the input to the PDM data line
      audioHardware.enableDigitalMicInputs(true);
      
      //Set mixer gain
      mixerIn.gain(0,1.0); 
      mixerIn.gain(1,1.0);

      //Set input gain to 0dB
      input_gain_dB = 0.0;
      audioHardware.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = config_pdm_mic;
      break;
  }
}

// control the recording phases
#define STATE_STOPPED 0
#define STATE_BEGIN 1
#define STATE_RECORDING 2
#define STATE_CLOSE 3
int current_state = STATE_STOPPED;
uint32_t recording_start_time_msec = 0;
void beginRecordingProcess(void) {
  if (current_state == STATE_STOPPED) {
    current_state = STATE_BEGIN;  
    startRecording();
  } else {
    audioHardware.println("beginRecordingProcess: already recording, or completed.");
  }
}

int recording_count = 0;
void startRecording(void) {
  if (current_state == STATE_BEGIN) {
    recording_count++; 
    if (recording_count > 9) recording_count = 0;
    char fname[] = "RECORDx.RAW";
    fname[6] = recording_count + '0';  //stupid way to convert the number to a character
    
    if (my_SD_writer.open(fname)) {
      audioHardware.print("startRecording: Opened "); audioHardware.print(fname); audioHardware.println(" on SD for writing.");
      queueLeft.begin(); queueRight.begin();
      audioHardware.setRedLED(LOW); audioHardware.setAmberLED(HIGH); //Turn ON the Amber LED
      recording_start_time_msec = millis();
    } else {
      audioHardware.print("startRecording: Failed to open "); audioHardware.print(fname); audioHardware.println(" on SD for writing.");
    }
    current_state = STATE_RECORDING;
  } else {
    audioHardware.println("startRecording: not in correct state to start recording.");
  }
}

void stopRecording(void) {
  if (current_state == STATE_RECORDING) {
      audioHardware.println("stopRecording: Closing SD File...");
      my_SD_writer.close();
      queueLeft.end();  queueRight.end();
      audioHardware.setRedLED(HIGH); audioHardware.setAmberLED(LOW); //Turn OFF the Amber LED
      current_state = STATE_STOPPED;
  }
}


float input_gain_dB = 0.0f;        //gain on the microphone
float vol_knob_gain_dB = 0.0;      //speaker output gain

// ///////////////// Main setup() and loop() as required for all Arduino programs
void setup() {
  audioHardware.beginBothSerial(); delay(1000);
  audioHardware.print(overall_name); audioHardware.println(": setup():...");
  audioHardware.print("Sample Rate (Hz): "); audioHardware.println(audio_settings.sample_rate_Hz);
  audioHardware.print("Audio Block Size (samples): "); audioHardware.println(audio_settings.audio_block_samples);


  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(MAX_F32_BLOCKS,audio_settings); //I can only seem to allocate 400 blocks
  Serial.println("Setup: memory allocated.");

  //activate the Tympan audio hardware
  audioHardware.enable(); // activate AIC

  //set input to digital PDM mics
  serialManager.respondToByte('d'); 

  //set volumes
  audioHardware.volume_dB(vol_knob_gain_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.

  //Set the state of the LEDs
  audioHardware.setRedLED(HIGH);
  audioHardware.setAmberLED(LOW);


  //setup SD card for recording
  my_SD_writer.init();
  if (PRINT_FULL_SD_TIMING)
    my_SD_writer.enablePrintElapsedWriteTime(); //for debugging.  make sure time is less than (audio_block_samples/sample_rate_Hz * 1e6) = 2900 usec for 128 samples at 44.1 kHz

  //End of setup
  audioHardware.println("Setup: complete."); serialManager.printHelp();

}

void loop() {

  //respond to Serial commands
  while (Serial.available())
  {
    serialManager.respondToByte((char)Serial.read());   //USB Serial
  }

  while (Serial1.available())
  {
    serialManager.respondToByte((char)Serial1.read()); //BT Serial
  }

  //service the SD recording
  serviceSD();
}



// ///////////////// Servicing routines
//here's a function to change the volume settings.   We'll also invoke it from our serialManager
void incrementInputGain(float increment_dB) {
  input_gain_dB += increment_dB;
  audioHardware.setInputGain_dB(input_gain_dB);
}

//Increment Headphone Output Volume
void incrementHeadphoneVol(float increment_dB) {
  vol_knob_gain_dB += increment_dB;
  audioHardware.volume_dB(vol_knob_gain_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
}


void printCPUandMemory(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    printCPUandMemoryMessage();  
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}



void printCPUandMemoryMessage(void) {
    audioHardware.print("CPU Cur/Peak: ");
    audioHardware.print(audio_settings.processorUsage());
    //audioHardware.print(AudioProcessorUsage());
    audioHardware.print("%/");
    audioHardware.print(audio_settings.processorUsageMax());
    //audioHardware.print(AudioProcessorUsageMax());
    audioHardware.print("%,   ");
    audioHardware.print("Dyn MEM Int16 Cur/Peak: ");
    audioHardware.print(AudioMemoryUsage());
    audioHardware.print("/");
    audioHardware.print(AudioMemoryUsageMax());
    audioHardware.print(",   ");
    audioHardware.print("Dyn MEM Float32 Cur/Peak: ");
    audioHardware.print(AudioMemoryUsage_F32());
    audioHardware.print("/");
    audioHardware.print(AudioMemoryUsageMax_F32());
    audioHardware.println();
}


void serviceSD(void) {
  if (my_SD_writer.isFileOpen()) {
    //if audio data is ready, write it to SD
    if ((queueLeft.available()) && (queueRight.available())) {
      //my_SD_writer.writeF32AsInt16(queueLeft.readBuffer(),audio_block_samples);  //mono
      my_SD_writer.writeF32AsInt16(queueLeft.readBuffer(), queueRight.readBuffer(), audio_block_samples); //stereo
      queueLeft.freeBuffer(); queueRight.freeBuffer();

      //print a warning if there has been an SD writing hiccup
      if (PRINT_OVERRUN_WARNING) {
        if (queueLeft.getOverrun() || queueRight.getOverrun() || i2s_in.get_isOutOfMemory()) {
          float blocksPerSecond = sample_rate_Hz / ((float)audio_block_samples);
          Serial.print("SD Write Warning: there was a hiccup in the writing.  Approx Time (sec): ");
          Serial.println( ((float)my_SD_writer.getNBlocksWritten()) / blocksPerSecond );
        }
      }

      //print timing information to help debug hiccups in the audio.  Are the writes fast enough?  Are there overruns?
      if (PRINT_FULL_SD_TIMING) {
        Serial.print("SD Write Status: ");
        Serial.print(queueLeft.getOverrun()); //zero means no overrun
        Serial.print(", ");
        Serial.print(queueRight.getOverrun()); //zero means no overrun
        Serial.print(", ");
        Serial.print(AudioMemoryUsageMax_F32());  //hopefully, is less than MAX_F32_BLOCKS
        Serial.print(", ");
        Serial.print(MAX_F32_BLOCKS);  // max possible memory allocation
        Serial.print(", ");
        Serial.println(i2s_in.get_isOutOfMemory());  //zero means i2s_input always had memory to work with.  Non-zero means it ran out at least once.

        //Now that we've read the flags, reset them.
        AudioMemoryUsageMaxReset_F32();
      }

      queueLeft.clearOverrun();
      queueRight.clearOverrun();
      i2s_in.clear_isOutOfMemory();
    }
  }
}
