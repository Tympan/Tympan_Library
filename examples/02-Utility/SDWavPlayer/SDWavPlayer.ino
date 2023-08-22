/*
 * SDWavPlayer
 * 
 * Created: Chip Audette, OpenAudio, Dec 2019
 * Based On: WaveFilePlayer from Paul Stoffregen, PJRC, Teensy
 * 
 * Play back a WAV file through the Typman.
 * 
 * For access to WAV files, please visit https://www.pjrc.com/teensy/td_libs_AudioDataFiles.html.
 * 
 */

#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = (int)(44100);  //choose sample rate ONLY from options in the table in AudioOutputI2S_F32
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio objects
AudioSDPlayer_F32        audioSDPlayer(audio_settings); //this is in the Tympan_Library
AudioOutputI2S_F32       audioOutput(audio_settings);
Tympan                   myTympan(TympanRev::E); //do TympanRev::D or TympanRev::E

//create audio connections
AudioConnection_F32      patchCord1(audioSDPlayer, 0, audioOutput, 0);
AudioConnection_F32      patchCord2(audioSDPlayer, 1, audioOutput, 1);


void setup() {
  myTympan.beginBothSerial(); delay(1200);
  myTympan.print("SDWavPlayer"); myTympan.println(": setup():...");
  myTympan.print("Sample Rate (Hz): "); myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("Audio Block Size (samples): "); myTympan.println(audio_settings.audio_block_samples);
  
  // Audio connections require memory to work.
  AudioMemory_F32(20, audio_settings); 

  // Start the Tympan
  myTympan.enable();
  myTympan.volume(0.5);  //any value 0 to 1.0??

  //prepare SD player
  audioSDPlayer.begin();

  //finish setup
  delay(2000);  //stall a second
  Serial.println("Setup complete.");
}

unsigned long end_millis = 0;
String filename = "SDTEST1.WAV";// filenames are always uppercase 8.3 format
void loop() {

  //service the audio player
  if (!audioSDPlayer.isPlaying()) { //wait until previous play is done
    //start playing audio
    myTympan.print("Starting audio player: ");
    myTympan.println(filename);
    audioSDPlayer.play(filename);
  }

  //do other things here, if desired...like, maybe check the volume knob?


  //stall, just to be nice?
  delay(5);
}
