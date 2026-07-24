
#ifndef _AudioFilterGain_F32_h
#define _AudioFilterGain_F32_h

#include <Tympan_Library.h>

// For a normal audio-processing class, you would inheret from AudioStream_F32, which has most of the underlying
// plumbing.  Here for a composite audio class, you need a bit more plumbing, so you need to inhereit from
// AudioStreamComposite_F32 instead. 
//
// Then, inside the constructor, you instantiate all of the smaller audio processing classes that you'll be
// joining together here in this composite class.  You'll also instantiate all of the AudioConnection_F32
// objects that join them together.
//
// In the creation of these classes, the first class should always be "startNode = new AudioSwitchMatrix4_F32()"
// and the last class should always be "endNode = new AudioForwarder4_F32(audio_settings, this);".  This is auio
// audio gets injected into and pulled out of the composite class.
//
class AudioFilterGain_F32 : public AudioStreamComposite_F32 {  // AudioStreamComposite_F32 is in the Tympan_Library
  public:

    AudioFilterGain_F32(const AudioSettings_F32 &_audio_settings) : AudioStreamComposite_F32(_audio_settings) {

      // Instantiate audio classes...we're only storing them in a vector so that it's easier to destroy them later (whenver destruction is allowed by AudioStream)
      audioObjects.push_back( startNode = new AudioSwitchMatrix4_F32( _audio_settings )); startNode->instanceName = String("Input Matrix");  //per AudioStreamComposite_F32, always have this first
      audioObjects.push_back( hp_filt1  = new AudioFilterBiquad_F32( _audio_settings )); hp_filt1->instanceName  = String("Highpass Filter");
      audioObjects.push_back( gain1  = new AudioEffectGain_F32( _audio_settings )); gain1->instanceName  = String("Gain");
      audioObjects.push_back( endNode   = new AudioForwarder4_F32( _audio_settings, this ) ); endNode->instanceName = String("Output Forwarder"); //per AudioStreamComposite_F32, always have this last

      // Make all audio connections, except the final one to the destination...we're only storing them in a vector so that it's easier to destroy them later
      patchCords.push_back( new AudioConnection_F32(*startNode, 0, *hp_filt1, 0)); 
      patchCords.push_back( new AudioConnection_F32(*hp_filt1, 0, *gain1, 0)); 
      patchCords.push_back( new AudioConnection_F32(*gain1, 0, *endNode, 0));  
 
      // Set up the parameters of the audio processing
      setupAudioProcessing();
      
     // Choose a human-readable name for this audio path
      instanceName = "Filter+Gain";  // "name" is defined as a String in AudioStream_F32

      // Initialize to being active
      setActive(true);  // it's possible you may prefer to start with it inactive (you'll have to manually activate it somewhere in your Tympan code later)
    }

    //
    // /////////// Getters/Setters for critical parameters of the constituent audiop processing classes
    //
    // Alternatively, you could move the class pointers up out of "protected" and put them here in 
    // "public".  If you did that, you could access the constituent audio processing classes
    // directly from outside this composite class.  As a result, you wouldn't have to create all
    // of these getter/setter methods.  Personally, I don't mind writing these getters/setters
    // and I like the additonal layer of protection.  It's your choice!

    // setupAudioProcess: Initialize the default settings
    virtual void setupAudioProcessing(void) {
      setCutoff_Hz(1000.0);  //initial setting for highpass filter
      setGain_dB(0.0f);       //initial setting for gain
    }

    float setCutoff_Hz(const float freq_Hz) {
      if (freq_Hz > 1.e-3) hp_filt1->setHighpass(0, freq_Hz); 
      return getCutoff_Hz();
    }
    float getCutoff_Hz(void) const { return hp_filt1->getCutoffFrequency_Hz(); }

    float setGain_dB(const float val_dB) {
      gain1->setGain_dB(val_dB);
      return getGain_dB();
    }
    float getGain_dB(void) const { return gain1->getGain_dB(); }

    // ////////////////// End of Getters/Setters

  protected:
    // //// AudioStream_F32 objects (These pointers are also copied in AudioStreamComposite_F32::audioObjects)
    //AudioSwitchMatrix4_F32 *startNode = NULL; //already exists in AudioStreamComposite.h
    AudioFilterBiquad_F32 *hp_filt1 = nullptr;  // you could move this up to "public", if you wanted outside people to change its settings directly
    AudioEffectGain_F32 *gain1 = nullptr;       // you could move this up to "public", if you wanted outside people to change its settings directly
    //AudioForwarder4_F32 *endNode = NULL;   //already exists in AudioStreamComposite.h

};


#endif