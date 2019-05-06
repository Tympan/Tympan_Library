/*
   StereoAudioToSD

   Digitizes two channels and records to SD card.

      >>> START recording by having potentiometer turned above half-way
      >>> STOP recording by having potentiometer turned below half-way

   Assumes Tympan Rev C or D.  Program in Arduino IDE as a Teensy 3.6.

   Uses super-fast SD library that is originall from Greiman, but which
      has been forked, made compatible with the Teensy Audio library, and
      included as part of the Tympan Library.

   Created: Chip Audette, OpenAudio, March 2018
    Jun 2018: updated for Tympan RevC or RevD
    Jun 2018: updated to adde automatic mic detection

   License: MIT License, Use At Your Own Risk
*/

//here are the libraries that we need
#include "SDAudioWriter_SdFat.h"
#include <Tympan_Library.h>


#define PRINT_OVERRUN_WARNING 1   //set to 1 to print a warning that the there's been a hiccup in the writing to the SD.
#define PRINT_FULL_SD_TIMING 0    //set to 1 to print timing information of *every* write operation.  Great for logging to file.  Bad for real-time human reading.
#define MAX_F32_BLOCKS (256)      //Can't seem to use more than 192, so you could set it to 192.  Won't run at all if much above 400.  

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan					  audioHardware(TympanRev::D);  //TympanRev::D or TympanRev::C
AudioInputI2S_F32         i2s_in(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC.
AudioRecordQueue_F32      queueLeft(audio_settings);     //gives access to audio data (will use for SD card)
AudioRecordQueue_F32      queueRight(audio_settings);     //gives access to audio data (will use for SD card)
AudioOutputI2S_F32        i2s_out(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC.


//Make all of the audio connections
AudioConnection_F32       patchcord1(i2s_in, 0, queueLeft, 0);  //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32       patchcord2(i2s_in, 1, queueRight, 0); //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32       patchcord3(i2s_in, 0, i2s_out, 0);    //echo audio to output
AudioConnection_F32       patchcord4(i2s_in, 1, i2s_out, 1);    //echo audio to output

// Create variables to decide how long to record to SD
SDAudioWriter_SdFat my_SD_writer;

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphones
float32_t potentiometer_value = 0.0f;  //set in Service Potentiometer
void setup() {
  //begin the serial comms (for debugging)
  Serial.begin(115200);  delay(500);
  Serial.println("StereoAudioToSD: Starting setup()...");

  //allocate the audio memory
  AudioMemory_F32(MAX_F32_BLOCKS, audio_settings); //I can only seem to allocate 400 blocks
  Serial.println("StereoAudioToSD: memory allocated.");

  //Enable the Tympan to start the audio flowing!
  audioHardware.enable(); // activate AIC
  Serial.print("StereoAudioToSD: runnng at a sample rate of (Hz): ");
  Serial.println(sample_rate_Hz);

  //enable the Tympman to detect whether something was plugged inot the pink mic jack
  audioHardware.enableMicDetect(true);

  //Choose the desired audio input on the Typman...this will be overridden by the serviceMicDetect() in loop()
  audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
  //audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired input gain level
  audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //Set the state of the LEDs
  audioHardware.setRedLED(HIGH);
  audioHardware.setAmberLED(LOW);

  // check the volume knob
  servicePotentiometer(millis(), 0); //the "0" is not relevant here.

  //setup SD card and start recording
  my_SD_writer.init();
  if (PRINT_FULL_SD_TIMING) my_SD_writer.enablePrintElapsedWriteTime(); //for debugging.  make sure time is less than (audio_block_samples/sample_rate_Hz * 1e6) = 2900 usec for 128 samples at 44.1 kHz
  my_SD_writer.setSampleRateWAV(sample_rate_Hz);

  Serial.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
int loop_counter = 0;
bool LED = 0;
void loop() {
  //service the SD recording
  serviceSD();

  //check the potentiometer
  servicePotentiometer(millis(), 100); //service the potentiometer every 100 msec

  //check the mic_detect signal
  serviceMicDetect(millis(), 500);

  //check to see whether to print the CPU and Memory Usage
  printCPUandMemory(millis(),3000); //print every 3000 msec

  //if recording, toggle the LEDs.  Otherwise set to RED
  if (my_SD_writer.isFileOpen()) {
    loop_counter++;
    if (loop_counter > 10000) {
      loop_counter = 0;
      LED = !LED;
      if (LED) {
        audioHardware.setAmberLED(true);
        audioHardware.setRedLED(false);
      } else {
        audioHardware.setAmberLED(false);
        audioHardware.setRedLED(true);
      }
    }
  } else {
    audioHardware.setAmberLED(false);
    audioHardware.setRedLED(true);
  }


} //end loop();


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    potentiometer_value = ((float32_t)(audioHardware.readPotentiometer())) / 1023.0;
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


//This routine prints the current and maximum CPU usage and the current usage of the AudioMemory that has been allocated
void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    if (Serial) {
      Serial.print("printCPUandMemory: ");
      Serial.print("CPU Cur/Peak: ");
      Serial.print(audio_settings.processorUsage());
      Serial.print("%/");
      Serial.print(audio_settings.processorUsageMax());
      Serial.print("%,   ");
      Serial.print("Dyn MEM Float32 Cur/Peak: ");
      Serial.print(AudioMemoryUsage_F32());
      Serial.print("/");
      Serial.print(AudioMemoryUsageMax_F32());
      Serial.println();
    }
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}


void serviceMicDetect(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  static unsigned int prev_val = 1111; //some sort of weird value
  unsigned int cur_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    cur_val = audioHardware.updateInputBasedOnMicDetect(); //if mic is plugged in, defaults to TYMPAN_INPUT_JACK_AS_MIC
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


void serviceSD(void) {
  static int recording_count = 0;
  
  if (my_SD_writer.isFileOpen()) {
    //if audio data is ready, write it to SD
    if ((queueLeft.available()) && (queueRight.available())) {
      //my_SD_writer.writeF32AsInt16(queueLeft.readBuffer(),audio_block_samples);  //mono
      my_SD_writer.writeF32AsInt16(queueLeft.readBuffer(), queueRight.readBuffer(), audio_block_samples); //stereo
      queueLeft.freeBuffer(); queueRight.freeBuffer();

      //print a warning if there has been an SD writing hiccup
      if (PRINT_OVERRUN_WARNING) {
        if (queueLeft.getOverrun() || queueRight.getOverrun() || i2s_in.get_isOutOfMemory()) {
          if (Serial) {
            float blocksPerSecond = sample_rate_Hz / ((float)audio_block_samples);
            Serial.print("SD Write Warning: there was a hiccup in the writing.  Approx Time (sec): ");
            Serial.println( ((float)my_SD_writer.getNBlocksWritten()) / blocksPerSecond );
          }
        }
      }

      //print timing information to help debug hiccups in the audio.  Are the writes fast enough?  Are there overruns?
      if (PRINT_FULL_SD_TIMING) {
        if (Serial) {
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
        }

        //Now that we've read the flags, reset them.
        AudioMemoryUsageMaxReset_F32();
      }

      queueLeft.clearOverrun();
      queueRight.clearOverrun();
      i2s_in.clear_isOutOfMemory();
    }

    //check to see if potentiometer is set to turn off recording
    if (potentiometer_value < 0.45) {  //turn below half-way to stop the recording
      //stop recording
      Serial.println("Closing SD File...");
      my_SD_writer.close();
      queueLeft.end();  queueRight.end();
      audioHardware.setRedLED(HIGH); audioHardware.setAmberLED(LOW); //Turn OFF the Amber LED
    }
  } else {
    //no SD recording currently, so no SD action

    //check to see if potentiometer has been set to start recording
    if (potentiometer_value > 0.55) {   //turn above half-way to start the recording
      //yes, start recording
      char fname[] = "AUDIOxxx.WAV";
      recording_count++;
      int hundreds = recording_count / 100;
      fname[5] = hundreds + '0';  //stupid way to convert the number to a character
      int tens = (recording_count - (hundreds*100)) / 10;  //truncates
      fname[6] = tens + '0';  //stupid way to convert the number to a character
      int ones = recording_count - (tens * 10) - (hundreds*100);
      fname[7] = ones + '0';  //stupid way to convert the number to a character

      if (my_SD_writer.openAsWAV(fname)) {
        Serial.print("Opened "); Serial.print(fname); Serial.println(" on SD for writing.");
        queueLeft.begin(); queueRight.begin();
        audioHardware.setRedLED(LOW); audioHardware.setAmberLED(HIGH); //Turn ON the Amber LED
      } else {
        Serial.print("Failed to open "); Serial.print(fname); Serial.println(" on SD for writing.");
      }
    }
  }
}

