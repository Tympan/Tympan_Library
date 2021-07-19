// Tympan_Earpieces
//
// Created: Chip Audette, Open Audio, Dec 2019
//
// This example code is in the public domain.
//
// Hardware: Assumes that you're using a Tympan RevD with an Earpiece_Shield
//    and two Tympan earpieces, each having its two PDM micrphones.
//
//  According to the schematic for the Tympan Earpiece_Shield, the two PDM
//    mics in the left earpiece go to the first AIC (ie, I2S channels 0 and 1)
//    while the two PDM mics in the right earpiece go to the second AIC (which
//    are I2S channels 2 and 3).  In this sketch, I will use a mixer to allow you
//    to mix the two mics from each earpiece, or just use the first mic in each
//    earpiece.
//
//  Also, according to the schematic, the speakers in the two earpieces are
//    driven by the second AIC, so they would be I2S channel 2 (left) and 3 (right).
//    In this sketch, I will copy the audio so that it goes to the headphone outputs
//    of both AICs, so you can listen through regular headphones on the first AIC
//    or through the Tympan earpieces (or headphones) on the second AIC.
//
// This sketch works with the TympanRemote App on the Play Store
//   https://play.google.com/store/apps/details?id=com.creare.tympanRemote&hl=en_US
//


#include <Tympan_Library.h>
#include "State.h"
#include "SerialManager.h"

//set the sample rate and block size
const float sample_rate_Hz    = 44100.0f ;  //Allowed values: 8000, 11025, 16000, 22050, 24000, 32000, 44100, 44118, or 48000 Hz
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


//setup the  Tympan using the default settings
Tympan                        myTympan(TympanRev::D);    //note: Rev C is not compatible with the AIC shield
#define AIC_RESET_PIN 20   //for I2C connection to AIC shield (eventually move into Tympan library)
#define AIC_I2C_BUS 2     //for I2C connection to AIC shield (eventually move into Tympan library)
AudioControlAIC3206           aicShield(AIC_RESET_PIN,AIC_I2C_BUS);  //for AIC_Shield

// Define audio objects
AudioInputI2SQuad_F32         i2s_in(audio_settings);        //Digital audio *from* the Tympan AIC.
AudioMixer4_F32               inputMixer_L(audio_settings);     //For mixing (or not) the two mics in the left earpiece
AudioMixer4_F32               inputMixer_R(audio_settings);     //For mixing (or not) the two mics in the right earpiece
AudioOutputI2SQuad_F32        i2s_out(audio_settings);       //Digital audio *to* the Tympan AIC.  Always list last to minimize latency
AudioSDWriter_F32             audioSDWriter(audio_settings); //this is stereo by default

//Connect the inputs to the input mixers
AudioConnection_F32           patchcord1(i2s_in, 0, inputMixer_L, 0);   
AudioConnection_F32           patchcord2(i2s_in, 1, inputMixer_L, 1);  
AudioConnection_F32           patchcord3(i2s_in, 2, inputMixer_L, 2);   
AudioConnection_F32           patchcord4(i2s_in, 3, inputMixer_L, 3);   
AudioConnection_F32           patchcord5(i2s_in, 0, inputMixer_R, 0);
AudioConnection_F32           patchcord6(i2s_in, 1, inputMixer_R, 1);
AudioConnection_F32           patchcord7(i2s_in, 2, inputMixer_R, 2);
AudioConnection_F32           patchcord8(i2s_in, 3, inputMixer_R, 3);

//Connect the mixers to the outputs
AudioConnection_F32           patchcord11(inputMixer_L, 0, i2s_out, 0);   //First AIC, left output
AudioConnection_F32           patchcord12(inputMixer_R, 0, i2s_out, 1); //First AIC, right output
AudioConnection_F32           patchcord13(inputMixer_L, 0, i2s_out, 2); //Second AIC (Earpiece!), left output
AudioConnection_F32           patchcord14(inputMixer_R, 0, i2s_out, 3); //Secibd AIC (Earpiece!), right output

//Connect the raw inputs to the SD card
AudioConnection_F32           patchcord21(i2s_in, 0, audioSDWriter, 0);   //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord22(i2s_in, 1, audioSDWriter, 1);  //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord23(i2s_in, 2, audioSDWriter, 2);   //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord24(i2s_in, 3, audioSDWriter, 3);  //connect Raw audio to queue (to enable SD writing)

//control display and serial interaction
SerialManager                 serialManager;
State                         myState(&audio_settings, &myTympan);
String overall_name = String("Tympan: Earpieces with PDM Mics");

//set the desired input source 
void setInputSource(int input_config) { 
  switch (input_config) {
    case State::INPUT_PCBMICS:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      aicShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);

      //Disable Digital Mic (enable analog inputs)
      myTympan.enableDigitalMicInputs(false);
      aicShield.enableDigitalMicInputs(false);

      //Set input gain in dB
      setInputGain_dB(15.0);

      //Store configuration
      myState.input_source = input_config;
      break;
      
    case State::INPUT_MICJACK_MIC:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);

      //Disable Digital Mic (enable analog inputs)
      myTympan.enableDigitalMicInputs(false);
      aicShield.enableDigitalMicInputs(false);

      //Set input gain in dB
      setInputGain_dB(15.0);

      //Store configuration
      myState.input_source = input_config;
      break;
      
  case State::INPUT_MICJACK_LINEIN:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the mic jack
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN);

      //Disable Digital Mic (enable analog inputs)
      myTympan.enableDigitalMicInputs(false);
      aicShield.enableDigitalMicInputs(false);

      //Set input gain in dB
      setInputGain_dB(0.0);

      //Store configuration
      myState.input_source = input_config;
      break;     
    case State::INPUT_LINEIN_SE:      
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes
      aicShield.inputSelect(TYMPAN_INPUT_LINE_IN);

      //Disable Digital Mic (enable analog inputs)
      myTympan.enableDigitalMicInputs(false);
      aicShield.enableDigitalMicInputs(false);

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

void setInputMixer(int inputmix_config) {
  //clear all previous connections
  for (int i=0; i<4; i++) {
    inputMixer_L.gain(i,0.0);
    inputMixer_R.gain(i,0.0);
  }

  //now, choose just the connections that we want
  switch (inputmix_config) {
    case State::MIC_FRONT:
      inputMixer_L.gain(0,1.0);  //first aic (left earpiece), left mic
      inputMixer_R.gain(2,1.0);  //second aic (right earpiece), left mic
      break;
    case State::MIC_REAR:
      inputMixer_L.gain(0+1,1.0);  //first aic (left earpiece), right mic
      inputMixer_R.gain(2+1,1.0);  //second aic (right earpiece), right mic
      break;
    case State::MIC_BOTH_INPHASE:
      inputMixer_L.gain(0,  0.5);    //first aic (left earpiece), left mic
      inputMixer_L.gain(0+1,0.5);  //first aic (left earpiece), right mic
      inputMixer_L.gain(2,  0.5);  //second aic (right earpiece), left mic
      inputMixer_L.gain(2+1,0.5);  //second aic (right earpiece), right mic
      break;
    case State::MIC_BOTH_INVERTED:
      inputMixer_L.gain(0,  0.5);    //first aic (left earpiece), left mic
      inputMixer_L.gain(0+1,-0.5);  //first aic (left earpiece), right mic inverted
      inputMixer_L.gain(2,  0.5);  //second aic (right earpiece), left mic
      inputMixer_L.gain(2+1,-0.5);  //second aic (right earpiece), right mic inverted
      break;
    case State::MIC_AIC0_LR: //for use with mic jack or bluetooth, first aic only
      inputMixer_L.gain(0,  1.0);    //first aic, left input
      inputMixer_R.gain(0+1,1.0);  //first aic right input
      break;
    case State::MIC_AIC1_LR: //for use with mic jack or bluetooth, second aic only
      inputMixer_L.gain(2,  1.0);    //first aic, left input
      inputMixer_R.gain(2+1,1.0);  //first aic right input
      break;
    case State::MIC_BOTHAIC_LR: //for use with mic jack or bluetooth, both aics
      inputMixer_L.gain(0,  0.5);    //first aic, left input
      inputMixer_L.gain(2,  0.5);    //first aic, left input
      inputMixer_R.gain(0+1,0.5);  //first aic right input
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

  //setup hardware
  setInputSource(State::INPUT_PDMMICS);
  setInputMixer(State::MIC_FRONT);

  //set headphone volume (will be overwritten by the volume pot)
  setOutputVolume_dB(0.0); //dB, -63.6 to +24 dB in 0.5dB steps.

  //Set the state of the LEDs
  myTympan.setRedLED(HIGH);
  myTympan.setAmberLED(LOW);

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(4);             //four channels for this quad recorder, but you could set it to 2
  audioSDWriter.setWriteDataType(AudioSDWriter::WriteDataType::INT16);  //this is the built-in the default, but here you could change it to FLOAT32
  Serial.print("SD configured for "); Serial.print(audioSDWriter.getNumWriteChannels()); Serial.println(" channels.");
  

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

}



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
    float val = float(myTympan.readPotentiometer()) / 1023.0; //Output 0.0 to 1.0
    val = (1.0/8.0) * (float)((int)(8.0 * val + 0.5)); //quantize X steps (to reduce chatter).  Output 0.0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around

      //choose the desired gain value based on the knob setting
      const float min_gain_dB = -20.0, max_gain_dB = 20.0; //set desired gain range
      float vol_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      //command the new gain setting
      vol_dB = setOutputVolume_dB(vol_dB);
      myTympan.print("servicePotentiometer: Headphone (dB) = "); myTympan.println(vol_dB); //print text to Serial port for debugging
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


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
  toggleLEDs(true,true);  //toggle both LEDs
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
          Serial.print("SD Write Warning: there was a hiccup in the writing.");//  Approx Time (sec): ");
          Serial.println(approx_time_sec );
        }
      }
    }
    i2s_in.clear_isOutOfMemory();
  }
}

// ////////////// Change settings of system

//here's a function to change the volume settings.   We'll also invoke it from our serialManager
float incrementInputGain(float increment_dB) {
  return setInputGain_dB(myState.inputGain_dB + increment_dB);
}
float setInputGain_dB(float gain_dB) { 
  myState.inputGain_dB = myTympan.setInputGain_dB(gain_dB);   //set the AIC on the main Tympan board
  aicShield.setInputGain_dB(gain_dB);  //set the AIC on the Earpiece Sheild
  return myState.inputGain_dB;
}

//Increment Headphone Output Volume
float incrementKnobGain(float increment_dB) { 
  return setOutputVolume_dB(myState.volKnobGain_dB+increment_dB);
}
float setOutputVolume_dB(float vol_dB) {
  vol_dB = max(min(vol_dB, 24.0),-60.0);
  myState.volKnobGain_dB = myTympan.volume_dB(vol_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  aicShield.volume_dB(vol_dB);
  serialManager.setOutputGainButtons(); //update the TympanRemote GUI...it's unfortunate that I have to do it here and not in SerialManager itself
  return myState.volKnobGain_dB;
}
