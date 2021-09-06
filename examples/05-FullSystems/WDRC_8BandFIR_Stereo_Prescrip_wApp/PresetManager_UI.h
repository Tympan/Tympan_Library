
#ifndef _PresetManager_UI_h
#define _PresetManager_UI_h

#include <BTNRH_WDRC_Types.h> //from Tympan_Library
//include <vector>


class BTNRH_Preset {
  public:
    BTNRH_Preset(){};
    BTNRH_Preset(const BTNRH_WDRC::CHA_DSL &dsl_L, const BTNRH_WDRC::CHA_DSL dsl_R, const BTNRH_WDRC::CHA_WDRC &bb_L, const BTNRH_WDRC::CHA_WDRC &bb_R) {
      specifyPreset(dsl_L, dsl_R, bb_L, bb_R);
    }
    void specifyPreset(const BTNRH_Preset &preset) {
      specifyPreset(preset.wdrc_perBand[0],preset.wdrc_perBand[1],preset.wdrc_broadband[0],preset.wdrc_broadband[1]);
    }
    void specifyPreset(const BTNRH_WDRC::CHA_DSL &dsl_L, const BTNRH_WDRC::CHA_DSL dsl_R, const BTNRH_WDRC::CHA_WDRC &bb_L, const BTNRH_WDRC::CHA_WDRC &bb_R) {
      wdrc_perBand[0] = CHA_DSL_SD(dsl_L);   //left
      wdrc_perBand[1] = CHA_DSL_SD(dsl_R);   //right
      wdrc_broadband[0] = CHA_WDRC_SD(bb_L); //left
      wdrc_broadband[1] = CHA_WDRC_SD(bb_R); //right
    }

    const int n_chan = 2; //will be set to 2...for left and right
    CHA_DSL_SD wdrc_perBand[2]; //left and right
    CHA_WDRC_SD wdrc_broadband[2]; //left and right
    //CHA_AFC_SD afc[2]; //left and right
    
    //used for saving and retrieving these presets from the SD card
    const int n_datatypes = 2; //there is dsl and bb (eventually there will be afc)
    const String var_names[2] = {String("dsl"), String("bb")}; //use for reading/writing to SD
    const String chan_names[2] = {String("_left"), String("_right")};
};

class PresetManager_UI : public SerialManager_UI {
  public: 
    PresetManager_UI() : SerialManager_UI() {};

    //bool addPreset(AlgPresetBase *preset) { presets.push_back(preset); }
    //AlgPresetBase* getPreset(int ind) { if (ind < (int)presets.size()) { return presets[ind]; } return NULL; }
      
    // here are the methods required (or encouraged) for SerialManager_UI classes
    //virtual void printHelp(void) {};
    //virtual bool processCharacter(char c); //not used here
    //virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char) {return false; };
    //virtual void setFullGUIState(bool activeButtonsOnly = false) {} ; //if commented out, use the one in StereoContrainer_UI.h

  protected:
    //std::vector<AlgPresetBase> presets;
};

#define N_PRESETS 2
class BTNRH_PresetManager_UI : public PresetManager_UI {
  public:
    BTNRH_PresetManager_UI() : PresetManager_UI() {
      initAllPresetsToDefault(); //initialize stuff, but do not load from SD (yet)
    };

    BTNRH_Preset presets[N_PRESETS];
    const String preset_fnames[N_PRESETS] = {String("Preset_00.txt"), String("Preset_01.txt")};  //filenames for reading off SD
    const int n_presets = N_PRESETS;

    void initAllPresetsToDefault(void ){ for (int i=0; i<N_PRESETS;i++) initPresetToDefault(i); }
    void initPresetToDefault(int Ipreset) { //sets both left and right
      if ((Ipreset < 0) || (Ipreset >= N_PRESETS)) return;  //out of bounds!
      
      //choose the one for the given preset
      switch (Ipreset) {
        case 0:
          {
            #include "Preset_00.h" //the hardwired settings
            presets[Ipreset].specifyPreset(dsl_left, dsl_right, bb_left, bb_right);
            break;
          }
        case 1:
          {
            #include "Preset_01.h" //the hardwired settings
            presets[Ipreset].specifyPreset(dsl_left, dsl_right, bb_left, bb_right);
            break;
          }        
      }
    }

    void loadAllPresetsFromSD(void) { for (int i=0; i<N_PRESETS;i++) loadPresetFromSD(i); }
    void loadPresetFromSD(int Ipreset) {
      //first, initialize to the hardwired default values
      int i = Ipreset;
      initPresetToDefault(i);
      
      // Try to read from the SD card
      bool any_fail = false;
      String fname = preset_fnames[i];
      for (int Ichan=0; Ichan < N_CHAN; Ichan++) {  //loop over left and right
        if ( ((presets[i].wdrc_perBand[Ichan]).readFromSD(fname, presets[i].chan_names[Ichan])) == 0 ) { //zero is success
          if ( ((presets[i].wdrc_broadband[Ichan]).readFromSD(fname, presets[i].chan_names[Ichan])) == 0 ) { //zero is success
            //if ((presets[i].afc.readFromSD(fname, presets[i].chan_names[i])) == 0) { //zero is success
              //anything more to do?  any more algorithm settings to load?
            //} else {
            // any_fail = true;
            //}
          } else {
            any_fail = true;
          }
        } else {
          any_fail = true;
        }
      } //end loop over left and right

        
      if (any_fail) {
        //print error messages
        Serial.print("PresetManager: specifyPresetAsDefault: *** WARNING *** could not read all preset elements from "); Serial.print(fname);
        Serial.println("    : Using built-in algorithm preset values instead.");

        //reset to the hardwired default
        initPresetToDefault(i);
      }
    }    

    // Save the current preset to the presets[] array and (maybe) writes to SD
    //void saveCurrentAlgPresetToSD(bool writeToSD = false) { //saves the current preset to the presets[] array and (maybe) writes to SD
    void savePresetToSD(int ind_preset, BTNRH_WDRC::CHA_DSL dsl_L, BTNRH_WDRC::CHA_DSL dsl_R, BTNRH_WDRC::CHA_WDRC bb_L, BTNRH_WDRC::CHA_WDRC bb_R, bool writeToSD = false) { 
      BTNRH_Preset foo_preset(dsl_L, dsl_R, bb_L, bb_R);
      savePresetToSD(ind_preset,foo_preset,writeToSD);
    }
    void savePresetToSD(int ind_preset, const BTNRH_Preset &foo_preset, bool writeToSD = false) {
     int i = ind_preset;
     if ( (i<0) || (i>=n_presets) ) return; //out of bounds!
         
      //save current settings to the preset that's in RAM (not yet on the SD)
      presets[i].specifyPreset(foo_preset);

      //now write preset to the SD card, if requested
      if (writeToSD) {
        String fname = preset_fnames[i];
        for (int Ichan=0; Ichan < N_CHAN; Ichan++) {
          presets[i].wdrc_perBand[Ichan].printToSD(fname,presets[i].var_names[0]+presets[i].chan_names[Ichan], true);  //the "true" says to start from a fresh file
          presets[i].wdrc_broadband[Ichan].printToSD(fname,presets[i].var_names[1]+presets[i].chan_names[Ichan]);  //append to the file
          //presets[i].afc.printToSD(fname,presets[i].var_names[2]+presets[i].chan_names[Ichan]);  //append to the file
        }
      }
    }
    
//    void revertPresetToDefault(int ind_preset, bool writeToSD = false) { //saves the current preset to the presets[] array and (maybe) writes to SD
//      int i = ind_preset;
//      if ( (i<0) || (i>=n_presets) ) return; //out of bounds!
//
//      //load the factor defaults into the preset
//      specifyPreset(i, false);  //false => do NOT read from SD card.  Use the built-in settings.
// 
//
//      //save to SD card
//      if (writeToSD) {
//        Serial.println("State: revertCurrentAlgPresetToDefault: writing to SD.");
//        String fname = preset_fnames[i];
//        for (int Ichan=0; Ichan < N_CHAN; Ichan++) {
//          presets[i].wdrc_perBand[Ichan].printToSD(fname,presets[i].var_names[0]+presets[i].chan_names[Ichan], true);  //the "true" says to start from a fresh file
//          presets[i].wdrc_broadband[Ichan].printToSD(fname,presets[i].var_names[1]+presets[i].chan_names[Ichan]);  //append to the file
//          //presets[i].afc.printToSD(fname,var_names[2]+chan_names[Ichan]);  //append to the file
//        }
//      }
//    } 
};

#endif
