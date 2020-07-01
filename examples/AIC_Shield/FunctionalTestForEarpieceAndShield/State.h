#ifndef State_h
#define State_h

enum Mic_Channels {
  MIC_FRONT_LEFT, 
  MIC_REAR_LEFT, 
  MIC_FRONT_RIGHT, 
  MIC_REAR_RIGHT, 
  ALL_MICS
  };

enum Mic_Input {
  INPUT_PCBMICS,
  INPUT_MICJACK_MIC,
  INPUT_MICJACK_LINEIN,
  INPUT_LINEIN_SE,
  INPUT_PDMMICS
  };

#endif
