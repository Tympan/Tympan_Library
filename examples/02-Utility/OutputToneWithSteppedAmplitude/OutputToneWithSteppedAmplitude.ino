/*
  Output Tone With Stepped Amplitude

  Chip Audette, OpenAudio 2018

  Plays a single tone followed by silence.  Repeats the tone+silence at increasing amplitude.
  The last tone should definitely distort.  The 2nd-to-last tone should be on the edge of distorting.

  MIT License, Use at your own risk.
*/

#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// Create the audio library objects that we'll use
Tympan                tympanHardware(TympanRev::D);   //use TympanRev::D or TympanRev::C
AudioSynthWaveform_F32    sineWave(audio_settings);   //from the Tympan_Library
AudioOutputI2S_F32        audioOutput(audio_settings);//from the Tympan_Library

// Create the audio connections from the sineWave object to the audio output object
AudioConnection_F32 patchCord10(sineWave, 0, audioOutput, 0);  //connect to left output
AudioConnection_F32 patchCord11(sineWave, 0, audioOutput, 1);  //connect to right output


// Define the parameters of the tone
float tone_freq_Hz = 1000.0;   //frequency of the tone
#define N_AMP (8)              //How many different amplitudes to test
float tone_amp[N_AMP] = {0.1, 0.2, 0.4, 0.5, 0.6, 0.8, 1.0, 1.2};     //here are the amplitudes (1.0 is digital full scale) to test
float tone_dur_msec = 1000.0;  // Length of time for the tone, milliseconds
int amp_index = N_AMP-1;       //a counter to use when stepping through the given amplitudes

// define setup()...this is run once when the hardware starts up
void setup(void)
{
  //Open serial link for debugging
  Serial.begin(115200); delay(500);
  Serial.println("OutputTone: starting setup()...");

  //allocate the audio memory
  AudioMemory(10); AudioMemory_F32(20,audio_settings); //I can only seem to allocate 400 blocks
  Serial.println("OutputTone: memory allocated.");

  //set the sine wave parameters
  sineWave.frequency(tone_freq_Hz);
  sineWave.amplitude(0.0);
  tympanHardware.setAmberLED(LOW);
  
  //start the audio hardware
  tympanHardware.enable();

  //Set the baseline volume levels
  tympanHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
 
  Serial.println("Setup complete.");
}

// define loop()...this is run over-and-over while the device is powered
#define START_SILENCE 0
#define PLAY_SILENCE 1
#define START_TONE 2
#define PLAY_TONE 3
int state = START_SILENCE;
unsigned long start_time_millis = 0;
float silence_dur_msec = 1000.f;
int iteration_count = 0;
void loop(void)
{
  switch (state) {
    case START_SILENCE:
      sineWave.amplitude(0.0f);
      start_time_millis = millis();
      tympanHardware.setAmberLED(LOW);
      state = PLAY_SILENCE;
      break;
    case PLAY_SILENCE:
      if ((millis() - start_time_millis) >= silence_dur_msec) {
        state = START_TONE;
      }
      break;
    case START_TONE:
      amp_index++; if (amp_index == N_AMP) { amp_index = 0; iteration_count++;}
      sineWave.amplitude(tone_amp[amp_index]);
      Serial.print("Changing to start tone, amplitude = ");Serial.println(tone_amp[amp_index]);
      tympanHardware.setAmberLED(HIGH);
      start_time_millis = millis();
      state = PLAY_TONE;
      break;
    case PLAY_TONE:
      if ((millis() - start_time_millis) >= tone_dur_msec) {
        state = START_SILENCE;
        Serial.println("Changing to start silence...");
      }
      break;     
  }
}

