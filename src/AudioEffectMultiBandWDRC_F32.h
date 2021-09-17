
#ifndef _AudioEffectMultiBandWDRC_F32_h
#define _AudioEffectMultiBandWDRC_F32_h

/* 
 *  AudioEffectMultiBandWDRC_F32_UI
 *  
 *  Created: Chip Audette, OpenAudio, July-Sept 2021
 *  
 *  Background: It is common for one to want to create a multband WDRC compressor.  Normally, the Tympan
 *  examples assemble such an algorithm from its parts: (1) a filter bank, (2) a bank of compressors, 
 *  (3) an mixer to rejoint the channels, and (4) a final broadband compressor to use as a limiter.
 *  It is good to assemble your own multiband WDRC compressor, if you care what kind of filters you
 *  use (FIR vs IIR) or if you care to insert more processing or debugging information in between
 *  the baseline components.  BUT, if you just want a standard multi-band compressor, why go through all
 *  that work?
 *  
 *  Purpose: This class assembles a full multiband WDRC compression system that you can instantiate
 *  with a single call.  It also provides a GUI for the TympanRemote App to let you control the 
 *  full system.
 *  
 *  Stereo extension: You may also wish to run in stereo. To run in stereo, you would create two
 *  instances of this class.  But, if you want to control the two instances from the TympanRemote App,
 *  do you need to have twice as many pages (one set of pages for the left and another set of pages
 *  for the right)???  NO!  While you still do create two instances of the audio processing class, you
 *  can use the class StereoContainerWDRC_UI to setup the App GUI with left/right buttons to toggle
 *  one set of algorithm controls to swap between the left and right sides.
 *  
 *  License: MIT License.  Use at your own risk.  Have fun!
*/

#include <Arduino.h>
#include <AudioStream_F32.h>          //from Tympan Library
#include <AudioFilterbank_F32.h>      //from Tympan Library
#include <AudioEffectCompBankWDRC_F32.h>  //from Tympan Library
#include <AudioEffectGain_F32.h>     //from Tympan Library
#include <AudioEffectCompWDRC_F32.h> //from Tympan Library
#include <arm_math.h> 

class AudioEffectMultiBandWDRC_F32_UI : public AudioStream_F32, public SerialManager_UI {
  public:
    AudioEffectMultiBandWDRC_F32_UI(void): AudioStream_F32(1,inputQueueArray), SerialManager_UI() { setup(); } 
    AudioEffectMultiBandWDRC_F32_UI(const AudioSettings_F32 &settings) : AudioStream_F32(1,inputQueueArray), SerialManager_UI() { setup(); }

    // setup
    virtual void setup(void) {
      compBroadband.name_for_UI = "WDRC Broadband";  //overwrite the default name for the compressor being used as the broadband compressor
    }

    // here are the mthods required (or encouraged) for AudioStream_F32 classes
    virtual void enable(bool _enable = true) { is_enabled = _enable; }
    virtual void update(void);  //basically, this manages the memory ad then just calls processAudioBlock()

    virtual int processAudioBlock(audio_block_f32_t *block_in, audio_block_f32_t *block_out);


    // here are the methods required (or encouraged) for SerialManager_UI classes
    virtual void printHelp(void) {};
    //virtual bool processCharacter(char c); //not used here
    virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char) {return false; };
    //virtual void setFullGUIState(bool activeButtonsOnly = false); //if commented out, use the one in StereoContrainer_UI.h


	//methods to get the BTNRH form of the settings
	virtual void getDSL(BTNRH_WDRC::CHA_DSL *new_dsl);
	virtual void getWDRC(BTNRH_WDRC::CHA_WDRC *new_bb);

    // here are the constituent classes that make up the multiband compressor
    AudioFilterbankFIR_F32_UI      filterbank;
    AudioEffectCompBankWDRC_F32_UI compbank;
    AudioEffectGain_F32            broadbandGain;//broad band gain (could be part of compressor below)
    AudioEffectCompWDRC_F32_UI     compBroadband;//broadband compressor    

  protected:
    audio_block_f32_t *inputQueueArray[1];  //required as part of AudioStream_F32.  One input.
    bool is_enabled = false;
  
};


// /////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Make a stereo container for the MultiBandWDRC to ease the App GUI
//
// /////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <StereoContainer_UI.h>      //from Tympan Library

class StereoContainerWDRC_UI : public StereoContainer_UI {
  public:
    StereoContainerWDRC_UI(void) : StereoContainer_UI() {};

    virtual TR_Page* addPage_filterbank(TympanRemoteFormatter *gui);
    virtual TR_Page* addPage_compressorbank_globals(TympanRemoteFormatter *gui);
    virtual TR_Page* addPage_compressorbank_perBand(TympanRemoteFormatter *gui);
    virtual TR_Page* addPage_compressor_broadband(TympanRemoteFormatter *gui);

    virtual void addPairMultiBandWDRC(AudioEffectMultiBandWDRC_F32_UI* _left, AudioEffectMultiBandWDRC_F32_UI *_right) {
      //set the local pointers
      leftWDRC = _left;  rightWDRC = _right;
      
      //attach its components to our lists of left-right algorithm elements to be controlled via GUI
      add_item_pair(&(leftWDRC->filterbank),    &(rightWDRC->filterbank));
      add_item_pair(&(leftWDRC->compbank),      &(rightWDRC->compbank));
      add_item_pair(&(leftWDRC->compBroadband), &(rightWDRC->compBroadband));

      //Set the ID_char used for GUI fieldnames so that the right's are the same as the left's.
      //Only do this if we're only going to have one set of GUI elements to display both left
      //and right.  This is my intention, so I'm going to go ahead and do it.  
      //
      //Discussion: We probably want to always do this for stereo pairs of algorithm blocks?  Why else
      //would we be using the stereo container?  If so, this changing of the ID_char should probably
      //be moved into StereoContainer_UI?  I'm thinking "yes", but I need to live with this a bit more
      for (int i=0; (unsigned int)i<items_L.size(); i++) { items_R[i]->set_ID_char_fn(items_L[i]->get_ID_char_fn()); }
    }

  protected:
    AudioEffectMultiBandWDRC_F32_UI  *leftWDRC=NULL, *rightWDRC=NULL;
  
};

#endif
