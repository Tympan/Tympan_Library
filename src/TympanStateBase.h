
#ifndef TympanStateBase_h
#define TympanStateBase_h

#include "AudioSettings_F32.h"
#include "Tympan.h"
#include "SerialManager_UI.h"  //for the UI stuff
#include "SerialManagerBase.h" //for the UI stuff

class TympanStateBase {
	
	public:
		TympanStateBase(AudioSettings_F32 *given_settings, Print *given_serial) {
			local_audio_settings = given_settings; 
			local_serial = given_serial;
		}

		//printing of CPU and memory status
		bool flag_printCPUandMemory = false;
		void printCPUandMemory(unsigned long curTime_millis = 0, unsigned long updatePeriod_millis = 0) {
			static unsigned long lastUpdate_millis = 0;
			//has enough time passed to update everything?
			if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
			if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
				printCPUandMemoryMessage();
				lastUpdate_millis = curTime_millis; //we will use this value the next time around.
			}
		}

		void printCPUandMemoryMessage(void) {
			local_serial->print("CPU Cur/Pk: ");
			local_serial->print(local_audio_settings->processorUsage(), 1);
			local_serial->print("%/");
			local_serial->print(local_audio_settings->processorUsageMax(), 1);
			local_serial->print("%, ");
			local_serial->print("Audio MEM Cur/Pk: ");
			local_serial->print(AudioMemoryUsage_F32());
			local_serial->print("/");
			local_serial->print(AudioMemoryUsageMax_F32());
			local_serial->print(", FreeRAM(B): ");
			local_serial->print(Tympan::FreeRam());
			local_serial->println();
		}
		float getCPUUsage(void) { return local_audio_settings->processorUsage(); }

	
	protected:
		Print *local_serial;
		AudioSettings_F32 *local_audio_settings;
};

class TympanStateBase_UI : public TympanStateBase, public SerialManager_UI {
	public:
		TympanStateBase_UI(AudioSettings_F32 *given_settings, Print *given_serial) : 
			TympanStateBase(given_settings, given_serial), SerialManager_UI() {};
		TympanStateBase_UI(AudioSettings_F32 *given_settings, Print *given_serial, SerialManagerBase *_sm) : 
			TympanStateBase(given_settings, given_serial), SerialManager_UI(_sm) {};
		
		// ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void) {
				String prefix = getPrefix(); //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
				Serial.println(F(" State: Prefix = ") + prefix);
				Serial.println(F("   c/C: Enable/Disable printing of CPU and Memory usage"));
		};
		//virtual bool processCharacter(char c); //no used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char){
			bool return_val = false;
			if (mode_char != ID_char) return return_val; //does the mode character match our ID character?  if so, it's us!

			//we ignore the chan_char and only work with the data_char
			return_val = true;  //assume that we will find this character
			switch (data_char) {    
				case 'c':
				  Serial.println("TympanStateBase_UI: printing memory and CPU.");
				  flag_printCPUandMemory = true;
				  setCPUButtons();
				  break;
				case 'C':
				  Serial.println("TympanStateBase_UI: stopping printing of memory and CPU.");
				  flag_printCPUandMemory = false;
				  setCPUButtons();
				  break;
				default:
				  return_val = false;  //we did not process this character
			}
			return return_val;	
		};
		virtual void setFullGUIState(bool activeButtonsOnly = false) {
			setCPUButtons();
		}
 
		// /////////////////////////////////	
		
		//create the button sets for the TympanRemote's GUI
		virtual TR_Card* addCard_cpuReporting(TR_Page *page_h) {
			return addCard_cpuReporting(page_h, getPrefix());
		}
		virtual TR_Card* addCard_cpuReporting(TR_Page *page_h, String prefix) {
			if (page_h == NULL) return NULL;
			TR_Card *card_h = page_h->addCard(String("CPU Usage (%)"));
			if (card_h == NULL) return NULL;
			
			card_h->addButton("Start", prefix+"c", "cpuStart", 4);  //label, command, id, width
			card_h->addButton(""     , "",         "cpuValue", 4);  //label, command, id, width //display the CPU value
			card_h->addButton("Stop",  prefix+"C", "",         4);  //label, command, id, width
			return card_h;
		};
		virtual TR_Page* addPage_globals(TympanRemoteFormatter *gui) {
			if (gui == NULL) return NULL;
			TR_Page *page_h = gui->addPage("State");
			if (page_h == NULL) return NULL;
			addCard_cpuReporting(page_h);
			return page_h;
		};
		virtual TR_Page* addPage_default(TympanRemoteFormatter *gui) {return addPage_globals(gui);}
		
		virtual void setCPUButtons(bool activeButtonsOnly = false) {
			if (flag_printCPUandMemory) {
				setButtonState("cpuStart",true);
			} else {
				if (!activeButtonsOnly) {
					setButtonState("cpuStart",false);
				}
			}
		};
		virtual void printCPUtoGUI(unsigned long curTime_millis = 0, unsigned long updatePeriod_millis = 0) {
			static unsigned long lastUpdate_millis = 0;
			//has enough time passed to update everything?
			if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
			if ((curTime_millis - lastUpdate_millis) >= updatePeriod_millis) { //is it time to update the user interface?
				Serial.println("TympanStateBase: printCPUtoGUI: sending " + String(getCPUUsage(),1));
				setButtonText("cpuValue", String(getCPUUsage(),1));
				lastUpdate_millis = curTime_millis; //we will use this value the next time around.
			}
		}
};

#endif