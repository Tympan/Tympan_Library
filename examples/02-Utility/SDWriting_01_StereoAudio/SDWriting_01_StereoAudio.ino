/*
   SDWriting_01_StereoAudio

   Digitizes two channels and records to SD card.

      >>> START recording by having potentiometer turned above half-way
      >>> STOP recording by having potentiometer turned below half-way

   Assumes Tympan Rev C or D.  Program in Arduino IDE as a Teensy 3.6.

   Uses super-fast SD library that is original from Greiman, but which
      has been forked, made compatible with the Teensy Audio library, and
      included as part of the Tympan Library.

   Created: Chip Audette, OpenAudio, March 2018
    Jun 2018: updated for Tympan RevC or RevD
    Jun 2018: updated to adde automatic mic detection

   License: MIT License, Use At Your Own Risk
*/

//here are the libraries that we need
#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::E);        //TympanRev::D or TympanRev::E
AudioInputI2S_F32         i2s_in(audio_settings);        //Digital audio input from the ADC
AudioSDWriter_F32         audioSDWriter(audio_settings); //this is stereo by default but can do 4 channels
AudioOutputI2S_F32        i2s_out(audio_settings);       //Digital audio output to the DAC.  Should always be last.

//Make all of the audio connections
AudioConnection_F32       patchcord1(i2s_in, 0, audioSDWriter, 0); //connect Raw audio to left channel of SD writer
AudioConnection_F32       patchcord2(i2s_in, 1, audioSDWriter, 1); //connect Raw audio to right channel of SD writer
AudioConnection_F32       patchcord3(i2s_in, 0, i2s_out, 0);    //echo audio to output
AudioConnection_F32       patchcord4(i2s_in, 1, i2s_out, 1);    //echo audio to output

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphones

void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(1000);
  myTympan.println("SDWriting_01_StereoAudio: Starting setup()...");
  myTympan.print("Sample Rate (Hz): "); myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("Audio Block Size (samples): "); myTympan.println(audio_settings.audio_block_samples);

  //allocate the audio memory
  AudioMemory_F32(60, audio_settings); //I can only seem to allocate 400 blocks
  myTympan.println("StereoAudioToSD: memory allocated.");

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC
  myTympan.print("StereoAudioToSD: runnng at a sample rate of (Hz): ");
  myTympan.println(sample_rate_Hz);

  //enable the Tympman to detect whether something was plugged inot the pink mic jack
  myTympan.enableMicDetect(true);

  //Choose the desired audio input on the Typman...this will be overridden by the serviceMicDetect() in loop()
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired input gain level
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);         //the library will print any error info to this serial stream (note that myTympan is also a serial stream)
  audioSDWriter.setWriteDataType(AudioSDWriter::WriteDataType::INT16);  //this is the built-in default, but here you could change it to FLOAT32
  audioSDWriter.setNumWriteChannels(2);       //this is also the built-in defaullt, but you could change it to 4 (maybe?), if you wanted 4 channels.

  myTympan.println("Setup complete.");
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
  serviceSD();

  //update the memory and CPU usage...if enough time has passed
  myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

  //service the LEDs
  serviceLEDs();

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
        myTympan.println("serviceMicDetect: detected plug-in microphone!  External mic now active.");
      } else {
        myTympan.println("serviceMicDetect: detected removal of plug-in microphone. On-board PCB mics now active.");
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
    if (potentiometer_value < 0.45) audioSDWriter.stopRecording();

  } else { //we are not already recording
    //check to see if potentiometer has been set to start recording
    if (potentiometer_value > 0.55) audioSDWriter.startRecording();
    
  }
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
          myTympan.print("SD Write Warning: there was a hiccup in the writing.");//  Approx Time (sec): ");
          myTympan.println(approx_time_sec );
        }
      }
    }
    i2s_in.clear_isOutOfMemory();
  }
}


void serviceLEDs(void) {
  static int loop_count = 0;
  loop_count++;
  
  if (audioSDWriter.getState() == AudioSDWriter::STATE::UNPREPARED) {
    if (loop_count > 200000) {  //slow toggle
      loop_count = 0;
      toggleLEDs(true,true); //blink both
    }
  } else if (audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING) {

    //let's flicker the LEDs while writing
    loop_count++;
    if (loop_count > 20000) { //fast toggle
      loop_count = 0;
      toggleLEDs(true,true); //blink both
    }
  } else {
    //myTympan.setRedLED(HIGH); myTympan.setAmberLED(LOW); //Go Red
    if (loop_count > 200000) { //slow toggle
      loop_count = 0;
      toggleLEDs(false,true); //just blink the red
    }
  }
}

void toggleLEDs(void) {
  toggleLEDs(true,true);  //toggle both
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
