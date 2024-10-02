

#ifndef State_h
#define State_h

extern const int N_CHAN;

// The purpose of this class is to be a central place to hold important information
// about the algorithm parameters or the operation of the system.  This is mainly to
// help *you* (the programmer) know and track what is important.  
//
// For example, if it is important enough to display or change in the GUI, you might
// want to put it here.  It is a lot easier to update the GUI if all the data feeding
// the GUI lives in one place.
//
// Also, in the future, you might imagine that you want to save all of the configuration
// and parameter settings together as some sort of system-wide preset.  Then, you might
// save the preset to the SD card to recall later.  If you were to get in the habit of
// referencing all your important parameters in one place (like in this class), it might
// make it easier to implement that future capability to save and recal presets

// define a class for tracking the state of system (primarily to help our implementation of the GUI)
class State : public TympanStateBase_UI { // look in TympanStateBase or TympanStateBase_UI for more state variables and helpful methods!!
  public:
    State(AudioSettings_F32 *given_settings, Print *given_serial, SerialManagerBase *given_sm) : TympanStateBase_UI(given_settings, given_serial, given_sm) {}

    //look in TympanStateBase for more state variables!  (like, bool flag_printCPUandMemory)

    //variables related to which input is used
    enum INP_SOURCE {INPUT_PCBMICS=0, INPUT_JACK_MIC, INPUT_JACK_LINE};
    int input_source = INPUT_JACK_LINE;

    //set highpass filter on ADC
    float adc_hp_cutoff_Hz = 31.25f;

    //variables related to the test tone mode
    enum Test_Tone_Mode { TONE_MODE_MUTE, TONE_MODE_STEADY, TONE_MODE_STEPPED_FREQUENCY};
    int current_test_tone_mode = TONE_MODE_STEADY;
    float stepped_test_start_freq_Hz = 20.0;  //starting frequency for stepped tones
    float stepped_test_end_freq_Hz = 20000.0; //ending frequency for stepped tones
    float stepped_test_step_Hz = 50.0;        //frequency inrement for stepped tones
    float stepped_test_step_dur_sec = 1.0;    //duration at each step
    unsigned long stepped_test_next_change_millis = 0UL;
  
    //variables related to the current sine wave
    float default_sine_freq_Hz   = 1000.0;  //used whenever switching the TONE_MODE
    float default_sine_amplitude = sqrt(2.0)*sqrt(pow(10.0,0.1*-20.0));   //(-20dBFS converted to linear and then converted from RMS to amplitude)
    float sine_freq_Hz           = 1000.0;  //the current frequency
    float sine_amplitude         = 0.0;     //the current amplitude 

    //variables relating to the output switching of the sine wave
    enum OUT_CHAN { OUT_LEFT=0, OUT_RIGHT=1, OUT_BOTH=9};
    int output_chan = OUT_BOTH;

    //variables associated with level measurement
    float calcLevel_timeWindow_sec = 0.125f;
    
    //Put different gain settings (except those in the compressors) here to ease the updating of the GUI
    float input_gain_dB  = 0.0;   //gain of the hardware PGA in the AIC
    //float output_gain_dB = 0.0;  //gain of the hardware headphone amplifier in the AIC

    //variables related ot printing of output
    bool enable_printCpuToUSB = false;
    bool flag_printInputLevelToUSB = true;
    bool flag_printOutputLevelToUSB = false;

};

#endif
