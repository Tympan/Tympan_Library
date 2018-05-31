/* 
	Tympan
	
	Created: Chip Audette, Open Audio
	Purpose: Classes to wrap up the hardware features of Tympan.
 
	License: MIT License.  Use at your own risk.
 */
 
#ifndef _Tympan_h
#define _Tympan_h

//constants to help define which version of Tympan is being used
#define TYMPAN_REV_C (3)
#define TYMPAN_REV_D (4)

//the Tympan is a Teensy audio library "control" object
#include "control_tlv320aic3206.h"  //see in here for more #define statements that are very relevant!


class TympanPins { //Teensy 3.6 Pin Numbering
	public:
		TympanPins(void) {}; //use default pins
		TympanPins(int _tympanRev) {
			setTympanRev(_tympanRev);
		}
		void setTympanRev(int _tympanRev) {
			tympanRev = _tympanRev;
			switch (tympanRev) {
				case (TYMPAN_REV_C) :
					//Teensy 3.6 Pin Numbering
					resetAIC = 21;  //PTD6
					potentiometer = 15;  //PTC0
					amberLED = 36;  //PTC9
					redLED = 35;  //PTC8
					BT_nReset = 6; //PTD4
					BT_PIO4 = 2;  //PTD0
					break;
				case (TYMPAN_REV_D) :
					//Teensy 3.6 Pin Numbering
					resetAIC = 35;  //PTC8
					potentiometer = 15; //PTC0
					amberLED = 36; //PTC9
					redLED = 10;  //PTC4
					BT_nReset = 34;  //PTE25
					BT_PIO4 = 33;  //PTE24
					break;
			}
		}
		
		//Defaults (Teensy 3.6 Pin Numbering), assuming Rev C
		int tympanRev = TYMPAN_REV_C;
 		int resetAIC = 21;  //PTD6
		int potentiometer = 15;  //PTC0
		int amberLED = 36;  //PTC9
		int redLED = 35;  //PTC8
		int BT_nReset = 6; //PTD4
		int BT_PIO4 = 2;  //PTD0

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
		};
		//TympanPins getTympanPins(void) { return &pins; }
		void setAmberLED(int _value) { digitalWrite(pins.amberLED,_value); }
		void setRedLED(int _value) { digitalWrite(pins.redLED,_value); }
		int readPotentiometer(void) { 
			//Serial.print("TympanBase: readPot, pin "); Serial.println(pins.potentiometer);
			return analogRead(pins.potentiometer); 
		};	
		int getTympanRev(void) { return pins.tympanRev; }
		int getPotentiometerPin(void) { return pins.potentiometer; }
		
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