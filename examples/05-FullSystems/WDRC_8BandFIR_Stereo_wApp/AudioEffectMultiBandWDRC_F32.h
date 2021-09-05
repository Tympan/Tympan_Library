
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
      compBroadband.name_for_UI = "WDRC Broadband";
    }

    // here are the mthods required (or encouraged) for AudioStream_F32 classes
    virtual void enable(bool _enable = true) { is_enabled = _enable; }
    virtual void update(void) {
     
      //get the input audio
      audio_block_f32_t *block_in = AudioStream_F32::receiveReadOnly_f32();
      if (block_in == NULL) return;
      
      //get the output audio
      audio_block_f32_t *block_out=allocate_f32();
      if (block_out == NULL) { AudioStream_F32::release(block_in); return;}  //there was no memory available
      
      // process audio
      int any_error = processAudioBlock(block_in, block_out);

      //if good, transmit the processed block of audio
      if (!any_error) AudioStream_F32::transmit(block_out); 
      
      //release the memory blocks
      AudioStream_F32::release(block_out);
      AudioStream_F32::release(block_in);
      
    } // close update()


    virtual int processAudioBlock(audio_block_f32_t *block_in, audio_block_f32_t *block_out) {
      //check validity of inputs
      int ret_val = -1;
      if ((block_in == NULL) || (block_out == NULL)) return ret_val;  //-1 is error

      //return if not enabled
      if (!is_enabled) return -1;

      //get a temporary working block for audio
      audio_block_f32_t * block_tmp = AudioStream_F32::allocate_f32();
      if (block_tmp == NULL) return ret_val;  //there was no memory available 

      //  /////////////////////////////////////////  loop over all the channels to do the per-band processing
      bool were_any_blocks_processed = false;
      int n_filters = filterbank.get_n_filters();
      bool firstChannelProcessed = true;
      for (int Ichan = 0; Ichan < n_filters; Ichan++) {

        if (filterbank.filters[Ichan].get_is_enabled()) {
          //apply the filter
          int any_error = filterbank.filters[Ichan].processAudioBlock(block_in,block_tmp);
          
          if (!any_error) {
            //apply the compressor
            any_error = compbank.compressors[Ichan].processAudioBlock(block_tmp,block_tmp);
            
            if (!any_error) {              
              //success!
              were_any_blocks_processed = true;  //if any one channel processes OK, this whole method will return OK

              //now we mix the processed signal with the other bands that have been processed
              if (firstChannelProcessed) {   
                             
                // First channel. Just copy it into the output
                for (int i=0; i < block_in->length; i++) block_out->data[i] = block_tmp->data[i];
                block_out->id = block_in->id;   block_out->length = block_in->length;
                firstChannelProcessed=false; //next time, we can't use this branch of code and we'll use the branch below
                
              } else {  
                              
                // Later channels.  Must sum this channel with the previous channels
                arm_add_f32(block_out->data, block_tmp->data, block_out->data, block_out->length);  
            
              }
            } else { // if(!any_error) for compbank
              //Serial.println(F("AudioEffectMultiBandWDRC_F32_UI: update: error in compbank chan ") + String(Ichan));
            } 
          } else { // if(!any_error) for filterbank
            //Serial.println(F("AudioEffectMultiBandWDRC_F32_UI: update: error in filterbank chan  ") + String(Ichan));
          }
        } else {  //if filter is enabled
          //Serial.print(F("AudioEffectMultiBandWDRC_F32_UI: update: filter is not enabled: Ichan = ")); Serial.println(Ichan);
        }
        
      } //close the loop over channels
        

      //release the temporary memory block
      AudioStream_F32::release(block_tmp);

      //  ////////////////////////////////////////////////////  now do broadband processing
      if (were_any_blocks_processed) {
        
        //apply some broadband gain (could probably be included in the compressor below
        broadbandGain.processAudioBlock(block_out, block_out);

        //apply final broadband compression
        int any_error = compBroadband.processAudioBlock(block_out, block_out);

        //if it got this far without an error, it means that the data processed OK!!
        if (!any_error) ret_val = 0;
      }

      //return
      return ret_val;

    } // close processAudioBlock()


    // here are the methods required (or encouraged) for SerialManager_UI classes
    virtual void printHelp(void) {};
    //virtual bool processCharacter(char c); //not used here
    virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char) {return false; };
    //virtual void setFullGUIState(bool activeButtonsOnly = false); //if commented out, use the one in StereoContrainer_UI.h


    // here are the constituent classes
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

    TR_Page* addPage_filterbank(TympanRemoteFormatter *gui);
    TR_Page* addPage_compressorbank_globals(TympanRemoteFormatter *gui);
    TR_Page* addPage_compressorbank_perBand(TympanRemoteFormatter *gui);
    TR_Page* addPage_compressor_broadband(TympanRemoteFormatter *gui);

    void addPairMultiBandWDRC(AudioEffectMultiBandWDRC_F32_UI* _left, AudioEffectMultiBandWDRC_F32_UI *_right) {
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

TR_Page* StereoContainerWDRC_UI::addPage_filterbank(TympanRemoteFormatter *gui) {
  if (gui == NULL) return NULL;
  TR_Page *page_h = gui->addPage("Filterbank");
  TR_Page page_foo; TR_Page *page_foo_h = &page_foo;
  if (page_h == NULL) return NULL;
  
  addCard_chooseChan(page_h); //see StereoContainer_UI.h
  if ((leftWDRC != NULL) && (rightWDRC != NULL)) {
    (leftWDRC->filterbank).addCard_crossoverFreqs(page_h);
    (rightWDRC->filterbank).addCard_crossoverFreqs(page_foo_h); //filterbank might track which GUI elements have been invoked.  This triggers that automatic behavior
  }

  return page_h;
}

TR_Page* StereoContainerWDRC_UI::addPage_compressorbank_globals(TympanRemoteFormatter *gui) {
  if (gui == NULL) return NULL;
  TR_Page *page_h = gui->addPage("Compressor Bank, Global Parameters");
  TR_Page page_foo; TR_Page *page_foo_h = &page_foo;
  if (page_h == NULL) return NULL;
  
  addCard_chooseChan(page_h); //see StereoContainer_UI.h
  if ((leftWDRC != NULL) && (rightWDRC != NULL)) {
    (leftWDRC->compbank).addCard_attack_global(page_h);
    (leftWDRC->compbank).addCard_release_global(page_h);
    (leftWDRC->compbank).addCard_scaleFac_global(page_h);

    (rightWDRC->compbank).addCard_attack_global(page_foo_h);   //compbank might track which GUI elements have been invoked.  This triggers that automatic behavior
    (rightWDRC->compbank).addCard_release_global(page_foo_h);
    (rightWDRC->compbank).addCard_scaleFac_global(page_foo_h);
  }

  return page_h;
}

TR_Page* StereoContainerWDRC_UI::addPage_compressorbank_perBand(TympanRemoteFormatter *gui) {
  if (gui == NULL) return NULL;
  TR_Page *page_h = gui->addPage("Compressor Bank");
  TR_Page page_foo; TR_Page *page_foo_h = &page_foo;
  if (page_h == NULL) return NULL;
  
  addCard_chooseChan(page_h); //see StereoContainer_UI.h
  if ((leftWDRC != NULL) && (rightWDRC != NULL)) {
    (leftWDRC->compbank).addCard_chooseMode(page_h);
    (leftWDRC->compbank).addCard_persist_perChan(page_h);

    (rightWDRC->compbank).addCard_chooseMode(page_foo_h);   //compbank might track which GUI elements have been invoked.  This triggers that automatic behavior
    (rightWDRC->compbank).addCard_persist_perChan(page_foo_h);
  }

  return page_h;
}

TR_Page* StereoContainerWDRC_UI::addPage_compressor_broadband(TympanRemoteFormatter *gui) {
  if (gui == NULL) return NULL;
  TR_Page *page_h = gui->addPage("Broadband Compressor");
  TR_Page page_foo; TR_Page *page_foo_h = &page_foo;
  if (page_h == NULL) return NULL;
  
  addCard_chooseChan(page_h); //see StereoContainer_UI.h
  if ((leftWDRC != NULL) && (rightWDRC != NULL)) {
    (leftWDRC->compBroadband).addCards_allParams(page_h);
    (rightWDRC->compBroadband).addCards_allParams(page_foo_h);  //compBroadband might track which GUI elements have been invoked.  This triggers that automatic behavior
  }

  return page_h;
}
#endif
