

#ifndef EarpieceMixer_F32_UI_h
#define EarpieceMixer_F32_UI_h

#include <Arduino.h>  //for Serial and String
#include "SerialManager_UI.h"  //for SerialManager_UI
#include "EarpieceMixer_F32.h"
#include "TympanRemoteFormatter.h"

class EarpieceMixer_F32_UI : public EarpieceMixer_F32, public SerialManager_UI {
	public:
		EarpieceMixer_F32_UI(const AudioSettings_F32 &settings) : EarpieceMixer_F32(settings), SerialManager_UI() {}

		 // ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void);
		//virtual bool processCharacter(char c); //not used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
		virtual void setFullGUIState(bool activeButtonsOnly = false);  
		// /////////////////////////////////

		//create the button sets for the TympanRemote's GUI
		virtual TR_Card* addCard_audioSource(TR_Page *page_h);
		virtual TR_Card* addCard_frontRearMics(TR_Page *page_h);
		virtual TR_Card* addCard_frontMicDelay(TR_Page *page_h);
		virtual TR_Card* addCard_rearMicDelay(TR_Page *page_h);
		virtual TR_Card* addCard_rearMicGain(TR_Page *page_h);
		virtual TR_Page* addPage_digitalEarpieces(TympanRemoteFormatter *gui);
		virtual TR_Page* addPage_default(TympanRemoteFormatter *gui) { return addPage_digitalEarpieces(gui); }

		//all other methods
		void setInputConfigButtons(bool activeButtonsOnly = false);    
		void setButtonState_rearMic(bool activeButtonsOnly = false);
		void setButtonState_frontRearMixer(bool activeButtonsOnly = false);
		void setButtonState_inputMixer(bool activeButtonsOnly = false);
		void setButtonState_micDelay(bool activeButtonsOnly = false);
		void setInputGainButtons(bool activeButtonsOnly = false);

	protected:
		float rearMicGainIncrement_dB = 0.5f;

    
};


#endif

