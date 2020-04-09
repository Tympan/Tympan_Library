// Demo sketch using the Tympan digital-mic earpieces.
//
// Created: Chip Audette, Open Audio, Mar 2020
//
// This example code is in the public domain.
//
// Hardware: 
//  Assumes that you're using a Tympan RevD with an Earpiece Shield
//    and two Tympan earpieces (each with a front and back PDM micrphone) 
//    connected through the earpiece audio ports (which uses the USB-B Mini connector).
//
// Sketch Features:
//    * Control the two mics in each earpiece: Front-only, back-only, add the two, or subtract the two
//    * After mixing the microphones, performs treble boost and WDRC compression on left and on right
//    * Allows you to record all four mics to a 4-channel WAV file on the SD Card
//    * Control the system via the Tympan App

#include <Tympan_Library.h>
#include "State.h"                          //For enums
#include "SerialManager.h"                  //For processing serial communication

//set the sample rate and block size
const float sample_rate_Hz    = 44100.0f;  //Allowed values: 8000, 11025, 16000, 22050, 24000, 32000, 44100, 44118, or 48000 Hz
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//setup the  Tympan using the default settings
Tympan                        myTympan(TympanRev::D);    //note: Rev C is not compatible with the AIC shield
#define AIC_RESET_PIN 20      //for I2C connection to AIC shield (eventually move into Tympan library)
#define AIC_I2C_BUS 2         //for I2C connection to AIC shield (eventually move into Tympan library)
AudioControlAIC3206           aicShield(AIC_RESET_PIN,AIC_I2C_BUS);  //for AIC_Shield

// Define audio objects
AudioInputI2SQuad_F32         i2s_in(audio_settings);         //Digital audio *from* the Tympan AIC.
AudioMixer4_F32               inputMixer_L(audio_settings);    //For mixing (or not) the two mics in the left earpiece
AudioMixer4_F32               inputMixer_R(audio_settings);    //For mixing (or not) the two mics in the left earpiece
AudioFilterBiquad_F32         hp_filt1(audio_settings);   //IIR filter doing a highpass filter.  Left.
AudioFilterBiquad_F32         hp_filt2(audio_settings);   //IIR filter doing a highpass filter.  Right.
AudioEffectCompWDRC_F32       comp1(audio_settings);      //Compresses the dynamic range of the audio.  Left.
AudioEffectCompWDRC_F32       comp2(audio_settings);      //Compresses the dynamic range of the audio.  Right.
AudioSummer4_F32              switchOut0_L(audio_settings), switchOut0_R(audio_settings);  
AudioSummer4_F32              switchOut1_L(audio_settings), switchOut1_R(audio_settings); 
AudioOutputI2SQuad_F32        i2s_out(audio_settings);        //Digital audio *to* the Tympan AIC.  Always list last to minimize latency
AudioSDWriter_F32             audioSDWriter(audio_settings);  //this is stereo by default

//Connect the front and rear mics (from each earpiece) to input mixers
//AudioConnection_F32           patchcord1(i2s_in, 3, inputMixer_L, 0);  //Left-Front Mic
//AudioConnection_F32           patchcord2(i2s_in, 2, inputMixer_L, 1);  //Left-Rear Mic
//AudioConnection_F32           patchcord3(i2s_in, 1, inputMixer_R, 0);  //Right-Front Mic
//AudioConnection_F32           patchcord4(i2s_in, 0, inputMixer_R, 1);  //Right-Rear Mic

AudioConnection_F32           patchcord1(i2s_in, 3, inputMixer_L, 0);  //Left-Front Digital Mic (AIC1)
AudioConnection_F32           patchcord2(i2s_in, 2, inputMixer_L, 1);  //Left-Rear Digital Mic (AIC1)
AudioConnection_F32           patchcord3(i2s_in, 0, inputMixer_L, 2);  //Left Input (AIC0)
AudioConnection_F32           patchcord4(i2s_in, 1, inputMixer_L, 3);  //Right Input (AIC0)

AudioConnection_F32           patchcord5(i2s_in, 1, inputMixer_R, 0);  //Right-Front Digital Mic (AIC0)
AudioConnection_F32           patchcord6(i2s_in, 0, inputMixer_R, 1);  //Right-Rear Digital Mic (AIC0)
AudioConnection_F32           patchcord7(i2s_in, 2, inputMixer_R, 2);  //Left input (AIC1)
AudioConnection_F32           patchcord8(i2s_in, 3, inputMixer_R, 3);  //Right input (AIC1)

//Connect to audio processing
AudioConnection_F32       patchCord31(inputMixer_L, 0, hp_filt1, 0);   //connect the Left input to the left highpass filter
AudioConnection_F32       patchCord32(inputMixer_R, 0, hp_filt2, 0);   //connect the Right input to the right highpass filter
AudioConnection_F32       patchCord33(hp_filt1, 0, comp1, 0);    //connect to the left gain/compressor/limiter
AudioConnection_F32       patchCord34(hp_filt2, 0, comp2, 0);    //connect to the right gain/compressor/limiter

//Connect to switches to control which outputs are active
AudioConnection_F32         patchCord41(comp1,0,switchOut0_L,0);   //left channel, lower outputs (AIC0)
AudioConnection_F32         patchCord42(comp2,0,switchOut0_R,0);   //right channel, lower outputs (AIC0)
AudioConnection_F32         patchCord43(comp1,0,switchOut1_L,0);   //left channel, upper outputs (AIC1)
AudioConnection_F32         patchCord44(comp2,0,switchOut1_R,0);   //right channel, upper outputs (AIC1)
  
//Connect the processed audio to both the Tympan and Shield audio outputs
AudioConnection_F32           patchcord11(switchOut0_L, 0, i2s_out, 0); //Tympan AIC, left output
AudioConnection_F32           patchcord12(switchOut0_R, 0, i2s_out, 1); //Tympan AIC, right output
AudioConnection_F32           patchcord13(switchOut1_L, 0, i2s_out, 2); //Shield AIC, left output
AudioConnection_F32           patchcord14(switchOut1_R, 0, i2s_out, 3); //Shield AIC, right output

//Connect the input mixer to the SD card
AudioConnection_F32           patchcord21(i2s_in, 3, audioSDWriter, 0);   //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord22(i2s_in, 2, audioSDWriter, 1);  //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord23(i2s_in, 0, audioSDWriter, 2);   //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord24(i2s_in, 1, audioSDWriter, 3);  //connect Raw audio to queue (to enable SD writing)

//control display and serial interaction
SerialManager                 serialManager;
State                         myState(&audio_settings, &myTympan);
//Settings                      defaultSettings;

String overall_name = String("Hear-Thru for Functional test of Tympan + Shield + Earpieces with PDM Mics");

//Static Variables
static float outputVolume_dB = 0.0;
static float inputGain_dB = 0.0;

// ///////////////// Main setup() and loop() as required for all Arduino programs
void setup() {
  myTympan.beginBothSerial(); delay(1000);
  myTympan.print(overall_name); myTympan.println(": setup():...");
  myTympan.print("Sample Rate (Hz): "); myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("Audio Block Size (samples): "); myTympan.println(audio_settings.audio_block_samples);

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(40,audio_settings); //I can only seem to allocate 400 blocks

  //Enable the Tympan and AIC shields to start the audio flowing!
  myTympan.enable(); 
  aicShield.enable();

  //Set the cutoff frequency for the highpassfilter
  setHPCutoffFreq_Hz(myState.hp_cutoff_Hz);  //defined towards the bottom of this file
  setupCompressors(comp1); setupCompressors(comp2);

  //Set the state of the LEDs
  myTympan.setRedLED(HIGH);
  myTympan.setAmberLED(LOW);

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(4);             //four channels for this quad recorder, but you could set it to 2
  audioSDWriter.setWriteDataType(AudioSDWriter::WriteDataType::INT16);  //this is the built-in the default, but here you could change it to FLOAT32
  Serial.print("SD configured for "); Serial.print(audioSDWriter.getNumWriteChannels()); Serial.println(" channels.");

  //set headphone volume (will be overwritten by the volume pot)
  setOutputVolume_dB(0.0); //dB, -63.6 to +24 dB in 0.5dB steps.

  //Setup the inputs and outputs
  setInputSource(State::INPUT_PDMMICS);
  setInputMixer(State::MIC_FRONT);
  setOutputAIC(State::OUT_BOTH);  //activate both AIC's outputs

  //mix bluetooth audio into the output via the AIC's analog output mixer
  myTympan.mixBTAudioWithOutput(true);   //note that it only goes to the first AIC!  ...not the one on the AIC/Earpiece shield
  
  //End of setup
  myTympan.println("Setup: complete."); 
  serialManager.printHelp();

}

void loop() {
  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial
  while (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //BT Serial

  //service the SD recording
  serviceSD();
  
  //service the LEDs
  serviceLEDs();

  //periodicallly check the potentiometer
  servicePotentiometer(millis(),100); //service the potentiometer every 100 msec

  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

  //periodically send data to be plotted on phone
  if (myState.flag_sendPlottingData) servicePlotting(millis(),500);
  
}

// /////////////////////////////////////////////////////////////

//void setupUsingSettingsClass(Settings &settings) {
//  setInputSource(settings.input_source);
//  setInputMixer(int inputmix_config); 
//
//  digital_mic_config = State::MIC_FRONT;
//  int analog_mic_config = State::MIC_AIC0_LR; //what state for the mic configuration when using analog inputs
//  int output_aic = OUT_BOTH; 
//  
//  //bool flag_printCPUandMemory = false;
//  float hp_cutoff_Hz = 1000.f;
//
//  float comp_exp_knee_dBSPL = 25.0;
//  float comp_exp_ratio = 0.57;
//  float comp_lin_gain_dB = 20.0;
//  float comp_comp_knee_dBSPL = 55.0;
//  float comp_comp_ratio = 2.0;
//  float comp_lim_knee_dBSPL = 90.0;
//  
//}

//set the desired input source 
void setInputSource(int input_config) { 

  if (input_config != State::INPUT_PDMMICS) {
      //Disable Digital Mic (enable analog inputs)
      myTympan.enableDigitalMicInputs(false);
      aicShield.enableDigitalMicInputs(false);
  }
  
  switch (input_config) {
    case State::INPUT_PCBMICS:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      aicShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);

      //Set input gain in dB
      setInputGain_dB(15.0);

      //Store configuration
      myState.input_source = input_config;
      break;
      
    case State::INPUT_MICJACK_MIC:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);

      //Set input gain in dB
      setInputGain_dB(15.0);

      //Store configuration
      myState.input_source = input_config;
      break;
      
  case State::INPUT_MICJACK_LINEIN:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the mic jack
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN);

      //Set input gain in dB
      setInputGain_dB(0.0);

      //Store configuration
      myState.input_source = input_config;
      break;     
    case State::INPUT_LINEIN_SE:      
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes
      aicShield.inputSelect(TYMPAN_INPUT_LINE_IN);

      //Set input gain in dB
      setInputGain_dB(0.0);

      //Store configuration
      myState.input_source = input_config;
      break;
      
    case State::INPUT_PDMMICS:
      //Set the AIC's ADC to digital mic mode. Assign MFP4 to output a clock for the PDM, and MFP3 as the input to the PDM data line
      myTympan.enableDigitalMicInputs(true);
      aicShield.enableDigitalMicInputs(true);
      
      //Set input gain in dB
      setInputGain_dB(0.0);

      //Store configuration
      myState.input_source = input_config;
      break;
  }
}

//Sets the input mixer gain for a given mic channel
void setInputMixer(int inputmix_config) {
  //clear all previous connections
  for (int i=0; i<4; i++) {
    inputMixer_L.gain(i,0.0);
    inputMixer_R.gain(i,0.0);
  }

  //now, choose just the connections that we want
  switch (inputmix_config) {
    case State::MIC_MUTE:
      //don't activate any.  Just leave it muted.
      break;
    case State::MIC_FRONT:
      inputMixer_L.gain(0,1.0);  //first aic (left earpiece), front mic
      inputMixer_R.gain(0,1.0);  //second aic (right earpiece), front mic
      break;
    case State::MIC_REAR:
      inputMixer_L.gain(0+1,1.0);  //first aic (left earpiece), rear mic
      inputMixer_R.gain(0+1,1.0);  //second aic (right earpiece), rear mic
      break;
    case State::MIC_BOTH_INPHASE:
      inputMixer_L.gain(0,0.5);  //first aic (left earpiece), front mic
      inputMixer_R.gain(0,0.5);  //second aic (right earpiece), front mic
      inputMixer_L.gain(0+1,1.0);  //first aic (left earpiece), rear mic
      inputMixer_R.gain(0+1,1.0);  //second aic (right earpiece), rear mic
      break;
    case State::MIC_BOTH_INVERTED:
      inputMixer_L.gain(0,1.0);  //first aic (left earpiece), front mic
      inputMixer_R.gain(0,1.0);  //second aic (right earpiece), front mic
      inputMixer_L.gain(0+1,-1.0);  //first aic (left earpiece), rear mic
      inputMixer_R.gain(0+1,-1.0);  //second aic (right earpiece), rear mic
      break;
    case State::MIC_AIC0_LR: //for use with mic jack or bluetooth, first aic only
      inputMixer_L.gain(2+0,  1.0);    //first aic, left input...yes this is weird
      inputMixer_R.gain(0+0,1.0);      //first aic right input...yes this is weird
      break;
    case State::MIC_AIC1_LR: //for use with mic jack or bluetooth, second aic only
      inputMixer_L.gain(0+1,  1.0);   //second aic, left input...yes, this is weird
      inputMixer_R.gain(2+1,1.0);    //second aic right input...yes, this is weird
      break;
    case State::MIC_BOTHAIC_LR: //for use with mic jack or bluetooth, both aics
      inputMixer_L.gain(0+1,  0.5);    //first aic, left input
      inputMixer_L.gain(2+9,  0.5);    //first aic, left input
      inputMixer_R.gain(0+0,0.5);  //first aic right input
      inputMixer_R.gain(2+1,0.5);  //first aic right input
      break;
  }

  if (myState.input_source == State::INPUT_PDMMICS) {
    //digital input configuration
    myState.digital_mic_config = inputmix_config;
  } else {
    //analog input configuration
    myState.analog_mic_config = inputmix_config;
  }
}

void setOutputAIC(int config) {
  const int chan_on = 0;  const int chan_off = 1;
  switch (config) {
    case State::OUT_BOTH:
      switchOut0_L.switchChannel(chan_on);
      switchOut0_R.switchChannel(chan_on);
      switchOut1_L.switchChannel(chan_on);
      switchOut1_R.switchChannel(chan_on);
      break;
    case State::OUT_AIC0:
      switchOut0_L.switchChannel(chan_on);
      switchOut0_R.switchChannel(chan_on);
      switchOut1_L.switchChannel(chan_off);
      switchOut1_R.switchChannel(chan_off);    
      break;
    case State::OUT_AIC1:
      switchOut0_L.switchChannel(chan_off);
      switchOut0_R.switchChannel(chan_off);
      switchOut1_L.switchChannel(chan_on);
      switchOut1_R.switchChannel(chan_on);      break;
    case State::OUT_NONE:
      switchOut0_L.switchChannel(chan_off);
      switchOut0_R.switchChannel(chan_off);
      switchOut1_L.switchChannel(chan_off);
      switchOut1_R.switchChannel(chan_off);    
      break;
  }
  myState.output_aic = config;
}


void setupCompressors(AudioEffectCompWDRC_F32 &comp) {
 
  //set the speed of the compressor's response
  float attack_msec = 5.0,release_msec = 300.0;
  comp.setAttackRelease_msec(attack_msec, release_msec); 

  //define full scale as 0dB 
  comp.setMaxdB(115.0);

  //define expansion (ie, noise gating)
  float knee_end_expansion_dBSPL = 25;  
  comp.setKneeExpansion_dBSPL(knee_end_expansion_dBSPL);
  float exp_comp_ratio = 0.57;  //1.0 is no expansion or compression;  less than 1 is expansion
  comp.setExpansionCompRatio(exp_comp_ratio);

  //define the linear gain
  float linear_gain_dB = 20.0;
  comp.setGain_dB(linear_gain_dB);
  
  //now define the compression kneepoint relative to full scale
  float knee_compressor_dBSPL = 55.0;  
  float comp_ratio = 2.0;  //compression ratio when audio level is above the kneepoint
  comp.setKneeCompressor_dBSPL(knee_compressor_dBSPL);  
  comp.setCompRatio(comp_ratio); 
  
  //finally, the WDRC module includes a limiter at the end.  set its parameters
  float knee_limiter_dBSPL = 90.0;  //was -2 for cup, set to 0 dB or higher to discable
  //note: the WDRC limiter is hard-wired to a compression ratio of 10:1
  comp.setKneeLimiter_dBSPL(knee_limiter_dBSPL);  
}



// ////////////// Change settings of system
//here's a function to change the volume settings.   We'll also invoke it from our serialManager
void incrementInputGain(float increment_dB) {
  setInputGain_dB(inputGain_dB+increment_dB);
}

void setInputGain_dB(float newGain_dB) { 
  //Record new gain
  inputGain_dB = newGain_dB;

  //Set gain
  myTympan.setInputGain_dB(inputGain_dB);   //set the AIC on the main Tympan board
  aicShield.setInputGain_dB(inputGain_dB);  //set the AIC on the Earpiece Shield
  myTympan.print("Input Gain: "); myTympan.print(inputGain_dB); myTympan.println("dB");
}

//Increment Headphone Output Volume
void incrementKnobGain(float increment_dB) { 
  setOutputVolume_dB(outputVolume_dB+increment_dB);
}

void setOutputVolume_dB(float newVol_dB) {
  //Update output volume;Limit vol_dB to safe values
  outputVolume_dB = max(min(newVol_dB, 24.0),-60.0);

  //Set output volume
  myTympan.volume_dB(outputVolume_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  aicShield.volume_dB(outputVolume_dB);
  myTympan.print("Output Volume: "); myTympan.print(outputVolume_dB); myTympan.println("dB");
}

//here's a function to change the highpass filter cutoff.  We'll also invoke it from our serialManager
float incrementHPCutoffFreq_Hz(float increment_frac) {
  return setHPCutoffFreq_Hz(increment_frac * myState.hp_cutoff_Hz);
}
float setHPCutoffFreq_Hz(float cutoff_Hz) {
  float min_allowed_Hz = 62.5f, max_allowed_Hz = 8000.f;
  myState.hp_cutoff_Hz = max(min_allowed_Hz,min(max_allowed_Hz, cutoff_Hz));

  myTympan.print("Setting highpass filter cutoff to ");myTympan.print(myState.hp_cutoff_Hz);myTympan.println(" Hz");
  hp_filt1.setHighpass(0, myState.hp_cutoff_Hz); //biquad IIR filter.
  hp_filt2.setHighpass(0, myState.hp_cutoff_Hz); //biquad IIR filter.

  return myState.hp_cutoff_Hz;
}



float incrementExpKnee_dBSPL(float increment_dB) {
  float newValue = comp1.getKneeExpansion_dBSPL() + increment_dB;
  comp2.setKneeExpansion_dBSPL(newValue);
  return comp1.setKneeExpansion_dBSPL(newValue);
}

float incrementLinearGain_dB(float increment_dB) {
  float newValue = comp1.getGain_dB() + increment_dB;
  comp2.setGain_dB(newValue);
  return comp1.setGain_dB(newValue);
}

float incrementCompKnee_dBSPL(float increment_dB) {
  float newValue = comp1.getKneeCompressor_dBSPL() + increment_dB;
  comp2.setKneeCompressor_dBSPL(newValue);
  return comp1.setKneeCompressor_dBSPL(newValue);
}

float incrementCompRatio(float increment) {
  float newValue = comp1.getCompRatio() + increment;
  newValue = max(newValue,1.0);
  comp2.setCompRatio(newValue);
  return comp1.setCompRatio(newValue);
}

float incrementLimiterKnee_dBSPL(float increment_dB) {
  float newValue = comp1.getKneeLimiter_dBSPL() + increment_dB;
  comp2.setKneeLimiter_dBSPL(newValue);
  return comp1.setKneeLimiter_dBSPL(newValue);
}

void resetAllParametersToDefault(void) {
  
  
}

// ///////////////// Servicing routines
//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) {
    lastUpdate_millis = 0; //handle wrap-around of the clock
  }
  
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //Output 0.0 to 1.0
    val = (1.0/8.0) * (float)((int)(8.0 * val + 0.5)); //quantize X steps (to reduce chatter).  Output 0.0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around

      //choose the desired gain value based on the knob setting
      const float min_gain_dB = -20.0, max_gain_dB = 20.0; //set desired gain range
      float vol_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      //command the new gain setting
      setOutputVolume_dB(vol_dB);
      myTympan.print("servicePotentiometer: Headphone (dB) = "); myTympan.println(vol_dB); //print text to Serial port for debugging
    }
    
    lastUpdate_millis = curTime_millis;
    
  } // end if
} //end servicePotentiometer();


void serviceLEDs(void) {
  static int loop_count = 0;
  loop_count++;
  
  //if (audioSDWriter.getState() == AudioSDWriter::STATE::UNPREPARED) {
  //  if (loop_count > 200000) {  //slow toggle
  //    loop_count = 0;
  //    toggleLEDs(true,true); //blink both
  //  }
  //} 
  //else if (audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING) {
  if (audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING) {
    //let's flicker the LEDs while writing
    loop_count++;
    if (loop_count > 20000) { //fast toggle
      loop_count = 0;
      toggleLEDs(true,true); //blink both
    }
  } 
  else {
    // //toggle slowly
    //if (loop_count > 200000) { //slow toggle
    //  loop_count = 0;
    //  toggleLEDs(false,true); //just blink the red
    //}
    //set based on output
    switch (myState.output_aic) {
      case State::OUT_BOTH:
        myTympan.setAmberLED(true);myTympan.setRedLED(true);
        break;
      case State::OUT_AIC0:
        myTympan.setAmberLED(true);myTympan.setRedLED(false);
        break;
      case State::OUT_AIC1:
        myTympan.setAmberLED(false);myTympan.setRedLED(true);
        break;
      case State::OUT_NONE:
        myTympan.setAmberLED(false);myTympan.setRedLED(false);
        break;
    }
  }
}

void toggleLEDs(void) {
  toggleLEDs(true,true);  //toggle both LEDs
}
void toggleLEDs(const bool &useAmber, const bool &useRed) {
  static bool LED = false;
  LED = !LED;
  if (LED) {
    if (useAmber) {
      myTympan.setAmberLED(true);
    }
    if (useRed) {
      myTympan.setRedLED(false);
    }
  } else {
    if (useAmber) {
      myTympan.setAmberLED(false);
    }
    if (useRed) {
      myTympan.setRedLED(true);
    }
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
          Serial.print("SD Write Warning: there was a hiccup in the writing.");//  Approx Time (sec): ");
          Serial.println(approx_time_sec );
        }
      }
    }
    i2s_in.clear_isOutOfMemory();
  }
}

void servicePlotting(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) {
    lastUpdate_millis = 0; //handle wrap-around of the clock
  }
  
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    if (myState.flag_sendPlotLegend) {
      myTympan.print("P");
      myTympan.print('"');myTympan.print("LeftSPL");myTympan.print('"');
      myTympan.print(",");
      myTympan.print('"');myTympan.print("RightSPL");myTympan.print('"');
      myTympan.println();
      myState.flag_sendPlotLegend = false;
    }
    
    myTympan.print("P");
    myTympan.print(comp1.getCurrentLevel_dB() + comp1.getMaxdB());
    myTympan.print(",");
    myTympan.print(comp2.getCurrentLevel_dB() + comp2.getMaxdB());
    myTympan.println();
    
    lastUpdate_millis = curTime_millis;
  }

} //end servicePotentiometer();  
