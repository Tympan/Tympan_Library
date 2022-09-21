/*
    control_aic3212

    Created: Eric Yuan for Tympan, 7/2/2021
    Purpose: Control module for Texas Instruments TLV320AIC3212 compatible with Teensy Audio Library

    License: MIT License.  Use at your own risk.
 */

#include "control_aic3212.h"

namespace tlv320aic3212
{

// //********************************  Constants  *******************************//
// #ifndef AIC_FS
// #  define AIC_FS                                                     44100UL
// #endif

// #define AIC_BITS                                                        32

// #define AIC_I2S_SLAVE                                                     1
// #if AIC_I2S_SLAVE
// // Direction of BCLK and WCLK (reg 27) is input if a slave:
// # define AIC_CLK_DIR                                                    0
// #else
// // If master, make outputs:
// # define AIC_CLK_DIR                                                   0x0C
// #endif

// //#ifndef AIC_CODEC_CLKIN_BCLK
// //# define AIC_CODEC_CLKIN_BCLK                                           0
// //#endif

// //**************************** Clock Setup **********************************//

// //**********************************  44100  *********************************//
// #if AIC_FS == 44100

// // MCLK = 180000000 * 16 / 255 = 11.294117 MHz // FROM TEENSY, FIXED

// // PLL setup.  PLL_OUT = MCLK * R * J.D / P
// //// J.D = 7.5264, P = 1, R = 1 => 90.32 MHz // FROM 12MHz CHA AND WHF //
// // J.D = 7.9968, P = 1, R = 1 => 90.3168 MHz // For 44.1kHz exact
// // J.D = 8.0000000002, P = 1, R = 1 => 9.35294117888MHz // for TEENSY 44.11764706kHz
// #define PLL_J                                                             8
// #define PLL_D                                                             0

// // Bitclock divisor.
// // BCLK = DAC_CLK/N = PLL_OUT/NDAC/N = 32*fs or 16*fs
// // PLL_OUT = fs*NDAC*MDAC*DOSR
// // BLCK = 32*fs = 1411200 = PLL
// #if AIC_BITS == 16
// #define BCLK_N                                                            8
// #elif AIC_BITS == 32
// #define BCLK_N                                                            4
// #endif

// // ADC/DAC FS setup.
// // ADC_MOD_CLK = CODEC_CLKIN / (NADC * MADC)
// // DAC_MOD_CLK = CODEC_CLKIN / (NDAC * MDAC)
// // ADC_FS = PLL_OUT / (NADC*MADC*AOSR)
// // DAC_FS = PLL_OUT / (NDAC*MDAC*DOSR)
// // FS = 90.3168MHz / (8*2*128) = 44100 Hz.
// // MOD = 90.3168MHz / (8*2) = 5644800 Hz

// // Actual from Teensy: 44117.64706Hz * 128 => 5647058.82368Hz * 8*2 => 90352941.17888Hz

// // DAC clock config.
// // Note: MDAC*DOSR/32 >= RC, where RC is 8 for the default filter.
// // See Table 2-21
// // http://www.ti.com/lit/an/slaa463b/slaa463b.pdf
// // PB1 - RC = 8.  Use M8, N2
// // PB25 - RC = 12.  Use M8, N2

// #define MODE_STANDARD	(1)
// #define MODE_LOWLATENCY (2)
// #define MODE_PDM	(3)
// #define ADC_DAC_MODE    (MODE_PDM)

// #if (ADC_DAC_MODE == MODE_STANDARD)

// 	//standard setup for 44 kHz
// 	#define DOSR                                                            128
// 	#define NDAC                                                              2
// 	#define MDAC                                                              8

// 	#define AOSR                                                            128
// 	#define NADC                                                              2
// 	#define MADC                                                              8

// 	// Signal Processing Modes, Playback and Recording....for standard operation (AOSR 128)
// 	#define PRB_P                                                             1
// 	#define PRB_R                                                             1

// #elif (ADC_DAC_MODE == MODE_LOWLATENCY)
// 	//low latency setup
// 	//standard setup for 44 kHz
// 	#define DOSR                                                            32
// 	#define NDAC                                                              (2*4/2)
// 	#define MDAC                                                              4

// 	#define AOSR                                                            32
// 	#define NADC                                                              (2*4/2)
// 	#define MADC                                                              4

// 	// Signal Processing Modes, Playback and Recording....for low-latency operation (AOSR 32)
// 	#define PRB_P                                                             17    //DAC
// 	#define PRB_R                                                             13    //ADC

// #elif (ADC_DAC_MODE == MODE_PDM)
// 	#define DOSR                                                            128
// 	#define NDAC                                                              2
// 	#define MDAC                                                              8

// 	#define AOSR                                                             64
// 	#define NADC                                                              4
// 	#define MADC                                                              8

// 	// Signal Processing Modes, Playback and Recording.
// 	#define PRB_P                                                             1
// 	#define PRB_R                                                             1

// #endif  //for standard vs low-latency vs PDM setup

// #endif // end fs if block

//**************************** Chip Setup **********************************//

// Software Reset
#define AIC3212_SOFTWARE_RESET_BOOK 0x00
#define AIC3212_SOFTWARE_RESET_PAGE 0x00
#define AIC3212_SOFTWARE_RESET_REG 0x01
#define AIC3212_SOFTWARE_RESET_INITIATE 0x01

//******************* INPUT DEFINITIONS *****************************//
// MIC routing registers
#define AIC3212_MICPGA_PAGE 0x01
#define AIC3212_MICPGA_LEFT_POSITIVE_REG 0x34  // page 1 register 52
#define AIC3212_MICPGA_RIGHT_POSITIVE_REG 0x37 // page 1 register 55
/* Possible settings for registers using 40kohm resistors:
        Left  Mic PGA P-Term (Reg 0x34)
        Right Mic PGA P-Term (Reg 0x37)*/
#define AIC3212_MIC_ROUTING_POSITIVE_IN1 0b11000000     //
#define AIC3212_MIC_ROUTING_POSITIVE_IN2 0b00110000     //
#define AIC3212_MIC_ROUTING_POSITIVE_IN3 0b00001100     //
#define AIC3212_MIC_ROUTING_POSITIVE_REVERSE 0b00000011 //

#define AIC3212_MICPGA_LEFT_NEGATIVE_REG 0x36  // page 1 register 54
#define AIC3212_MICPGA_RIGHT_NEGATIVE_REG 0x39 // page 1 register 57
/* Possible settings for registers:
        Left  Mic PGA M-Term (Reg 0x36)
        Right Mic PGA M-Term (Reg 0x39) */
#define AIC3212_MIC_ROUTING_NEGATIVE_CM_TO_CM1L 0b11000000  //
#define AIC3212_MIC_ROUTING_NEGATIVE_IN2_REVERSE 0b00110000 //
#define AIC3212_MIC_ROUTING_NEGATIVE_IN3_REVERSE 0b00001100 //
#define AIC3212_MIC_ROUTING_NEGATIVE_CM_TO_CM2L 0b00000011  //

/* Make sure to "&" with these values to set the resistance */
#define AIC3212_MIC_ROUTING_RESISTANCE_10k 0b01010101
#define AIC3212_MIC_ROUTING_RESISTANCE_20k 0b10101010
#define AIC3212_MIC_ROUTING_RESISTANCE_40k 0b11111111
#define AIC3212_MIC_ROUTING_RESISTANCE_DEFAULT AIC3212_MIC_ROUTING_RESISTANCE_10k // datasheet (application notes) defaults to 20K...why?

// Volume for Mic PGA
/*At boot up, the volume is muted and requires a value written to it*/
#define AIC3212_MICPGA_LEFT_VOLUME_REG 0x3B     // page 1 register 59; 0 to 47.5dB in 0.5dB steps
#define AIC3212_MICPGA_RIGHT_VOLUME_REG 0x3C    // page 1 register 60;  0 to 47.5dB in 0.5dB steps
#define AIC3212_MICPGA_VOLUME_ENABLE 0b00000000 // default is 0b11000000 - clear to 0 to enable

// Mic Bias
#define AIC3212_MIC_BIAS_PAGE 0x01 // page 1 reg 0x33
#define AIC3212_MIC_BIAS_REG 0x33  // page 1 reg 0x33
/*Possible settings for Mic Bias EXT*/
#define AIC3212_MIC_BIAS_EXT_MASK 0b11110000
#define AIC3212_MIC_BIAS_EXT_POWER_ON 0b01000000 // only on if jack is inserted
#define AIC3212_MIC_BIAS_EXT_POWER_OFF 0b00000000
#define AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_1_62 0b00000000 // for CM = 0.9V
#define AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_2_4 0b00010000  // for CM = 0.9V
#define AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_3_0 0x00100000  // for CM = 0.9V
#define AIC3212_MIC_BIAS_EXT_OUTPUT_VOLTAGE_3_3 0x00110000  // regardless of CM

/*Possible settings for Mic Bias*/
#define AIC3212_MIC_BIAS_MASK 0b00001111
#define AIC3212_MIC_BIAS_POWER_ON 0b00000100 // only on if jack is inserted
#define AIC3212_MIC_BIAS_POWER_OFF 0b00000000
#define AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_1_62 0b00000000 // for CM = 0.9V
#define AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_2_4 0b00000001  // for CM = 0.9V
#define AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_3_0 0x00000010  // for CM = 0.9V
#define AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_3_3 0x00000011  // regradless of CM

// ADC Processing Block
#define AIC3212_ADC_PROCESSING_BLOCK_PAGE 0x00
#define AIC3212_ADC_PROCESSING_BLOCK_REG 0x3d // page 0 register 61

// Enable the ADC and configure for digital mics (if desired)
#define AIC3212_ADC_CHANNEL_POWER_PAGE 0x00 // register81
#define AIC3212_ADC_CHANNEL_POWER_REG 0x51  // register81
#define AIC3212_ADC_CHANNEL_POWER_REG_PWR_MASK 0b11000000
#define AIC3212_ADC_CHANNELS_ON 0b11000000 // power up left and right

#define AIC3212_ADC_CHANNEL_POWER_REG_L_DIG_MIC_MASK 0b00111100
#define AIC3212_ADC_LEFT_CONFIGURE_FOR_DIG_MIC 0b00010000  // configure ADC left for digital mics
#define AIC3212_ADC_RIGHT_CONFIGURE_FOR_DIG_MIC 0b00000100 // configure ADC left for digital mics

// Mute the ADC
#define AIC3212_ADC_MUTE_PAGE 0x00
#define AIC3212_ADC_MUTE_REG 0x52 // register 82
#define AIC3212_ADC_UNMUTE 0b00000000
#define AIC3212_ADC_MUTE 0b10001000 // Mute both channels

// DAC Processing Block
#define AIC3212_DAC_PROCESSING_BLOCK_PAGE 0x00 // page 0 register 60
#define AIC3212_DAC_PROCESSING_BLOCK_REG 0x3c  // page 0 register 60

// DAC Volume (Digital Volume Control)
#define AIC3212_DAC_VOLUME_PAGE 0x00      // page 0
#define AIC3212_DAC_VOLUME_LEFT_REG 0x41  // page 0 reg 65
#define AIC3212_DAC_VOLUME_RIGHT_REG 0x42 // page 0 reg 66

// SPKR Volume (for Class D Speaker drivers...but this is fed by the Line-Out, so it's volume is affected by LOL and LOR, too)
#define AIC3212_SPKR_VOLUME_PAGE 0x01 // page 1
#define AIC3212_SPKR_VOLUME_REG 0x30  // page 1 reg 48

// PDM Digital Mic Pin Control
#define AIC3212_BCLK2_PIN_CTRL_PAGE 0x04
#define AIC3212_BCLK2_PIN_CTRL_REG 0x46
#define AIC3212_BCLK2_DISABLED 0b00000000
#define AIC3212_BCLK2_ENABLE_PDM_CLK 0b00101000

#define AIC3212_DIN2_PIN_CTRL_PAGE 0x04
#define AIC3212_DIN2_PIN_CTRL_REG 0x48
#define AIC3212_DIN2_DISABLED 0b00000000
#define AIC3212_DIN2_ENABLED 0b00100000

#define AIC3212_DIGITAL_MIC_SETTING_PAGE 0x04
#define AIC3212_DIGITAL_MIC_SETTING_REG 0x65
#define AIC3212_DIGITAL_MIC_DIN2_LEFT_RIGHT 0b00000011 // Rising Edge: Left; Falling Edge Right

#define AIC3212_ADC_POWERTUNE_PAGE 0x01
#define AIC3212_ADC_POWERTUNE_REG 0x3D

    // -------------------- Device Configuration --------------------

    const Config DefaultConfig{
        .i2s_bits = I2S_Word_Length::I2S_32_BITS,
        .i2s_clk_dir = I2S_Clock_Dir::AIC_INPUT,
        .pll = {
            .range = PLL_Clock_Range::LOW_RANGE,
            .src = PLL_Source::MCLK1,
            .r = 1,
            .j = 8,
            .d = 0,
            .p = 1,
            .clkin_div = 1,
            .enabled = true},
        .dac = {.clk_src = ADC_DAC_Clock_Source::PLL_CLK, .ndac = 8, .mdac = 8, .dosr = 32, .prb_p = 1, .ptm_p = DAC_PowerTune_Mode::PTM_P4},
        .adc = {.clk_src = ADC_DAC_Clock_Source::PLL_CLK, .nadc = 8, .madc = 8, .aosr = 32, .prb_r = 13, .ptm_r = ADC_PowerTune_Mode::PTM_R4}};

    // -------------------- Local Variables --------------
    // AIC3212_I2C_Address i2cAddress = AIC3212_I2C_Address::Bus_0;

    // ---------------------- Functions -------------------
    /*Set I2C Bus based on two possible bus addresses.
     */
    void AudioControlAIC3212::setI2Cbus(int i2cBusIndex)
    {
        // Setup for Master mode, pins 18/19, external pullups, 400kHz, 200ms default timeout
        switch (i2cBusIndex)
        {
        case 0:
            myWire = &Wire;
            break;
        case 1:
            myWire = &Wire1;
            break;
        case 2:
            myWire = &Wire2;
            break;
        default:
            myWire = &Wire;
            break;
        }
    }

    bool AudioControlAIC3212::enable(void)
    {
        delay(10);
        myWire->begin();
        delay(5);

        // Hard reset the AIC
        if (debugToSerial)
            Serial.println("# Hardware reset of AIC...");
        pinMode(resetPinAIC, OUTPUT);
        digitalWrite(resetPinAIC, HIGH);
        delay(50); // not reset
        digitalWrite(resetPinAIC, LOW);
        delay(50); // reset
        digitalWrite(resetPinAIC, HIGH);
        delay(50); // not reset

        aic_reset(); // delay(50);  //soft reset
        aic_init();  // delay(10);
        // aic_initADC(); //delay(10);
        // aic_initDAC(); //delay(10);

        aic_readPage(0, 27); // check a specific register - a register read test

        if (debugToSerial)
            Serial.println("# AIC3212 enable done");

        return true;
    }

    bool AudioControlAIC3212::disable(void)
    {
        return true;
    }

    // dummy function to keep compatible with Teensy Audio Library
    bool AudioControlAIC3212::inputLevel(float volume)
    {
        return false;
    }

    bool AudioControlAIC3212::inputSelect(int n)
    {
        bool success = true;
        switch (n)
        {
        case AUDIO_INPUT_MIC:
            success = inputSelect(Inputs::MIC, Inputs::MIC);
            break;
        default:
            Serial.println("Selected input not implemented.");
            break;
        }
        return success;
    }

    bool AudioControlAIC3212::inputSelect(Inputs left, Inputs right)
    {
        if (debugToSerial)
            Serial.println("# AudioControlAIC3212: inputSelect");
        bool success = true;

        uint8_t
            // 	// Input common mode?  (B0_P1_R8_D2 already set (0.9??)
            b0_p1_r51 = 0x00, // Mic Bias (B0_P1_R51)
            b0_p1_r52 = 0x00, // Left ADCPGA P-Terminal (B0_P1_R52 & R53)
            b0_p1_r53 = 0x00,
            b0_p1_r54 = 0x00, // Left ADCPGA M-Terminal (B0_P1_R54)
            b0_p1_r55 = 0x00, // Right ADCPGA P-Terminal (B0_P1_R55 & R56)
            b0_p1_r56 = 0x00,
            b0_p1_r57 = 0x00,  // Right ADCPGA M-Terminal (B0_P1_R57)
            b0_p1_r58 = 0x00,  // Input Common Mode Control
            b0_p1_r59 = 0x80,  // Left MICPGA Volume (B0_P1_R59)
            b0_p1_r60 = 0x80,  // Right MICPGA Volume (B0_P1_R60)
                               //	b0_p1_r61 = 0x00, // ADC PowerTune Mode (B0_P1_R61)
            b0_p4_r87 = 0x00,  // GPIO2 as Digital MIC ADC_MOD_CLK (B0_P4_R87)
            b0_p4_r91 = 0x00,  // GPI1 as digital input (B0_P4_R92)
            b0_p4_r101 = 0x06, // Digital MIC DIN Control (B0_P4_R101)   48??, B0_P4_R65??) Digital MIC DIN
            b0_p0_r81 = 0x00,  // Power up, Digital Mic Selection (B0_P0_R81)
            b0_p0_r82 = 0x88;  // Unmute, Fine Gain (B0_P0_R82)

        if (left == Inputs::MIC)
        {
            // IN1L with 2.4V Mic Bias EXT
            b0_p1_r51 |= 0b11011000; // Power up 2.4V MICBIAS_EXT
            b0_p1_r52 |= 0b01000000; // IN1L to Left Mic PGA+ (RIN = 10K)
            b0_p1_r54 |= 0b01000000; // CM1L to Left Mic PGA- (RIN = 10K)
            b0_p1_r59 = 0x00;        // Enable Left MICPGA Volume, 0.0dB
            b0_p0_r81 |= 0b10000000; // Power up left ADC
            b0_p0_r82 &= 0x7F;       // Unmute Left ADC
        }
        if (right == Inputs::MIC)
        {
            // IN1R with 2.4V Mic Bias EXT
            b0_p1_r51 |= 0b11011000; // Power up 2.4V MICBIAS_EXT
            b0_p1_r55 |= 0b01000000; // IN1R to Right Mic PGA+ (RIN = 10K)
            b0_p1_r57 |= 0b01000000; // CM1R to Right Mic PGA- (RIN = 10K)
            b0_p1_r60 = 0x00;        // Enable Right MICPGA Volume, 0.0dB
            b0_p0_r81 |= 0b01000000; // Power up right ADC
            b0_p0_r82 &= 0xF7;       // Unmute Right ADC
        }
        if (left == Inputs::PDM)
        {
            // PDM Mic with PDM_CLK on GPIO2, PDM_DAT on GPI1 (Left)
            b0_p4_r87 = 0b00101000;  // Set GPIO2 to ADC_MOD_CLK for digital mics
            b0_p4_r91 = 0b00000010;  // Set GPI1 to data input
            b0_p4_r101 = 0b00000000; // Left and Right DIN Both on GPI1
            b0_p0_r81 |= 0b10010000; // Power up left ADC, Configure left for digital mic
            b0_p0_r82 &= 0x7F;       // Unmute Left ADC
        }
        if (right == Inputs::PDM)
        {
            // PDM Mic with PDM_CLK on GPIO2, PDM_DAT on GPI1 (Right)
            b0_p4_r87 = 0b00101000;  // Set GPIO2 to ADC_MOD_CLK for digital mics
            b0_p4_r91 = 0b00000010;  // Set GPI1 to data input
            b0_p4_r101 = 0b00000000; // Left and Right DIN Both on GPI1
            b0_p0_r81 |= 0b01000100; // Power up Right ADC, Configure right for digital mic
            b0_p0_r82 &= 0xF7;       // Unmute Right ADC
        }

        aic_goToBook(0);
        aic_writePage(1, 51, b0_p1_r51);   // Mic Bias (B0_P1_R51)
        aic_writePage(1, 52, b0_p1_r52);   // Left ADC PGA+ (B0_P1_R52
        aic_writePage(1, 53, b0_p1_r53);   //   & B0_P1_R53)
        aic_writePage(1, 54, b0_p1_r54);   // Left ADC PGA- (B0_P1_R54)
        aic_writePage(1, 55, b0_p1_r55);   // Right ADC PGA+ (B0_P1_R55
        aic_writePage(1, 56, b0_p1_r56);   //   & B0_P1_R56)
        aic_writePage(1, 57, b0_p1_r57);   // Right ADC PGA- (B0_P1_R57)
        aic_writePage(1, 58, b0_p1_r58);   // Input Common Mode Control
        aic_writePage(1, 59, b0_p1_r59);   // Left MIC PGA Volume (B0_P1_R59)
        aic_writePage(1, 60, b0_p1_r60);   // Right MIC PGA Volume (B0_P1_R60)
        aic_writePage(4, 87, b0_p4_r87);   // GPIO2 as Digital MIC ADC_MOD_CLK (B0_P4_R87)
        aic_writePage(4, 91, b0_p4_r91);   // GPI1 as digital input (B0_P4_R92)
        aic_writePage(4, 101, b0_p4_r101); // Digital MIC DIN Control (B0_P4_R101)
        aic_writePage(0, 81, b0_p0_r81);   // Power up, Digital Mic Selection (B0_P0_R81)
        aic_writePage(0, 82, b0_p0_r82);   // Unmute, Fine Gain (B0_P0_R82)

        return success;
    }

    bool AudioControlAIC3212::setMicBias(int n)
    {
        aic_goToPage(AIC3212_MIC_BIAS_PAGE);

        if (n == AIC3212_MIC_BIAS_1_62)
        {
            aic_writeRegister(AIC3212_MIC_BIAS_REG, AIC3212_MIC_BIAS_POWER_ON | AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_1_62); // power up mic bias
            return true;
        }
        else if (n == AIC3212_MIC_BIAS_2_4)
        {
            aic_writeRegister(AIC3212_MIC_BIAS_REG, AIC3212_MIC_BIAS_POWER_ON | AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_2_4); // power up mic bias
            return true;
        }
        else if (n == AIC3212_MIC_BIAS_3_0)
        {
            aic_writeRegister(AIC3212_MIC_BIAS_REG, AIC3212_MIC_BIAS_POWER_ON | AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_3_0); // power up mic bias
            return true;
        }
        else if (n == AIC3212_MIC_BIAS_3_3)
        {
            aic_writeRegister(AIC3212_MIC_BIAS_REG, AIC3212_MIC_BIAS_POWER_ON | AIC3212_MIC_BIAS_OUTPUT_VOLTAGE_3_3); // power up mic bias
            return true;
        }
        else if (n == AIC3212_MIC_BIAS_OFF)
        {
            aic_writeRegister(AIC3212_MIC_BIAS_REG, AIC3212_MIC_BIAS_POWER_OFF); // power up mic bias
            return true;
        }
        Serial.print("AudioControlAIC3212: ERROR: Unable to set MIC BIAS - Value not supported: ");
        Serial.println(n);
        return false;
    }

    bool AudioControlAIC3212::enableDigitalMicInputs(bool _enable)
    {
        if (_enable == true)
        {
            aic_goToBook(0);
            // Set GPIO2 to ADC_MOD_CLK for digital mics
            aic_writePage(4, 0x57, 0b00101000);
            // Set GPI1 to data input
            aic_writePage(4, 0x5B, 0b00000010);
            // Set digital mic input to GPI1 (Left = rising edge; Right = falling edge)
            aic_writePage(4, 0x65, 0b00000000);

            // Enable ADC; Configure Left and Right for digital mics (no soft-stepping)
            aic_writePage(0, 0x51, 0b11010100);
        }
        else
        {
            aic_goToBook(0);
            // Disable ADC; Configure Left and Right channels as not Digital Mics
            aic_writePage(0, 0x51, 0x00);
            // Disable GPI1 as digital mic input
            aic_writePage(4, 0x5C, 0x00);
            // Disable ADC_MOD_CLK on GPIO2
            aic_writePage(4, 0x57, 0x00);
        }
        return _enable;
    }

    void AudioControlAIC3212::aic_reset()
    {
        if (debugToSerial)
        {
            Serial.println("# INFO: Resetting AIC3212");
            Serial.print("#   7-bit I2C address: 0x");
            Serial.println(static_cast<uint8_t>(i2cAddress), HEX);
        }
        aic_goToBook(AIC3212_SOFTWARE_RESET_BOOK);
        aic_writePage(AIC3212_SOFTWARE_RESET_PAGE, AIC3212_SOFTWARE_RESET_REG, AIC3212_SOFTWARE_RESET_INITIATE);

        delay(10); // msec
        if (debugToSerial)
            Serial.println("d 10");
    }

    // set MICPGA volume, 0-47.5dB in 0.5dB setps
    float AudioControlAIC3212::applyLimitsOnInputGainSetting(float gain_dB)
    {
        if (gain_dB < 0.0)
        {
            gain_dB = 0.0; // 0.0 dB
                           // Serial.println("AudioControlAIC3212: WARNING: Attempting to set MIC volume outside range");
        }
        if (gain_dB > 47.5)
        {
            gain_dB = 47.5; // 47.5 dB
                            // Serial.println("AudioControlAIC3212: WARNING: Attempting to set MIC volume outside range");
        }
        return gain_dB;
    }

    float AudioControlAIC3212::setInputGain_dB(float orig_gain_dB, int Ichan)
    {
        float gain_dB = applyLimitsOnInputGainSetting(orig_gain_dB);
        if (abs(gain_dB - orig_gain_dB) > 0.01)
        {
            Serial.println("AudioControlAIC3212: WARNING: Attempting to set input gain outside allowed range");
        }

        // convert to proper coded value for the AIC3212
        float volume = gain_dB * 2.0;                // convert to value map (0.5 dB steps)
        int8_t volume_int = (int8_t)(round(volume)); // round

        if (debugToSerial)
        {
            Serial.print("AIC3212: Setting Input volume to ");
            Serial.print(gain_dB, 1);
            Serial.print(".  Converted to volume map => ");
            Serial.println(volume_int);
        }

        if (Ichan == 0)
        {
            aic_writePage(AIC3212_MICPGA_PAGE, AIC3212_MICPGA_LEFT_VOLUME_REG, AIC3212_MICPGA_VOLUME_ENABLE | volume_int); // enable Left MicPGA
        }
        else
        {
            aic_writePage(AIC3212_MICPGA_PAGE, AIC3212_MICPGA_RIGHT_VOLUME_REG, AIC3212_MICPGA_VOLUME_ENABLE | volume_int); // enable Right MicPGA
        }
        return gain_dB;
    }

    float AudioControlAIC3212::setInputGain_dB(float gain_dB)
    {
        gain_dB = setInputGain_dB(gain_dB, 0); // left channel
        return setInputGain_dB(gain_dB, 1);    // right channel
    }

    //******************* OUTPUT  *****************************//
    // volume control, similar to Teensy Audio Board
    // value between 0.0 and 1.0.  Set to span -58 to +15 dB
    bool AudioControlAIC3212::volume(float volume)
    {
        volume = max(0.0, min(1.0, volume));
        float vol_dB = -58.f + (15.0 - (-58.0f)) * volume;
        volume_dB(vol_dB);
        return true;
    }

    bool AudioControlAIC3212::enableAutoMuteDAC(bool enable, uint8_t mute_delay_code = 7)
    {
        if (enable)
        {
            mute_delay_code = max(0, min(mute_delay_code, 7));
            if (mute_delay_code == 0)
                enable = false;
        }
        else
        {
            mute_delay_code = 0; // this disables the auto mute
        }
        uint8_t val = aic_readPage(0, 64);
        val = val & 0b10001111;             // clear these bits
        val = val | (mute_delay_code << 4); // set these bits
        aic_writePage(0, 64, val);
        return enable;
    }

    // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
    float AudioControlAIC3212::applyLimitsOnVolumeSetting(float vol_dB)
    {
        // Constrain to limits
        if (vol_dB > 24.0)
        {
            vol_dB = 24.0;
            // Serial.println("AudioControlAIC3212: WARNING: Attempting to set DAC Volume outside range");
        }
        if (vol_dB < -63.5)
        {
            vol_dB = -63.5;
            // Serial.println("AudioControlAIC3212: WARNING: Attempting to set DAC Volume outside range");
        }
        return vol_dB;
    }
    float AudioControlAIC3212::volume_dB(float orig_vol_dB, int Ichan)
    { // 0 = Left; 1 = right;
        float vol_dB = applyLimitsOnVolumeSetting(orig_vol_dB);
        if (abs(vol_dB - orig_vol_dB) > 0.01)
        {
            Serial.println("AudioControlAIC3212: WARNING: Attempting to set DAC Volume outside range");
        }
        int8_t volume_int = (int8_t)(round(vol_dB * 2.0)); // round to nearest 0.5 dB step and convert to int

        if (debugToSerial)
        {
            Serial.print("AudioControlAIC3212: Setting DAC");
            Serial.print(Ichan);
            Serial.print(" volume to ");
            Serial.print(vol_dB, 1);
            Serial.print(".  Converted to volume map => ");
            Serial.println(volume_int);
        }

        if (Ichan == 0)
        {
            aic_writePage(AIC3212_DAC_VOLUME_PAGE, AIC3212_DAC_VOLUME_LEFT_REG, volume_int);
        }
        else
        {
            aic_writePage(AIC3212_DAC_VOLUME_PAGE, AIC3212_DAC_VOLUME_RIGHT_REG, volume_int);
        }
        return vol_dB;
    }

    float AudioControlAIC3212::volume_dB(float vol_left_dB, float vol_right_dB)
    {
        volume_dB(vol_right_dB, 1);       // set right channel
        return volume_dB(vol_left_dB, 0); // set left channel
    }

    float AudioControlAIC3212::volume_dB(float vol_dB)
    {
        vol_dB = volume_dB(vol_dB, 1); // set right channel
        return volume_dB(vol_dB, 0);   // set left channel
    }
    float AudioControlAIC3212::setSpeakerVolume_dB(float target_vol_dB)
    {
        int int_targ_vol_dB = (int)(target_vol_dB + 0.5); // round to nearest dB

        uint8_t val = 0x00;
        if (int_targ_vol_dB < 0)
        {
            int_targ_vol_dB = -99; // mute the output!
        }
        else if (int_targ_vol_dB < 12)
        {
            int_targ_vol_dB = 6;
            val = 0b00010001;
        }
        else if (int_targ_vol_dB < 18)
        {
            int_targ_vol_dB = 12;
            val = 0b00100010;
        }
        else if (int_targ_vol_dB < 24)
        {
            int_targ_vol_dB = 18;
            val = 0b00110011;
        }
        else if (int_targ_vol_dB < 30)
        {
            int_targ_vol_dB = 24;
            val = 0b01000100;
        }
        else
        {
            int_targ_vol_dB = 30;
            val = 0b01010101;
        }

        aic_goToBook(0);
        aic_writePage(AIC3212_SPKR_VOLUME_PAGE, AIC3212_SPKR_VOLUME_REG, val);
        return (float)int_targ_vol_dB;
    }

    bool AudioControlAIC3212::outputSelect(Outputs left, Outputs right, bool flag_full)
    {
        if (debugToSerial)
            Serial.println("# AudioControlAIC3212: outputSelect");
        static bool firstTime = true;
        if (firstTime)
        {
            flag_full = true; // always do a full reconfiguration the first time through.
            firstTime = false;
        }

        aic_goToBook(0);

        if (flag_full)
        {
            // Mute HP and Speaker Drivers
            aic_writePage(1, 0x1F, 0xB9);
            aic_writePage(1, 0x20, 0xB9);
            aic_writePage(1, 0x30, 0x00);

            // Unroute DAC outputs
            aic_writePage(1, 0x16, 0x00);
            aic_writePage(1, 0x1B, 0x00);
            aic_writePage(1, 0x1C, 0x7F);
            aic_writePage(1, 0x1D, 0x7F);
            aic_writePage(1, 0x24, 0x7F);
            aic_writePage(1, 0x25, 0x7F);

            // Reset charge pump control
            aic_writePage(1, 0x21, 0x28); // Charge Pump Clock Divide = 4 (Default)
            aic_writePage(1, 0x22, 0x3E); // Headphone output offset correction (Default)
            aic_writePage(1, 0x23, 0x30); // Enable dynamic offset calibration

            // TODO: Enable pop reduction?
            // 	//set the pop reduction settings, Page 1 Register 20 "Headphone Driver Startup Control"
            // 	aic_writeRegister(20, 0b10100101);  //soft routing step is 200ms, 5.0 time constants, assume 6K resistance
        }

        uint8_t
            p1_r16 = 0x00,
            p1_r1B = 0x00,
            p1_r2E = 0x7F,
            p1_r2F = 0x7F,
            p1_r16_2 = 0x00,
            p1_r1F = 0xB9,
            p1_r20 = 0xB9,
            p1_r30 = 0x00,
            p1_r1B_2 = 0x00,
            p1_r2D = 0x00;

        if (left == Outputs::HP)
        {
            p1_r1B |= 0x20;   // Left DAC is routed to HPL driver.
            p1_r1F = 0x80;    // Unmute HPL driver, set gain = 0dB
            p1_r1B_2 |= 0x22; // Left DAC is routed and HPL driver powered on.
        }
        if (right == Outputs::HP)
        {
            p1_r1B |= 0x10;   // Right DAC is routed to HPR driver.
            p1_r20 = 0x80;    // Unmute HPR driver, set gain = 0dB
            p1_r1B_2 |= 0x11; // Right DAC is routed and HPR driver powered on.
        }
        if (left == Outputs::SPK)
        {
            p1_r16 |= 0x80;   // Left DAC M-terminal is routed to LOL driver.
            p1_r2E = 0x00;    // LOL Output Routed to SPKL Driver, Volume Control: 0dB
            p1_r16_2 |= 0x82; // Routed & LOL output driver power-up
            p1_r30 |= 0x40;   // SPKL Driver Volume = 6 dB
            p1_r2D |= 0x02;   // SPKL Driver Power-Up
        }
        if (right == Outputs::SPK)
        {
            p1_r16 |= 0x40;   // Right DAC M-terminal is routed to LOR driver.
            p1_r2F = 0x00;    // LOR Output Routed to SPKR Driver, Volume Control: 0dB
            p1_r16_2 |= 0x41; // Routed & LOR output driver power-up
            p1_r30 |= 0x04;   // SPKR Driver Volume = 6 dB
            p1_r2D |= 0x01;   // SPKR Driver Power-Up
        }

        // Set DAC PowerTune Mode
        aic_writePage(1, 0x03, static_cast<uint8_t>(pConfig->dac.ptm_p) << 2); // Left DAC PTM
        aic_writePage(1, 0x04, static_cast<uint8_t>(pConfig->dac.ptm_p) << 2); // Right DAC PTM

        // Route DAC outputs
        aic_writePage(1, 0x16, p1_r16);
        aic_writePage(1, 0x1B, p1_r1B);
        aic_writePage(1, 0x2E, p1_r2E);
        aic_writePage(1, 0x2F, p1_r2F);

        // Power up LDAC and RDAC
        aic_writePage(0, 0x3F,
                      ((left == Outputs::NONE) ? 0x00 : 0x80)          // Left DAC channel
                          | ((right == Outputs::NONE) ? 0x00 : 0x40)); // Right DAC channel

        // Power up LOL and LOR
        aic_writePage(1, 0x16, p1_r16_2);

        // Unmute drivers
        aic_writePage(1, 0x1F, p1_r1F); // Unmute HPL driver
        aic_writePage(1, 0x20, p1_r20); // Unmute HPR driver
        aic_writePage(1, 0x30, p1_r30); // Unmute SPKL & SPLR

        // Headphone power-up
        aic_writePage(1, 0x09, 0x70);     // Headphone Driver Output Stage is 25%.
        aic_writePage(1, 0x1B, p1_r1B_2); // Power up HP Drivers
        // CAB TODO: Add delay?
        aic_writePage(1, 0x09, 0x10); // Headphone Driver Output Stage is 100%.

        // Power up Speaker Drivers
        aic_writePage(1, 0x2D, p1_r2D);
        // CAB TODO: Add delay?

        // Unmute LDAC and RDAC
        aic_writePage(0, 0x40,
                      ((left == Outputs::NONE) ? 0x08 : 0x00)          // Left DAC channel
                          | ((right == Outputs::NONE) ? 0x04 : 0x00)); // Right DAC channel

        return true;
    }

    void AudioControlAIC3212::muteLineOut(bool flag)
    {

        byte curValL = aic_readPage(1, 18);
        byte curValR = aic_readPage(1, 19);

        aic_goToPage(1);
        if (flag == true)
        {
            // mute
            if (!(curValL & 0b01000000))
            { // is this bit low?
              // already muted
            }
            else
            {
                // mute
                aic_writeRegister(18, curValL & 0b10111111); // Page1, mute LOL driver, same gain as before
            }
            // unmute
            if (!(curValR & 0b01000000))
            { // is this bit low?
              // already muted
            }
            else
            {
                // mute
                aic_writeRegister(19, curValR & 0b10111111); // Page1, mute LOR driver, same gain as before
            }
        }
        else
        {
            // unmute
            if (curValL & 0b01000000)
            { // is this bit high?
              // already active
            }
            else
            {
                // unmute
                aic_writeRegister(18, curValL | 0b0100000000); // Page1, unmute LOL driver, same gain as before
            }
            // unmute
            if (curValR & 0b01000000)
            { // is this bit high?
              // already active
            }
            else
            {
                // unmute
                aic_writeRegister(19, curValR | 0b0100000000); // Page1, unmute LOR driver, same gain as before
            }
        }
    }

    void AudioControlAIC3212::aic_init()
    {
        if (debugToSerial)
        {
            Serial.println("# AudioControlAIC3212: Initializing AIC");
        }

        // ################################################################
        // # Power and Analog Configuration
        // ################################################################

        aic_goToBook(0);
        aic_writePage(1, 0x01, 0x00); // Disable weak AVDD to DVDD connection and make analog supplies available
        aic_writePage(1, 0x7a, 0x01); // REF charging time = 40ms
        aic_writePage(1, 0x79, 0x33); // Set the quick charge of input coupling cap for analog inputs

        // ################################################################
        // # Clock configuration
        // ################################################################

        // Set ADC_CLKIN and DAC_CLKIN
        aic_writePage(0, 0x04, static_cast<uint8_t>(pConfig->dac.clk_src) << 4 | static_cast<uint8_t>(pConfig->adc.clk_src));
        // PLL Settings:
        if (pConfig->pll.enabled)
        {
            // Set PLL_CLKIN and PLL Speed Range
            aic_writePage(0, 0x05, static_cast<uint8_t>(pConfig->pll.range) | static_cast<uint8_t>(pConfig->pll.src) << 2);
            // Power up the PLL; set scalers P and R
            aic_writePage(0, 0x06, 0x80 // Power up
                                       | ((pConfig->pll.p & 0x07) << 4) | (pConfig->pll.r & 0x0F));
            // Set Scaler J
            aic_writePage(0, 0x07, pConfig->pll.j & 0x3F);
            // Set Scaler D (MSB)
            aic_writePage(0, 0x08, (pConfig->pll.d >> 8) & 0x3F);
            // Set Scaler D (LSB)
            aic_writePage(0, 0x09, pConfig->pll.d & 0xFF);
            // Set PLL Divider (CLKIN_DIV)
            aic_writePage(0, 0x0A, pConfig->pll.clkin_div & 0x7F);
        }
        else
        {
            aic_writePage(0, 0x06, 0x00); // Power down PLL
        }

        // Set up DAC Clock
        //  NDAC divider ON, Scaler NDAC
        aic_writePage(0, 0x0B, (pConfig->dac.ndac != 0 ? 0x80 : 0x00) | (pConfig->dac.ndac & 0x7F));
        //  MDAC divider ON, Scaler MDAC
        aic_writePage(0, 0x0C, (pConfig->dac.mdac != 0 ? 0x80 : 0x00) | (pConfig->dac.mdac & 0x7F));
        //  DOSR (MSB)
        aic_writePage(0, 0x0D, (pConfig->dac.dosr >> 8) & 0x03);
        //  DOSR (LSB)
        aic_writePage(0, 0x0E, pConfig->dac.dosr & 0xFF);

        // Set up ADC Clock
        //  NADC divider ON, Scaler NADC
        aic_writePage(0, 0x12, (pConfig->adc.nadc != 0 ? 0x80 : 0x00) | (pConfig->adc.nadc & 0x7F));
        //  MADC divider ON, Scaler MADC
        aic_writePage(0, 0x13, (pConfig->adc.madc != 0 ? 0x80 : 0x00) | (pConfig->adc.madc & 0x7F));
        //  AOSR
        aic_writePage(0, 0x14, pConfig->adc.aosr & 0xFF);

        // ################################################################
        // # Audio Serial Interface Routing Configuration - Audio Serial Interface #1
        // ################################################################

        // ASI #1 in I2S Mode, xx-bit, DOUT1 is output
        //  TODO: Check whether we need DOUT1 High Impedance Output Control
        aic_writePage(4, 0x01, (static_cast<uint8_t>(pConfig->i2s_bits) << 3));
        if (pConfig->i2s_clk_dir == I2S_Clock_Dir::AIC_INPUT)
        {
            // Configure WCLK1 and BCLK1 as inputs to ASI #1
            aic_writePage(4, 0x0A, 0x00);
        }
        else
        {
            // Configure WCLK1 and BCLK1 as outputs of ASI #1
            aic_writePage(4, 0x0A, 0x24);
        }
        aic_writePage(4, 0x41, 0x04); // WCLK1 Pin is ASI1 Word Clock I/O
        aic_writePage(4, 0x43, 0x02); // DOUT1 Pin is ASI1 Data Output
        aic_writePage(4, 0x44, 0x20); // Enable DIN1 Pin
        aic_writePage(4, 0x07, 0x01); // ASI1_DOUT is sourced from ADC
        aic_writePage(4, 0x08, 0x50); // Enable ASI1 to DAC datapath
        aic_writePage(4, 0x76, 0x06); // DAC input data is sourced from ASI1_DIN

        //??? B0_P4_R14???

        // ################################################################
        // # Signal Processing Settings
        // ################################################################

        // Processing Blocks (PRB)
        aic_writePage(0, 0x3C, pConfig->dac.prb_p & 0x1F); // Set DAC PRB_P
        aic_writePage(0, 0x3D, pConfig->adc.prb_r & 0x1F); // Set ADC PRB_R

        // PowerTune Modes (PTM)
        aic_writePage(1, 0x03, static_cast<uint8_t>(pConfig->dac.ptm_p) << 2); // Set Left DAC PTM_P
        aic_writePage(1, 0x04, static_cast<uint8_t>(pConfig->dac.ptm_p) << 2); // Set Right DAC PTM_P
        aic_writePage(1, 0x3D, static_cast<uint8_t>(pConfig->adc.ptm_r) << 6); // Set ADC PTM_R

        Serial.println("# CAB: TODO ");

        // // CAB TODO: Move power control to input and output selection blocks
        // // POWER
        // aic_goToPage(1);
        // aic_writeRegister(0x01, 8); //Page 1, Reg 1, Val = 8 = 0b00001000 = disable weak connection AVDD to DVDD.	Keep headphone charge pump disabled.
        // aic_writeRegister(0x02, 0); //Page 1,  Reg 2, Val = 0 = 0b00000000 = Enable Master Analog Power Control
        // aic_writeRegister(0x7B, 1); //Page 1,  Reg 123, Val = 1 = 0b00000001 = Set reference to power up in 40ms when analog blocks are powered up
        // aic_writeRegister(0x7C, 6); //Page 1,  Reg 124, Val = 6 = 0b00000110 = Charge Pump, full peak current (000), clock divider (110) to Div 6 = 333 kHz
        // aic_writeRegister(0x01, 10); //Page 1,  Reg 1, Val = 10 = 0x0A = 0b00001010.  Activate headphone charge pump.
        // aic_writeRegister(0x0A, 0); //Page 1,  Reg 10, Val = 0 = common mode 0.9 for full chip, HP, LO  // from WHF/CHA
        // aic_writeRegister(0x47, 0x31); //Page 1,  Reg 71, val = 0x31 = 0b00110001 = Set input power-up time to 3.1ms (for ADC)
        // aic_writeRegister(0x7D, 0x53); //Page 1,  Reg 125, Val = 0x53 = 0b01010011 = 0 10 1 00 11: HPL is master gain, Enable ground-centered mode, 100% output power, DC offset correction  // from WHF/CHA
    }

    unsigned int AudioControlAIC3212::aic_readPage(uint8_t page, uint8_t reg)
    {
        unsigned int val;

        if (aic_goToPage(page))
        {
            myWire->beginTransmission(static_cast<uint8_t>(i2cAddress));
            myWire->write(reg);
            unsigned int result = myWire->endTransmission();
            if (result != 0)
            {
                Serial.print("AudioControlAIC3212: ERROR: Read Page.  Page: ");
                Serial.print(page);
                Serial.print(" Reg: ");
                Serial.print(reg);
                Serial.print(".  Received Error During Read Page: ");
                Serial.println(result);
                val = 300 + result;
                return val;
            }
            if (myWire->requestFrom(static_cast<uint8_t>(i2cAddress), 1U) < 1)
            {
                Serial.print("AudioControlAIC3212: ERROR: Read Page.  Page: ");
                Serial.print(page);
                Serial.print(" Reg: ");
                Serial.print(reg);
                Serial.println(".  Nothing to return");
                val = 400;
                return val;
            }
            if (myWire->available() >= 1)
            {
                uint16_t val = myWire->read();
                if (debugToSerial)
                {
                    Serial.print("# AudioControlAIC3212: Read Page.   Page: ");
                    Serial.print(page);
                    Serial.print(" Reg: ");
                    Serial.print(reg);
                    Serial.print(".  Received: 0x");
                    Serial.println(val, HEX);
                }
                return val;
            }
        }
        else
        {
            Serial.print("AudioControlAIC3212: INFO: Read Page.   Page: ");
            Serial.print(page);
            Serial.print(" Reg: ");
            Serial.print(reg);
            Serial.println(".  Failed to go to read page.  Could not go there.");
            val = 500;
            return val;
        }
        val = 600;
        return val;
    }

    bool AudioControlAIC3212::aic_writePage(uint8_t page, uint8_t reg, uint8_t val)
    {
        bool success = true;

        if (debugToSerial)
        {
            Serial.print("# AudioControlAIC3212: Write Page.  Page: ");
            Serial.print(page);
            Serial.print(" Reg: ");
            Serial.print(reg);
            Serial.print(" Val: 0x");
            Serial.println(val, HEX);
        }

        success = aic_goToPage(page);
        if (success)
        {
            success = aic_writeRegister(reg, val);
        }
        else
        {
            Serial.print("AudioControlAIC3212: Received Error During aic_goToPage()");
        }

        return success;
    }

    unsigned int AudioControlAIC3212::aic_readRegister(uint8_t reg, uint8_t *pVal)
    {
        unsigned int errcode = 0;

        myWire->beginTransmission(static_cast<uint8_t>(i2cAddress));
        myWire->write(reg);
        unsigned int result = myWire->endTransmission();
        if (result != 0)
        {
            Serial.print("AudioControlAIC3212: ERROR: Read register.  Reg: ");
            Serial.print(reg);
            Serial.print(".  Received Error During Write Address: ");
            Serial.println(result);
            errcode = 300 + result;
        }

        if (errcode == 0)
        {
            if (myWire->requestFrom(static_cast<uint8_t>(i2cAddress), 1U) < 1)
            {
                Serial.print("AudioControlAIC3212: ERROR: Read register.  Reg: ");
                Serial.print(reg);
                Serial.println(".  Received Error During Request.");
                errcode = 400;
            }
        }
        else
        {
            // Already failed
        }

        if (errcode == 0)
        {
            if (myWire->available() >= 1)
            {
                *pVal = myWire->read();
                // if (debugToSerial) {
                // 	Serial.print("AudioControlAIC3212: Read Reg: "); Serial.print(reg);
                // 	Serial.print(".  Received: ");
                // 	Serial.println(*pVal, HEX);
                // }
            }
            else
            {
                // Empty response
                Serial.print("AudioControlAIC3212: ERROR: Read register.  Reg: ");
                Serial.print(reg);
                Serial.println(".  Empty Response.");
                errcode = 500;
            }
        }
        else
        {
            // Already failed
        }

        return errcode;
    }

    bool AudioControlAIC3212::aic_writeRegister(uint8_t reg, uint8_t val)
    { // assumes page has already been set
        if (debugToSerial)
        {
            // Serial.print("aic_writeRegister: 0x"); Serial.print(reg, HEX); Serial.print(" 0x"); Serial.println(val, HEX);
            Serial.print("w 30 ");
            Serial.print(reg, HEX);
            Serial.print(" ");
            Serial.println(val, HEX);
        }

        myWire->beginTransmission(static_cast<uint8_t>(i2cAddress));
        myWire->write(reg); // delay(1); //delay(10); //was delay(10)
        myWire->write(val); // delay(1);//delay(10); //was delay(10)
        uint8_t result = myWire->endTransmission();
        if (result == 0)
        {
            return true;
        }
        else
        {
            Serial.print("AudioControlAIC3212: Received Error During writeRegister(): Error = ");
            Serial.println(result);
        }
        return false;
    }

    bool AudioControlAIC3212::aic_goToPage(uint8_t page)
    {
        return aic_writeRegister(0x00, page);
    }

    bool AudioControlAIC3212::aic_goToBook(uint8_t book)
    {
        bool success = true;

        if (debugToSerial)
        {
            Serial.print("# AudioControlAIC3212: Go To Book ");
            Serial.print(book);
            Serial.println(".");
        }
        // Go to page 0 of current book
        success = aic_goToPage(0x00);
        if (success)
        {
            // Select book by writing to register 0x7F (127)
            success = aic_writeRegister(0x7F, book);
        }
        else
        {
            // Already failed
        }
        return success;
    }

    // bool AudioControlAIC3212::updateInputBasedOnMicDetect(int setting) {
    // 	//read current mic detect setting
    // 	int curMicDetVal = readMicDetect();
    // 	if (curMicDetVal != prevMicDetVal) {
    // 		if (curMicDetVal) {
    // 			//enable the microphone input jack as our input
    // 			inputSelect(setting);
    // 		} else {
    // 			//switch back to the on-board mics
    // 			inputSelect(AIC3212_INPUT_ON_BOARD_MIC);
    // 		}
    // 	}
    // 	prevMicDetVal = curMicDetVal;
    // 	return (bool)curMicDetVal;
    // }

    // bool AudioControlAIC3212::enableMicDetect(bool state) {
    // 	//page 0, register 67
    // 	byte curVal = aic_readPage(0,67);
    // 	byte newVal = curVal;
    // 	if (state) {
    // 		//enable
    // 		newVal = 0b111010111 & newVal;  //set bits 4-2 to be 010 to set debounce to 64 msec
    // 		newVal = 0b10000000 | curVal;  //force bit 1 to 1 to enable headset to detect
    // 		aic_writePage(0,67,newVal);  //bit 7 (=1) enable headset detect, bits 4-2 (=010) debounce to 64ms
    // 	} else {
    // 		//disable
    // 		newVal = 0b01111111 & newVal;  //force bit 7 to zero to disable headset detect
    // 		aic_writePage(0,67,newVal);  //bit 7 (=1) enable headset detect, bits 4-2 (=010) debounce to 64ms
    // 	}
    // 	return state;
    // }

    // int AudioControlAIC3212::readMicDetect(void) {
    // 	//page 0, register 46, bit D4 (for D7-D0)
    // 	byte curVal = aic_readPage(0,46);
    // 	curVal = (curVal & 0b00010000);
    // 	curVal = (curVal != 0);
    // 	return curVal;
    // }

    float AudioControlAIC3212::setBiquadOnADC(int type, float cutoff_Hz, float sampleRate_Hz, int chanIndex, int biquadIndex)
    {
        // Purpose: Creare biquad filter coefficients to be applied within 3212 hardware, ADC (input) side
        //
        //  type is type of filter: 1 = Lowpass, 2=highpass
        //  cutoff_Hz is the cutoff frequency in Hz
        //  chanIndex is 0=both, 1=left, 2=right
        //  biquadIndex is 0-4 for biquad A through biquad B, depending upon ADC mode

        const int ncoeff = 5;
        float coeff_f32[ncoeff];
        uint32_t coeff_uint32[ncoeff];
        float q = 0.707; // assume critically damped (sqrt(2)), which makes this a butterworth filter
        if (type == 1)
        {
            // lowpass
            computeBiquadCoeff_LP_f32(cutoff_Hz, sampleRate_Hz, q, coeff_f32);
        }
        else if (type == 2)
        {
            // highpass
            computeBiquadCoeff_HP_f32(cutoff_Hz, sampleRate_Hz, q, coeff_f32);
        }
        else
        {
            // unknown
            return -1.0;
        }
        convertCoeff_f32_to_i32(coeff_f32, (int32_t *)coeff_uint32, ncoeff);
        setBiquadCoeffOnADC(chanIndex, biquadIndex, coeff_uint32); // needs twos-compliment
        return cutoff_Hz;
    }

    void AudioControlAIC3212::computeBiquadCoeff_LP_f32(float freq_Hz, float sampleRate_Hz, float q, float *coeff)
    {
        // Compute common filter functions...all second order filters...all with Matlab convention on a1 and a2 coefficients
        // http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt

        // cutoff_Hz = freq_Hz;

        // int coeff[5];
        double w0 = freq_Hz * (2.0 * 3.141592654 / sampleRate_Hz);
        double sinW0 = sin(w0);
        double alpha = sinW0 / ((double)q * 2.0);
        double cosW0 = cos(w0);
        // double scale = 1073741824.0 / (1.0 + alpha);
        double scale = 1.0 / (1.0 + alpha); // which is equal to 1.0 / a0
        /* b0 */ coeff[0] = ((1.0 - cosW0) / 2.0) * scale;
        /* b1 */ coeff[1] = (1.0 - cosW0) * scale;
        /* b2 */ coeff[2] = coeff[0];
        /* a0 = 1.0 in Matlab style */
        /* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
        /* a2 */ coeff[4] = (1.0 - alpha) * scale;

        // flip signs for TI convention...see section 2.3.3.1.10.2 of TI Application Guide http://www.ti.com/lit/an/slaa463b/slaa463b.pdf
        coeff[1] = coeff[1] / 2.0;
        coeff[3] = -coeff[3] / 2.0;
        coeff[4] = -coeff[4];
    }

    void AudioControlAIC3212::computeBiquadCoeff_HP_f32(float freq_Hz, float sampleRate_Hz, float q, float *coeff)
    {
        // Compute common filter functions...all second order filters...all with Matlab convention on a1 and a2 coefficients
        // http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt

        // cutoff_Hz = freq_Hz;

        double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
        double sinW0 = sin(w0);
        double alpha = sinW0 / ((double)q * 2.0);
        double cosW0 = cos(w0);
        double scale = 1.0 / (1.0 + alpha); // which is equal to 1.0 / a0
        /* b0 */ coeff[0] = ((1.0 + cosW0) / 2.0) * scale;
        /* b1 */ coeff[1] = -(1.0 + cosW0) * scale;
        /* b2 */ coeff[2] = coeff[0];
        /* a0 = 1.0 in Matlab style */
        /* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
        /* a2 */ coeff[4] = (1.0 - alpha) * scale;

        // flip signs and scale for TI convention...see section 2.3.3.1.10.2 of TI Application Guide http://www.ti.com/lit/an/slaa463b/slaa463b.pdf
        coeff[1] = coeff[1] / 2.0;
        coeff[3] = -coeff[3] / 2.0;
        coeff[4] = -coeff[4];
    }

#define CONST_2_31_m1 (2147483647) // 2^31 - 1
    // void AudioControlAIC3212::computeFirstOrderHPCoeff_i32(float cutoff_Hz, float fs_Hz, int32_t *coeff) {
    //	float coeff_f32[3];
    //	computeFirstOrderHPCoeff_f32(cutoff_Hz,fs_Hz,coeff_f32);
    //	for (int i=0; i<3; i++) {
    //		//scale
    //		coeff_f32[i] *= (float)CONST_2_31_m1;
    //
    //		//truncate
    //		coeff[i] = (int32_t)coeff_f32[i];
    //	}
    // }
    void AudioControlAIC3212::convertCoeff_f32_to_i32(float *coeff_f32, int32_t *coeff_i32, int ncoeff)
    {
        for (int i = 0; i < ncoeff; i++)
        {
            // scale
            coeff_f32[i] *= (float)CONST_2_31_m1;

            // truncate
            coeff_i32[i] = (int32_t)coeff_f32[i];
        }
    }

    int AudioControlAIC3212::setBiquadCoeffOnADC(int chanIndex, int biquadIndex, uint32_t *coeff_uint32) // needs twos-compliment
    {
        // See TI application guide for the AIC3212 http://www.ti.com/lit/an/slaa463b/slaa463b.pdf
        // See section 2.3.3.1.10.2

        // power down the AIC to allow change in coefficients
        uint32_t prev_state = aic_readPage(0x00, 0x51);
        aic_writePage(0x00, 0x51, prev_state & (0b00111111)); // clear first two bits

        // use table 2-14 from TI Application Guide...
        int page_reg_table[] = {
            8, 36, 9, 44, // N0, start of Biquad A
            8, 40, 9, 48, // N1
            8, 44, 9, 52, // N2
            8, 48, 9, 56, // D1
            8, 52, 8, 64, // D2
            8, 56, 9, 64, // start of biquad B
            8, 60, 9, 68,
            8, 64, 9, 72,
            8, 68, 9, 76,
            8, 72, 9, 80,
            8, 76, 9, 84, // start of Biquad C
            8, 80, 9, 88,
            8, 84, 9, 92,
            8, 88, 9, 96,
            8, 92, 9, 100,
            8, 96, 9, 104, // start of Biquad D
            8, 100, 9, 108,
            8, 104, 9, 112,
            8, 108, 9, 116,
            8, 112, 9, 120,
            8, 116, 9, 124, // start of Biquad E
            8, 120, 10, 8,
            8, 124, 10, 12,
            9, 8, 10, 16,
            9, 12, 10, 20};

        const int rows_per_biquad = 5;
        const int table_ncol = 4;
        int chan_offset;

        switch (chanIndex)
        {
        case BOTH_CHAN:
            chan_offset = 0;
            writeBiquadCoeff(coeff_uint32, page_reg_table + chan_offset + biquadIndex * rows_per_biquad * table_ncol, table_ncol);
            chan_offset = 1;
            writeBiquadCoeff(coeff_uint32, page_reg_table + chan_offset + biquadIndex * rows_per_biquad * table_ncol, table_ncol);
            break;
        case LEFT_CHAN:
            chan_offset = 0;
            writeBiquadCoeff(coeff_uint32, page_reg_table + chan_offset + biquadIndex * rows_per_biquad * table_ncol, table_ncol);
            break;
        case RIGHT_CHAN:
            chan_offset = 1;
            writeBiquadCoeff(coeff_uint32, page_reg_table + chan_offset + biquadIndex * rows_per_biquad * table_ncol, table_ncol);
            break;
        default:
            return -1;
            break;
        }

        // power the ADC back up
        aic_writePage(0x00, 0x51, prev_state); // clear first two bits
        return 0;
    }

    void AudioControlAIC3212::writeBiquadCoeff(uint32_t *coeff_uint32, int *page_reg_table, int table_ncol)
    {
        int page, reg;
        uint32_t c;
        for (int i = 0; i < 5; i++)
        {
            page = page_reg_table[i * table_ncol];
            reg = page_reg_table[i * table_ncol + 1];
            c = coeff_uint32[i];
            aic_writePage(page, reg, (uint8_t)(c >> 24));
            aic_writePage(page, reg + 1, (uint8_t)(c >> 16));
            aic_writePage(page, reg + 2, (uint8_t)(c >> 8));
        }
        return;
    }

    void AudioControlAIC3212::setHPFonADC(bool enable, float cutoff_Hz, float fs_Hz)
    { // fs_Hz is sample rate
        // see TI application guide Section 2.3.3.1.10.1: http://www.ti.com/lit/an/slaa463b/slaa463b.pdf
        uint32_t coeff[3];
        if (enable)
        {
            HP_cutoff_Hz = cutoff_Hz;
#if 0
			//original
			sample_rate_Hz = fs_Hz;
			computeFirstOrderHPCoeff_i32(cutoff_Hz,fs_Hz,(int32_t *)coeff);
#else
            // new
            float coeff_f32[3];
            computeFirstOrderHPCoeff_f32(cutoff_Hz, fs_Hz, coeff_f32);
            convertCoeff_f32_to_i32(coeff_f32, (int32_t *)coeff, 3);
#endif

            // Serial.print("enableHPFonADC: coefficients, Hex: ");
            // Serial.print(coeff[0],HEX);
            // Serial.print(", ");
            // Serial.print(coeff[1],HEX);
            // Serial.print(", ");
            // Serial.print(coeff[2],HEX);
            // Serial.println();
        }
        else
        {
            // disable
            HP_cutoff_Hz = cutoff_Hz;

            // see Table 5-4 in TI application guide  Coeff C4, C5, C6
            coeff[0] = 0x7FFFFFFF;
            coeff[1] = 0;
            coeff[2] = 0;
        }

        setHpfIIRCoeffOnADC(BOTH_CHAN, coeff); // needs twos-compliment
    }

    void AudioControlAIC3212::computeFirstOrderHPCoeff_f32(float cutoff_Hz, float fs_Hz, float *coeff)
    {
        // cutoff_Hz is the cutoff frequency in Hz
        // fs_Hz is the sample rate in Hz

        // First-order Butterworth IIR
        // From https://www.dsprelated.com/showcode/199.php
        const float pi = 3.141592653589793;
        float T = 1.0f / fs_Hz; // sample period
        float w = cutoff_Hz * 2.0 * pi;
        float A = 1.0f / (tan((w * T) / 2.0));
        coeff[0] = A / (1.0 + A);         // first b coefficient
        coeff[1] = -coeff[0];             // second b coefficient
        coeff[2] = (1.0 - A) / (1.0 + A); // second a coefficient (Matlab sign convention)
        coeff[2] = -coeff[2];             // flip to be TI sign convention
    }

    // set first-order IIR filter coefficients on ADC
    void AudioControlAIC3212::setHpfIIRCoeffOnADC(int chan, uint32_t *coeff)
    {

        // power down the AIC to allow change in coefficients
        uint32_t prev_state = aic_readPage(0x00, 0x51);
        aic_writePage(0x00, 0x51, prev_state & (0b00111111)); // clear first two bits

        if (chan == BOTH_CHAN)
        {
            setHpfIIRCoeffOnADC_Left(coeff);
            setHpfIIRCoeffOnADC_Right(coeff);
        }
        else if (chan == LEFT_CHAN)
        {
            setHpfIIRCoeffOnADC_Left(coeff);
        }
        else
        {
            setHpfIIRCoeffOnADC_Right(coeff);
        }

        // power the ADC back up
        aic_writePage(0x00, 0x51, prev_state); // clear first two bits
    }

    void AudioControlAIC3212::setHpfIIRCoeffOnADC_Left(uint32_t *coeff)
    {
        int page;
        uint32_t c;

        // See TI AIC3212 Application Guide, Table 2-13: http://www.ti.com/lit/an/slaa463b/slaa463b.pdf

        // Coeff N0, Coeff C4
        page = 8;
        c = coeff[0];
        aic_writePage(page, 24, (uint8_t)(c >> 24));
        aic_writePage(page, 25, (uint8_t)(c >> 16));
        aic_writePage(page, 26, (uint8_t)(c >> 8));
        // int foo  = aic_readPage(page,24);	Serial.print("setIIRCoeffOnADC: first coefficient: ");  Serial.println(foo);

        // Coeff N1, Coeff C5
        c = coeff[1];
        aic_writePage(page, 28, (uint8_t)(c >> 24));
        aic_writePage(page, 29, (uint8_t)(c >> 16));
        aic_writePage(page, 30, (uint8_t)(c >> 8));

        // Coeff N2, Coeff C6
        c = coeff[2];
        aic_writePage(page, 32, (uint8_t)(c >> 24));
        aic_writePage(page, 33, (uint8_t)(c >> 16));
        aic_writePage(page, 34, (uint8_t)(c >> 8));
    }
    void AudioControlAIC3212::setHpfIIRCoeffOnADC_Right(uint32_t *coeff)
    {
        int page;
        uint32_t c;

        // See TI AIC3212 Application Guide, Table 2-13: http://www.ti.com/lit/an/slaa463b/slaa463b.pdf

        // Coeff N0, Coeff C36
        page = 9;
        c = coeff[0];
        aic_writePage(page, 32, (uint8_t)(c >> 24));
        aic_writePage(page, 33, (uint8_t)(c >> 16));
        aic_writePage(page, 34, (uint8_t)(c >> 8));

        // Coeff N1, Coeff C37
        c = coeff[1];
        aic_writePage(page, 36, (uint8_t)(c >> 24));
        aic_writePage(page, 37, (uint8_t)(c >> 16));
        aic_writePage(page, 38, (uint8_t)(c >> 8));

        // Coeff N2, Coeff C39
        c = coeff[2];
        ;
        aic_writePage(page, 40, (uint8_t)(c >> 24));
        aic_writePage(page, 41, (uint8_t)(c >> 16));
        aic_writePage(page, 42, (uint8_t)(c >> 8));
    }

    bool AudioControlAIC3212::mixInput1toHPout(bool state)
    {
        int page = 1;
        int reg;
        uint8_t val;

        // loop over both channels
        for (reg = 12; reg <= 13; reg++)
        { // reg 12 is Left, reg 13 is right
            val = aic_readPage(page, reg);
            if (state == true)
            {                           // activate
                val = val | 0b00000100; // set this bit.  Route IN1L to HPL
            }
            else
            {
                val = val & 0b11111011; // clear this bit.  Un-do routing of IN1L to HPL
            }
            aic_writePage(page, reg, val);
        }
        return state;
    }

} // namespace tlv320aic3212