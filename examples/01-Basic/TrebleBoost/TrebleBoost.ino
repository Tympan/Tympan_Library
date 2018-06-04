/*
*   TrebleBoost
*
*   Created: Chip Audette, OpenAudio, Apr 2017
*   Purpose: Process audio by applying a high-pass filter followed by gain
*
*   Uses Tympan Audio Adapter.
*   Blue potentiometer adjusts the digital gain applied to the filtered audio signal.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 24000.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
AudioControlTLV320AIC3206 audioHardware;
AudioInputI2S_F32         i2s_in(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC.
AudioFilterBiquad_F32     hp_filt1(audio_settings);   //IIR filter doing a highpass filter.  Left.
AudioFilterBiquad_F32     hp_filt2(audio_settings);   //IIR filter doing a highpass filter.  Right.
AudioEffectGain_F32       gain1;                      //Applies digital gain to audio data.  Left.
AudioEffectGain_F32       gain2;                      //Applies digital gain to audio data.  Right.
AudioOutputI2S_F32        i2s_out(audio_settings);    //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, hp_filt1, 0);   //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, hp_filt2, 0);   //connect the Right input
AudioConnection_F32       patchCord3(hp_filt1, 0, gain1, 0);    //Left
AudioConnection_F32       patchCord4(hp_filt2, 0, gain2, 0);    //right
AudioConnection_F32       patchCord5(gain1, 0, i2s_out, 0);     //connect the Left gain to the Left output
AudioConnection_F32       patchCord6(gain2, 0, i2s_out, 1);     //connect the Right gain to the Right output


//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A1  //potentiometer is tied to this pin

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  Serial.begin(115200);  delay(500);
  Serial.println("TrebleBoost: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  audioHardware.enable(); // activate AIC

  //Choose the desired input
  audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  audioHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //Set the cutoff frequency for the highpassfilter
  float cutoff_Hz = 1000.f;  //frequencies below this will be attenuated
  Serial.print("Highpass filter cutoff at ");Serial.print(cutoff_Hz);Serial.println(" Hz");
  hp_filt1.setHighpass(0, cutoff_Hz); //biquad IIR filter.  left channel
  hp_filt2.setHighpass(0, cutoff_Hz); //biquad IIR filter.  right channel

  // setup any other other features
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT

  // check the volume knob
  servicePotentiometer(millis(),0);  //the "0" is not relevant here.

  Serial.println("Setup complete.");
} //end setup()


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
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around

      //choose the desired gain value based on the knob setting
      const float min_gain_dB = -20.0, max_gain_dB = 40.0; //set desired gain range
      vol_knob_gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      //command the new gain setting
      gain1.setGain_dB(vol_knob_gain_dB);  //set the gain of the Left-channel gain processor
      gain2.setGain_dB(vol_knob_gain_dB);  //set the gain of the Right-channel gain processor
      Serial.print("servicePotentiometer: Digital Gain dB = "); Serial.println(vol_knob_gain_dB); //print text to Serial port for debugging
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
    Serial.print("Dyn MEM Float32 Cur/Peak: ");
    Serial.print(AudioMemoryUsage_F32());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax_F32());
    Serial.println();

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
