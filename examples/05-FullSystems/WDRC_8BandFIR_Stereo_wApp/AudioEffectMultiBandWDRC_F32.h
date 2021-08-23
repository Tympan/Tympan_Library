
#ifndef _AudioEffectMultiBandWDRC_F32_h
#define _AudioEffectMultiBandWDRC_F32_h

#include <Arduino.h>
#include "AudioStream_F32.h"
#include "AudioFilterbank_F32.h"
#include "AudioEffectCompBankWDRC_F32.h"
#include "AudioEffectGain_F32.h"
#include "AudioEffectCompWDRC_F32.h"
#include "StereoContainer_UI.h"
#include <arm_math.h> 

class AudioEffectMultiBandWDRC_F32_UI : public AudioStream_F32, public SerialManager_UI {
  public:
    AudioEffectMultiBandWDRC_F32_UI(void): AudioStream_F32(1,inputQueueArray), SerialManager_UI() { } 
    AudioEffectMultiBandWDRC_F32_UI(const AudioSettings_F32 &settings) : AudioStream_F32(1,inputQueueArray), SerialManager_UI() { }

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
    virtual void setFullGUIState(bool activeButtonsOnly = false) {}; 


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

class StereoContainerWDRC_UI : public StereoContainer_UI {
  public:
    StereoContainerWDRC_UI(void) : StereoContainer_UI() {};

    TR_Page* addPage_filterbank(TympanRemoteFormatter *gui) {};
    TR_Page* addPage_compressorbank(TympanRemoteFormatter *gui) {};
    TR_Page* addPage_broadbandcompressor(TympanRemoteFormatter *gui) {};

    void addPairMultiBandWDRC(AudioEffectMultiBandWDRC_F32_UI* left, AudioEffectMultiBandWDRC_F32_UI *right) {
      add_item_pair(&(left->filterbank),    &(right->filterbank));
      add_item_pair(&(left->compbank),      &(right->compbank));
      add_item_pair(&(left->compBroadband), &(right->compBroadband));
    }

  protected:
  
};

#endif
