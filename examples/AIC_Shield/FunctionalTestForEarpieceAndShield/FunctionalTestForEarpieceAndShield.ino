// Functional Test for Tympan Earpieces and Shield
//
// Created: Eric Yuan, Open Audio, Jan 2020
//
// This example code is in the public domain.
//
// Hardware: 
//  Assumes that you're using a Tympan RevD with an Earpiece Shield
//    and two Tympan earpieces (each with a front and back PDM micrphone) 
//    connected through the earpiece audio ports (which uses the USB-B Mini connector).
//
//  Mixing:
//  The front and back mic for each earpiece will be mixed into a single channel.
//  The output will be routed to both the Tympan AIC (i2s_out[0,1]) and the 
//  Shield AIC (i2s_out[2,3]), which can be heard using the earpiece receivers 
//  or a headphone plugged into the 3.5mm audio jacks on either the Tympan or Shield

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
AudioMixer4_F32               inputMixerL(audio_settings);    //For mixing (or not) the two mics in the left earpiece
AudioMixer4_F32               inputMixerR(audio_settings);    //For mixing (or not) the two mics in the left earpiece
AudioOutputI2SQuad_F32        i2s_out(audio_settings);        //Digital audio *to* the Tympan AIC.  Always list last to minimize latency
AudioSDWriter_F32             audioSDWriter(audio_settings);  //this is stereo by default

//Connect the front and rear mics (from each earpiece) to input mixers
AudioConnection_F32           patchcord1(i2s_in, 1, inputMixerL, 0);  //Left-Front Mic
AudioConnection_F32           patchcord2(i2s_in, 0, inputMixerL, 1);  //Left-Rear Mic
AudioConnection_F32           patchcord3(i2s_in, 3, inputMixerR, 0);  //Right-Front Mic
AudioConnection_F32           patchcord4(i2s_in, 2, inputMixerR, 1);  //Right-Rear Mic

//Connect the input mixers to both the Tympan and Shield audio outputs
//NOTE: The left and right RIC is correct, but the headphone jacks have the left and right swapped.  
AudioConnection_F32           patchcord11(inputMixerL, 0, i2s_out, 1); //Tympan AIC, left output
AudioConnection_F32           patchcord12(inputMixerR, 0, i2s_out, 0); //Tympan AIC, right output
AudioConnection_F32           patchcord13(inputMixerL, 0, i2s_out, 3); //Shield AIC, left output
AudioConnection_F32           patchcord14(inputMixerR, 0, i2s_out, 2); //Shield AIC, right output

//Connect the input mixer to the SD card
AudioConnection_F32           patchcord21(inputMixerL, 0, audioSDWriter, 0);   //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord22(inputMixerL, 1, audioSDWriter, 1);  //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord23(inputMixerR, 0, audioSDWriter, 2);   //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord24(inputMixerR, 1, audioSDWriter, 3);  //connect Raw audio to queue (to enable SD writing)

//control display and serial interaction
SerialManager                 serialManager;

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

  //Enable PDM mics
  setInputSource(INPUT_PDMMICS);

  //For each earpiece, mix front and back mics equally
  setInputMixer(ALL_MICS, 1.0);
  
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
}



//set the desired input source 
void setInputSource(Mic_Input micInput) { 
  switch (micInput){
    case INPUT_PCBMICS:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      aicShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);
      
    case INPUT_MICJACK_MIC:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);
      
  case INPUT_MICJACK_LINEIN:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the mic jack
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN);

    case INPUT_LINEIN_SE:      
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes
      aicShield.inputSelect(TYMPAN_INPUT_LINE_IN);
      
    case INPUT_PDMMICS:
      //Set the AIC's ADC to digital mic mode. Assign MFP4 to output a clock for the PDM, and MFP3 as the input to the PDM data line
      myTympan.enableDigitalMicInputs(true);
      aicShield.enableDigitalMicInputs(true);
      break;
  }

  // If turning on digital mics, enable them. Otherwise disable them.
  if (micInput==INPUT_PDMMICS)
  {
    //Enable Digital Mic (enable analog inputs)
    myTympan.enableDigitalMicInputs(true);
    aicShield.enableDigitalMicInputs(true);
  }
  else
  {
    //Disable Digital Mic (enable analog inputs)
    myTympan.enableDigitalMicInputs(false);
    aicShield.enableDigitalMicInputs(false);
  }
    
  //Set input gain in dB
  setInputGain_dB(0.0);
}


//Sets the input mixer gain for a given mic channel
void setInputMixer(Mic_Channels micChannelName, int gainVal) {
  switch (micChannelName) {
    case MIC_FRONT_LEFT:
      inputMixerL.gain(0,gainVal); 
      myTympan.print("Mic Left-Front Gain: ");
      break;
    case MIC_REAR_LEFT:
      inputMixerL.gain(1,gainVal); 
      myTympan.print("Mic Left-Rear Gain: ");
      break;
    case MIC_FRONT_RIGHT:
      inputMixerR.gain(0,gainVal);
      myTympan.print("Mic Right-Front Gain: ");
      break;
    case MIC_REAR_RIGHT:
      myTympan.print("Mic Right-Rear Gain: ");
      inputMixerR.gain(1,gainVal); 
      break;   
    case ALL_MICS:
      myTympan.print("All Mic Gain: ");
      inputMixerL.gain(0,gainVal);  //Left-Front Mic
      inputMixerL.gain(1,gainVal);  //Left-Rear Mic
      inputMixerR.gain(0,gainVal);  //Right-Front Mic
      inputMixerR.gain(1,gainVal);  //Right-Rear Mic
      break;         
  }
  myTympan.println(gainVal);
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
  
  if (audioSDWriter.getState() == AudioSDWriter::STATE::UNPREPARED) {
    if (loop_count > 200000) {  //slow toggle
      loop_count = 0;
      toggleLEDs(true,true); //blink both
    }
  } 
  else if (audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING) {
    //let's flicker the LEDs while writing
    loop_count++;
    if (loop_count > 20000) { //fast toggle
      loop_count = 0;
      toggleLEDs(true,true); //blink both
    }
  } 
  else {
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
