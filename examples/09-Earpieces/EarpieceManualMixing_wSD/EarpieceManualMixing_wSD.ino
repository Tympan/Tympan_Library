//
// EarpieceManualMixing_wSD
//
// Created: Eric Yuan, Jan 2020 (Updated by Chip Audette Aug 2021)
//
// Purpose: This example uses the earpieces and shows you how to setup your own audio mixers to mix the earpiece's 
// front and rear microphones.  This example can also record the raw microphone audio to the SD card.
//
// You control this sketch through the USB Serial via the Arduino IDE's Serial Monitor.  You can always type an
// "h" (without quotes) to get the help menu.
//
// The volume pot on the side of the Tympan also controls the overall output volume.
//
// Hardware: 
//    Assumes that you're using a Tympan RevE with an Earpiece Shield
//    and two Tympan earpieces (each with a front and back PDM micrphone) 
//    connected through the earpiece audio ports (which uses the USB-B Mini connector).
//
// Mixing:
//    The front and back mic for each earpiece will be mixed into a single channel.
//    The output will be routed to both the Tympan AIC (i2s_out[0,1]) and the 
//    Shield AIC (i2s_out[2,3]), which can be heard using the earpiece receivers 
//    or a headphone plugged into the 3.5mm audio jacks on either the Tympan or Shield
//
//    This example does NOT use the EarpieceMixer class.  The EarpieceMixer class helps
//    manage all the potential left-right / front-back mixing that you might want to do,
//    but it also hides how one interacts with the hardware.  This sketch exposes some
//    of those details.  So, this sketch might be better to learn from, if you care to
//    learn the details.
//
// There is no support for the Tympan Remote App in this sketch
//
// MIT License.  Use at your own risk.  Have fun!
//

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ;  //24000 to 44117 to 96000 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than audio_block_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// define classes to control the Tympan and the AIC_Shield
Tympan           myTympan(TympanRev::E, audio_settings);         //choose TympanRev::D or TympanRev::E
EarpieceShield   earpieceShield(TympanRev::E, AICShieldRev::A);  //Note that EarpieceShield is defined in the Tympan_Libarary in AICShield.h 

// Instantiate the audio classes
AudioInputI2SQuad_F32   i2s_in(audio_settings);         //Bring audio in
AudioMixer4_F32         inputMixerL(audio_settings);    //For mixing (or not) the two mics in the left earpiece
AudioMixer4_F32         inputMixerR(audio_settings);    //For mixing (or not) the two mics in the right earpiece
AudioOutputI2SQuad_F32  i2s_out(audio_settings);        //Send audio out
AudioSDWriter_F32       audioSDWriter(audio_settings);  //Write audio to the SD card (if activated)

//Connect the front and rear mics (from each earpiece) to input mixer for the left ear
AudioConnection_F32     patchcord1(i2s_in, EarpieceShield::PDM_LEFT_FRONT,  inputMixerL, 0);    //Left-Front Mic
AudioConnection_F32     patchcord2(i2s_in, EarpieceShield::PDM_LEFT_REAR,   inputMixerL, 1);    //Left-Rear Mic
AudioConnection_F32     patchcord3(i2s_in, EarpieceShield::PDM_RIGHT_FRONT, inputMixerL, 2);    //Right-Front Mic
AudioConnection_F32     patchcord4(i2s_in, EarpieceShield::PDM_RIGHT_REAR,  inputMixerL, 3);    //Right-Rear Mic

//Connect the front and rear mics (from each earpiece) to input mixer for the right ear
AudioConnection_F32     patchcord5(i2s_in, EarpieceShield::PDM_LEFT_FRONT,  inputMixerR, 0);    //Left-Front Mic
AudioConnection_F32     patchcord6(i2s_in, EarpieceShield::PDM_LEFT_REAR,   inputMixerR, 1);    //Left-Rear Mic
AudioConnection_F32     patchcord7(i2s_in, EarpieceShield::PDM_RIGHT_FRONT, inputMixerR, 2);    //Right-Front Mic
AudioConnection_F32     patchcord8(i2s_in, EarpieceShield::PDM_RIGHT_REAR,  inputMixerR, 3);    //Right-Rear Mic

//Connect the input mixers to both the Tympan and Shield audio outputs...which i2s output is associated with each audio output is in EarpieceShield.cpp  
AudioConnection_F32     patchcord11(inputMixerL, 0, i2s_out, EarpieceShield::OUTPUT_LEFT_TYMPAN);    //Tympan AIC, left output
AudioConnection_F32     patchcord12(inputMixerR, 0, i2s_out, EarpieceShield::OUTPUT_RIGHT_TYMPAN);   //Tympan AIC, right output
AudioConnection_F32     patchcord13(inputMixerL, 0, i2s_out, EarpieceShield::OUTPUT_LEFT_EARPIECE);  //Shield AIC, left output
AudioConnection_F32     patchcord14(inputMixerR, 0, i2s_out, EarpieceShield::OUTPUT_RIGHT_EARPIECE); //Shield AIC, right output

//Connect the input mixer to the SD card
AudioConnection_F32     patchcord21(i2s_in, EarpieceShield::PDM_LEFT_FRONT,  audioSDWriter, 0);   //connect Raw audio to SD writer
AudioConnection_F32     patchcord22(i2s_in, EarpieceShield::PDM_LEFT_REAR,   audioSDWriter, 1);   //connect Raw audio to SD writer
AudioConnection_F32     patchcord23(i2s_in, EarpieceShield::PDM_RIGHT_FRONT, audioSDWriter, 2);   //connect Raw audio to SD writer
AudioConnection_F32     patchcord24(i2s_in, EarpieceShield::PDM_RIGHT_REAR,  audioSDWriter, 3);   //connect Raw audio to SD writer

// //////////////// Manually define our own functions for choosing inputs and doing the front-back mixing
#include "MixerFunctions.h"  // for setInputSource() and for setInputMixer()


// //////////////// Control display and serial interaction via USB Serial
#include "State.h"                          //For enums
#include "SerialManager.h"                  //For processing serial communication
SerialManager                 serialManager;

//Static Variables
static float outputVolume_dB = 0.0;
static float inputGain_dB = 0.0;


// ///////////////// Main setup() and loop() as required for all Arduino programs
void setup() {
  myTympan.beginBothSerial(); delay(1500);
  Serial.println("EarpieceManualMixing_wSD: setup():...");
  Serial.print("Sample Rate (Hz): "); Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("Audio Block Size (samples): "); Serial.println(audio_settings.audio_block_samples);

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(40,audio_settings); //I can only seem to allocate 400 blocks

  //Enable the Tympan and AIC shields to start the audio flowing!
  myTympan.enable(); 
  earpieceShield.enable();

  //Set the state of the LEDs
  myTympan.setRedLED(HIGH);
  myTympan.setAmberLED(LOW);

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(4);             //four channels for this quad recorder, but you could set it to 2
  Serial.print("SD configured for "); Serial.print(audioSDWriter.getNumWriteChannels()); Serial.println(" channels.");

  //set headphone volume (will be overwritten by the volume pot)
  setOutputVolume_dB(10.0); //dB, -63.6 to +24 dB in 0.5dB steps.

  //Choose the input source
  setInputSource(INPUT_PDMMICS); //choose PDM Mics, which are the mics in the Tympan Earpieces

  //For each earpiece, mix front and back mics equally
  setInputMixer(ALL_MICS, 0.5);
  
  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();

}

void loop() {
  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial
  //while (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //BT Serial

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info
  
  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  //periodicallly check the potentiometer
  servicePotentiometer(millis(),100); //service the potentiometer every 100 msec
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
      const float min_gain_dB = -10.0, max_gain_dB = 30.0; //set desired gain range
      float vol_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      //command the new gain setting
      setOutputVolume_dB(vol_dB);
      Serial.print("servicePotentiometer: Headphone (dB) = "); Serial.println(vol_dB); //print text to Serial port for debugging
    }
    
    lastUpdate_millis = curTime_millis;
    
  } // end if
} //end servicePotentiometer();



// ////////////// Change settings of system from the Serial Monitor

//here's a function to change the volume settings.   We'll also invoke it from our serialManager
void incrementInputGain(float increment_dB) { setInputGain_dB(inputGain_dB+increment_dB);}
void setInputGain_dB(float newGain_dB) { 
  //Record new gain
  inputGain_dB = newGain_dB;

  //Set gain
  myTympan.setInputGain_dB(inputGain_dB);   //set the AIC on the main Tympan board
  earpieceShield.setInputGain_dB(inputGain_dB);  //set the AIC on the Earpiece Shield
  Serial.print("Input Gain: "); Serial.print(inputGain_dB); Serial.println("dB");
}

//Increment Headphone Output Volume
void incrementKnobGain(float increment_dB) {  setOutputVolume_dB(outputVolume_dB+increment_dB);}
void setOutputVolume_dB(float newVol_dB) {
  //Update output volume;Limit vol_dB to safe values
  outputVolume_dB = max(min(newVol_dB, 24.0),-60.0);

  //Set output volume
  myTympan.volume_dB(outputVolume_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  earpieceShield.volume_dB(outputVolume_dB);
  Serial.print("Output Volume: "); Serial.print(outputVolume_dB); Serial.println("dB");
}
