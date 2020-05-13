
#include "Tympan.h"

void TympanBase::setupPins(const TympanPins &_pins) {
	AudioControlAIC3206::setResetPin(_pins.resetAIC);
	pins = _pins; //shallow copy to local version

	//Serial.print("TympanBase: setupPins: pins.potentiometer, given / act: ");
	//Serial.print(_pins.potentiometer); Serial.print(" / "); Serial.println(pins.potentiometer);

	//setup the CCP related pins
	//if (pins.CCP_enable28V != NOT_A_FEATURE) { pinMode(pins.CCP_enable28V,OUTPUT);  setCCPEnable28V(LOW); }
	//if (pins.CCP_atten1 != NOT_A_FEATURE) { pinMode(pins.CCP_atten1,OUTPUT); digitalWrite(pins.CCP_atten1,LOW); }
	//if (pins.CCP_atten2 != NOT_A_FEATURE) { pinMode(pins.CCP_atten2,OUTPUT); digitalWrite(pins.CCP_atten2,LOW); }
	//if (pins.CCP_bigLED != NOT_A_FEATURE) { pinMode(pins.CCP_bigLED,OUTPUT); digitalWrite(pins.CCP_bigLED,LOW); }
	//if (pins.CCP_littleLED != NOT_A_FEATURE) { pinMode(pins.CCP_littleLED,OUTPUT); digitalWrite(pins.CCP_littleLED,LOW); }
	
	//setup the Tympan pins]
	pinMode(pins.potentiometer,INPUT);
	pinMode(pins.amberLED,OUTPUT); digitalWrite(pins.amberLED,LOW);
	pinMode(pins.redLED,OUTPUT); digitalWrite(pins.redLED,LOW);
	if (pins.enableStereoExtMicBias != NOT_A_FEATURE) {
		pinMode(pins.enableStereoExtMicBias,OUTPUT);
		setEnableStereoExtMicBias(true); //enable stereo external mics (REV_D)
	}
	
	//setup the AIC shield pins
	//if (pins.AIC_Shield_enableStereoExtMicBias != NOT_A_FEATURE) {
	//	pinMode(pins.AIC_Shield_enableStereoExtMicBias,OUTPUT);
	//	setEnableStereoExtMicBiasAIC(true); //enable stereo external mics (REV_D)
	//}
	

	//get the comm pins and setup the regen and reset pins
	USB_Serial = pins.getUSBSerial();
	BT_Serial = pins.getBTSerial();
	if (pins.BT_REGEN != NOT_A_FEATURE) {
		pinMode(pins.BT_REGEN,OUTPUT);digitalWrite(pins.BT_REGEN,HIGH); //pull high for normal operation
		delay(10);  digitalWrite(pins.BT_REGEN,LOW); //hold at least 5 msec, then return low

	}
	if (pins.BT_nReset != NOT_A_FEATURE) {
		pinMode(pins.BT_nReset,OUTPUT);
		digitalWrite(pins.BT_nReset,LOW);delay(10); //reset the device
		digitalWrite(pins.BT_nReset,HIGH);  //normal operation.
	}
	if (pins.BT_PIO0 != NOT_A_FEATURE) {
		pinMode(pins.BT_PIO0,INPUT);
	}

	forceBTtoDataMode(true);
};

void TympanBase::forceBTtoDataMode(bool state) {
	if (pins.BT_PIO4 != NOT_A_FEATURE) {
		if (state == true) {
			pinMode(pins.BT_PIO4,OUTPUT);
			digitalWrite(pins.BT_PIO4,HIGH);
		} else {
			//pinMode(pins.BT_PIO4,INPUT);  //go high-impedance (ie disable this pin)
			pinMode(pins.BT_PIO4,OUTPUT);
			digitalWrite(pins.BT_PIO4,LOW);
		}
	}
}

int TympanBase::setEnableStereoExtMicBias(int new_state) {
	if (pins.enableStereoExtMicBias != NOT_A_FEATURE) {
		digitalWrite(pins.enableStereoExtMicBias,new_state);
		return new_state;
	} else {
		return pins.enableStereoExtMicBias;
	}
}

/*
int TympanBase::setEnableStereoExtMicBiasAIC(int new_state) {  //for AIC Shield
	if (pins.AIC_Shield_enableStereoExtMicBias != NOT_A_FEATURE) {
		digitalWrite(pins.AIC_Shield_enableStereoExtMicBias,new_state);
		return new_state;
	} else {
		return pins.AIC_Shield_enableStereoExtMicBias;
	}
}
*/	

void TympanBase::beginBluetoothSerial(int BT_speed) {
	BT_Serial->begin(BT_speed);

	switch (getTympanRev()) {
		case (TYMPAN_REV_D) : case (TYMPAN_REV_D0) : case (TYMPAN_REV_D1) : case (TYMPAN_REV_D2) : case (TYMPAN_REV_D3) :
			clearAndConfigureBTSerialRevD();
			break;
		default:
			delay(50);
			break;
	}
}

void TympanBase::clearAndConfigureBTSerialRevD(void) {
   //clear out any text that is waiting
	delay(500);  //do this to wait for BT serial to come alive upon startup.  Is there a bitter way?
	while(BT_Serial->available()) { BT_Serial->read(); delay(5); }

	//transition to data mode
	//Serial.println("Transition BT to data mode");
	BT_Serial->print("ENTER_DATA");BT_Serial->write(0x0D); //enter data mode.  Finish with carraige return
	delay(100);
	int count = 0;
	while ((count < 3) & (BT_Serial->available())) { //we should receive on "OK"
	  //Serial.print((char)BT_SERIAL.read());
	  BT_Serial->read(); count++;  delay(5);
	}
	//Serial.println("BT Should be ready.");
}

void TympanBase::setBTAudioVolume(int vol) {  //only works when you are connected via Bluetooth!!!!
	//vol is 0 (min) to 15 (max)
	if (pins.tympanRev >= TYMPAN_REV_D0) {   //This is only for the BC127 BT module
		//get into command mode
		USB_Serial->println("*** Changing BT into command mode...");
		forceBTtoDataMode(false); //un-forcing (via hardware pin) the BT device to be in data mode
		delay(500);
		BT_Serial->print("$");  delay(400);
		BT_Serial->print("$$$");  delay(400);
		delay(2000);
		echoIncomingBTSerial();

		// clear the buffer by forcing an error
		USB_Serial->println("*** Clearing buffers.  Sending carraige return.");
		BT_Serial->print('\r'); delay(500);
		echoIncomingBTSerial();
		USB_Serial->println("*** Should have gotten 'ERROR', which is fine here.");

		//check A2DP volume
		USB_Serial->println("*** Check A2DP and HFP Volume...");
		BT_Serial->print("VOLUME A2DP"); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();
		BT_Serial->print("VOLUME HFP"); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();

		//change volume
		USB_Serial->println("*** Setting A2DP Volume...");
		BT_Serial->print("VOLUME A2DP="); BT_Serial->print(vol); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();
		BT_Serial->print("VOLUME HFP="); BT_Serial->print(vol); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();

		//check A2DP volume again
		USB_Serial->println("*** Check A2DP and HFP Volume again...");
		BT_Serial->print("VOLUME A2DP"); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();
		BT_Serial->print("VOLUME HFP"); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();

		//confirm GPIO State
		USB_Serial->println("*** Setting GPIOCONTROL Mode...");
		BT_Serial->print("SET GPIOCONTROL=OFF");BT_Serial->print('\r'); delay(500);
		echoIncomingBTSerial();

		//save
		USB_Serial->println("*** Saving Settings...");
		BT_Serial->print("WRITE"); BT_Serial->print('\r'); delay(500);

		USB_Serial->println("*** Changing into transparanet data mode...");
		BT_Serial->print("ENTER_DATA");BT_Serial->print('\r'); delay(500);
		echoIncomingBTSerial();
		forceBTtoDataMode(true); //forcing (via hardware pin) the BT device to be in data mode
	} else {
		//need code for other models of Tympan
	}
}