
#ifndef _BTNRH_Stereo_Preset_h
#define _BTNRH_Stereo_Preset_h

#include <BTNRH_WDRC_Types.h> //from Tympan_Library
#include <PresetManager_UI.h> //from Tympan_Library

#define DEFAULT_FILTER_ORDER 96  //assume FIR (96) or IIR (6)
#define INIT_MAX_ALLOWED_N_CHAN 16 //this can be set larger by commanding something different in the class

class BTNRH_Preset {
  public:
    BTNRH_Preset(){};
    BTNRH_Preset(const BTNRH_WDRC::CHA_DSL &dsl_L, const BTNRH_WDRC::CHA_WDRC &bb_L, const int filt_order) {
      specifyPreset(dsl_L, bb_L, filt_order);
    }

    void specifyPreset(const BTNRH_Preset & preset) {
      specifyPreset(preset.wdrc_perBand, 
                    preset.wdrc_broadband, 
                    preset.n_filter_order);
    }
    void specifyPreset(const BTNRH_WDRC::CHA_DSL &dsl, const BTNRH_WDRC::CHA_WDRC &bb, const int n_order) {
      if (n_order < 1) {
        Serial.println("BTNRH_Preset: *** WARNING ***: setting filter order to " + String(n_order) + " which should be >= 1"); 
      }
      wdrc_perBand = CHA_DSL_SD(dsl); 
      wdrc_broadband = CHA_WDRC_SD(bb); 
      n_filter_order = n_order;   
    }

   //used for making new presets
    virtual int interpolateSettingsGivenFrequencies(const BTNRH_Preset &prevPreset, int new_n_chan, float *new_cross_Hz) {
      if (new_n_chan < 3) return -1;  //error
      int new_n_crossover = new_n_chan-1;

      //create some simpler names
      const BTNRH_WDRC::CHA_DSL *prev_wdrc = &(prevPreset.wdrc_perBand); //create a simpler name
      BTNRH_WDRC::CHA_DSL *new_wdrc = &(wdrc_perBand);             //create a simpler name
      int old_n_chan = prev_wdrc->nchannel;                  //create a simpler name
      int old_n_crossover = old_n_chan-1;
      const float *old_cross_Hz = prev_wdrc->cross_freq;           //create a simpler name
      
      //copy the globals from the previous prescription to the new one
      new_wdrc->attack  = prev_wdrc->attack;        
      new_wdrc->release = prev_wdrc->release; 
      new_wdrc->maxdB   = prev_wdrc->maxdB; 
      new_wdrc->ear     = left_or_right;  
      new_wdrc->nchannel = new_n_chan; 

      //set the filter crossover frequencies based on the given new crossover frequencies
      for (int i=0; i < new_n_crossover; i++) new_wdrc->cross_freq[i] = new_cross_Hz[i];

      //compute the center frequencies of the channels (for both the old and new prescriptions)
      float old_center_Hz[old_n_chan], new_center_Hz[new_n_chan];
      calcCenterFromCrossover(old_n_chan, old_cross_Hz, old_center_Hz);
      calcCenterFromCrossover(new_n_chan, new_cross_Hz, new_center_Hz);

      #if 1
        //quality check
        if (fabs(new_center_Hz[0]-old_center_Hz[0]) > 1.0) {
          Serial.println("BTNRH_Preset: interpolateSettings: *** ERROR ***: old and new first freqs should be the same:");
          Serial.println("    : " + String(old_center_Hz[0]) + ", " + String(new_center_Hz[0]));
        }
        if (fabs(new_center_Hz[new_n_crossover-1]-old_center_Hz[old_n_crossover-1]) > 1.0) {
          Serial.println("BTNRH_Preset: interpolateSettings: *** ERROR ***: old and new last freqs should be the same:");
          Serial.println("    : " + String(old_center_Hz[old_n_crossover-1]) + ", " + String(new_center_Hz[new_n_crossover-1]));
        }
      #endif

      //interpolate values given the center frequencies
      for (int i=0; i<new_n_chan; i++) {
        //find index where the source frequency is above the current target frequency
        float targ_freq_Hz = new_center_Hz[i];
        int source_ind = 1; //start at the second (ie, ind = 1) so that we can always look back one step
        while ((targ_freq_Hz > old_center_Hz[source_ind]) && (source_ind < (new_n_chan-1))) source_ind++;
        float scale_fac = (logf(targ_freq_Hz)-logf(old_center_Hz[source_ind-1]))/(logf(old_center_Hz[source_ind])-logf(old_center_Hz[source_ind-1]));

        //apply interpolation to every data member of CHA_DSL that is frequency dependent (make sure you get them all!!)
        const float *prev_vals;
        prev_vals = prev_wdrc->exp_cr;       new_wdrc->exp_cr[i]       = calcInterpValue(scale_fac,prev_vals[source_ind-1],prev_vals[source_ind]);
        prev_vals = prev_wdrc->exp_end_knee; new_wdrc->exp_end_knee[i] = calcInterpValue(scale_fac,prev_vals[source_ind-1],prev_vals[source_ind]);
        prev_vals = prev_wdrc->tkgain;       new_wdrc->tkgain[i]       = calcInterpValue(scale_fac,prev_vals[source_ind-1],prev_vals[source_ind]);
        prev_vals = prev_wdrc->cr;           new_wdrc->cr[i]           = calcInterpValue(scale_fac,prev_vals[source_ind-1],prev_vals[source_ind]);
        prev_vals = prev_wdrc->tk;           new_wdrc->tk[i]           = calcInterpValue(scale_fac,prev_vals[source_ind-1],prev_vals[source_ind]);
        prev_vals = prev_wdrc->bolt;         new_wdrc->bolt[i]         = calcInterpValue(scale_fac,prev_vals[source_ind-1],prev_vals[source_ind]);         
      }

      return 0;  //normal finish, no error
    }

    int saveToSD(String fname, bool start_new_file) {
      String var_name1 = perBand_var_name + suffix_var_name;
      Serial.println("BTNRH_Preset: saveToSD: writing preset, " + var_name1);
      wdrc_perBand.printToSD(fname, var_name1, start_new_file);
      
      start_new_file= false; //don't start a new file for future channels...everything else will be appended
  
      String var_name2 = broadband_var_name + suffix_var_name;
      Serial.println("BTNRH_Preset: saveToSD: writing preset, " + var_name2);
      wdrc_broadband.printToSD(fname, var_name2, start_new_file);

      //add saving of AFC once adding AFC
//      String var_name3 = afc_var_name + suffix_var_name;
//      Serial.println("BTNRH_Preset: saveToSD: writing preset, " + var_name3);
//      afc.printToSD(fname, var_name3, start_new_file);

      return 0; //return OK
    }

   //data members for the algorithms that this preset works for
    int n_filter_order = DEFAULT_FILTER_ORDER; //assumed same for left and right, default (96 = FIR, 6 = IIR)...can be changed simply by changing n_filter_order in your code
    CHA_DSL_SD wdrc_perBand;    //left and right
    CHA_WDRC_SD wdrc_broadband; //left and right
    //CHA_AFC_SD afc; //left and right    
    int left_or_right = 0; //default to left

    //used for saving and retrieving these presets from the SD card
    String name = "Preset";
    const int n_datatypes = 2; //there is dsl and bb (eventually there will be afc)
    String perBand_var_name = String("dsl");
    String broadband_var_name = String("bb");
    String suffix_var_name = String("");  //default to no suffix on the variable names in the preset file

    
  protected:
    virtual void calcCenterFromCrossover(const int n_center, const float *cross_Hz, float *center_Hz) {
      int n_crossover = n_center -1;
      float scale_fac = expf(logf(cross_Hz[n_crossover-1]/cross_Hz[0]) / ((float)(n_crossover-1)));
      for (int i=0; i<n_center; i++) {
        if (i==0) {
          //center_Hz[i] = cross_Hz[i]*1.0/sqrtf(scale_fac);
          center_Hz[i] = cross_Hz[i];
        } else if (i == (n_center-1)) {
          center_Hz[i] = cross_Hz[n_crossover-1]*sqrtf(scale_fac);
        } else {
          //center_Hz[i] = cross_Hz[i-1]*expf(0.5*logf(cross_Hz[i]/cross_Hz[i-1]));
          center_Hz[i] = cross_Hz[i];
        }        
      }
    }
    virtual float calcInterpValue(float scale_fac, float old_val_high, float old_val_low) {
      return scale_fac*(old_val_high-old_val_low)+old_val_low;
    }

};


class BTNRH_Stereo_Preset {
  public:
    BTNRH_Stereo_Preset(){};
    BTNRH_Stereo_Preset(const BTNRH_WDRC::CHA_DSL &dsl_L, const BTNRH_WDRC::CHA_DSL &dsl_R, const BTNRH_WDRC::CHA_WDRC &bb_L, const BTNRH_WDRC::CHA_WDRC &bb_R, const int filt_order) {
      specifyPreset(dsl_L, dsl_R, bb_L, bb_R, filt_order);
    }

    //specify this preset using another preset
    void specifyPreset(const BTNRH_Stereo_Preset &preset) {
      specifyPreset(preset.preset_LR[0].wdrc_perBand,  preset.preset_LR[1].wdrc_perBand,
                    preset.preset_LR[0].wdrc_broadband,preset.preset_LR[1].wdrc_broadband,
                    preset.preset_LR[0].n_filter_order);
    }

    //specify this preset using individual BTNRH data types
    void specifyPreset(const BTNRH_WDRC::CHA_DSL &dsl_L, const BTNRH_WDRC::CHA_DSL &dsl_R, const BTNRH_WDRC::CHA_WDRC &bb_L, const BTNRH_WDRC::CHA_WDRC &bb_R, const int n_order) {
      for (int I_LR=0; I_LR < n_LR; I_LR++) {  //loop over left-right
        if (I_LR == 0) {
          preset_LR[I_LR].specifyPreset(dsl_L, bb_L, n_order); 
        } else {
          preset_LR[I_LR].specifyPreset(dsl_R, bb_R, n_order); 
        }
        
        preset_LR[I_LR].left_or_right = I_LR;  
        preset_LR[I_LR].suffix_var_name = LR_suffix_var_names[I_LR];
      }
    }
    
    //used for making new presets
    virtual int interpolateSettingsGivenFrequencies(const BTNRH_Stereo_Preset &prevPreset, int new_n_chan, float *new_cross_Hz) {
      int ret_val; int any_error = 0;
      for (int I_LR=0; I_LR < prevPreset.n_LR; I_LR++) { //loop over left-right
        ret_val = preset_LR[I_LR].interpolateSettingsGivenFrequencies(prevPreset.preset_LR[I_LR], new_n_chan, new_cross_Hz);
        if (ret_val < 0) {
          any_error = ret_val;
          Serial.println("BTNRH_Stereo_Preset: interpolateSettingsGivenFrequencies (stereo): failed for (L=0)/(R=1): " + String(I_LR));
        }
      }
      return any_error; 
    }

    //data members for the algorithms that this preset works for
    #define PRESET_N_LR 2
    const int n_LR = PRESET_N_LR; //will be set to 2...for left and right
    BTNRH_Preset preset_LR[PRESET_N_LR];
    
    //used for saving and retrieving these presets from the SD card
    String name = "Preset";
    const String LR_suffix_var_names[2] = {String("_left"), String("_right")};
   
};


#define N_PRESETS 2
class BTNRH_StereoPresetManager_UI : public PresetManager_UI {   //most of the App GUI functionality is in PresetManager_UI.h in the Tympan_Library
  public:
    BTNRH_StereoPresetManager_UI() : PresetManager_UI(N_PRESETS) {
      resetAllPresetsToFactory(); //initialize stuff, but do not load from SD (yet)
    };

    //note: n_presets and getPresetInd() is in PresetManagerBase
    BTNRH_Stereo_Preset presets[N_PRESETS];
    const String preset_fnames[N_PRESETS] = {String("Preset_16_00.txt"), String("Preset_16_01.txt")};  //filenames for reading off SD      

    //attach the underlying algorithms that will be manipulated by changing the preset
    virtual void attachAlgorithms(AudioEffectMultiBandWDRC_Base_F32_UI *left, AudioEffectMultiBandWDRC_Base_F32_UI *right) { leftWDRC = left; rightWDRC = right; };

    //methods to implement that are required (or nearly required) by PresetManagerBase and PresetManager_UI (see PresetManager_UI.h in Tympan_Library)
    virtual int setToPreset(int i, bool update_gui = false);
    virtual int switchToPreset(int i, bool update_gui = false);
    virtual String getPresetName(int i) { if ((i<0) || (i >=n_presets)) { return String("Preset"); } else { return presets[i].name; } };
 
    virtual int savePresetToSD(int i, bool rebuild_from_algs=true);  //required to be defined by PresetManager_UI.h (because it was a pure virtual function)
    virtual int readPresetFromSD(int i, bool update_algs=false, bool update_gui = false);     //required by PresetManager_UI.h
    virtual int resetPresetToFactory(int i, bool update_algs=false, bool update_gui = false); //required by PresetManager_UI.h
     
    virtual void resetAllPresetsToFactory(void) { for (int i=0; i<N_PRESETS;i++) resetPresetToFactory(i); }
    virtual void readAllPresetsFromSD(void) { for (int i=0; i<N_PRESETS;i++) readPresetFromSD(i); }
    virtual void rebuildPresetFromSources(int i);

    //change the underlying algorithms
    virtual int incrementNumWDRCChannels(int increment_fac, bool update_gui=false) { 
      return setNumWDRCChannels(leftWDRC->get_n_chan() + increment_fac, update_gui);   //assume left and right are the same number of WDRC channels
    }
    virtual int setNumWDRCChannels(int new_n_chan, bool update_gui=false);             //assume left and right are the same number of WDRC channels
    virtual int getNumWDRCChannels(void) { return leftWDRC->get_n_chan(); }  //assume left and right are the same number of WDRC channels
    virtual int setMaxAllowedWDRCChannels(int val) { if ((val >= 1) && (val < 50)) { max_allowed_n_chan = val; } return getMaxAllowedWDRCChannels();};
    virtual int getMaxAllowedWDRCChannels(void) { return max_allowed_n_chan; };
    //virtual int interpolateNewPreset(const BTNRH_Stereo_Preset *prevPreset, BTNRH_Stereo_Preset *newPreset, int new_n_chan);

    //adjust the presets for info not contained in the BTNRH data types
    virtual int setFilterOrder(const int i, const int n_filter_order) {  
      if ((i>=0) && (i < n_presets)) { 
        presets[i].preset_LR[0].n_filter_order = presets[i].preset_LR[1].n_filter_order = n_filter_order;
      }
      return getFilterOrder(i); 
    }
    virtual int getFilterOrder(const int i) { if ((i<0) || (i >=n_presets)) { return -1; } return presets[i].preset_LR[0].n_filter_order; }

    // ///////////////////////////// Methods required (or encouraged) for SerialManager_UI classes
    virtual void printHelp(void);
    //virtual bool processCharacter(char c); //not used here
    virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
    virtual void setFullGUIState(bool activeButtonsOnly = false);
  
    // ///////////////////////////// Methods related to managing the App GUI (expanding upon those in PresetManager_UI
    virtual TR_Card* addCard_incrementWDRCChannels(TR_Page *page_h);
    
    //methods to update the GUI fields
    bool flag_send_WDRCChannels = false; 
    void updateCard_incrementWDRCChannels(bool activeButtonsOnly = false);
    void setIncrementChannelsMessage(String);

  protected:
    AudioEffectMultiBandWDRC_Base_F32_UI *leftWDRC=NULL, *rightWDRC=NULL;
    int max_allowed_n_chan = INIT_MAX_ALLOWED_N_CHAN;
};

int BTNRH_StereoPresetManager_UI::setToPreset(int Ipreset, bool update_gui) {
  if ((Ipreset < 0) || (Ipreset >= n_presets)) return Ipreset;  //out of bounds!
  
  //push the left and right presets to the left and right algorithms
  int i_left=0, i_right = 1; 
  
  Serial.println("BTNRH_PresetManager: setToPreset: setting to preset " + String(Ipreset) + ", left channel...");
  leftWDRC->setupFromBTNRH(  presets[Ipreset].preset_LR[i_left].wdrc_perBand,  
                             presets[Ipreset].preset_LR[i_left].wdrc_broadband,  
                             presets[Ipreset].preset_LR[i_left].n_filter_order); 
                             
  Serial.println("BTNRH_PresetManager: setToPreset: setting to preset " + String(Ipreset) + ", right channel...");
  rightWDRC->setupFromBTNRH( presets[Ipreset].preset_LR[i_right].wdrc_perBand, 
                             presets[Ipreset].preset_LR[i_right].wdrc_broadband, 
                             presets[Ipreset].preset_LR[i_right].n_filter_order); 

  //set state
  cur_preset_ind = Ipreset;

  //update GUI
  if (update_gui) { if (sm != NULL) sm->setFullGUIState(); }

  return cur_preset_ind;
}


int BTNRH_StereoPresetManager_UI::switchToPreset(int Ipreset, bool update_gui) {
  if ((Ipreset < 0) || (Ipreset >= n_presets)) return Ipreset;  //out of bounds!

  //Gather up the settings back into the current preset as the settings could have been changed by the user away
  //from the preset values.  This will allow the current settings to be brought back if the user switches back to
  //the current preset.
  rebuildPresetFromSources(cur_preset_ind); //allows us to not lose any changes that we made to the current preset

  //now set the new preset
  return setToPreset(Ipreset, update_gui);
}


// Save the current preset to the presets[] array and (maybe) writes to SD  
int BTNRH_StereoPresetManager_UI::savePresetToSD(int ind_preset, bool rebuild_preset_from_algs) {
  int i = ind_preset;
  if ( (i<0) || (i>=n_presets) ) return -1; //out of bounds!

  if (rebuild_preset_from_algs) {
    //rebuild the preset based on what the algorithms are acutally holding for parameter values
    rebuildPresetFromSources(i);  //pulls the values into leftWDRC and rightWDRC and stores in the current preset
  }
  
  //now write preset to the SD card, if requested
  String fname = preset_fnames[i];
  bool start_new_file = true;
  for (int I_LR=0; I_LR < 2; I_LR++) {
    BTNRH_Preset *preset = &(presets[i].preset_LR[I_LR]); //simpler name
    preset->saveToSD(fname,start_new_file);  //save to the SD card
    start_new_file= false; //don't start a new file for future channels...everything else will be appended
  }  
  return 0; //0 is OK
}


int BTNRH_StereoPresetManager_UI::readPresetFromSD(int Ipreset, bool update_algs, bool update_gui) { //returns 0 if good, returns -1 if fail
  //first, initialize to the hardwired default values
  int i = Ipreset;
  resetPresetToFactory(i, false, false); //don't update the algorithms

  bool printDebug = true;
  
  // Try to read from the SD card
  bool any_fail = false;
  String fname = preset_fnames[i];
  for (int I_LR=0; I_LR < presets[i].n_LR; I_LR++) {  //loop over left and right
    BTNRH_Preset *preset = &(presets[i].preset_LR[I_LR]); //simpler name
    
    //if (printDebug) Serial.println(F("BTNRH_StereoPresetManager_UI: readPresetFromSD: reading preset, per-band, ") + String(i) + ", " +  presets[i].LR_names[I_LR]);
    if (printDebug) Serial.println(F("BTNRH_StereoPresetManager_UI: readPresetFromSD: reading preset, per-band, ") + String(i) + ", " +  preset->suffix_var_name);
    if ( ((preset->wdrc_perBand).readFromSD(fname, preset->suffix_var_name)) == 0 ) { //zero is success
      if (printDebug) { preset->wdrc_perBand.printAllValues(); delay(10); }
    
      if (printDebug) Serial.println(F("BTNRH_StereoPresetManager_UI: readPresetFromSD: reading preset, broadband, ") + String(i) + ", " +  preset->suffix_var_name);
      if ( ((preset->wdrc_broadband).readFromSD(fname, preset->suffix_var_name)) == 0 ) { //zero is success
        if (printDebug) { preset->wdrc_broadband.printAllValues(); delay(10); }
        
        //if ((presets[i].afc.readFromSD(fname, presets[i].LR_names[i])) == 0) { //zero is success
          //anything more to do?  any more algorithm settings to load?
        //} else {
        // any_fail = true;
        //}
      
      } else {
        Serial.println(F("PresetManager: readPresetFromSD: *** WARNING *** could not read all preset elements from ") + String(fname));
        Serial.println(F("    : Failed reading wdrc_perBand for left(0)/right(1):") + String(I_LR) + " using left/right name: " + String(preset->suffix_var_name));
        any_fail = true;
      }
      
    } else {
      Serial.println(F("PresetManager: readPresetFromSD: *** WARNING *** could not read all preset elements from ") + String(fname));
      Serial.println(F("    : Failed reading wdrc_broadband for left(0)/right(1):") + String(I_LR) + " using left/right name: " + String(preset->suffix_var_name));
      any_fail = true;
    }
    
  } //end loop over left and right

    
  if (any_fail) {
    //print error messages
    Serial.println(F("PresetManager: readPresetFromSD: *** WARNING *** could not read all preset elements from ") + String(fname));
    Serial.println(F("    : Using built-in algorithm preset values instead."));

    //reset to the hardwired default
    resetPresetToFactory(i,false, false);
    if (update_algs) { if (Ipreset == cur_preset_ind) { setToPreset(Ipreset); } else { switchToPreset(Ipreset); } }
    if (update_gui) { if (sm != NULL) sm->setFullGUIState(); }
    return -1;
  }

  if (update_algs) { if (Ipreset == cur_preset_ind) { setToPreset(Ipreset); } else { switchToPreset(Ipreset); } }
  if (update_gui) { if (sm != NULL) sm->setFullGUIState(); }
  
  return 0;  //0 is OK
}    


int BTNRH_StereoPresetManager_UI::resetPresetToFactory(int Ipreset, bool update_algs, bool update_gui) { //sets both left and right
  if ((Ipreset < 0) || (Ipreset >= n_presets)) return -1;  //out of bounds!

  Serial.println(F("BTNRH_StereoPresetManager_UI: resetPresetToFactory: Ipreset = ") + String(Ipreset));
  
  //choose the one for the given preset
  switch (Ipreset) {
    case 0:
      {
        #include "Preset_16_00.h" //the hardwired settings here in this sketch's directory          
        presets[Ipreset].name = String("Preset A");       
        presets[Ipreset].specifyPreset(dsl_left, dsl_right, bb_left, bb_right, presets[Ipreset].preset_LR[0].n_filter_order); //dsl_left, dsl_right, bb_left, and bb_right are from the *.h file
        break;
      }
    case 1:
      {
        #include "Preset_16_01.h" //the hardwired settings here in this sketch's directory            
        presets[Ipreset].name = String("Preset B");
        presets[Ipreset].specifyPreset(dsl_left, dsl_right, bb_left, bb_right, presets[Ipreset].preset_LR[0].n_filter_order); //dsl_left, dsl_right, bb_left, and bb_right are from the *.h file
        break;
      }        
  }

  if (update_algs) { if (Ipreset == cur_preset_ind) { setToPreset(Ipreset); } else { switchToPreset(Ipreset); } }
  if (update_gui) { if (sm != NULL) sm->setFullGUIState(); } 

  return 0; //0 is OK
}

void BTNRH_StereoPresetManager_UI::rebuildPresetFromSources(int Ipreset) {
  int i = Ipreset;
  if ( (i<0) || (i>=n_presets) ) return; //out of bounds!
  const int left_ind = 0, right_ind = 1;

  Serial.println("BTNRH_StereoPresetManager_UI: rebuildPresetFromSources: rebuilding preset " + String(Ipreset) + " from underlying algorithms.");

  //reconstruct all components of the preset
  if (leftWDRC != NULL) {
    leftWDRC->getDSL(   &(presets[i].preset_LR[left_ind].wdrc_perBand  ));
    leftWDRC->getWDRC(  &(presets[i].preset_LR[left_ind].wdrc_broadband));
    //leftWDRC->getAFC( &(presets[i].preset_LR[left_ind].afc           ));
  }
  if (rightWDRC != NULL) {
    rightWDRC->getDSL(  &(presets[i].preset_LR[right_ind].wdrc_perBand   ));
    rightWDRC->getWDRC( &(presets[i].preset_LR[right_ind].wdrc_broadband));
    //rightWDRC->getAFC(&(presets[i].preset_LR[right_ind].afc            ));     
  }

}

int BTNRH_StereoPresetManager_UI::setNumWDRCChannels(int new_n_chan, bool update_gui) {
  int orig_n_chan = leftWDRC->get_n_chan(), orig_n_crossover = orig_n_chan-1;
  if ((new_n_chan < 3) || (new_n_chan > max_allowed_n_chan)) {
    Serial.println("BTNRH_StereoPresetManager_UI: setNumWDRCChannels: requested value of " + String(new_n_chan) + " must be 3 <= val <= " + String(max_allowed_n_chan)); 
    return orig_n_chan; //do not change
  }

  //Gather up the settings back into the current preset as the settings could have been changed by the user away
  //from the preset values.
  rebuildPresetFromSources(cur_preset_ind); //allows us to not lose any changes that we made to the current preset

  //copy the old preset into a temporary holder
  BTNRH_Stereo_Preset prevPreset = BTNRH_Stereo_Preset(presets[cur_preset_ind]);
  
  //compute the new target frequencies (log-spaced)
  float *orig_freq_Hz = &((prevPreset.preset_LR[0].wdrc_perBand).cross_freq[0]);
  int new_n_crossover = new_n_chan-1;
  float scale_fac = expf(logf(orig_freq_Hz[orig_n_crossover-1]/orig_freq_Hz[0]) / ((float)(new_n_crossover-1)));
  float new_freq_Hz[new_n_crossover];
  new_freq_Hz[0] = orig_freq_Hz[0]; //start at the same frequency
  for (int i=1; i<new_n_crossover; i++) new_freq_Hz[i] = new_freq_Hz[i-1]*scale_fac; //calc the new frequency values

  #if 1
    Serial.print("BTNRH_PresetManager_UI: setNumWDRCChannels: orig freqs = ");
    for (int i=0; i<orig_n_crossover; i++) Serial.print(String(orig_freq_Hz[i]) + ", ");
    Serial.println();delay(10);

    Serial.print("BTNRH_PresetManager_UI: setNumWDRCChannels: new freqs = ");
    for (int i=0; i<new_n_crossover; i++) Serial.print(String(new_freq_Hz[i]) + ", ");
    Serial.println();delay(10);
  #endif

  //build the new preset by interpolating based on freuqency
  presets[cur_preset_ind].interpolateSettingsGivenFrequencies(prevPreset,new_n_chan,new_freq_Hz);

  #if 1
    Serial.println("BTNRH_PresetManager_UI: setNumWDRCChannels: interpolated preset, left:");
    presets[cur_preset_ind].preset_LR[0].wdrc_perBand.printAllValues();delay(10);
    Serial.println("BTNRH_PresetManager_UI: setNumWDRCChannels: interpolated preset, right:");
    presets[cur_preset_ind].preset_LR[1].wdrc_perBand.printAllValues();delay(10);
  #endif

  //set the new preset as the active preset
  setToPreset(cur_preset_ind, update_gui);

  return getNumWDRCChannels();
}


// ////////////////////////////////////////////////////////////////// GUI Methods


void BTNRH_StereoPresetManager_UI::printHelp(void) {
  PresetManager_UI::printHelp();
  Serial.println("   : c/C: incr/decrease number of WDRC channels (cur = " + String(getNumWDRCChannels()) + ")");
}

bool BTNRH_StereoPresetManager_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
  //check the mode_char to see if it corresponds with this instance of this class.  If not, return with no action.
  if (mode_char != ID_char) return false;  //ID_char is from SerialManager_UI.h

  bool return_val = PresetManager_UI::processCharacterTriple(mode_char, chan_char, data_char);
  if (return_val == true) return return_val;

  return_val = true;
  bool flag_fail;
  switch (data_char) {
    case 'c':
        Serial.println("BTNRH_StereoPresetManager_UI: increasing WDRC channels..");
        setIncrementChannelsMessage("Increasing...");
        flag_fail = incrementNumWDRCChannels(+1, true);  //the "true" says to update the GUI
        delay(10); if (flag_fail < 0) { setIncrementChannelsMessage("Failed"); } else { setIncrementChannelsMessage("Complete");}
        break;
      case 'C':
        Serial.println("BTNRH_StereoPresetManager_UI: decreasing WDRC channels...");
        setIncrementChannelsMessage("Decreasing...");
        flag_fail = incrementNumWDRCChannels(-1, true);  //the "true" says to update the GUI
        delay(10); if (flag_fail < 0) { setIncrementChannelsMessage("Failed"); } else { setIncrementChannelsMessage("Complete");}
        break;
      default:
        return_val = false;
  }
  return return_val;
}
  
void BTNRH_StereoPresetManager_UI::setFullGUIState(bool activeButtonsOnly) {
  PresetManager_UI::setFullGUIState(activeButtonsOnly);

  if (flag_send_WDRCChannels) updateCard_incrementWDRCChannels(activeButtonsOnly);
}


TR_Card* BTNRH_StereoPresetManager_UI::addCard_incrementWDRCChannels(TR_Page *page_h) {
  flag_send_WDRCChannels = true;

  //this method is in the parent class: SerialManager_UI.cpp
  TR_Card* card_h = addCardPreset_UpDown(page_h, "WDRC Channels",  "nChan", 'C', 'c');

  //add a status display
  String ID_fn = String(ID_char_fn);
  card_h->addButton("", "", ID_fn+"chStat",12);

  return card_h;
}

void BTNRH_StereoPresetManager_UI::updateCard_incrementWDRCChannels(bool activeButtonsOnly) {
  updateCardPreset_UpDown("nChan",String(getNumWDRCChannels())); //this method is in SerialManager_UI.h  
  setIncrementChannelsMessage(String(""));
}

void BTNRH_StereoPresetManager_UI::setIncrementChannelsMessage(String s) {
  String ID_fn = String(ID_char_fn); //for use with button names
  String but_id = ID_fn+"chStat";
  setButtonText(but_id,s);
}

#endif
