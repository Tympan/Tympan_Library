/*
   SDWriter_Advanced

   Digitizes two channels and records to SD card.  Shows some advanced features
   of the AudioSDWriter_f32 class

      >>> START by sending a 'r' via the SerialMonitor
      >>> STOP by sending a 's' via the SerialMonitor

   Set Tympan Rev C or D.  Program in Arduino IDE as a Teensy 3.6.
   Set Tympan Rev E or F.  Program in Arduino IDE as a Teensy 4.1.

   Created: Chip Audette, OpenAudio, Oct 2025

  To compile for MTP, you need to:
     * Arduino IDE 1.8.15 or later
     * Teensyduino 1.58 or later
     * MTP_Teensy library from https://github.com/KurtE/MTP_Teensy
     * In the Arduino IDE, under the "Tools" menu, choose "USB Type" and then "Serial + MTP Disk (Experimental)"

   To use MTP, you need to:
     * Connect to a PC via USB
     * Open a SerialMonitor (such as through the Arduino IDE)
     * Send the character '>' (without quotes) to the Tympan
     * After a second, it should appear in Windows as a drive
     * After using MTP to transfer files, you must cycle the power on the Tympan to get out of MTP mode.

   MTP Support is VERY EXPERIMENTAL!!  There are weird behaviors that come with the underlying
   MTP support provided by Teensy and its libraries.   

   License: MIT License, Use At Your Own Risk
*/

//here are the libraries that we need
#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ;  //Choose sample rate up to 96000
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// /////////// Define audio objects...they are configured later

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::F, audio_settings);   //do TympanRev::D or E or F
AudioInputI2S_F32         i2s_in(audio_settings);        //Digital audio input from the ADC
AudioSDWriter_F32         audioSDWriter(audio_settings); //this is stereo by default but can do 4 channels
AudioOutputI2S_F32        i2s_out(audio_settings);       //Digital audio output to the DAC.  Should always be last.

//Connect to outputs
AudioConnection_F32       patchcord1(i2s_in, 0, i2s_out, 0);    //echo audio to output
AudioConnection_F32       patchcord2(i2s_in, 1, i2s_out, 1);    //echo audio to output

//Connect to SD logging
AudioConnection_F32       patchcord3(i2s_in, 0, audioSDWriter, 0); //connect Raw audio to left channel of SD writer
AudioConnection_F32       patchcord4(i2s_in, 1, audioSDWriter, 1); //connect Raw audio to right channel of SD writer


// /////////// Create classes for controlling the system, espcially via USB Serial and via the App
#include      "SerialManager.h"
#include      "State.h"                
SerialManager serialManager;
State         myState;

/* Create the MTP servicing stuff so that one can access the SD card via USB */
/* If you want this, be sure to set the USB mode via the Arduino IDE,  Tools Menu -> USB Type -> Serial + MTP (experimental) */
#include "setup_MTP.h"  //put this line sometime after the audioSDWriter has been instantiated


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

void setup() {
  myTympan.beginBothSerial(); delay(1000);
  Serial.println("Tympan: SDWriter: setup():...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //activate the Tympan audio hardware
  myTympan.enable();        // activate the flow of audio
  myTympan.volume_dB(0.0);  // headphone amplifier

  //Select the input that we will use 
  setConfiguration(myState.input_source);      //use the default that is written in State.h 

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);         //the library will print any error info to this serial stream (note that myTympan is also a serial stream)
  audioSDWriter.setNumWriteChannels(2);       //this is also the built-in defaullt, but you could change it to 4 (maybe?), if you wanted 4 channels.
  Serial.println("Setup: SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

	//Advanced SD Writing Option: Change the size of the SDWriter's buffer in RAM
	//  The SD Writer uses a large amount of RAM as a buffer to hold data until the SD card
	//  will accept the data.  The SD card sometimes stalls for 100s of milliseconds.  You
	//  need a buffer in RAM that is large enough so that it can hold all the audio samples 
	//  that might accumulate while the SD card itself is stalled.  The default buffer size
	//  is 150000 bytes.  But, it might not be enough, so you might wish to make it larger.
	//  Or, maybe other parts of your program require more RAM and you need to make this
	//  buffer smaller.  You can use the command below to change its size.
	int ret_val = audioSDWriter.allocateBuffer(75000);  //I believe that 150000 is the default
	if (ret_val == 0) Serial.println("setup: *** ERROR ***: Failed to allocate RAM buffer for audioSDWriter.");
	
	//Advanced SD Writing Option: Decimate the audio data before buffering and writing
	//   Normally, the audioSDWriter writes a WAV file with the same sample rate as the audio
	//   flowing through the Tympan.  But, there are scenarios where you might want to write
	//   at a lower sample rate.  If so, audioSDWriter can decimate the data prior to buffering
	//   and writing.  You can use the command below to set the decimation factor.  A decimation factor
  //   of "1" is no decimation.  "2" cuts the sample rate in half.  "3" cuts to 1/3rd.  etc.
	//   If you are using decimation, you will want to use a lowpass filter prior to sending to the 
	//   audioSDwriter in order to avoid aliasing down of any high frequency energy that is above 
	//   your new nyquist frequency.  This example does not include any such lowpass filter.
	audioSDWriter.setDecimationFactor(2);       //use any value that is 1 or higher
	
  //Advanced SD Writing Option: Pre-allocate space on the SD Card.
  //   Occasionally, during writing, the SD card itself stalls while its own built-in microprocessor
  //   takes time to find and allocate space for you to continue writing.  It can stall for 100s of
  //   milliseconds, which might cause hiccups in your audio data.  Supposedly, you can preallocate
  //   space on the SD card, which should avoid this problem.  I'm not sure that this is actually
  //   true in practice, but below is the command that you use to tell the audioSDWriter that it should
  //   pre-allocate and by how much to pre-allocate.  The default is no pre-allocation.
  audioSDWriter.setPreAllocateWavBytes(16000000ULL);  //pre-allocate about 16MB of space on the SD whenever a file is opened

  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
int loop_counter = 0;
bool LED = 0;
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info

  // Did the user activate MTP mode?  If so, service the MTP and nothing else
  if (use_MTP) service_MTP();  //Find in Setup_MTP.h 

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(), audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

} //end loop();

// //////////////////////////////////// Control the audio processing from the SerialManager

//here's a function to change the volume settings.   We'll also invoke it from our serialManager
float incrementInputGain(float increment_dB) {
  return setInputGain_dB(myState.input_gain_dB + increment_dB);
}

