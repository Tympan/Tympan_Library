
#ifndef _StereoContainer_Biquad_WDRC_UI_h
#define _StereoContainer_Biquad_WDRC_UI_h

#include <AudioEffectMultiBandWDRC_F32.h> //in Tympan_Library, includes StereoContainerWDRC_UI

class StereoContainer_Biquad_WDRC_UI : public StereoContainerWDRC_UI {  //StereoContainerWDRC_UI is in Tympan_Library in AudioEffectMultiBandWDRC_F32.h
  public:
    StereoContainer_Biquad_WDRC_UI(void) : StereoContainerWDRC_UI() {};

    virtual void addPairBiquads(AudioFilterBiquad_F32_UI *_left, AudioFilterBiquad_F32_UI *_right) {
      //set the local pointers
      leftBiquad = _left;  rightBiquad = _right;

      //attach its components to our lists of left-right algorithm elements to be controlled via GUI
      add_item_pair(_left, _right);

      //Set the ID_char used for GUI fieldnames so that the right's are the same as the left's.
      //Only do this if we're only going to have one set of GUI elements to display both left
      //and right.  This is my intention, so I'm going to go ahead and do it.  
      //
      //Discussion: We probably want to always do this for stereo pairs of algorithm blocks?  Why else
      //would we be using the stereo container?  If so, this changing of the ID_char should probably
      //be moved into StereoContainer_UI?  I'm thinking "yes", but I need to live with this a bit more
      for (int i=0; (unsigned int)i<items_L.size(); i++) { items_R[i]->set_ID_char_fn(items_L[i]->get_ID_char_fn()); }
    }

    virtual TR_Page* addPage_filterSettings(TympanRemoteFormatter *gui) {
      if (gui == NULL) return NULL;
      if (leftBiquad == NULL) return NULL;

      TR_Page *page_h = gui->addPage(leftBiquad->name_for_UI);
      TR_Page page_foo; TR_Page *page_foo_h = &page_foo;  //these are created as dummies just to hand to UI elements that need to be activated not actually shown in the App's GUI
      if (page_h == NULL) return NULL;
      
      addCard_chooseChan(page_h); //see StereoContainer_UI.h
      if ((leftBiquad != NULL) && (leftBiquad != NULL)) {
        leftBiquad->addCard_filterBypass(page_h);
        leftBiquad->addCard_cutoffFreq(page_h);
        leftBiquad->addCard_filterQ(page_h);
        leftBiquad->addCard_filterBW(page_h);

        //AudioFilterBiquad might track which GUI elements have been invoked.  The code below triggers that automatic behavior
        rightBiquad->addCard_filterBypass(page_foo_h);
        rightBiquad->addCard_cutoffFreq(page_foo_h);
        rightBiquad->addCard_filterQ(page_foo_h);
        rightBiquad->addCard_filterBW(page_foo_h);
      }

      return page_h;
    }    

  protected:
    AudioFilterBiquad_F32_UI *leftBiquad = NULL, *rightBiquad = NULL;
};

#endif
