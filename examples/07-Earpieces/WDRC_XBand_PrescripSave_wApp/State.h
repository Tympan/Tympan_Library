
#ifndef State_h
#define State_h

#include <AudioEffectCompWDRC_F32.h>  //for CHA_DSL and CHA_WDRC data types


typedef struct {
  BTNRH_WDRC::CHA_DSL wdrc_perBand;
  BTNRH_WDRC::CHA_WDRC wdrc_broadband;
  BTNRH_WDRC::CHA_AFC afc;
} Alg_Preset;
#define N_PRESETS 3

// define structure for tracking state of system (for GUI)
extern "C" char* sbrk(int incr);
class State {
  public:
    //State(void) {};
    State(AudioSettings_F32 *given_settings, Print *given_serial) {
      local_audio_settings = given_settings; 
      local_serial = given_serial;
      defineAlgorithmPresets(); //don't read from SD, just use the hardwired
    }

    enum ANALOG_VS_PDM {INPUT_ANALOG, INPUT_PDM};
    enum ANALOG_INPUTS {INPUT_PCBMICS, INPUT_MICJACK_MIC, INPUT_LINEIN_SE, INPUT_MICJACK_LINEIN};
    //enum GAIN_TYPE {INPUT_GAIN_ID, DSL_GAIN_ID, GHA_GAIN_ID, OUTPUT_GAIN_ID};

    int input_analogVsPDM = INPUT_PDM;
    int input_analog_config = INPUT_PCBMICS;
   
    //int input_source = NO_STATE;
    float inputGain_dB = 0.0;  //gain on the analog inputs (ie, the AIC ignores this when in PDM mode, right?)
    //float vol_knob_gain_dB = 0.0;  //will be overwritten once the potentiometer is read

    //set the front-rear parameters
    enum FRONTREAR_CONFIG_TYPE {MIC_FRONT, MIC_REAR, MIC_BOTH_INPHASE, MIC_BOTH_INVERTED, MIC_BOTH_INVERTED_DELAYED};
    int input_frontrear_config = MIC_FRONT;
    int targetRearDelay_samps = 1;  //in samples
    int currentRearDelay_samps = 0; //in samples
    float rearMicGain_dB = 0.0;

    //set input streo/mono configuration
    enum INPUTMIX {INPUTMIX_STEREO, INPUTMIX_MONO, INPUTMIX_MUTE, INPUTMIX_BOTHLEFT, INPUTMIX_BOTHRIGHT};
    int input_mixer_config = INPUTMIX_BOTHLEFT;
    int input_mixer_nonmute_config = INPUTMIX_BOTHLEFT;
    
    //algorithm settings
    enum ALG_CONFIG {ALG_PRESET_A=0, ALG_PRESET_B, ALG_PRESET_C};
    int current_alg_config = ALG_PRESET_A; //default to whatever the first algorithm is
    BTNRH_WDRC::CHA_DSL wdrc_perBand;
    BTNRH_WDRC::CHA_WDRC wdrc_broadband;
    BTNRH_WDRC::CHA_AFC afc;
    Alg_Preset presets[N_PRESETS];

    char GUI_tuner_persistent_mode = 'g';
    int getNChan(void) { return wdrc_perBand.nchannel; };

    void printPerBandSettings(void) {   printPerBandSettings("myState: printing per-band settings:",wdrc_perBand);  }
    static void printPerBandSettings(String s, BTNRH_WDRC::CHA_DSL &this_dsl) {  Serial.print(s);  this_dsl.printAllValues(); } 
    void printBroadbandSettings(void) {   printBroadbandSettings("myState: printing broadband settings:",wdrc_broadband);  }
    static void printBroadbandSettings(String s, BTNRH_WDRC::CHA_WDRC &this_gha) {Serial.print(s); this_gha.printAllValues();  }      
    void printAFCSettings(void) {   printAFCSettings("myState: printing AFC settings:",afc);  }
    static void printAFCSettings(String s, BTNRH_WDRC::CHA_AFC &this_afc) {  Serial.print(s);  this_afc.printAllValues(); }

    //printing of CPU and memory status
    bool flag_printCPUandMemory = false;
    bool flag_printCPUtoGUI = false;
    void printCPUandMemory(unsigned long curTime_millis = 0, unsigned long updatePeriod_millis = 0) {
       static unsigned long lastUpdate_millis = 0;
      //has enough time passed to update everything?
      if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
      if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
        printCPUandMemoryMessage();
        lastUpdate_millis = curTime_millis; //we will use this value the next time around.
      }
    }
    int FreeRam() {
      char top; //this new variable is, in effect, the mem location of the edge of the heap
      return &top - reinterpret_cast<char*>(sbrk(0));
    }
    void printCPUandMemoryMessage(void) {
      local_serial->print("CPU Cur/Pk: ");
      local_serial->print(local_audio_settings->processorUsage(), 1);
      local_serial->print("%/");
      local_serial->print(local_audio_settings->processorUsageMax(), 1);
      local_serial->print("%, ");
      local_serial->print("MEM Cur/Pk: ");
      local_serial->print(AudioMemoryUsage_F32());
      local_serial->print("/");
      local_serial->print(AudioMemoryUsageMax_F32());
      local_serial->print(", FreeRAM(B) ");
      local_serial->print(FreeRam());
      local_serial->println();
    }
    float getCPUUsage(void) { return local_audio_settings->processorUsage(); }
  
//    void printGainSettings(void) {
//      local_serial->print("Gains (dB): ");
//      local_serial->print("In ");  local_serial->print(inputGain_dB,1);
//      local_serial->print(", UComm ");  local_serial->print(ucommGain_dB,1);
//      local_serial->print(", HearThru ");  local_serial->print(hearthruGain_dB,1);
//      local_serial->print(", Out "); local_serial->print(vol_knob_gain_dB,1);
//      local_serial->println();
//    }

    bool flag_printPlottableData = false;


    const char *preset_fnames[N_PRESETS] = {"GHA_Constants.txt", "GHA_FullOn.txt", "GHA_RTS.txt"};  //filenames for reading off SD
    const char *var_names[N_PRESETS*3] = {"dsl", "gha", "afc", "dsl_fullon", "gha_fullon", "afc_fullon", "dsl_rts", "gha_rts", "afc_rts"}; //use for writing to SD          
    void setPresetToDefault(int Ipreset) {
      //Define the hard-wired settings
      #include "GHA_Constants.h"  //this sets dsl and gha settings, which will be the defaults...includes regular settings, full-on gain, and RTS

      //choose the one for the given preset
      int i = Ipreset;
      switch (i) {
        case ALG_PRESET_A:
          presets[i].wdrc_perBand = dsl;  
          presets[i].wdrc_broadband = gha; 
          presets[i].afc = afc;
          break;
        case ALG_PRESET_B:
          presets[i].wdrc_perBand = dsl_fullon;  
          presets[i].wdrc_broadband = gha_fullon; 
          presets[i].afc = afc_fullon;
          break;
        case ALG_PRESET_C:
          presets[i].wdrc_perBand = dsl_rts;  
          presets[i].wdrc_broadband = gha_rts; 
          presets[i].afc = afc_rts;
          break;          
      }
    }
    void defineAlgorithmPresets(bool loadFromSD = false){ for (int i=0; i<N_PRESETS;i++) defineAlgorithmPreset(i,loadFromSD); }
    void defineAlgorithmPreset(int Ipreset, bool loadFromSD = false) {

      int i = Ipreset;
      setPresetToDefault(i);
      
      //if we want to read from the SD card, try to read from the SD card
      bool is_SD_success = false;
      const char *fname = preset_fnames[i];
      if (loadFromSD) {
        if ((presets[i].wdrc_perBand.readFromSD(fname)) == 0) { //zero is success
          if ((presets[i].wdrc_broadband.readFromSD(fname)) == 0) { //zero is success
            if ((presets[i].afc.readFromSD(fname)) == 0) { //zero is success
              is_SD_success = true;
            }
          }
        }

//          if (i==0) {
//            presets[i].wdrc_perBand.printAllValues(); 
//            presets[i].wdrc_broadband.printAllValues(); 
//            presets[i].afc.printAllValues();
//          }
        
        if (!is_SD_success) {
          //print error messages
          Serial.print("State: defineAlgorithmPresets: *** WARNING *** could not read all preset elements from "); Serial.print(fname);
          Serial.println("    : Using built-in algorithm presets instead.");
        }
      }
  
      //did all three get successfully read?
      if (!loadFromSD || !is_SD_success) setPresetToDefault(i);
    }    

    void saveCurrentAlgPresetToSD(bool writeToSD = false) { //saves the current preset to the presets[] array and (maybe) writes to SD
      int i = current_alg_config;
         
      //save current settings to the preset
      presets[i].wdrc_perBand = wdrc_perBand;
      presets[i].wdrc_broadband = wdrc_broadband;
      presets[i].afc = afc;

      //write preset to the SD card
      if (writeToSD) {
        const char *fname = preset_fnames[i];
        presets[i].wdrc_perBand.printToSD(fname,var_names[i*3+0], true);  //the "true" says to start from a fresh file
        presets[i].wdrc_broadband.printToSD(fname,var_names[i*3+1]);  //append to the file
        presets[i].afc.printToSD(fname,var_names[i*3+2]);  //append to the file
      }
    }
    void revertCurrentAlgPresetToDefault(bool writeToSD = false) { //saves the current preset to the presets[] array and (maybe) writes to SD
      Serial.print("State: revertCurrentAlgPresetToDefault: current_alg_config = ");
      Serial.println(current_alg_config);

      //load the factor defaults into the preset
      int i = current_alg_config;
      defineAlgorithmPreset(current_alg_config, false);  //false => do NOT read from SD card.  Use the built-in settings.

//      if (i==0) {
//        presets[i].wdrc_perBand.printAllValues(); 
//        presets[i].wdrc_broadband.printAllValues(); 
//        presets[i].afc.printAllValues();
//      }    

      //save to SD card
      if (writeToSD) {
        Serial.println("State: revertCurrentAlgPresetToDefault: writing to SD.");
        const char *fname = preset_fnames[i];
        presets[i].wdrc_perBand.printToSD(fname,var_names[i*3+0], true);  //the "true" says to start from a fresh file
        presets[i].wdrc_broadband.printToSD(fname,var_names[i*3+1]);  //append to the file
        presets[i].afc.printToSD(fname,var_names[i*3+2]);  //append to the file
      }
    }

  private:
    Print *local_serial;
    AudioSettings_F32 *local_audio_settings;
};

#endif
