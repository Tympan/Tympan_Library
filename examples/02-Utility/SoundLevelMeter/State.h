
#ifndef State_h
#define State_h


// define structure for tracking state of system (for GUI)
class State : public TympanStateBase_UI {  // look in TympanStateBase for more state variables and helpful methods!!
  public:
    State(AudioSettings_F32 *given_settings, Print *given_serial) : TympanStateBase_UI(given_settings, given_serial) {}

    bool enable_printTextToUSB=true;
    bool enable_printTextToBLE=false;
    bool enable_printPlotToBLE=false;

    enum FREQ_WEIGHT {FREQ_A_WEIGHT=0, FREQ_C_WEIGHT};
    int cur_freq_weight = FREQ_A_WEIGHT; //default
    
    enum TIME_WEIGHT {TIME_SLOW=0, TIME_FAST};
    int cur_time_averaging = TIME_SLOW;  //default

};

#endif
