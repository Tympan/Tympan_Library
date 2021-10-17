
#ifndef _BTNRH_Stereo_Preset_h
#define _BTNRH_Stereo_Preset_h

#include <BTNRH_WDRC_Types.h> //from Tympan_Library
#include <PresetManager_UI.h> //from Tympan_Library

#define DEFAULT_FILTER_ORDER 96  //assume FIR (96) or IIR (6)

class BTNRH_Stereo_Preset {
  public:
    BTNRH_Stereo_Preset(){};
    BTNRH_Stereo_Preset(const BTNRH_WDRC::CHA_DSL &dsl_L, const BTNRH_WDRC::CHA_DSL &dsl_R, const BTNRH_WDRC::CHA_WDRC &bb_L, const BTNRH_WDRC::CHA_WDRC &bb_R, const int filt_order) {
      specifyPreset(dsl_L, dsl_R, bb_L, bb_R, filt_order);
    }

    //specify this preset using another preset
    void specifyPreset(const BTNRH_Stereo_Preset &preset) {
      specifyPreset(preset.wdrc_perBand[0],preset.wdrc_perBand[1],preset.wdrc_broadband[0],preset.wdrc_broadband[1],preset.n_filter_order);
    }

    //specify this preset using individual BTNRH data types
    void specifyPreset(const BTNRH_WDRC::CHA_DSL &dsl_L, const BTNRH_WDRC::CHA_DSL &dsl_R, const BTNRH_WDRC::CHA_WDRC &bb_L, const BTNRH_WDRC::CHA_WDRC &bb_R, const int n_order) {
      if (n_order < 1) {
        Serial.println("BTNRH_Stereo_Preset: *** WARNING ***: setting filter order to " + String(n_order) + " which should be >= 1"); 
      }
      wdrc_perBand[0] = CHA_DSL_SD(dsl_L);   //left
      wdrc_perBand[1] = CHA_DSL_SD(dsl_R);   //right
      wdrc_broadband[0] = CHA_WDRC_SD(bb_L); //left
      wdrc_broadband[1] = CHA_WDRC_SD(bb_R); //right
      n_filter_order = n_order; //assumed same for left and right
    }

    //data members for the algorithms that this preset works for
    #define PRESET_N_LR 2
    const int n_LR = PRESET_N_LR; //will be set to 2...for left and right
    int n_filter_order = DEFAULT_FILTER_ORDER; //assumed same for left and right, default (96 = FIR, 6 = IIR)...can be changed simply by changing n_filter_order in your code
    CHA_DSL_SD wdrc_perBand[PRESET_N_LR];    //left and right
    CHA_WDRC_SD wdrc_broadband[PRESET_N_LR]; //left and right
    //CHA_AFC_SD afc[PRESET_N_LR]; //left and right
    
    //used for saving and retrieving these presets from the SD card
    String name = "Preset";
    const int n_datatypes = 2; //there is dsl and bb (eventually there will be afc)
    const String var_names[2] = {String("dsl"), String("bb")}; //use for reading/writing to SD
    const String LR_names[2] = {String("_left"), String("_right")};
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

    //adjust the presets for info not contained in the BTNRH data types
    virtual int setFilterOrder(const int i, const int n_filter_order) {  if ((i>=0) && (i < n_presets)) { presets[i].n_filter_order = n_filter_order;} return getFilterOrder(i); }
    virtual int getFilterOrder(const int i) { if ((i<0) || (i >=n_presets)) { return -1; } return presets[i].n_filter_order; }
  

  protected:
    AudioEffectMultiBandWDRC_Base_F32_UI *leftWDRC=NULL, *rightWDRC=NULL;
};

int BTNRH_StereoPresetManager_UI::setToPreset(int Ipreset, bool update_gui) {
  if ((Ipreset < 0) || (Ipreset >= n_presets)) return Ipreset;  //out of bounds!
  
  //push the left and right presets to the left and right algorithms
  int i_left=0, i_right = 1; 
  Serial.println("BTNRH_PresetManager: setToPreset: setting to preset " + String(Ipreset) + ", left channel...");
  leftWDRC->setupFromBTNRH(  presets[Ipreset].wdrc_perBand[i_left],  presets[Ipreset].wdrc_broadband[i_left],  presets[Ipreset].n_filter_order); 
  Serial.println("BTNRH_PresetManager: setToPreset: setting to preset " + String(Ipreset) + ", right channel...");
  rightWDRC->setupFromBTNRH( presets[Ipreset].wdrc_perBand[i_right], presets[Ipreset].wdrc_broadband[i_right], presets[Ipreset].n_filter_order); 

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
    Serial.println("BTNRH_StereoPresetManager_UI::savePresetToSD: writing preset " + String(i) + ", " + presets[i].var_names[0] + presets[i].LR_names[I_LR]);
    presets[i].wdrc_perBand[I_LR].printToSD(fname,presets[i].var_names[0]+presets[i].LR_names[I_LR], start_new_file);  //the "true" says to start from a fresh file
    start_new_file= false; //don't start a new file for future channels...everything else will be appended

    Serial.println("BTNRH_StereoPresetManager_UI::savePresetToSD: writing preset " + String(i) + ", " + presets[i].var_names[1] + presets[i].LR_names[I_LR]);
    presets[i].wdrc_broadband[I_LR].printToSD(fname,presets[i].var_names[1]+presets[i].LR_names[I_LR]);  //append to the file
    //presets[i].afc.printToSD(fname,presets[i].var_names[2]+presets[i].LR_names[I_LR]);  //append to the file
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
    
    if (printDebug) Serial.println(F("BTNRH_StereoPresetManager_UI: readPresetFromSD: reading preset, per-band, ") + String(i) + ", " +  presets[i].LR_names[I_LR]);
    if ( ((presets[i].wdrc_perBand[I_LR]).readFromSD(fname, presets[i].LR_names[I_LR])) == 0 ) { //zero is success
      if (printDebug) presets[i].wdrc_perBand[I_LR].printAllValues();
    
      if (printDebug) Serial.println(F("BTNRH_StereoPresetManager_UI: readPresetFromSD: reading preset, broadband, ") + String(i) + ", " +  presets[i].LR_names[I_LR]);
      if ( ((presets[i].wdrc_broadband[I_LR]).readFromSD(fname, presets[i].LR_names[I_LR])) == 0 ) { //zero is success
        if (printDebug) presets[i].wdrc_broadband[I_LR].printAllValues();
        
        //if ((presets[i].afc.readFromSD(fname, presets[i].LR_names[i])) == 0) { //zero is success
          //anything more to do?  any more algorithm settings to load?
        //} else {
        // any_fail = true;
        //}
      
      } else {
        Serial.println(F("PresetManager: readPresetFromSD: *** WARNING *** could not read all preset elements from ") + String(fname));
        Serial.println(F("    : Failed reading wdrc_perBand for left(0)/right(1):") + String(I_LR) + " using left/right name: " + String(presets[i].LR_names[I_LR]));
        any_fail = true;
      }
      
    } else {
      Serial.println(F("PresetManager: readPresetFromSD: *** WARNING *** could not read all preset elements from ") + String(fname));
      Serial.println(F("    : Failed reading wdrc_broadband for left(0)/right(1):") + String(I_LR) + " using left/right name: " + String(presets[i].LR_names[I_LR]));
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
        presets[Ipreset].specifyPreset(dsl_left, dsl_right, bb_left, bb_right, presets[Ipreset].n_filter_order); //dsl_left, dsl_right, bb_left, and bb_right are from the *.h file
        break;
      }
    case 1:
      {
        #include "Preset_16_01.h" //the hardwired settings here in this sketch's directory            
        presets[Ipreset].name = String("Preset B");
        presets[Ipreset].specifyPreset(dsl_left, dsl_right, bb_left, bb_right, presets[Ipreset].n_filter_order); //dsl_left, dsl_right, bb_left, and bb_right are from the *.h file
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
    leftWDRC->getDSL(   &(presets[i].wdrc_perBand[left_ind]   ));
    leftWDRC->getWDRC(  &(presets[i].wdrc_broadband[left_ind] ));
    //leftWDRC->getAFC( &(presets[i].afc[left_ind]            ));
  }
  if (rightWDRC != NULL) {
    rightWDRC->getDSL(  &(presets[i].wdrc_perBand[right_ind]   ));
    rightWDRC->getWDRC( &(presets[i].wdrc_broadband[right_ind] ));
    //rightWDRC->getAFC(&(presets[i].afc[right_ind]            ));     
  }

}


#endif
