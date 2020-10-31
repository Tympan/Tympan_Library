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
#define TYMPAN_INPUT_LINE_IN            (AudioControlAIC3206::IN1)   //uses IN1, on female Arduino-style headers (shared with BT Audio)
#define TYMPAN_INPUT_BT_AUDIO	        (AudioControlAIC3206::IN1)   //uses IN1, for Bluetooth Audio (ahred with LINE_IN)
#define TYMPAN_INPUT_ON_BOARD_MIC       (AudioControlAIC3206::IN2)   //uses IN2, for analog signals from microphones on PCB
#define TYMPAN_INPUT_JACK_AS_LINEIN     (AudioControlAIC3206::IN3)   //uses IN3, for analog signals from mic jack, no mic bias 
#define TYMPAN_INPUT_JACK_AS_MIC        (AudioControlAIC3206::IN3_wBIAS)   //uses IN3, for analog signals from mic jack, with mic bias


//convenience names to use with outputSelect()
#define TYMPAN_OUTPUT_HEADPHONE_JACK_OUT 1  //DAC left and right to headphone left and right
#define TYMPAN_OUTPUT_LINE_OUT 2 //DAC left and right to lineout left and right
#define TYMPAN_OUTPUT_HEADPHONE_AND_LINE_OUT 3  //DAC left and right to both headphone and line out 
#define TYMPAN_OUTPUT_LEFT2DIFFHP_AND_R2DIFFLO 4 //DAC left to differential headphone, DAC right to line out

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
	enum INPUTS {IN1 = 0, IN2, IN3, IN3_wBIAS};
	virtual bool enable(void);
	virtual bool disable(void);
	bool outputSelect(int n, bool flag_full = true); //flag_full is whether to do a full reconfiguration.  True is more complete but false is faster. 
	bool volume(float n);
	static float applyLimitsOnVolumeSetting(float vol_dB);
	float volume_dB(float vol_dB);  //set both channels to the same volume
	float volume_dB(float vol_left_dB, float vol_right_dB); //set both channels, but to their own values
	float volume_dB(float vol_left_dB, int chan); //set each channel seperately (0 = left; 1 = right)
	bool inputLevel(float n);  //dummy to be compatible with Teensy Audio Library
	bool inputSelect(int n);
	float applyLimitsOnInputGainSetting(float gain_dB);  
	float setInputGain_dB(float gain_dB);   //set both channels to the same gain
	float setInputGain_dB(float gain_dB, int chan); //set each channel seperately (0 = left; 1 = right)
	bool setMicBias(int n);
	bool updateInputBasedOnMicDetect(int setting = TYMPAN_INPUT_JACK_AS_MIC);
	bool enableMicDetect(bool);
	int  readMicDetect(void);
	bool debugToSerial;
    unsigned int aic_readPage(uint8_t page, uint8_t reg);
    bool aic_writePage(uint8_t page, uint8_t reg, uint8_t val);
	
	void setHPFonADC(bool enable, float cutoff_Hz, float fs_Hz); //first-order HP applied within this 3206 hardware, ADC (input) side
	float getHPCutoff_Hz(void) { return HP_cutoff_Hz; }
	void setHpfIIRCoeffOnADC(int chan, uint32_t *coeff); //alternate way of settings the same 1st-order HP filter
	float setBiquadOnADC(int type, float cutoff_Hz, float sampleRate_Hz, int chan, int biquadIndex); //lowpass applied within 3206 hardware, ADC (input) side
	int setBiquadCoeffOnADC(int chanIndex, int biquadIndex, uint32_t *coeff_uint32);
	void writeBiquadCoeff(uint32_t *coeff_uint32, int *page_reg_table, int table_ncol);
	
	float getSampleRate_Hz(void) { return sample_rate_Hz; }
	bool enableAutoMuteDAC(bool, uint8_t);
	bool mixInput1toHPout(bool state);
	void muteLineOut(bool state);
	bool enableDigitalMicInputs(void) { return enableDigitalMicInputs(true); }
	bool enableDigitalMicInputs(bool desired_state);
	
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
  bool aic_writeRegister(uint8_t reg, uint8_t val);  //assumes page has already been set
  int prevMicDetVal = -1;
  int resetPinAIC = AIC3206_DEFAULT_RESET_PIN;  //AIC reset pin, Rev C
  float HP_cutoff_Hz = 0.0f;
  float sample_rate_Hz = 44100; //only used with HP_cutoff_Hz to design HP filter on ADC, if used
  void setHpfIIRCoeffOnADC_Left(uint32_t *coeff);
  void setHpfIIRCoeffOnADC_Right(uint32_t *coeff);

  void computeFirstOrderHPCoeff_f32(float cutoff_Hz, float fs_Hz, float *coeff);
  //void computeFirstOrderHPCoeff_i32(float cutoff_Hz, float fs_Hz, int32_t *coeff);
  void computeBiquadCoeff_LP_f32(float cutoff_Hz, float sampleRate_Hz, float q, float *coeff);
  void computeBiquadCoeff_HP_f32(float cutoff_Hz, float sampleRate_Hz, float q, float *coeff);
  void convertCoeff_f32_to_i32(float *coeff_f32, int32_t *coeff_i32, int ncoeff);

  
};


#endif
