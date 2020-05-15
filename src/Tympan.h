/*
	Tympan

	Created: Chip Audette, Open Audio
	Purpose: Classes to wrap up the hardware features of Tympan.

	License: MIT License.  Use at your own risk.
 */

#ifndef _Tympan_h
#define _Tympan_h

enum class TympanRev { A, C, D, D0, D1, D2, D3, D4 };

//constants to help define which version of Tympan is being used
#define TYMPAN_REV_A (TympanRev::A)
#define TYMPAN_REV_C (TympanRev::C)
#define TYMPAN_REV_D0 (TympanRev::D0)
#define TYMPAN_REV_D1 (TympanRev::D1)
#define TYMPAN_REV_D2 (TympanRev::D2)
#define TYMPAN_REV_D3 (TympanRev::D3)
#define TYMPAN_REV_D4 (TympanRev::D4)
#define TYMPAN_REV_D (TympanRev::D)
//define TYMPAN_REV_D_CCP (TympanRev::D_CCP)

//the Tympan is a Teensy audio library "control" object
//include <usb_desc.h>  //to know if we're using native or emulated USB serial
#include "control_aic3206.h"  //see in here for more #define statements that are very relevant!
#include <Arduino.h>  //for the Serial objects
#include <Print.h>
#include "AudioStream_F32.h"
#include "AudioSettings_F32.h"

#ifndef NOT_A_FEATURE
#define NOT_A_FEATURE (-9999)
#endif

class TympanPins { //Teensy 3.6 Pin Numbering
	public:
		TympanPins(void) {
			setTympanRev(TympanRev::D);  //assume RevD is default
		};
		TympanPins(TympanRev _tympanRev) {
			setTympanRev(_tympanRev);
		}
		void setTympanRev(TympanRev _tympanRev) {
			tympanRev = _tympanRev;
			switch (tympanRev) {
				case (TympanRev::A) :    //Earliest prototypes...like RevC but with errors
					//Teensy 3.6 Pin Numbering
					resetAIC = 21;  //PTD6
					potentiometer = 15;  //PTC0
					amberLED = 36;  //PTC9
					redLED = 35;  //PTC8
					BT_nReset = 6; //
					BT_REGEN = NOT_A_FEATURE;
					BT_PIO4 = 2;  //PTD0
					reversePot = true;
					enableStereoExtMicBias = NOT_A_FEATURE; //mic jack is already stereo, can't do mono.
					break;
				case (TympanRev::C) :   //First released Tympan hardware.  Sold in blue case.
					//Teensy 3.6 Pin Numbering
					resetAIC = 21;  //PTD6
					potentiometer = 15;  //PTC0
					amberLED = 36;  //PTC9
					redLED = 35;  //PTC8
					BT_nReset = 6; //PTD4
					BT_REGEN = NOT_A_FEATURE;
					BT_PIO4 = 2;  //PTD0
					enableStereoExtMicBias = NOT_A_FEATURE; //mic jack is already mono, can't do stereo.
					break;
				case (TympanRev::D0) : case (TympanRev::D1) :  //RevD development...first with BC127 BT module
					//Teensy 3.6 Pin Numbering
					resetAIC = 35;  //PTC8
					potentiometer = 15; //PTC0
					amberLED = 36; //PTC9
					redLED = 10;  //PTC4
					BT_nReset = 34;  //PTE25, active LOW reset
					BT_REGEN = 31;  //must pull high to enable BC127
					BT_PIO4 = 33;  //PTE24
					enableStereoExtMicBias = 20; //PTD5
					BT_serial_speed = 9600;
					break;
				case (TympanRev::D2) :   //RevD development...includes on-board preamp
					//Teensy 3.6 Pin Numbering
					resetAIC = 42;	// was 35;  //PTC8
					potentiometer = 20;	// PTA16	was 15; //PTC0
					amberLED = 36; //PTC9
					redLED = 10;  //PTC4
					BT_nReset = 34;  //PTE25, active LOW reset
					BT_REGEN = 31;  //must pull high to enable BC127
					BT_PIO0 = A10;  //hard reset for the BT module if HIGH at start.  Otherwise, outputs the connection state
					BT_PIO4 = 33;  //PTE24
					enableStereoExtMicBias = 20; //PTD5
					BT_serial_speed = 9600;
					break;
				case (TympanRev::D3) :  //Built for OpenTact
					//Teensy 3.6 Pin Numbering
					resetAIC = 35;  //PTC8
					potentiometer = 39; //A20
					amberLED = 36; //PTC9
					redLED = 10;  //PTC4
					BT_nReset = 34;  //PTE25, active LOW reset
					BT_REGEN = 31;  //must pull high to enable BC127
					BT_PIO0 = 56; // JM: was A10;   //hard reset for the BT module if HIGH at start.  Otherwise, outputs the connection state
					BT_PIO4 = 33;  //PTE24...actually it's BT_PIO5 ??? JM: Yes, it's BT_PIO5
					enableStereoExtMicBias = 20; //PTD5
					BT_serial_speed = 9600;
					Rev_Test = 44;
					break;
				case (TympanRev::D4) :  case (TympanRev::D) :  //RevD release candidate
					//Teensy 3.6 Pin Numbering
					resetAIC = 35;  //PTC8
					potentiometer = 39; //A20
					amberLED = 36; //PTC9
					redLED = 10;  //PTC4
					BT_nReset = 34;  //PTE25, active LOW reset
					BT_REGEN = 31;  //must pull high to enable BC127
					BT_PIO0 = 56;   //hard reset for the BT module if HIGH at start.  Otherwise, outputs the connection state
					BT_PIO4 = 33;  //PTE24...actually it's BT_PIO5 ??? JM: YES, IT IS BT_PIO5!
					enableStereoExtMicBias = 20; //PTD5
					//AIC_Shield_enableStereoExtMicBias = 41;
					BT_serial_speed = 9600;
					Rev_Test = 44;
					break;
/* 				case (TympanRev::D_CCP):  //the Tympan functions itself are same as RevD, but added some features to support the CCP shield
					//Teensy 3.6 Pin Numbering
					resetAIC = 35;  //PTC8
					potentiometer = 39; //A20
					amberLED = 36; //PTC9
					redLED = 10;  //PTC4
					BT_nReset = 34;  //PTE25, active LOW reset
					BT_REGEN = 31;  //must pull high to enable BC127
					BT_PIO0 = 56;   //hard reset for the BT module if HIGH at start.  Otherwise, outputs the connection state
					BT_PIO4 = 33;  //PTE24...actually it's BT_PIO5 ??? JM: YES, IT IS BT_PIO5!
					enableStereoExtMicBias = 20; //PTD5
					AIC_Shield_enableStereoExtMicBias = 41;
					BT_serial_speed = 9600;
					Rev_Test = 44;		
					CCP_atten1 = 52;  //enable attenuator #1.  Same as MOSI_2 (alt)
					CCP_atten2 = 51;  //enable attenuator #2.  Same as MISO_2 (alt)
					CCP_bigLED =  53;    //same as SCK_2 (alt)
					CCP_littleLED = 41;    //same as AIC_Shield_enableStereoExtMicBias
					CCP_enable28V = 5; //enable the 28V power supply.  Same as SS_2 */
			}
		}
		//#if defined(SEREMU_INTERFACE)
		//	usb_seremu_class * getUSBSerial(void) { return USB_Serial; }
		//#else
			usb_serial_class * getUSBSerial(void) { return USB_Serial; }
		//#endif
		HardwareSerial * getBTSerial(void) { return BT_Serial; }

		//Defaults (Teensy 3.6 Pin Numbering), assuming Rev C
		TympanRev tympanRev = TympanRev::C;
 		int resetAIC = 21;  //PTD6
		int potentiometer = 15;  //PTC0
		int amberLED = 36;  //PTC9
		int redLED = 35;  //PTC8
		int BT_nReset = 6; //PTD4
		int BT_REGEN = NOT_A_FEATURE;
		int BT_PIO0 = NOT_A_FEATURE;
		int BT_PIO4 = NOT_A_FEATURE;  //PTD0
		int Rev_Test = NOT_A_FEATURE;
		bool reversePot = false;
		int enableStereoExtMicBias = NOT_A_FEATURE;
		//int AIC_Shield_enableStereoExtMicBias = NOT_A_FEATURE;
		//int CCP_atten1 = NOT_A_FEATURE, CCP_atten2 = NOT_A_FEATURE;
		//int CCP_bigLED = NOT_A_FEATURE, CCP_littleLED = NOT_A_FEATURE;
		//int CCP_enable28V = NOT_A_FEATURE;
		//#if defined(SEREMU_INTERFACE)
		//	usb_seremu_class *USB_Serial = &Serial;
		//#else
			usb_serial_class *USB_Serial = &Serial; //true for Rev_A/C/D
		//#endif
		HardwareSerial *BT_Serial = &Serial1; //true for Rev_A/C/D
		int BT_serial_speed = 115200; //true for Rev_A/C
};

//This code works for Teensy 3.x (ie, Tympan Rev C and Rev D)
extern "C" char* sbrk(int incr);
//End of code specific to Teensy 3.x

class TympanBase : public AudioControlAIC3206, public Print
{
	public:
		TympanBase(void) : AudioControlAIC3206() {}
		TympanBase(bool _debugToSerial) : AudioControlAIC3206(_debugToSerial) {}
		TympanBase(const TympanPins &_pins) : AudioControlAIC3206() {
			setupPins(_pins);
		}
		TympanBase(const TympanPins &_pins, bool _debugToSerial) : AudioControlAIC3206(_debugToSerial) {
			setupPins(_pins);
		}
		TympanBase(const AudioSettings_F32 &_as) : AudioControlAIC3206() {
			setAudioSettings(_as);
		}
		TympanBase(const TympanPins &_pins, const AudioSettings_F32 &_as) : AudioControlAIC3206() {
			setupPins(_pins);
			setAudioSettings(_as);
		}
		TympanBase(const TympanPins &_pins, const AudioSettings_F32 &_as, bool _debugToSerial) : AudioControlAIC3206(_debugToSerial) {
			setupPins(_pins);
			setAudioSettings(_as);
		}

		void setupPins(const TympanPins &_pins);
		void setAudioSettings(const AudioSettings_F32 &_aud_set) { audio_settings = _aud_set; }  //shallow copy
		void forceBTtoDataMode(bool state);

		//TympanPins getTympanPins(void) { return &pins; }
		int setAmberLED(int _value) { digitalWrite(pins.amberLED,_value); return _value; }
		int setRedLED(int _value) { digitalWrite(pins.redLED,_value); return _value; }
		//int setCCPBigLED(int _value) { if (pins.CCP_bigLED != NOT_A_FEATURE) { digitalWrite(pins.CCP_bigLED,_value); return _value; } return NOT_A_FEATURE;}
		//int setCCPLittleLED(int _value) { if (pins.CCP_littleLED != NOT_A_FEATURE) { digitalWrite(pins.CCP_littleLED,_value); return _value; } return NOT_A_FEATURE;}
		//int setCCPEnable28V(int _value) { if (pins.CCP_enable28V != NOT_A_FEATURE) { digitalWrite(pins.CCP_enable28V,_value); return _value; } return NOT_A_FEATURE; }
		//int setCCPEnableAtten1(int _value) { if (pins.CCP_atten1 != NOT_A_FEATURE) { digitalWrite(pins.CCP_atten1,_value); return _value; } return NOT_A_FEATURE; }
		//int setCCPEnableAtten2(int _value) { if (pins.CCP_atten2 != NOT_A_FEATURE) { digitalWrite(pins.CCP_atten2,_value); return _value; } return NOT_A_FEATURE; }
		int readPotentiometer(void) {
			//Serial.print("TympanBase: readPot, pin "); Serial.println(pins.potentiometer);
			int val = analogRead(pins.potentiometer);
			if (pins.reversePot) val = 1023 - val;
			return val;
		};
		int setEnableStereoExtMicBias(int new_state);  //use for base Tympan board (not the AIC shield)
		//int setEnableStereoExtMicBiasAIC(int new_state);  //use for AIC Shield
	
		TympanRev getTympanRev(void) { return pins.tympanRev; }
		int getPotentiometerPin(void) { return pins.potentiometer; }
		//#if defined(SEREMU_INTERFACE)
		//	usb_seremu_class *getUSBSerial(void) { return USB_Serial; }
		//#else
			usb_serial_class *getUSBSerial(void) { return USB_Serial; }
		//#endif
		HardwareSerial *getBTSerial(void) { return BT_Serial; }
		void beginBothSerial(void) { beginBothSerial(115200, pins.BT_serial_speed); }
		void beginBothSerial(int USB_speed, int BT_speed) {
			USB_Serial->begin(USB_speed); delay(50);
			beginBluetoothSerial(BT_speed);
		}
		int USB_dtr() { return USB_Serial->dtr(); }

		void beginBluetoothSerial(void) { beginBluetoothSerial(pins.BT_serial_speed); }
		void beginBluetoothSerial(int BT_speed);
		void clearAndConfigureBTSerialRevD(void);

		bool mixBTAudioWithOutput(bool state) { return mixInput1toHPout(state); } //bluetooth audio is on Input1
		void echoIncomingBTSerial(void) {
			while (BT_Serial->available()) USB_Serial->write(BT_Serial->read());//echo messages from BT serial over to USB Serial
		}
		void setBTAudioVolume(int vol); //vol is 0 (min) to 15 (max).  Only Rev D.  Only works when you are connected via Bluetooth!!!!


		//I want to enable an easy way to print to both USB and BT serial with one call.
		//So, I inhereted the Print class, which gives me all of the Arduino print/write
		//methods except for the most basic write().  Here, I define write() so that all
		//of print() and println() and all of that works transparently.  Yay!
		using Print::write;
		virtual size_t write(uint8_t foo) {
			if (USB_dtr()) USB_Serial->write(foo); //the USB Serial can jam up, so make sure that something is open on the PC side
			return BT_Serial->write(foo);
			//if (USB_dtr()) Serial.write(foo); //the USB Serial can jam up, so make sure that something is open on the PC side
			//return Serial1.write(foo);
		}
		virtual size_t write(const uint8_t *buffer, size_t orig_size) { //this should be faster than the core write(uint8_t);
			//USB_Serial->write('t');
			size_t count = 0;
			size_t size = orig_size;
			int i=0;
			if (USB_dtr()) {
				//while (size--) count += USB_Serial->write(*buffer++);
				while (size--) USB_Serial->write(buffer[i++]);
				//return count;
			}
			size = orig_size;
			while (size--) count += BT_Serial->write(*buffer++);
			return count;
		}
		virtual size_t write(const char *str) { return write((const uint8_t *)str, strlen(str)); } //should use the faster write
		virtual void flush(void) { USB_Serial->flush(); BT_Serial->flush(); }

		//using TympanPrint::print;
		//using TympanPrint::println;
		//virtual size_t print(const char *s) { return write(s); }  //should use the faster write
		//virtual size_t println(const char *s) { return print(s) + println(); }  //should use the faster write
		//virtual size_t println(void) { 	uint8_t buf[2]={'\r', '\n'}; return write(buf, 2); }
		
		void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis = 3000) {
			//static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
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
		  print("CPU Cur/Pk: ");
		  print(audio_settings.processorUsage(), 1);
		  print("%/");
		  print(audio_settings.processorUsageMax(), 1);
		  print("%, ");
		  print("MEM Cur/Pk: ");
		  print(AudioMemoryUsage_F32());
		  print("/");
		  print(AudioMemoryUsageMax_F32());
		  print(", FreeRAM(B) ");
		  print(FreeRam());
		  println();
		}
		
		#if defined(SEREMU_INTERFACE)
			usb_seremu_class *USB_Serial;
		#else
			usb_serial_class *USB_Serial;
		#endif
		HardwareSerial *BT_Serial;

	protected:
		TympanPins pins;
		AudioSettings_F32 audio_settings;
		

};

class Tympan : public TympanBase {
	public:
		Tympan(void) : TympanBase() {
			TympanPins myPins;  //assumes defaults set by TympanPins
			TympanBase::setupPins(myPins);
		}
		Tympan(const TympanRev &_myRev) : TympanBase() {
			//initialize the TympanBase
			TympanPins myPins(_myRev);
			TympanBase::setupPins(myPins);
		}
		Tympan(const TympanRev &_myRev, bool _debugToSerial) : TympanBase(_debugToSerial) {
			//initialize the TympanBase
			TympanPins myPins(_myRev);
			TympanBase::setupPins(myPins);
		}
		Tympan(const TympanRev &_myRev, const AudioSettings_F32 &_as) : TympanBase() {
			//initialize the TympanBase
			TympanPins myPins(_myRev);
			TympanBase::setupPins(myPins);
			TympanBase::setAudioSettings(_as);
		}
		Tympan(const TympanRev &_myRev, const AudioSettings_F32 &_as, bool _debugToSerial) : TympanBase(_debugToSerial) {
			//initialize the TympanBase
			TympanPins myPins(_myRev);
			TympanBase::setupPins(myPins);
			TympanBase::setAudioSettings(_as);
		}
};

#endif
