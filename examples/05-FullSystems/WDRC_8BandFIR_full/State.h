

#ifndef State_h
#define State_h

// define structure for tracking state of system (for GUI)
class State : public TympanStateBase_UI { // look in TympanStateBase or TympanStateBase_UI for more state variables and helpful methods!!
  public:
    State(AudioSettings_F32 *given_settings, Print *given_serial, SerialManagerBase *given_sm) : TympanStateBase_UI(given_settings, given_serial, given_sm) {}

    //look in TympanStateBase for more state variables!  (like, bool flag_printCPUandMemory)
    
    //Put states here to ease the updating of the GUI
    float input_gain_dB = 0.0;   //gain of the hardware PGA in the AIC
    float digital_gain_dB = 0.0; //overall digital gain to apply to the signal
    float output_gain_dB = 0.0;  //gain of the hardware headphone amplifier in the AIC

};

#endif
