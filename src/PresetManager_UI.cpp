
#include <PresetManager_UI.h>


void PresetManager_UI::printHelp(void) {
  Serial.println(" " + name_for_UI + ": Prefix = " + getPrefix()); //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
  Serial.println("   : 1-" + String(n_presets) + ": Switch to Preset 1-" + String(n_presets));
  Serial.println("   : s/r/f: save to SD, reload from SD, reset-to-factory current preset");
}

bool PresetManager_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
  
  //check the mode_char to see if it corresponds with this instance of this class.  If not, return with no action.
  if (mode_char != ID_char) return false;  //ID_char is from SerialManager_UI.h

  //at this point we should always return true, I believe
  bool return_val = true;  //assume that we will find this character

  //we ignore the chan_char and only work with the mode_char (above) the data_char (below)

  //interpret the data_char as if it were '1' - '9' as the preset number
  int preset_num = data_char - '1';  //should be zero for '1' through 8 for '9'
  if ((preset_num >= 0) && (preset_num < n_presets)) {
    //switch to the requested preset
    Serial.println("PresetManager_UI: processCharacterTriple: switching to preset " + String(preset_num));
	setSwitchStatusMessage("Switching...");
    switchToPreset(preset_num, true); //update the gui
	setSwitchStatusMessage("Complete");

 
  } else {
    
    //try interpretting the character directly
	int ret_val;
    switch (data_char) {
      case 's':
        Serial.println("PresetManager_UI: saving preset to SD...");
		setSaveStatusMessage("Saving...");
        ret_val = savePresetToSD(getPresetInd());
		delay(10); if (ret_val) { setSaveStatusMessage("Failed"); } else { setSaveStatusMessage("Success");}
        break;
      case 'r':
        Serial.println("PresetManager_UI: reload preset from SD...");
		setSaveStatusMessage("Reading...");
        ret_val = readPresetFromSD(getPresetInd(), true, true); //update the algorithms with the new settings and update the full GUI
		delay(10); if (ret_val) { setSaveStatusMessage("Failed"); } else { setSaveStatusMessage("Success");}
        break;
      case 'f':
	  setSaveStatusMessage("Resetting...");
        Serial.println("PresetManager_UI: reset preset to factory...");
        ret_val = resetPresetToFactory(getPresetInd(), true, true); //update the algorithms with the new settings and update the full GUI
		delay(10); if (ret_val) { setSaveStatusMessage("Failed"); } else { setSaveStatusMessage("Success");}
        break;
      default:
        return_val = false;
    }
  }
  return return_val;
}

void PresetManager_UI::setFullGUIState(bool activeButtonsOnly) {
  if (flag_send_presetSelect) updateCard_presetSelect(activeButtonsOnly);
  if (flag_send_presetSaving) updateCard_presetSaving(activeButtonsOnly);
}

TR_Card* PresetManager_UI::addCard_presetSelect(TR_Page *page_h) {
  if (page_h == NULL) return NULL;
  TR_Card *card_h = page_h->addCard("Choose Preset");
  if (card_h == NULL) return NULL;

  //add a "button" to act as the title
  String prefix = getPrefix();  // for use with button commands
  String ID_fn = String(ID_char_fn); //for use with button names
  for (int i=0; i<n_presets;i++) {
    String but_cmd = prefix+String(i+1);
    String but_id = ID_fn+"pre"+String(i+1);
    card_h->addButton(getPresetName(i), but_cmd, but_id, 12);  //label, command, id, width...this is the minus button  
  }
  card_h->addButton("", "", ID_fn+"SwStat",12);

  flag_send_presetSelect = true;  //flag that says to send this info when updating the GUI
  return card_h;
}

TR_Card* PresetManager_UI::addCard_presetSaving(TR_Page *page_h) {
  if (page_h == NULL) return NULL;
  TR_Card *card_h = page_h->addCard("Preset Saving");
  if (card_h == NULL) return NULL;

  //add a "button" to act as the title
  String prefix = getPrefix();  // for use with button commands
  String ID_fn = String(ID_char_fn); //for use with button names

  //add "button" that is just a label for the current preset
  card_h->addButton(getCurrentPresetName(), "", ID_fn+"name", 12);  //label, command, id, width...this is the minus button  

  //add buttons to save and restore from SD
  card_h->addButton("Save to SD Card",     prefix+'s', "", 12);  //label, command, id, width...this is the minus button  
  card_h->addButton("Reload from SD Card", prefix+'r', "", 12);  //label, command, id, width...this is the minus button  
  card_h->addButton("Reset to Factory",    prefix+'f', "", 12);  //label, command, id, width...this is the minus button  
  card_h->addButton("",                    ""        ,ID_fn+"SavStat", 12);   //label, command, id, width...this is the minus button  

  flag_send_presetSaving = true;  //flag that says to send this info when updating the GUI
  return card_h; 
}

void PresetManager_UI::updateCard_presetSelect(bool activeButtonsOnly) {
  String ID_fn = String(ID_char_fn); //for use with button names
  for (int i=0; i<n_presets;i++) {
    String but_id = ID_fn+"pre"+String(i+1);
    if (i == getPresetInd()) {
      //this is the active one!
      setButtonState(but_id,true);
    } else {
      //this is an inactive one
      if (activeButtonsOnly == false) setButtonState(but_id,false);
    }  
  } // end loop over presets  
  setSwitchStatusMessage(String(""));
}

void PresetManager_UI::updateCard_presetSaving(bool activeButtonsOnly) {
  //update the string showing the current preset name
  String ID_fn = String(ID_char_fn); //for use with button names
  String but_id = ID_fn+"name";
  setButtonText(but_id,getCurrentPresetName());
  setSaveStatusMessage(String(""));
}
void PresetManager_UI::setSwitchStatusMessage(String s) {
	String ID_fn = String(ID_char_fn); //for use with button names
	String but_id = ID_fn+"SwStat";
	setButtonText(but_id,s);
}
void PresetManager_UI::setSaveStatusMessage(String s) {
	String ID_fn = String(ID_char_fn); //for use with button names
	String but_id = ID_fn+"SavStat";
	setButtonText(but_id,s);
}