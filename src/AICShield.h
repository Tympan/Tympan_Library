
/*
	AICShield

	Created: Chip Audette, Open Audio
	Purpose: Classes to wrap up the hardware features of AIC shield.

	License: MIT License.  Use at your own risk.
 */

#ifndef _AICShield_h
#define _AICShield_h

#include <Tympan.h>  //need to know the Tympan rev

enum class AICShieldRev { A, CCP, CCP_A };

//the AICShield is a Teensy audio library "control" object
#include "control_aic3206.h"  //see in here for more #define statements that are very relevant!
#include <Arduino.h>  //for the Serial objects (to print debug info)
#include "AudioStream_F32.h"
#include "AudioSettings_F32.h"

#ifndef NOT_A_FEATURE
#define NOT_A_FEATURE (-9999)
#endif

#define AICSHIELD_DEFAULT_RESET_PIN 42
#define AICSHIELD_DEFAULT_I2C_BUS	2
#define AICSHIELD_VARIANT_I2C_BUS  0
#define AICSHIELD_I2C_BUS_SET_INTERNAL	4


class AICShieldPins {
	public:
		AICShieldPins(void) {
			setAICShieldRev(TympanRev::D, AICShieldRev::A);  //assume RevD is default
		};
		AICShieldPins(TympanRev _tympanRev, AICShieldRev _AICRev) {
			setAICShieldRev(_tympanRev, _AICRev);
		}

		void setAICShieldRev(TympanRev _tympanRev, AICShieldRev _AICRev) {
			tympanRev = _tympanRev;
			AICRev = _AICRev;
			
			switch (tympanRev) { //which Tympan are we connecting to?

				case (TympanRev::D) : case (TympanRev::D4) :   //we're connecting to a Rev D tympan, so the pin numbers below are correct for the TympanD

					switch (AICRev) {  //which AIC_shield are we connecting to?

						case (AICShieldRev::A) :    //Basic AIC shield (2019 and 2020)
							//Teensy 3.6 Pin Numbering
							resetAIC = AICSHIELD_DEFAULT_RESET_PIN; //Fixed 2020-06-01
							i2cBus = 2;
							enableStereoExtMicBias = 41;
							defaultInput = AudioControlAIC3206::IN3;  //IN3 is the pink mic/line jack
							break;

						case (AICShieldRev::CCP):  case (AICShieldRev::CCP_A): //First generation CCP shield (May 2020)
							//Teensy 3.6 Pin Numbering
							resetAIC = AICSHIELD_DEFAULT_RESET_PIN;   //Fixed 2020-06-01
							pinMode(7,INPUT_PULLUP); delay(10); int pinValue = digitalRead(7);
							if(pinValue == 1){	// production Rev D
								i2cBus = 2;
							} else {					// prototype Rev D with op amps
								i2cBus = 0;
								resetAIC = 35;
							}
							Serial.print("Shield I2C Select: "); Serial.println(pinValue+1);
							// i2cBus = 2;
							enableStereoExtMicBias = 41;
							CCP_atten1 = 52;  //enable attenuator #1.  Same as MOSI_2 (alt)
							CCP_atten2 = 51;  //enable attenuator #2.  Same as MISO_2 (alt)
							CCP_bigLED =  53;    //same as SCK_2 (alt)
							CCP_littleLED_1 = 41;    //same as AIC_Shield_enableStereoExtMicBias
							CCP_littleLED_2 = 8;     //prototype board variant uses TX_3 pin
							CCP_enable28V = 5; //enable the 28V power supply.  Same as SS_2
							defaultInput = AudioControlAIC3206::IN3;  //IN3 are the screw jacks
							break;
					}
					break;

				case  (TympanRev::E) : case (TympanRev::E1) :  //we're connecting to a Rev E tympan, so the pin numbers below are correct for the TympanE

					switch (AICRev) {  //which AIC_shield are we connecting to?

						case (AICShieldRev::A) :    //Basic AIC shield (2019 and 2020)
							//Teensy 4.1 Pin Numbering
							resetAIC = 31;
							i2cBus = 2;
							enableStereoExtMicBias = 29;
							defaultInput = AudioControlAIC3206::IN3;  //IN3 is the pink mic/line jack
							break;

						case (AICShieldRev::CCP):  case (AICShieldRev::CCP_A): //First generation CCP shield (May 2020)
							//Teensy 4.1 Pin Numbering
							resetAIC = 31;
							i2cBus = 2;
							enableStereoExtMicBias =29;
							CCP_atten1 = 52;  //enable attenuator #1.  Same as MOSI_2 (alt)...NEED TO UPDATE!!!
							CCP_atten2 = 51;  //enable attenuator #2.  Same as MISO_2 (alt)...NEED TO UPDATE!!!
							CCP_bigLED =  53;    //same as SCK_2 (alt)...NEED TO UPDATE!!!
							CCP_littleLED_1 = 41;    //same as AIC_Shield_enableStereoExtMicBias...NEED TO UPDATE!!!
							CCP_littleLED_2 = 8;     //prototype board variant uses TX_3 pin
							CCP_enable28V = 5; //enable the 28V power supply.  Same as SS_2...NEED TO UPDATE!!!
							defaultInput = AudioControlAIC3206::IN3;  //IN3 are the screw jacks
							break;
					}
					break;

				default:
					Serial.println("AICSheildPins: *** WARNING *** This Teensy Rev is not supported.");
					Serial.println("    : Assuming defaults and hoping for the best.");
					break;
			}
		}
		//#if defined(SEREMU_INTERFACE)
		//	usb_seremu_class * getUSBSerial(void) { return USB_Serial; }
		//#else
		//	usb_serial_class * getUSBSerial(void) { return USB_Serial; }
		//#endif
		//HardwareSerial * getBTSerial(void) { return BT_Serial; }

		//Defaults
		TympanRev tympanRev = TympanRev::D;
		AICShieldRev AICRev = AICShieldRev::A;
 		int resetAIC = AICSHIELD_DEFAULT_RESET_PIN;
		int i2cBus = AICSHIELD_DEFAULT_I2C_BUS;
		int enableStereoExtMicBias = NOT_A_FEATURE;
		int CCP_atten1 = NOT_A_FEATURE, CCP_atten2 = NOT_A_FEATURE;
		int CCP_bigLED = NOT_A_FEATURE, CCP_littleLED_1 = NOT_A_FEATURE, CCP_littleLED_2 = NOT_A_FEATURE;
		int CCP_enable28V = NOT_A_FEATURE;
		int defaultInput = NOT_A_FEATURE;


};


class AICShieldBase : public AudioControlAIC3206
{
	public:
		AICShieldBase(void) :
			AudioControlAIC3206(pins.resetAIC, pins.i2cBus) {
		}
		AICShieldBase(bool _debugToSerial) :
			AudioControlAIC3206(pins.resetAIC, pins.i2cBus,_debugToSerial) {
		}
		AICShieldBase(const AICShieldPins &_pins) :
			AudioControlAIC3206(_pins.resetAIC,_pins.i2cBus) {
			setupPins(_pins);
		}
		AICShieldBase(const AICShieldPins &_pins, bool _debugToSerial) :
			AudioControlAIC3206(_pins.resetAIC,_pins.i2cBus,_debugToSerial) {
			setupPins(_pins);
		}
		AICShieldBase(const AudioSettings_F32 &_as) :
			AudioControlAIC3206(pins.resetAIC, pins.i2cBus) {
			setAudioSettings(_as);
		}
		AICShieldBase(const AICShieldPins &_pins, const AudioSettings_F32 &_as) :
			AudioControlAIC3206(_pins.resetAIC,_pins.i2cBus) {
			setupPins(_pins);
			setAudioSettings(_as);
		}
		AICShieldBase(const AICShieldPins &_pins, const AudioSettings_F32 &_as, bool _debugToSerial) :
			AudioControlAIC3206(_pins.resetAIC,_pins.i2cBus,_debugToSerial) {
			setupPins(_pins);
			setAudioSettings(_as);
		}

		void setupPins(const AICShieldPins &_pins);
		void setAudioSettings(const AudioSettings_F32 &_aud_set) { audio_settings = _aud_set; }  //shallow copy
		virtual bool enable(void) {
			bool foo = AudioControlAIC3206::enable();
			if (pins.defaultInput != NOT_A_FEATURE)	inputSelect(pins.defaultInput);
			return foo;
		}


		//TympanPins getTympanPins(void) { return &pins; }
		int setBigLED_CCP(int _value) { if (pins.CCP_bigLED != NOT_A_FEATURE) { digitalWrite(pins.CCP_bigLED,_value); return _value; } return NOT_A_FEATURE;}
		int setLittleLED_CCP(int _value) {
			if (pins.CCP_littleLED_1 != NOT_A_FEATURE) {
				digitalWrite(pins.CCP_littleLED_1,_value);
				digitalWrite(pins.CCP_littleLED_2,_value);
				return _value;
			}
			return NOT_A_FEATURE;
		}
		int enable28V_CCP(int _value) { if (pins.CCP_enable28V != NOT_A_FEATURE) { digitalWrite(pins.CCP_enable28V,_value); return _value; } return NOT_A_FEATURE; }
		int enableCCPAtten1(int _value) { if (pins.CCP_atten1 != NOT_A_FEATURE) { digitalWrite(pins.CCP_atten1,_value); return _value; } return NOT_A_FEATURE; }
		int enableCCPAtten2(int _value) { if (pins.CCP_atten2 != NOT_A_FEATURE) { digitalWrite(pins.CCP_atten2,_value); return _value; } return NOT_A_FEATURE; }

		int setEnableStereoExtMicBias(int new_state);

		TympanRev getTympanRev(void) { return pins.tympanRev; }
		AICShieldRev getAICRev(void) { return pins.AICRev; }


	protected:
		AICShieldPins pins;
		AudioSettings_F32 audio_settings;


};


class AICShield : public AICShieldBase {
	public:
		AICShield(void) : AICShieldBase() {
			AICShieldPins myPins;  //assumes defaults
			AICShieldBase::setupPins(myPins);
		}
		AICShield(const TympanRev &_myRev, const AICShieldRev &_aicRev) : 
			AICShieldBase(AICShieldPins(_myRev,_aicRev)) {
		}
		AICShield(const TympanRev &_myRev, const AICShieldRev &_aicRev, bool _debugToSerial) :
			AICShieldBase(AICShieldPins(_myRev,_aicRev),_debugToSerial) {
		}
		AICShield(const TympanRev &_myRev, const AICShieldRev &_aicRev, const AudioSettings_F32 &_as) : 
			AICShieldBase(AICShieldPins(_myRev,_aicRev),_as) {
		}
		AICShield(const TympanRev &_myRev, const AICShieldRev &_aicRev, const AudioSettings_F32 &_as, bool _debugToSerial) : 
			AICShieldBase(AICShieldPins(_myRev,_aicRev),_as,_debugToSerial) {
		}
};

class EarpieceShield : public AICShield {
	public:
		EarpieceShield(void) :
			AICShield() {
		}
		EarpieceShield(const TympanRev &_myRev, const AICShieldRev &_aicRev) :
			AICShield(_myRev, _aicRev) {
		}
		EarpieceShield(const TympanRev &_myRev, const AICShieldRev &_aicRev, bool _debugToSerial) :
			AICShield(_myRev, _aicRev, _debugToSerial) {
		}
		EarpieceShield(const TympanRev &_myRev, const AICShieldRev &_aicRev, const AudioSettings_F32 &_as) :
			AICShield(_myRev, _aicRev, _as) { ;
		}
		EarpieceShield(const TympanRev &_myRev, const AICShieldRev &_aicRev, const AudioSettings_F32 &_as, bool _debugToSerial) :
			AICShield(_myRev, _aicRev,_as, _debugToSerial) {
		}


		//variables to simplify access to the Earpiece channel mapping...these are overwriten in setEarpiceValues()
		//what I2S channel maps to what input or what output
		static const int PDM_LEFT_FRONT, PDM_LEFT_REAR, PDM_RIGHT_FRONT, PDM_RIGHT_REAR;  //Front/Rear is weird.  Left/Right matches the enclosure labeling		.
		static const int  OUTPUT_LEFT_TYMPAN, OUTPUT_RIGHT_TYMPAN;  //left/right for headphone jack on main tympan board
		static const int  OUTPUT_LEFT_EARPIECE, OUTPUT_RIGHT_EARPIECE;  //Left/Right matches the enclosure...but is backwards from ideal
};

#endif
