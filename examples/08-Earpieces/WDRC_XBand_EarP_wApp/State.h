

#ifndef State_h
#define State_h

extern const int N_CHAN;

// define structure for tracking state of system (for GUI)
class State : public TympanStateBase_UI { // look in TympanStateBase or TympanStateBase_UI for more state variables and helpful methods!!
  public:
    State(AudioSettings_F32 *given_settings, Print *given_serial, SerialManagerBase *given_sm) : TympanStateBase_UI(given_settings, given_serial, given_sm) {}

    //look in TympanStateBase for more state variables!  (like, bool flag_printCPUandMemory)
    
    //Put different gain settings here to ease the updating of the GUI
    //float input_gain_dB = 0.0;   //gain of the hardware PGA in the AIC
    float digital_gain_dB = -25.0; //overall digital gain to apply to the signal
    float output_gain_dB = 0.0;  //gain of the hardware headphone amplifier in the AIC

    //Put different algorithm prescriptions here
    enum {DSL_NORMAL=0, DSL_FULLON=1};
    int MAX_DSL_SETTING = DSL_FULLON;
    int current_dsl_config = DSL_NORMAL;

    //Other classes holding states
    EarpieceMixerState *earpieceMixer;
    AudioFilterbankState *filterbank;
    AudioEffectCompBankWDRCState *compbank;

    //keep track of the signal levels in teh different bands
    float overall_cal_dBSPL_at0dBFS = 115.0f; //dB SPL at full scale (0dB FS).  This will be set by the DSL_GHA_Preset0.h or DSL_GHA_Preset1.h
    bool enable_printAveSignalLevels = false;
    bool printAveSignalLevels_as_dBSPL = false;
    float aveSignalLevels_dBFS[N_CHAN];
    void printAveSignalLevels(unsigned long curTime_millis, unsigned long updatePeriod_millis = 3000) {
      static unsigned long lastUpdate_millis = 0;
      if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
      if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
        printAveSignalLevelsMessage(printAveSignalLevels_as_dBSPL);
        lastUpdate_millis = curTime_millis; //we will use this value the next time around.
      }
    }
    void printAveSignalLevelsMessage(bool as_dBSPL) {
      float offset_dB = 0.0f;
      String units_txt = String("dBFS");
      if (as_dBSPL) { offset_dB = overall_cal_dBSPL_at0dBFS;  units_txt = String("dBSPL, approx"); }
      Serial.println("Ave Input Level (" + units_txt + "), Per-Band = ");
      for (int i=0; i<N_CHAN; i++) Serial.print(String(aveSignalLevels_dBFS[i]+offset_dB,1) + ", "); 
      Serial.println();
    }

    

};

#endif
