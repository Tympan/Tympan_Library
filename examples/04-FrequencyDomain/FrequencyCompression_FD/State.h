
#ifndef State_h
#define State_h

// define structure for tracking state of system (for GUI)
class State : public TympanStateBase_UI { // look in TympanStateBase for more state variables and helpful methods!!
  public:
    State(AudioSettings_F32 *given_settings, Print *given_serial) : TympanStateBase_UI(given_settings, given_serial) {}
    
    //Put states here to ease the updating of the GUI
    float input_gain_dB = 0.0;
    float digital_gain_dB = 0.0;
    float output_gain_dB = 0.0;

    //Frequency processing
    float freq_knee_Hz = 1000.0;  //knee point (Hz), above which the frequency compression begins
    float freq_CR = 1.0;          //frequency compression factor, 1.0 is no compression, >1.0 is compression
    float freq_shift_Hz = 0.0;    //frequency shifting (Hz)
    
};

#endif
