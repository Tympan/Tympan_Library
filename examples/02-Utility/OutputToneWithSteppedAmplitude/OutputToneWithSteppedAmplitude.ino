/*
  Output Tone With Stepped Amplitude

  Chip Audette, OpenAudio 2018

  Plays a single tone followed by silence.  Repeats the tone+silence at increasing amplitude.
  The last tone should definitely distort.  The 2nd-to-last tone should be on the edge of distorting.
  WARNING: DO NOT LISTEN TO THIS WITH HEADPHONES ON

  MIT License, Use at your own risk.
*/

#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// Create the audio library objects that we'll use
Tympan                    myTympan(TympanRev::E, audio_settings);   //use TympanRev::D or TympanRev::E
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
  myTympan.beginBothSerial(); delay(1000); //both Serial (USB) and Serial1 (BT) are started here
  myTympan.println("OutputTone: starting setup()...");

  //allocate the audio memory
  AudioMemory_F32(20,audio_settings); //I can only seem to allocate 400 blocks
  
  //set the sine wave parameters
  sineWave.frequency(tone_freq_Hz);
  sineWave.amplitude(0.0);
  myTympan.setAmberLED(LOW);
  
  //start the audio hardware
  myTympan.enable();

  //Set the baseline volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
 
  myTympan.println("Setup complete.");
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
      myTympan.setAmberLED(LOW);
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
      myTympan.print("Changing to start tone, amplitude = ");myTympan.println(tone_amp[amp_index]);
      myTympan.setAmberLED(HIGH);
      start_time_millis = millis();
      state = PLAY_TONE;
      break;
    case PLAY_TONE:
      if ((millis() - start_time_millis) >= tone_dur_msec) {
        state = START_SILENCE;
        myTympan.println("Changing to start silence...");
      }
      break;     
  }
}
