/*
	control_tlv320aic3206

	Created: Brendan Flynn (http://www.flexvoltbiosensor.com/) for Tympan, Jan-Feb 2017
	Purpose: Control module for Texas Instruments TLV320AIC3206 compatible with Teensy Audio Library

	License: MIT License.  Use at your own risk.
 */

#ifndef control_aic3206_h_
#define control_aic3206_h_

#include "TeensyAudioControl.h"
#include <Arduino.h>
#include <Wire.h>  //for using multiple Teensy I2C busses simultaneously


//convenience names to use with inputSelect() to set whnch analog inputs to use
#define TYMPAN_INPUT_LINE_IN            1   //uses IN1
#define TYMPAN_INPUT_ON_BOARD_MIC       2   //uses IN2 analog inputs
#define TYMPAN_INPUT_JACK_AS_LINEIN     3   //uses IN3 analog inputs
#define TYMPAN_INPUT_JACK_AS_MIC        4   //uses IN3 analog inputs *and* enables mic bias

//convenience names to use with outputSelect()
#define TYMPAN_OUTPUT_HEADPHONE_JACK_OUT 1
#define TYMPAN_OUTPUT_LINE_OUT 2
#define TYMPAN_OUTPUT_HEADPHONE_AND_LINE_OUT 3

//names to use with setMicBias() to set the amount of bias voltage to use
#define TYMPAN_MIC_BIAS_OFF             0
#define TYMPAN_MIC_BIAS_1_25            1
#define TYMPAN_MIC_BIAS_1_7             2
#define TYMPAN_MIC_BIAS_2_5             3
#define TYMPAN_MIC_BIAS_VSUPPLY         4
#define TYMPAN_DEFAULT_MIC_BIAS TYMPAN_MIC_BIAS_2_5

#define BOTH_CHAN 0
#define LEFT_CHAN 1
#define RIGHT_CHAN 2

//define AIC3206_DEFAULT_I2C_BUS 0   //bus zero is &Wire
#define AIC3206_DEFAULT_RESET_PIN 21

class AudioControlAIC3206: public TeensyAudioControl
{
public:
	//GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
	AudioControlAIC3206(void) {   //specify nothing
		//setI2Cbus(AIC3206_DEFAULT_I2C_BUS);
		debugToSerial = false; 
	}
	AudioControlAIC3206(bool _debugToSerial) {  //specify debug
		//setI2Cbus(AIC3206_DEFAULT_I2C_BUS);
		debugToSerial = _debugToSerial;		
	}
	AudioControlAIC3206(int _resetPin) {  //specify reset pin (minimum recommended!)
		resetPinAIC = _resetPin; 
		//setI2Cbus(AIC3206_DEFAULT_I2C_BUS);
		debugToSerial = false; 
	}	
	AudioControlAIC3206(int _resetPin, int i2cBusIndex) {  //specify reset pin and i2cBus (minimum if using for 2nd AIC)
		setResetPin(_resetPin); 
		setI2Cbus(i2cBusIndex);
		debugToSerial = false; 
	}
	AudioControlAIC3206(int _resetPin, int i2cBusIndex, bool _debugToSerial) {  //specify everything
		setResetPin(_resetPin); 
		setI2Cbus(i2cBusIndex);
		debugToSerial = _debugToSerial;
	}
	bool enable(void);
	bool disable(void);
	bool outputSelect(int n);
	bool volume(float n);
	bool volume_dB(float n);
	bool inputLevel(float n);  //dummy to be compatible with Teensy Audio Library
	bool inputSelect(int n);
	bool setInputGain_dB(float n);
	bool setMicBias(int n);
	bool updateInputBasedOnMicDetect(int setting = TYMPAN_INPUT_JACK_AS_MIC);
	bool enableMicDetect(bool);
	int  readMicDetect(void);
	bool debugToSerial;
    unsigned int aic_readPage(uint8_t page, uint8_t reg);
    bool aic_writePage(uint8_t page, uint8_t reg, uint8_t val);
	void setHPFonADC(bool enable, float cutoff_Hz, float fs_Hz);
	float getHPCutoff_Hz(void) { return HP_cutoff_Hz; }
	float getSampleRate_Hz(void) { return sample_rate_Hz; }
	void setIIRCoeffOnADC(int chan, uint32_t *coeff);
	bool enableAutoMuteDAC(bool, uint8_t);
	bool mixInput1toHPout(bool state);
	bool enableDigitalMicInputs(void) { return enableDigitalMicInputs(true); }
	bool enableDigitalMicInputs(bool desired_state);
	void computeFirstOrderHPCoeff_f32(float cutoff_Hz, float fs_Hz, float *coeff);
	void computeFirstOrderHPCoeff_i32(float cutoff_Hz, float fs_Hz, int32_t *coeff);
protected:
  TwoWire *myWire = &Wire;  //from Wire.h
  void setI2Cbus(int i2cBus);
  void aic_reset(void);
  void aic_init(void);
  void aic_initDAC(void);
  void aic_initADC(void);
  void setResetPin(int pin) { resetPinAIC = pin; }
  
  bool aic_writeAddress(uint16_t address, uint8_t val);
  bool aic_goToPage(uint8_t page);
  int prevMicDetVal = -1;
  int resetPinAIC = AIC3206_DEFAULT_RESET_PIN;  //AIC reset pin, Rev C
  float HP_cutoff_Hz = 0.0f;
  float sample_rate_Hz = 44100; //only used with HP_cutoff_Hz to design HP filter on ADC, if used
  void setIIRCoeffOnADC_Left(uint32_t *coeff);
  void setIIRCoeffOnADC_Right(uint32_t *coeff);
  
};


#endif
