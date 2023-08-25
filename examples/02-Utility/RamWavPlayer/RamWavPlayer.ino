/*
 * RamWavPlayer  (aka, AudioPlayMemoryI16_F32)
 * 
 * Created: Chip Audette, OpenAudio,Aug 2023
 * 
 * Purpose: Plays two audio samples stored in RAM
 *  
 */

#include <Tympan_Library.h>  //includes "AudioPlayMemoryI16_F32, which is the item being demonstrated here

//include the audio samples
#include "sample_YES.h"
#include "sample_NO.h"

//set the sample rate and block size
const float sample_rate_Hz = (int)(44100);  //choose a sample rate (anything up to 96000)
const int audio_block_samples = 128;        //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio objects
Tympan                   myTympan(TympanRev::E); //do TympanRev::D or TympanRev::E
AudioPlayMemoryI16_F32   audioPlayMemory(audio_settings);
AudioOutputI2S_F32       audioOutput(audio_settings);

//create audio connections
AudioConnection_F32      patchCord1(audioPlayMemory, 0, audioOutput, 0);
AudioConnection_F32      patchCord2(audioPlayMemory, 0, audioOutput, 1);


void setup() {
  myTympan.beginBothSerial(); delay(1200);
  myTympan.print("RamWavPlayer"); myTympan.println(": setup():...");
  myTympan.print("Sample Rate (Hz): "); myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("Audio Block Size (samples): "); myTympan.println(audio_settings.audio_block_samples);
  
  // Audio connections require memory to work.
  AudioMemory_F32(20, audio_settings); 

  // Start the Tympan
  myTympan.enable();
  myTympan.volume(0.5);  //any value 0 to 1.0??

  //finish setup
  delay(2000);  //stall a second
  Serial.println("Setup complete.");
}

int which_sound = 0, n_sounds = 3; //variables to keep track of which audio samples to play
void loop() {

  //start an audio sample?
  if (audioPlayMemory.isPlaying() == false) { //is any audio playing already?
    
    //start the new audio
    switch (which_sound) {
      case 0:
        Serial.println("Starting sample 'YES'...len = " + String(sample_YES_len) + " samples");
        audioPlayMemory.play(sample_YES, sample_YES_len, sample_YES_sample_rate_Hz); //will automatically upsample
        break;
      case 1:
        Serial.println("Starting sample 'NO'...len = " + String(sample_NO_len) + " samples");
        audioPlayMemory.play(sample_NO, sample_NO_len, sample_NO_sample_rate_Hz);
        break;
      case 2:
        {
        Serial.println("Starting combination of 'YES'/'NO' using a queue...");
        AudioPlayMemoryQueue queue;
        queue.addSample(sample_YES, sample_YES_len, sample_YES_sample_rate_Hz);
        queue.addSample(sample_NO, sample_NO_len, sample_NO_sample_rate_Hz);
        audioPlayMemory.play(queue);
        }
        break;
    }
    which_sound = (which_sound+1) % n_sounds; //increment to the next sound

    delay(1500); //stall a bit so that there is some space between the samples
  }
  
  //stall, just to be nice?
  delay(5);
}