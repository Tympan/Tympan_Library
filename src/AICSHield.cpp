
#include "AICShield.h"

//set the static variables

//Front/Rear is weird.  Left/Right matches the enclosure labeling.
const int EarpieceShield::PDM_LEFT_FRONT = 1;
const int EarpieceShield::PDM_LEFT_REAR = 0;    
const int EarpieceShield::PDM_RIGHT_FRONT = 3;
const int EarpieceShield::PDM_RIGHT_REAR = 2;

 //left/right for headphone jack on main tympan board
const int EarpieceShield::OUTPUT_LEFT_TYMPAN = 0;
const int EarpieceShield::OUTPUT_RIGHT_TYMPAN = 1;  
		
//Left/Right matches the enclosure...but is backwards from ideal
const int EarpieceShield::OUTPUT_LEFT_EARPIECE = 3;
const int EarpieceShield::OUTPUT_RIGHT_EARPIECE = 2;  


void AICShieldBase::setupPins(const AICShieldPins &_pins) {
	AudioControlAIC3206::setResetPin(_pins.resetAIC);
	pins = _pins; //shallow copy to local version

	//setup the CCP related pins
	if (pins.CCP_enable28V != NOT_A_FEATURE) { pinMode(pins.CCP_enable28V,OUTPUT);  enable28V_CCP(LOW); }
	if (pins.CCP_atten1 != NOT_A_FEATURE) { pinMode(pins.CCP_atten1,OUTPUT); digitalWrite(pins.CCP_atten1,LOW); }
	if (pins.CCP_atten2 != NOT_A_FEATURE) { pinMode(pins.CCP_atten2,OUTPUT); digitalWrite(pins.CCP_atten2,LOW); }
	if (pins.CCP_bigLED != NOT_A_FEATURE) { pinMode(pins.CCP_bigLED,OUTPUT); digitalWrite(pins.CCP_bigLED,LOW); }
	if (pins.CCP_littleLED != NOT_A_FEATURE) { pinMode(pins.CCP_littleLED,OUTPUT); digitalWrite(pins.CCP_littleLED,LOW); }
	
	
	
	//setup the AIC shield pins
	if (pins.enableStereoExtMicBias != NOT_A_FEATURE) {
		pinMode(pins.enableStereoExtMicBias,OUTPUT);
		setEnableStereoExtMicBias(true); //enable stereo external mics (REV_D)
	}
	
};

int AICShieldBase::setEnableStereoExtMicBias(int new_state) {
	if (pins.enableStereoExtMicBias != NOT_A_FEATURE) {
		digitalWrite(pins.enableStereoExtMicBias,new_state);
		return new_state;
	} else {
		return pins.enableStereoExtMicBias;
	}
}


