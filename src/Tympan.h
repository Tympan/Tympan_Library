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
#include <Print.h>

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
					BT_nReset = 6; //
					BT_REGEN = NOT_A_FEATURE;
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
					BT_REGEN = NOT_A_FEATURE;
					BT_PIO4 = 2;  //PTD0
					enableStereoExtMicBias = NOT_A_FEATURE; //mic jack is already mono, can't do stereo.
					break;
				case (TYMPAN_REV_D) :
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
			}
		}
		usb_serial_class * getUSBSerial(void) { return USB_Serial; }
		HardwareSerial * getBTSerial(void) { return BT_Serial; }
		
		//Defaults (Teensy 3.6 Pin Numbering), assuming Rev C
		int tympanRev = TYMPAN_REV_C;
 		int resetAIC = 21;  //PTD6
		int potentiometer = 15;  //PTC0
		int amberLED = 36;  //PTC9
		int redLED = 35;  //PTC8
		int BT_nReset = 6; //PTD4
		int BT_REGEN = NOT_A_FEATURE;  
		int BT_PIO4 = 2;  //PTD0
		bool reversePot = false;
		int enableStereoExtMicBias = NOT_A_FEATURE;
		usb_serial_class *USB_Serial = &Serial; //true for Rev_A/C/D
		HardwareSerial *BT_Serial = &Serial1; //true for Rev_A/C/D
		int BT_serial_speed = 115200; //true for Rev_A/C
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

//include "utility/TympanPrint.h"

class TympanBase : public AudioControlTLV320AIC3206, public Print
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
			
			//get the comm pins and setup the regen and reset pins
			USB_Serial = pins.getUSBSerial();
			BT_Serial = pins.getBTSerial();
			if (pins.BT_REGEN != NOT_A_FEATURE) {
				pinMode(pins.BT_REGEN,OUTPUT);digitalWrite(pins.BT_REGEN,HIGH); //pull high for normal operation
				delay(10);  digitalWrite(pins.BT_REGEN,LOW); //hold at least 5 msec, then return low
				
			}
			if (pins.BT_nReset != NOT_A_FEATURE) {
				pinMode(pins.BT_nReset,OUTPUT);
				digitalWrite(pins.BT_nReset,LOW);delay(1); //reset the device
				digitalWrite(pins.BT_nReset,HIGH);  //normal operation.
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
				case (TYMPAN_REV_D) :
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

		//using TympanPrint::print;
		//using TympanPrint::println;
		//virtual size_t print(const char *s) { return write(s); }  //should use the faster write
		//virtual size_t println(const char *s) { return print(s) + println(); }  //should use the faster write
		//virtual size_t println(void) { 	uint8_t buf[2]={'\r', '\n'}; return write(buf, 2); }

	protected:
		TympanPins pins;
		usb_serial_class *USB_Serial;
		HardwareSerial *BT_Serial;
		
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