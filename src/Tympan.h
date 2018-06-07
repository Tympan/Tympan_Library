/* 
	Tympan
	
	Created: Chip Audette, Open Audio
	Purpose: Classes to wrap up the hardware features of Tympan.
 
	License: MIT License.  Use at your own risk.
 */
 
#ifndef _Tympan_h
#define _Tympan_h


//constants to help define which version of Tympan is being used
#define TYMPAN_REV_A (2)
#define TYMPAN_REV_C (3)
#define TYMPAN_REV_D (4)

//the Tympan is a Teensy audio library "control" object
#include "control_tlv320aic3206.h"  //see in here for more #define statements that are very relevant!
#include <Arduino.h>  //for the Serial objects

#define NOT_A_FEATURE (-9999)

class TympanPins { //Teensy 3.6 Pin Numbering
	public:
		TympanPins(void) {}; //use default pins
		TympanPins(int _tympanRev) {
			setTympanRev(_tympanRev);
		}
		//int NOT_A_FEATURE = -9999;
		void setTympanRev(int _tympanRev) {
			tympanRev = _tympanRev;
			switch (tympanRev) {
				case (TYMPAN_REV_A) :
					//Teensy 3.6 Pin Numbering
					resetAIC = 21;  //PTD6
					potentiometer = 15;  //PTC0
					amberLED = 36;  //PTC9
					redLED = 35;  //PTC8
					BT_nReset = 6; //PTD4
					BT_PIO4 = 2;  //PTD0
					reversePot = true;
					enableStereoExtMicBias = NOT_A_FEATURE; //mic jack is already stereo, can't do mono.
					break;				
				case (TYMPAN_REV_C) :
					//Teensy 3.6 Pin Numbering
					resetAIC = 21;  //PTD6
					potentiometer = 15;  //PTC0
					amberLED = 36;  //PTC9
					redLED = 35;  //PTC8
					BT_nReset = 6; //PTD4
					BT_PIO4 = 2;  //PTD0
					enableStereoExtMicBias = NOT_A_FEATURE; //mic jack is already mono, can't do stereo.
					break;
				case (TYMPAN_REV_D) :
					//Teensy 3.6 Pin Numbering
					resetAIC = 35;  //PTC8
					potentiometer = 15; //PTC0
					amberLED = 36; //PTC9
					redLED = 10;  //PTC4
					BT_nReset = 34;  //PTE25
					BT_PIO4 = 33;  //PTE24
					enableStereoExtMicBias = 20; //PTD5
					break;
			}
		}
		Stream * getUSBSerial(void) { return USB_Serial; }
		Stream * getBTSerial(void) { return BT_Serial; }
		
		//Defaults (Teensy 3.6 Pin Numbering), assuming Rev C
		int tympanRev = TYMPAN_REV_C;
 		int resetAIC = 21;  //PTD6
		int potentiometer = 15;  //PTC0
		int amberLED = 36;  //PTC9
		int redLED = 35;  //PTC8
		int BT_nReset = 6; //PTD4
		int BT_PIO4 = 2;  //PTD0
		bool reversePot = false;
		int enableStereoExtMicBias = NOT_A_FEATURE;
		Stream *USB_Serial = &Serial; //Rev_A, Rev_C, Rev_D
		Stream *BT_Serial = &Serial1; //Rev_A, Rev_C, Rev_D
};
class TympanPins_RevA : public TympanPins {
	public:
		TympanPins_RevA(void) : TympanPins(TYMPAN_REV_A) {};
};
class TympanPins_RevC : public TympanPins {
	public:
		TympanPins_RevC(void) : TympanPins(TYMPAN_REV_C) {};
};
class TympanPins_RevD : public TympanPins {
	public:
		TympanPins_RevD(void) : TympanPins(TYMPAN_REV_D) {};
};

class TympanBase : public AudioControlTLV320AIC3206
{
	public:
		TympanBase(TympanPins &_pins) : AudioControlTLV320AIC3206(_pins.resetAIC) {
			setupPins(_pins);
		}
		TympanBase(TympanPins &_pins, bool _debugToSerial) : AudioControlTLV320AIC3206(_pins.resetAIC, _debugToSerial) {
			setupPins(_pins);
		}
		void setupPins(TympanPins &_pins) {
			pins = _pins; //copy to local version
			
			//Serial.print("TympanBase: setupPins: pins.potentiometer, given / act: "); 
			//Serial.print(_pins.potentiometer); Serial.print(" / "); Serial.println(pins.potentiometer);
			
			pinMode(pins.potentiometer,INPUT);
			pinMode(pins.amberLED,OUTPUT); digitalWrite(pins.amberLED,LOW);
			pinMode(pins.redLED,OUTPUT); digitalWrite(pins.redLED,LOW);
			if (pins.enableStereoExtMicBias != NOT_A_FEATURE) {
				pinMode(pins.enableStereoExtMicBias,OUTPUT);
				setEnableStereoExtMicBias(true); //enable stereo external mics (REV_D)
			}
		};
		//TympanPins getTympanPins(void) { return &pins; }
		void setAmberLED(int _value) { digitalWrite(pins.amberLED,_value); }
		void setRedLED(int _value) { digitalWrite(pins.redLED,_value); }
		int readPotentiometer(void) { 
			//Serial.print("TympanBase: readPot, pin "); Serial.println(pins.potentiometer);
			int val = analogRead(pins.potentiometer);
			if (pins.reversePot) val = 1023 - val;
			return val;
		};	
		int setEnableStereoExtMicBias(int new_state) {
			if (pins.enableStereoExtMicBias != NOT_A_FEATURE) {
				digitalWrite(pins.enableStereoExtMicBias,new_state);
				return new_state;
			} else {
				return pins.enableStereoExtMicBias;
			}
		}
		int getTympanRev(void) { return pins.tympanRev; }
		int getPotentiometerPin(void) { return pins.potentiometer; }
		Stream *getUSBSerial(void) { return pins.getUSBSerial(); }
		Stream *getBTSerial(void) { return pins.getBTSerial(); }
		
		
	private:
		TympanPins pins;
	
};
		

class TympanRevC : public TympanBase
{
	public:
		TympanRevC(void) : TympanBase(RevC_pins) {};
		TympanRevC(bool _debugToSerial) : TympanBase(RevC_pins, _debugToSerial) {};
		TympanPins_RevC RevC_pins;
	private:
};

class TympanRevD : public TympanBase
{
	public:
		TympanRevD(void) : TympanBase(RevD_pins)  {};
		TympanRevD(bool _debugToSerial) : TympanBase(RevD_pins, _debugToSerial) {};
		TympanPins_RevD RevD_pins;
	private:

};




#endif