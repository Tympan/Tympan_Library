/*
   SerialControlledSDWriter_Quad
   
   Created: Chip Audette, OpenAudio, May 2019
   Purpose: Write 4 channels of audio to SD based on serial commands
      via USB serial or via BT serial.
	  
	  Writes to a single 4-channel WAV file.  Can be opened in Audacity.

   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

//define state...the "myState" instance is actually created in SerialManager.h
#define NO_STATE (-1)
const int INPUT_PCBMICS = 0, INPUT_MICJACK = 1, INPUT_LINEIN_SE = 2, INPUT_LINEIN_JACK = 3;
class State_t {
  public:
    int input_source = NO_STATE;
};

//local files
#include "SerialManager.h"

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ; //24000 or 44117 or 96000 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 64;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// Define the overall setup
String overall_name = String("Tympan: Quad SD Audio Writer");
float default_input_gain_dB = 5.0f; //gain on the microphone
float input_gain_dB = default_input_gain_dB;
#define MAX_AUDIO_MEM 60

// /////////// Define audio objects...they are configured later

// define classes to control the Tympan and the AIC_Shield
Tympan                        myTympan(TympanRev::D);    //note: Rev C is not compatible with the AIC shield
AICShield                     aicShield(TympanRev::D, AICShieldRev::A);  //for AIC_Shield
AudioInputI2SQuad_F32         i2s_in(audio_settings);   //Digital audio input from the ADC
AudioSDWriter_F32             audioSDWriter(audio_settings); //this is stereo by default
AudioOutputI2SQuad_F32        i2s_out(audio_settings);  //Digital audio output to the DAC.  Should always be last.

//Connect to outputs
AudioConnection_F32           patchcord1(i2s_in, 0, i2s_out, 0);    //Left channel input to left channel output
AudioConnection_F32           patchcord2(i2s_in, 1, i2s_out, 1);    //Right channel input to right channel output
AudioConnection_F32           patchcord3(i2s_in, 2, i2s_out, 2);    //AIC Shield Left channel input to AIC Shield left channel output
AudioConnection_F32           patchcord4(i2s_in, 3, i2s_out, 3);    //AIC Shield Left channel input to AIC Shield right channel output

//Connect to SD logging
AudioConnection_F32           patchcord5(i2s_in, 0, audioSDWriter, 0);   //connect Raw audio to SD writer
AudioConnection_F32           patchcord6(i2s_in, 1, audioSDWriter, 1);   //connect Raw audio to SD writer
AudioConnection_F32           patchcord7(i2s_in, 2, audioSDWriter, 2);   //connect Raw audio to SD writer
AudioConnection_F32           patchcord8(i2s_in, 3, audioSDWriter, 3);   //connect Raw audio to SD writer


//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) {  enable_printCPUandMemory = !enable_printCPUandMemory; }; 
void setPrintMemoryAndCPU(bool state) { enable_printCPUandMemory = state;};
SerialManager serialManager;
#define BOTH_SERIAL myTympan

//keep track of state
State_t myState;

void setConfiguration(int config) {
  myState.input_source = config;

  switch (config) {
    case INPUT_PCBMICS:
      //Select Input and set gain
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      aicShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones..the AIC sheild doesn't have on-board mics, so you probably want the line below
      //aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // AIC sheild doesn't have PCB mics, so force to input jack
      input_gain_dB = 15;
      myTympan.setInputGain_dB(input_gain_dB); aicShield.setInputGain_dB(input_gain_dB);
      break;

    case INPUT_MICJACK:
      //Select Input and set gain
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels...does this also do the aicShield?
      //aicShield.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      input_gain_dB = default_input_gain_dB;
      myTympan.setInputGain_dB(input_gain_dB);
      aicShield.setInputGain_dB(input_gain_dB);
      break;
      
    case INPUT_LINEIN_JACK:
      //Select Input and set gain
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      input_gain_dB = 0;
      myTympan.setInputGain_dB(input_gain_dB);
      aicShield.setInputGain_dB(input_gain_dB);
      break;
      
    case INPUT_LINEIN_SE:
      //Select Input and set gain
      myTympan.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes
      aicShield.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes
      input_gain_dB = default_input_gain_dB;
      myTympan.setInputGain_dB(input_gain_dB);
      aicShield.setInputGain_dB(input_gain_dB);
      break;
  }
 }

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  delay(100);
  myTympan.beginBothSerial(); delay(1000);
  BOTH_SERIAL.print(overall_name); BOTH_SERIAL.println(": setup():...");
  BOTH_SERIAL.print("Sample Rate (Hz): "); BOTH_SERIAL.println(audio_settings.sample_rate_Hz);
  BOTH_SERIAL.print("Audio Block Size (samples): "); BOTH_SERIAL.println(audio_settings.audio_block_samples);

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(MAX_AUDIO_MEM, audio_settings); 

  //activate the Tympan audio hardware
  myTympan.enable();   aicShield.enable();     // activate AIC
  myTympan.volume_dB(0.0);  aicShield.volume_dB(0); // output amp: -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit

  //Configure for Tympan PCB mics
  BOTH_SERIAL.println("Setup: Using Mic Jack with Mic Bias.");
  setConfiguration(INPUT_MICJACK); //this will also unmute the system

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(4);             //four channels for this quad recorder, but you could set it to 2
  audioSDWriter.setWriteDataType(AudioSDWriter::WriteDataType::INT16);  //this is the built-in the default, but here you could change it to FLOAT32
  BOTH_SERIAL.print("Configured for "); BOTH_SERIAL.print(audioSDWriter.getNumWriteChannels()); BOTH_SERIAL.println(" channels to SD.");

  //End of setup
  BOTH_SERIAL.println("Setup: complete."); serialManager.printHelp();

} //end setup()

// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial
  if (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //BT Serial

  //service the SD recording
  serviceSD();

  //update the memory and CPU usage...if enough time has passed
  if (enable_printCPUandMemory) printCPUandMemory(millis());

  //service the LEDs
  serviceLEDs();

} //end loop()



// ///////////////// Servicing routines

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

extern "C" char* sbrk(int incr);
int FreeRam() {
  char top; //this new variable is, in effect, the mem location of the edge of the heap
  return &top - reinterpret_cast<char*>(sbrk(0));
}
void printCPUandMemoryMessage(void) {
  BOTH_SERIAL.print("CPU Cur/Pk: ");
  BOTH_SERIAL.print(audio_settings.processorUsage(), 1);
  BOTH_SERIAL.print("%/");
  BOTH_SERIAL.print(audio_settings.processorUsageMax(), 1);
  BOTH_SERIAL.print("%, ");
  BOTH_SERIAL.print("MEM Cur/Pk: ");
  BOTH_SERIAL.print(AudioMemoryUsage_F32());
  BOTH_SERIAL.print("/");
  BOTH_SERIAL.print(AudioMemoryUsageMax_F32());
  BOTH_SERIAL.print(", FreeRAM(B) ");
  BOTH_SERIAL.print(FreeRam());
  BOTH_SERIAL.println();
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
          BOTH_SERIAL.print("SD Write Warning: there was a hiccup in the writing.");//  Approx Time (sec): ");
          BOTH_SERIAL.println(approx_time_sec );
        }
      }
    }
    i2s_in.clear_isOutOfMemory();
  }
}

// //////////////////////////////////// Control the audio processing from the SerialManager
//here's a function to change the volume settings.   We'll also invoke it from our serialManager
void incrementInputGain(float increment_dB) {
  input_gain_dB += increment_dB;
  if (input_gain_dB < 0.0) {
    BOTH_SERIAL.println("Error: cannot set input gain less than 0 dB.");
    BOTH_SERIAL.println("Setting input gain to 0 dB.");
    input_gain_dB = 0.0;
  }
  myTympan.setInputGain_dB(input_gain_dB);
}


