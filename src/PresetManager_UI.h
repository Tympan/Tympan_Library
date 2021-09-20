/*
 * PresetManager_UI
 * 
 * Created: Chip Audette, OpenAudio, Sept 2021
 * 
 * Purpose: This class tries to simplify the managing, saving, and recalling of presets
 *          (especially for BTNRH-type algorithms)
 *
 * MIT License.  Use at your own risk.
*/

#ifndef _PresetManager_UI_h
#define _PresetManager_UI_h

#include <SerialManager_UI.h>         //from Tympan_Library
#include <TympanRemoteFormatter.h> //from Tympan_Library

class PresetManagerBase {
  public:
    PresetManagerBase(void) : n_presets(1) {};
    PresetManagerBase(int _n_presets) : n_presets(_n_presets) {};

    virtual int setToPreset(int i, bool update_gui) = 0;
    virtual int switchToPreset(int i, bool update_gui) { return setToPreset(i, update_gui); } //you should overwrite ethis in your child class as switching isn't always the same as setting
    virtual int getPresetInd(void) { return cur_preset_ind; }
    
    // here are some data members
    const int n_presets;
    int cur_preset_ind = 0;
    
};

class PresetManager_UI : public PresetManagerBase, public SerialManager_UI {
  public: 
    PresetManager_UI(int _n_presets) : 
      PresetManagerBase(_n_presets), 
      SerialManager_UI() {};
      
    // here are the methods required (or encouraged) for SerialManager_UI classes
    virtual void printHelp(void);
    //virtual bool processCharacter(char c); //not used here
    virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
    virtual void setFullGUIState(bool activeButtonsOnly = false);

    // here are other methods that we need
    virtual String getPresetName(int Ipreset) { return String("Preset"); }; //you should overwrite this in your child class!!
    virtual String getCurrentPresetName(void) { return getPresetName(getPresetInd()); };
    virtual int savePresetToSD(int i) = 0;       //you need to write one of these
    virtual int readPresetFromSD(int i, bool update_algs, bool update_gui) = 0;     //you need to write one of these
    virtual int resetPresetToFactory(int i, bool update_algs, bool update_gui) = 0; //you need to write one of these
	//virtual int savePresetToSD(void) { return savePresetToSD(getPresetInd()); }
    //virtual int readPresetFromSD(void) { return readPresetFromSD(getPresetInd()); }
    //virtual int resetPresetToFactory(void) { return resetPresetToFactory(getPresetInd()); }
	

    // ///////////////////////////// Methods related to managing the App GUI
    String name_for_UI = "Preset Manager";  //used for printHelp() and in App GUI
    
    //create the button sets for the TympanRemote's GUI
    virtual TR_Card* addCard_presetSelect(TR_Page *page_h);
    virtual TR_Card* addCard_presetSaving(TR_Page *page_h);
    
    //overall default page, if sometone just blindly calls addPage_default();
    virtual TR_Page* addPage_default(TympanRemoteFormatter *gui) {
      if (gui == NULL) return NULL;
      TR_Page *page_h = gui->addPage(name_for_UI);
      if (page_h == NULL) return NULL;
      
      addCard_presetSelect(page_h);
      addCard_presetSaving(page_h);
      return page_h; 
    }; 

    //methods to update the GUI fields
    bool flag_send_presetSelect = false;
    bool flag_send_presetSaving = false;
    void updateCard_presetSelect(bool activeButtonsOnly = false);
    void updateCard_presetSaving(bool activeButtonsOnly = false);
    
};

#endif
