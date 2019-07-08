// FrequencyDomainDemo1
//
// Demonstrate audio procesing in the frequency domain.
//
// Created: Chip Audette  Sept-Oct 2016
//
// Approach:
//    * Take samples in the time domain
//    * Take FFT to convert to frequency domain
//    * Manipulate the frequency bins as desired (LP filter?  BP filter?  Formant shift?)
//    * Take IFFT to convert back to time domain
//    * Send samples back to the audio interface
//
// Assumes the use of the Audio library from PJRC
//
// This example code is in the public domain (MIT License)

#include <Tympan_Library.h>
#include "AudioEffectLowpassFD_F32.h"  //the local file holding your custom function

//set the sample rate and block size
const float sample_rate_Hz = 24000.f; ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan audioHardware(TympanRev::D); //TympanRev::D or TympanRev::C
AudioInputI2S_F32         i2s_in(audio_settings);           //Digital audio *from* the Tympan AIC.
AudioSynthWaveformSine_F32  sinewave(audio_settings);
AudioEffectLowpassFD_F32  audioEffectLowpassFD(audio_settings);  //create the frequency-domain processing block
AudioOutputI2S_F32        i2s_out(audio_settings);          //Digital audio *to* the Tympan AIC.
//AudioOutputUSB_F32        usb_out(audio_settings);

//Make all of the audio connections
#if 1
  AudioConnection_F32       patchCord1(i2s_in, 0, audioEffectLowpassFD, 0);    //connect the Left input
#else
  AudioConnection_F32       patchCord1(sinewave, 0, audioEffectLowpassFD, 0);    //connect the Left input
#endif
AudioConnection_F32       patchCord12(audioEffectLowpassFD, 0, i2s_out, 0);  //connect the input to the local Freq Domain processor
AudioConnection_F32       patchCord13(audioEffectLowpassFD, 0, i2s_out, 1);  //connect the Right gain to the Right output
//AudioConnection_F32       patchCord14(i2s_in, 0, usb_out, 0);  //connect the input to the USB Audio Output
//AudioConnection_F32       patchCord14(sinewave, 0, usb_out, 0);  //connect the sinewave to the USB Audio Output
//AudioConnection_F32       patchCord15(audioEffectLowpassFD, 0, usb_out, 1);  //connect processed audio to the USB Audio Output

#define POT_PIN A20  //potentiometer is tied to this pin

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
int is_windowing_active = 0;
void setup() {
 //begin the serial comms (for debugging)
  Serial.begin(115200);  delay(500);
  Serial.println("FrequencyDomainDemo2: starting setup()...");
  Serial.print("    : sample rate (Hz) = ");  Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("    : block size (samples) = ");  Serial.println(audio_settings.audio_block_samples);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(10); AudioMemory_F32(20, audio_settings);

  // Configure the frequency-domain algorithm
  int N_FFT = 128;
  audioEffectLowpassFD.setup(audio_settings,N_FFT); //do after AudioMemory_F32();

 //Enable the Tympan to start the audio flowing!
  audioHardware.enable(); // activate AIC
  Serial.println("Tympan enabled.");

  //Choose the desired input
  audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  // audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //  audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  audioHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  sinewave.frequency(1000.0f);
  sinewave.amplitude(0.025f);

  // configure the blue potentiometer
  pinMode(POT_PIN, INPUT); delay(50);  //set the potentiometer's input pin as an INPUT
  servicePotentiometer(millis(),0);  //update based on the knob setting the "0" is not relevant here.

  Serial.println("Setup complete.");
}


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //check the potentiometer
  servicePotentiometer(millis(),100); //service the potentiometer every 100 msec

  //check to see whether to print the CPU and Memory Usage
  printCPUandMemory(millis(),3000); //print every 3000 msec

} //end loop();


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
    val = (1.0/15.0) * (float)((int)(15.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around

      #if 0
        //use the potentiometer as a volume knob
        const float min_val = 0, max_val = 40.0; //set desired range
        float new_value = min_val + (max_val - min_val)*val;
        input_gain_dB = new_value;
        audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps
        Serial.print("servicePotentiometer: Input Gain (dB) = "); Serial.println(new_value); //print text to Serial port for debugging
      #else
        //use the potentiometer to set the freq-domain low-pass filter
        const float min_val = logf(200.f), max_val = logf(12000.f); //set desired range
        float lowpass_Hz = expf(min_val + (max_val - min_val)*val);
        audioEffectLowpassFD.setLowpassFreq_Hz(lowpass_Hz);
        Serial.print("servicePotentiometer: Lowpass (Hz) = "); Serial.println(lowpass_Hz); //print text to Serial port for debugging
      #endif
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


//This routine prints the current and maximum CPU usage and the current usage of the AudioMemory that has been allocated
void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    Serial.print("printCPUandMemory: ");
    Serial.print("CPU Cur/Peak: ");
    Serial.print(audio_settings.processorUsage());
    //Serial.print(AudioProcessorUsage()); //if not using AudioSettings_F32
    Serial.print("%/");
    Serial.print(audio_settings.processorUsageMax());
    //Serial.print(AudioProcessorUsageMax());  //if not using AudioSettings_F32
    Serial.print("%,   ");
    Serial.print("Dyn MEM Int16 Cur/Peak: ");
    Serial.print(AudioMemoryUsage());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax());
    Serial.print(",   ");
    Serial.print("Dyn MEM Float32 Cur/Peak: ");
    Serial.print(AudioMemoryUsage_F32());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax_F32());
    Serial.println();

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
