/*
  CalibrateInput

  Created: Chip Audette, OpenAudio, Oct 2024
  Purpose: Measure the signal level of the left analog input (pink jack).  If you know 
    the voltage of the input signal, then you can compare it to the Tympan-reported
    digital signal level, which will allow you to compute the calibration coefficient!

    This example also lets you record the audio to the SD card for analysis on your
    PC or Mac.

    Remember that the Tympan's audio codec chip (AIC) can provide both filtering
    and additional gain before it digitizes the analog sigal.  So, if using this
    example to calibrate the Tympan inputs, be sure that you know whether your own
    program is using the AIC's filtering and gain differently than being used here.

  For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
  For Tympan Rev E and F, program in Arduino IDE as a Teensy 4.1.

  MIT License.  use at your own risk.
*/

// Include all the of the needed libraries
#include <Tympan_Library.h>
#include "SerialManager.h"
  
#define SINE_WAVE_TEST true                     // True: Use sine wave as input; False: Use line input  

//set the sample rate and block size
const float       sample_rate_Hz = 44100.0f ;   //24000 or 44117 or 96000 (or other frequencies in the table in AudioOutputI2S_F32)
const int         audio_block_samples = 128;    //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
constexpr float   sineRmsLevel_dB [] = {-36.0f, -6.0f};
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// Create the audio objects and then connect them
Tympan                     myTympan(TympanRev::F,audio_settings);  //do TympanRev::D or TympanRev::E or TympanRev::F
AudioInputI2S_F32          i2s_in(audio_settings);                 //Digital audio input from the ADC
AudioSynthWaveform_F32     sineWave(audio_settings);   //from the Tympan_Library
AudioCalcLeq_F32           calcInputLevel(audio_settings);         //use this to measure the input signal level
AudioSDWriter_F32          audioSDWriter(audio_settings);          //will record the input signal
AudioOutputI2S_F32         i2s_out(audio_settings);                //Digital audio output to the DAC.  Should always be last.

#if SINE_WAVE_TEST == false
//Connect the audio input to the level monitor
AudioConnection_F32        patchcord01(i2s_in, 0, calcInputLevel, 0);    //Left input to the level monitor

//Connect the audio inputs to the SD recorder
AudioConnection_F32        patchcord02(i2s_in, 0, audioSDWriter,  0);    //Left input to the SD writer
AudioConnection_F32        patchcord03(i2s_in, 1, audioSDWriter,  1);    //Right input to the SD writer

//Connect the audio inputs to the audio outputs (to enable monitoring via headphones)
AudioConnection_F32        patchcord10(i2s_in, 0, i2s_out, 0);           //Left input to left output
AudioConnection_F32        patchcord11(i2s_in, 1, i2s_out, 1);           //Right input to right output

#else //Use Sine Wave as Input
// Connect a sine wave generator to level measurement
AudioConnection_F32        patchCord01(sineWave, 0, calcInputLevel, 0);  //Sine wave to level monitor

//Connect the sine wave to the SD recorder
AudioConnection_F32        patchcord02(sineWave, 0, audioSDWriter,  0);    //Left input to the SD writer
AudioConnection_F32        patchcord03(sineWave, 0, audioSDWriter,  1);    //Right input to the SD writer

//Connect the sine wave to the audio outputs (to enable monitoring via headphones)
AudioConnection_F32        patchcord10(sineWave, 0, i2s_out, 0);           //Left input to left output
AudioConnection_F32        patchcord11(sineWave, 0, i2s_out, 1);           //Right input to right output
#endif


// ///////////////// Create classes for controlling the system, espcially via USB Serial and via the App        
SerialManager     serialManager;     //create the serial manager for real-time control (via USB or App)
bool              flag_printInputLevelToUSB = true;

// ///////////////// Functions to control the audio
float input_gain_dB = 0.0;  // default value of analog gain in the AIC
float setInputGain_dB(float val_dB) { return input_gain_dB = myTympan.setInputGain_dB(val_dB); }

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(500);
  while (!Serial && (millis() < 2000UL)) delay(5); //wait for 2 seconds to see if USB Serial comes up (try not to miss messages!)
  myTympan.println("CalibrateInput: Starting setup()...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //activate the Tympan audio hardware
  myTympan.enable();        // activate the flow of audio

  //set the sine wave parameters
  sineWave.frequency(250.0f); //Hz
  sineWave.amplitude( getSineAmp(0) ); // Amplitude of sine wave
  //Choose the desired audio input on the Tympan
  //myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on-board micropphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Setup the input on the AIC
  myTympan.setHPFonADC(true,25.0,audio_settings.sample_rate_Hz); //setup a highpass filter at 25 Hz to remove any DC
  myTympan.setInputGain_dB(input_gain_dB);
  
  //Setup the level measuring algorithm
  calcInputLevel.setTimeWindow_sec(0.125);  //average over 0.125 seconds

  //Setup the output of the Tympan
  myTympan.volume_dB(0.0);  // AIC's output gain

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);         //the library will print any error info to this serial stream (note that myTympan is also a serial stream)
  audioSDWriter.setNumWriteChannels(2);       //this is also the built-in defaullt, but you could change it to 4 (maybe?), if you wanted 4 channels.
  Serial.println("Setup: SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  // Periodically increase sine wave amplitude
#if SINE_WAVE_TEST == true
  increaseSineAmplitude(millis(), 10000); //increase every 5000 msec
#endif

  //periodically print the input signal levels
  if (flag_printInputLevelToUSB) printInputSignalLevels(millis(),1000);  //print every 1000 msec

} //end loop()

// //////////////////////////////////////// Other functions
// Get the sine amplitude by index.
float getSineAmp(size_t levelIdx) {
  return (sqrtf(2.0f) * pow10f(sineRmsLevel_dB[levelIdx]/20.0f) );
}

void increaseSineAmplitude(unsigned long cur_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0UL;
  static size_t ampIdx = 0;

  if ( (cur_millis < lastUpdate_millis) || (cur_millis >= lastUpdate_millis + updatePeriod_millis) ) {
    // Toggle which ampllitude level to use
    if (ampIdx == 0) {
      ampIdx = 1;

    // Else reset back to index-0 and reset peak level
    } else {
      ampIdx = 0;
      Serial.println("Reset peak level");
      Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
      calcInputLevel.resetPeakLvl();
    }
    // Set amplitude of sine wave
    sineWave.amplitude( getSineAmp(ampIdx) ); // Set amplitude to 0.0f to start out

    // Update timer
    lastUpdate_millis = cur_millis;
  }
}


void printInputSignalLevels(unsigned long cur_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0UL;
  if ( (cur_millis < lastUpdate_millis) || (cur_millis >= lastUpdate_millis + updatePeriod_millis) ) {

#if SINE_WAVE_TEST == true
    float sineRmsLevel = sineWave.getAmplitude() / sqrtf(2.0f);
    float sineRmsLevel_db = 20.0f * log10f(sineRmsLevel);
    Serial.println("Sine RMS Level: " + String(sineRmsLevel) + " (" + String(sineRmsLevel_db) + " dB re: input FS)");

    // Print Input Gain
#else
    Serial.println("Input gain = " + String(input_gain_dB,1) + " dB");
#endif

    // Print the current level (time-averaged)
    Serial.println("Measured Input = " + String(calcInputLevel.getCurrentLevel_dB(), 2) + " dB re: input FS");
    
    // Print the level averaged over the entire duration since starting the application
    float32_t cumLeq = 0.0f;
    if ( calcInputLevel.getCumLevelRms_dB(cumLeq) ) {
      Serial.println( "Equivalent Level RMS = " + String(cumLeq) + " dB re: input FS");
    }

    // Print peak level
    Serial.println( "Peak Level: " + String(calcInputLevel.getPeakLevel_dB()) + " dB re: input FS");
    Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

    // Print error if Leq value is invalid
    if ( !calcInputLevel.isCumLeqValid() ) Serial.println("Error. Leq method wrapped and was reset.");

    lastUpdate_millis = cur_millis;    
  }
}