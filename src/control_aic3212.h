/*
    control_tlv320aic3212

    Created: Eric Yuan for Tympan, 7/2/2021
    Purpose: Control module for Texas Instruments TLV320AIC3212 compatible with Teensy Audio Library

    License: MIT License.  Use at your own risk.
 */

#ifndef control_aic3212_h_
#define control_aic3212_h_

#include "TeensyAudioControl.h"
#include <Arduino.h>
#include <Wire.h> //for using multiple Teensy I2C busses simultaneously

namespace tlv320aic3212
{

    // //convenience names to use with inputSelect() to set whnch analog inputs to use
    // #define AIC3212_INPUT_LINE_IN            (AudioControlAIC3212::IN1)   //uses IN1, on female Arduino-style headers (shared with BT Audio)
    // #define AIC3212_INPUT_BT_AUDIO	         (AudioControlAIC3212::IN1)   //uses IN1, for Bluetooth Audio (ahred with LINE_IN)
    // #define AIC3212_INPUT_ON_BOARD_MIC       (AudioControlAIC3212::IN2)   //uses IN2, for analog signals from microphones on PCB
    // #define AIC3212_INPUT_JACK_AS_LINEIN     (AudioControlAIC3212::IN3)   //uses IN3, for analog signals from mic jack, no mic bias
    // #define AIC3212_INPUT_JACK_AS_MIC        (AudioControlAIC3212::IN3_wBIAS)   //uses IN3, for analog signals from mic jack, with mic bias

    enum class Inputs
    {
        // LINE_IN = 0,
        MIC = 1, // IN1L/IN1R with Mic Bias EXT
        PDM,     // PDM Mic with PDM_CLK on GPIO2, PDM_DAT on GPI1
        NONE
    };

    // //convenience names to use with outputSelect()
    // #define AIC3212_OUTPUT_HEADPHONE_JACK_OUT 1  //DAC left and right to headphone left and right
    // #define AIC3212_OUTPUT_LINE_OUT 2 //DAC left and right to lineout left and right
    // #define AIC3212_OUTPUT_HEADPHONE_AND_LINE_OUT 3  //DAC left and right to both headphone and line out
    // #define AIC3212_OUTPUT_LEFT2DIFFHP_AND_R2DIFFLO 4 //DAC left to differential headphone, DAC right to line out

    enum class Outputs
    {
        HP, // Headphone HPL/HPR
        // LO,  // Line Out LOL/LOR
        SPK, // Speakers (SPKLP-SPKLM)/(SPKRP-SPKRM)
        NONE
    };

// names to use with setMicBias() to set the amount of bias voltage to use
#define AIC3212_MIC_BIAS_OFF 0
#define AIC3212_MIC_BIAS_1_62 1
#define AIC3212_MIC_BIAS_2_4 2
#define AIC3212_MIC_BIAS_3_0 3
#define AIC3212_MIC_BIAS_3_3 4
#define AIC3212_DEFAULT_MIC_BIAS AIC3212_MIC_BIAS_2_4

#define BOTH_CHAN 0
#define LEFT_CHAN 1
#define RIGHT_CHAN 2

// define AIC3212_DEFAULT_I2C_BUS              0  // bus zero is &Wire
#define AIC3212_DEFAULT_RESET_PIN 24          // A10 on the Rev-E2
#define AIC3212_DEFAULT_I2C_ADDRESS 0b0011000 // 7-Bit address

    // Link I2C Address to the AIC bus that is used.
    /*I2C address is set by GPI3 & GPI4 (active high) on the AIC:
            7-Bit I2C Address = "001 1000" + GPI4<<1 + GPI3 */
    // enum class AIC3212_I2C_Address : uint8_t {
    // 	Bus_0 = 0b0011000,
    // 	Bus_1 = 0b0011001,
    // 	Bus_2 = 0b0011010,
    // 	Bus_3 = 0b0011011,
    // };

    enum class DAC_PowerTune_Mode : uint8_t
    {
        PTM_P4 = 0x00, // (default) also PTM_P3
        PTM_P2 = 0x01,
        PTM_P1 = 0x02
    };

    enum class ADC_PowerTune_Mode : uint8_t
    {
        PTM_R4 = 0x00, // default setting at AIC startup
        PTM_R3 = 0x01,
        PTM_R2 = 0x02,
        PTM_R1 = 0x03
    };

    enum class I2S_Clock_Dir : uint8_t
    {
        AIC_INPUT = 0x00,
        AIC_OUTPUT = 0x0C
    };

    // PLL Clock Range (See SLAU360, Table 2-41)
    enum class PLL_Clock_Range : uint8_t
    {
        LOW_RANGE = 0x00,
        HIGH_RANGE = 0x40
    };

    enum class PLL_Source : uint8_t
    {
        MCLK1 = 0x00,
        BCLK1 = 0x01,
        GPIO1 = 0x02,
        DIN1 = 0x03,
        BCLK2 = 0x04,
        GPI1 = 0x05,
        HF_REF_CLK = 0x06,
        GPIO2 = 0x07,
        GPI2 = 0x08,
        MCLK2 = 0x09
    };

    enum class ADC_DAC_Clock_Source : uint8_t
    {
        MCLK1 = 0x00,
        BCLK1 = 0x01,
        GPIO1 = 0x02,
        PLL_CLK = 0x03,
        BCLK2 = 0x04,
        GPI1 = 0x05,
        HF_REF_CLK = 0x06,
        HF_OSC_CLK = 0x07,
        MCLK2 = 0x08,
        GPIO2 = 0x09,
        GPI2 = 0x0A
    };

    enum class I2S_Word_Length : uint8_t
    {
        I2S_16_BITS = 0x00,
        I2S_20_BITS = 0x01,
        I2S_24_BITS = 0x02,
        I2S_32_BITS = 0x03
    };

    struct PLL_Config
    {
        PLL_Clock_Range range;
        PLL_Source src;
        uint8_t r;
        uint8_t j;
        uint16_t d;
        uint8_t p;
        uint8_t clkin_div;
        bool enabled;
    };

    struct DAC_Config
    {
        ADC_DAC_Clock_Source clk_src;
        uint8_t ndac;
        uint8_t mdac;
        uint16_t dosr;
        uint8_t prb_p;
        DAC_PowerTune_Mode ptm_p;
    };

    struct ADC_Config
    {
        ADC_DAC_Clock_Source clk_src;
        uint8_t nadc; // If NADC == 0, ADC_CLK is the same as DAC_CLK
        uint8_t madc; // If MADC == 0, ADC_MOD_CLK is same as DAC_MOD_CLK
        uint16_t aosr;
        uint8_t prb_r;
        ADC_PowerTune_Mode ptm_r;
    };

    struct Config
    {
        I2S_Word_Length i2s_bits;  // I2S data word length
        I2S_Clock_Dir i2s_clk_dir; // I2S clock direction
        PLL_Config pll;            // PLL configuration
        DAC_Config dac;            // DAC configuration
        ADC_Config adc;            // ADC configuration
    };

    //*******************************  Constants  ********************************//

    extern const Config DefaultConfig;

    //***************************  AudioControl Class  ***************************//

    class AudioControlAIC3212 final : public TeensyAudioControl
    {
    public:
        // GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
        AudioControlAIC3212(void)
        { // specify nothing
            debugToSerial = false;
        }
        AudioControlAIC3212(bool _debugToSerial)
        { // specify debug
            debugToSerial = _debugToSerial;
        }
        AudioControlAIC3212(int _resetPin)
        { // specify reset pin (minimum recommended!)
            resetPinAIC = _resetPin;
            debugToSerial = false;
        }
        AudioControlAIC3212(int _resetPin, int i2cBusIndex, uint8_t _i2cAddress)
        { // specify reset pin and i2cBus (minimum if using for 2nd AIC)
            setResetPin(_resetPin);
            setI2Cbus(i2cBusIndex);
            i2cAddress = _i2cAddress;
            debugToSerial = false;
        }
        AudioControlAIC3212(int _resetPin, int i2cBusIndex, uint8_t _i2cAddress, bool _debugToSerial)
        { // specify everything
            setResetPin(_resetPin);
            setI2Cbus(i2cBusIndex);
            i2cAddress = _i2cAddress;
            debugToSerial = _debugToSerial;
        }
        // enum INPUTS {IN1 = 0, IN2, IN3, IN3_wBIAS};
        typedef tlv320aic3212::Inputs Inputs;
        typedef tlv320aic3212::Outputs Outputs;
        virtual bool enable(void);
        virtual bool disable(void);
        void setConfig(const Config *_pConfig) { pConfig = _pConfig; };

        // bool outputSelect(int n, bool flag_full = true); //flag_full is whether to do a full reconfiguration.  True is more complete but false is faster.
        bool outputSelect(Outputs both, bool flag_full = true) { return outputSelect(both, both, flag_full); };
        bool outputSelect(Outputs left, Outputs right, bool flag_full = true);

        bool outputSelectTest();

        bool volume(float n);
        static float applyLimitsOnVolumeSetting(float vol_dB);
        float volume_dB(float vol_dB);                          // set both channels to the same volume
        float volume_dB(float vol_left_dB, float vol_right_dB); // set both channels, but to their own values
        float volume_dB(float vol_left_dB, int chan);           // set each channel seperately (0 = left; 1 = right)
        void setHeadphoneGain_dB(float vol_left_dB, float vol_right_dB); // set HP volume
        float setSpeakerVolume_dB(float target_vol_dB);         // sets the volume of both Class D Speaker Outputs
        bool inputLevel(float n);                               // dummy to be compatible with Teensy Audio Library
        bool inputSelect(int n);
        bool inputSelect(Inputs both) { return inputSelect(both, both); };
        bool inputSelect(Inputs left, Inputs right);
        float applyLimitsOnInputGainSetting(float gain_dB);
        float setInputGain_dB(float gain_dB);           // set both channels to the same gain
        float setInputGain_dB(float gain_dB, int chan); // set each channel seperately (0 = left; 1 = right)
        bool setMicBias(int n);
        // bool updateInputBasedOnMicDetect(int setting = AIC3212_INPUT_JACK_AS_MIC);
        // bool enableMicDetect(bool);
        // int  readMicDetect(void);
        bool debugToSerial;
        unsigned int aic_readPage(uint8_t page, uint8_t reg);
        bool aic_writePage(uint8_t page, uint8_t reg, uint8_t val);

        void setHPFonADC(bool enable, float cutoff_Hz, float fs_Hz); // first-order HP applied within this 3212 hardware, ADC (input) side
        float getHPCutoff_Hz(void) { return HP_cutoff_Hz; }
        void setHpfIIRCoeffOnADC(int chan, uint32_t *coeff);                                             // alternate way of settings the same 1st-order HP filter
        float setBiquadOnADC(int type, float cutoff_Hz, float sampleRate_Hz, int chan, int biquadIndex); // lowpass applied within 3212 hardware, ADC (input) side
        int setBiquadCoeffOnADC(int chanIndex, int biquadIndex, uint32_t *coeff_uint32);
        void writeBiquadCoeff(uint32_t *coeff_uint32, int *page_reg_table, int table_ncol);

        float getSampleRate_Hz(void) { return sample_rate_Hz; }
        bool enableAutoMuteDAC(bool, uint8_t);
        bool mixInput1toHPout(bool state);
        void muteLineOut(bool state);
        bool enableDigitalMicInputs(void) { return enableDigitalMicInputs(true); }
        bool enableDigitalMicInputs(bool desired_state);

        bool aic_goToBook(uint8_t book);
        bool aic_goToPage(uint8_t page);

        unsigned int aic_readRegister(uint8_t reg, uint8_t *pVal); // Assumes page has already been set
        bool aic_writeRegister(uint8_t reg, uint8_t val);          // assumes page has already been set
        bool outputSpk(void);
        bool outputHp(void);
        bool inputPdm(void);

    protected:
        const Config *pConfig = &DefaultConfig;
        TwoWire *myWire = &Wire; // from Wire.h
        uint8_t i2cAddress = AIC3212_DEFAULT_I2C_ADDRESS;
        void setI2Cbus(int i2cBus);
        void aic_reset(void);
        void aic_init(void);
        // void aic_initDAC(void);
        // void aic_initADC(void);
        void setResetPin(int pin) { resetPinAIC = pin; }

        int prevMicDetVal = -1;
        int resetPinAIC = AIC3212_DEFAULT_RESET_PIN; // AIC reset pin, Rev C
        float HP_cutoff_Hz = 0.0f;
        float sample_rate_Hz = 44100; // only used with HP_cutoff_Hz to design HP filter on ADC, if used
        void setHpfIIRCoeffOnADC_Left(uint32_t *coeff);
        void setHpfIIRCoeffOnADC_Right(uint32_t *coeff);

        void computeFirstOrderHPCoeff_f32(float cutoff_Hz, float fs_Hz, float *coeff);
        // void computeFirstOrderHPCoeff_i32(float cutoff_Hz, float fs_Hz, int32_t *coeff);
        void computeBiquadCoeff_LP_f32(float cutoff_Hz, float sampleRate_Hz, float q, float *coeff);
        void computeBiquadCoeff_HP_f32(float cutoff_Hz, float sampleRate_Hz, float q, float *coeff);
        void convertCoeff_f32_to_i32(float *coeff_f32, int32_t *coeff_i32, int ncoeff);
    };

} // namespace tlv320aic3212

#endif
