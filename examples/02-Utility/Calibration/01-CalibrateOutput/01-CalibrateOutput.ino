/*
  01-CalibrateOutput

  Created: Chip Audette, OpenAudio, Oct 2024
  Purpose: Generates a sine wave and outputs it to the black headphone jack.  You can
    measure the amplitude of the signal using an external volt meter.

    Remember that the Tympan's audio codec chip (AIC) can provide additional
    gain (DAC gain and headphone amp gain).  So, if your own program deviates from
    default Tympan setings for these AIC gains, you will need to re-calibrate
    using your exact settings.

  For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
  For Tympan Rev E and F, program in Arduino IDE as a Teensy 4.1.

  MIT License.  use at your own risk.
*/

// Include all the of the needed libraries
#include <Tympan_Library.h>
#include "SerialManager.h"

//set the sample rate and block size
const float                sample_rate_Hz = 44100.0f ; //24000 or 44117 or 96000 (or other frequencies in the table in AudioOutputI2S_F32)
const int                  audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
AudioSettings_F32          audio_settings(sample_rate_Hz, audio_block_samples);

// Create the audio objects and then connect them
Tympan                     myTympan(TympanRev::F,audio_settings); //do TympanRev::D or TympanRev::E or TympanRev::F
AudioInputI2S_F32          i2s_in(audio_settings);                //Digital audio input from the ADC...not used in this program
AudioSynthWaveform_F32     sineWave(audio_settings);              //generate a synthetic sine wave
AudioOutputI2S_F32         i2s_out(audio_settings);               //Digital audio output to the DAC.  Should always be last.

//Connect the sineWave to its destinations
AudioConnection_F32        patchcord01(sineWave, 0, i2s_out, 0);   //Sine wave to left output
AudioConnection_F32        patchcord02(sineWave, 0, i2s_out, 1);   //Sine wave to right output

// Create classes for controlling the system, espcially via USB Serial and via the App        
SerialManager     serialManager;     //create the serial manager for real-time control (via USB or App)

// ///////////////// Functions to control the audio
float sine_freq_Hz = 1000.0;
float sine_amplitude = sqrt(2.0)*sqrt(pow(10.0,0.1*-20.0));   //(-20dBFS converted to linear and then converted from RMS to amplitude)
float setFrequency_Hz(float freq_Hz) { return sine_freq_Hz = sineWave.setFrequency_Hz(freq_Hz); }
float setAmplitude(float amplitude) { return sine_amplitude = sineWave.setAmplitude(amplitude); }


// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(500);
  while (!Serial && (millis() < 2000UL)) delay(5); //wait for 2 seconds to see if USB Serial comes up (try not to miss messages!)
  myTympan.println("CalibrateOutput: Starting setup()...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //activate the Tympan audio hardware
  myTympan.enable();        // activate the flow of audio
  myTympan.volume_dB(0.0);  // headphone amplifier

  //setup the test tone
  setFrequency_Hz(sine_freq_Hz);
  setAmplitude(sine_amplitude);

  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //service the LEDs...the "false" tells it to blink slowly (simply to prove that it's alive)
  myTympan.serviceLEDs(millis(),false); 

} //end loop()



