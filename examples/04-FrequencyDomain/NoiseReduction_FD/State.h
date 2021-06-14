

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
    bool NR_enable = true;
    float NR_attack_sec = 10.0;
    float NR_release_sec = 3.0;
    float NR_max_atten_dB = 16.0;
    float NR_SNR_at_max_atten_dB = 6.0;
    float NR_transition_width_dB = 6.0;
    float NR_gain_smooth_sec = 0.005;
    bool NR_enable_noise_est_updates = true;
    
};

#endif
