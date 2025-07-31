/*
   SDWriter

   Digitizes two channels and records to SD card.

      >>> START recording by having potentiometer turned above half-way
      >>> STOP recording by having potentiometer turned below half-way

   Set Tympan Rev C or D.  Program in Arduino IDE as a Teensy 3.6.
   Set Tympan Rev E.  Program in Arduino IDE as a Teensy 4.1.

   Created: Chip Audette, OpenAudio, March 2018
    Jun 2018: updated for Tympan RevC or RevD
    Jun 2018: updated to add automatic mic detection
    Jul 2021: updated to support Tympan RevE 
		May 2024: updated to support Tympan RevF

   License: MIT License, Use At Your Own Risk
*/

//here are the libraries that we need
#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
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

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphones

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

  //enable the Tympman to detect whether something was plugged into the pink mic jack
  myTympan.enableMicDetect(true);

  //Choose the desired audio input on the Typman...this will be overridden by the serviceMicDetect() in loop()
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired input gain level
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);         //the library will print any error info to this serial stream (note that myTympan is also a serial stream)
  audioSDWriter.setNumWriteChannels(2);       //this is also the built-in defaullt, but you could change it to 4 (maybe?), if you wanted 4 channels.

  Serial.println("Setup complete.");
  Serial.println();
  Serial.println("To Start Recording: Turn the volume pot all the way up.");
  Serial.println("To Stop Recording: Turn the volume pot all the way down.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
int loop_counter = 0;
bool LED = 0;
void loop() {
 
  //check the mic_detect signal
  serviceMicDetect(millis(), 500); 
 
  //check the potentiometer
  servicePotentiometer(millis(), 100); //service the potentiometer every 100 msec

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  //update the memory and CPU usage...if enough time has passed
  myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

} //end loop();


// ///////////////// Servicing routines

void serviceMicDetect(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  static unsigned int prev_val = 1111; //some sort of weird value
  unsigned int cur_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    cur_val = myTympan.updateInputBasedOnMicDetect(); //if mic is plugged in, defaults to TYMPAN_INPUT_JACK_AS_MIC
    if (cur_val != prev_val) {
      if (cur_val) {
        Serial.println("serviceMicDetect: detected plug-in microphone!  External mic now active.");
      } else {
        Serial.println("serviceMicDetect: detected removal of plug-in microphone. On-board PCB mics now active.");
      }
    }
    prev_val = cur_val;
    lastUpdate_millis = curTime_millis;
  }
}

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    
    float potentiometer_value = ((float32_t)(myTympan.readPotentiometer())) / 1023.0;
    startOrStopSDRecording(potentiometer_value);
    
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();

void startOrStopSDRecording(float potentiometer_value) {

  //are we already recording?
  if (audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING) {
    //check to see if potentiometer is set to turn off recording
    if (potentiometer_value < 0.45)	audioSDWriter.stopRecording();

  } else { //we are not already recording
    //check to see if potentiometer has been set to start recording
    if (potentiometer_value > 0.55) {
			if (audioSDWriter.isSdCardPresent()) {
				audioSDWriter.startRecording();
			} else {
				Serial.println("WARNING: SD card is not present in your Tympan.");
			}
		}
  }
}
