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

//the Tympan is a Teensy audio library "control" object
#include "control_aic3206.h"  //see in here for more #define statements that are very relevant!
#include <Arduino.h>  //for the Serial objects
#include <Print.h>

#define NOT_A_FEATURE (-9999)

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
				case (TympanRev::D3) : case (TympanRev::D) :   //Built for OpenTact
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
				case (TympanRev::D4) :   //RevD release candidate
					//Teensy 3.6 Pin Numbering
					resetAIC = 35;  //PTC8
					potentiometer = 39; //A20
					amberLED = 36; //PTC9
					redLED = 10;  //PTC4
					BT_nReset = 34;  //PTE25, active LOW reset
					BT_REGEN = 31;  //must pull high to enable BC127
					BT_PIO0 = 56;   //hard reset for the BT module if HIGH at start.  Otherwise, outputs the connection state
					BT_PIO4 = 33;  //PTE24...actually it's BT_PIO5 ???
					enableStereoExtMicBias = 20; //PTD5
					AIC_Shield_enableStereoExtMicBias = 41;
					BT_serial_speed = 9600;
					Rev_Test = 44;
					break;
			}
		}
		usb_serial_class * getUSBSerial(void) { return USB_Serial; }
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
		int AIC_Shield_enableStereoExtMicBias = NOT_A_FEATURE;
		usb_serial_class *USB_Serial = &Serial; //true for Rev_A/C/D
		HardwareSerial *BT_Serial = &Serial1; //true for Rev_A/C/D
		int BT_serial_speed = 115200; //true for Rev_A/C
};

//include "utility/TympanPrint.h"
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


		void setupPins(const TympanPins &_pins) {
			AudioControlAIC3206::setResetPin(_pins.resetAIC);
			pins = _pins; //shallow copy to local version

			//Serial.print("TympanBase: setupPins: pins.potentiometer, given / act: ");
			//Serial.print(_pins.potentiometer); Serial.print(" / "); Serial.println(pins.potentiometer);

			pinMode(pins.potentiometer,INPUT);
			pinMode(pins.amberLED,OUTPUT); digitalWrite(pins.amberLED,LOW);
			pinMode(pins.redLED,OUTPUT); digitalWrite(pins.redLED,LOW);
			if (pins.enableStereoExtMicBias != NOT_A_FEATURE) {
				pinMode(pins.enableStereoExtMicBias,OUTPUT);
				setEnableStereoExtMicBias(false); //enable stereo external mics (REV_D)
			}

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
		void forceBTtoDataMode(bool state) {
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
		TympanRev getTympanRev(void) { return pins.tympanRev; }
		int getPotentiometerPin(void) { return pins.potentiometer; }
		usb_serial_class *getUSBSerial(void) { return USB_Serial; }
		HardwareSerial *getBTSerial(void) { return BT_Serial; }
		void beginBothSerial(void) { beginBothSerial(115200, pins.BT_serial_speed); }
		void beginBothSerial(int USB_speed, int BT_speed) {
			USB_Serial->begin(USB_speed); delay(50);
			beginBluetoothSerial(BT_speed);
		}
		int USB_dtr() { return USB_Serial->dtr(); }

		void beginBluetoothSerial(void) { beginBluetoothSerial(pins.BT_serial_speed); }
		void beginBluetoothSerial(int BT_speed) {
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
		void clearAndConfigureBTSerialRevD(void) {
		   //clear out any text that is waiting
			//Serial.println("Clearing BT serial buffer...");
			delay(500);
			while(BT_Serial->available()) {
			  //Serial.print((char)BT_SERIAL.read());
			  BT_Serial->read(); delay(5);
			}

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

		bool mixBTAudioWithOutput(bool state) { return mixInput1toHPout(state); } //bluetooth audio is on Input1
		void echoIncomingBTSerial(void) {
			while (BT_Serial->available()) USB_Serial->write(BT_Serial->read());//echo messages from BT serial over to USB Serial
		}
		void setBTAudioVolume(int vol) {  //only works when you are connected via Bluetooth!!!!
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
				}
		}


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

		usb_serial_class *USB_Serial;
		HardwareSerial *BT_Serial;

	protected:
		TympanPins pins;

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
};

#endif
