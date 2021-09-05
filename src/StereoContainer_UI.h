
#ifndef _StereoContainer_UI_h
#define _StereoContainer_UI_h

/* 
 *  StereoContainer_UI
 *  
 *  Created: Chip Audette, OpenAudio, July-Sept 2021
 *  
 *  Purpose: The purpose of this class is to help you make your TympanRemote App GUI handle seperate settings 
 *  for the left and right ears.  
 *  
 *      * This class expects that the algorithm blocks are the same for the left and right
 *        but that you want to be able to change the settings independetly for the left and right.
 *        
 *      * This class assumes that the algorithm blocks used by this container have their own
 *        App GUI functionality (ie, they inherit from SerialManager_UI
 *        
 *      * Typically, this StereoContainer_UI is insufficient by itself.  It ends up being the parent
 *        class for your own speciallized child class that inherits the core functionality from this
 *        parent.
 *        
 *  Functionality Provided:  Here's how you use it and what functionality it provides:
 *      * You need to create the audio classes for the left and right ears.  This class here is used to 
 *        hold pointers to those classes.  This class holds the pointers in pairs: one pointer for the 
 *        left ear and the corresponding for the right ear.  You can append as many pairs as you'd like.
 *        This is the setup required in order to get the benefits of the bullets below.
 *      * You then create your TympanRemote App GUI in your normal way.  Presumably, you will have a page
 *        (or more) to control the algorithms.  Instead of creating a  page for the left-ear instance 
 *        and a separate page for the right-ear instance, you can put a pair of buttons at the top
 *        of the page to toggle the view between the left-ear and right-ear settings.  This class provides
 *        those buttons.
 *      * Once you only have one set of controls for the left/right, this class remembers what left/right
 *        state the App GUI is in and, for any button presses controlling the alogirthms, routes the
 *        commands to the correct left or right instance of the algorithm.  T
 *        
 *  License: MIT License.  Use at your own risk.  Have fun!
 *   
 */

#include <Arduino.h>
#include <SerialManager_UI.h>       //from Tympan_Library
#include <TympanRemoteFormatter.h>      //from Tympan_Library
#include <vector>

class StereoContainerState {
  public:
    StereoContainerState(void) {};
    int cur_channel = 0; 
        
  protected:
};

class StereoContainer_UI : public SerialManager_UI {
  public:
    StereoContainer_UI(void) : SerialManager_UI() {};

    enum channel {LEFT=0, RIGHT=1};


     void add_item_pair(SerialManager_UI *left_item, SerialManager_UI *right_item);

    virtual int get_cur_channel(void) { return state.cur_channel; }
    virtual int set_cur_channel(int val) { val = max(LEFT,min(RIGHT, val)); return state.cur_channel = val; }
    
    std::vector<SerialManager_UI *> items_L;  //this really only needs to be [max_n_filters-1] in length, but we'll generally allocate [max_n_filters] just to avoid mistaken overruns
    std::vector<SerialManager_UI *> items_R;  //this really only needs to be [max_n_filters-1] in length, but we'll generally allocate [max_n_filters] just to avoid mistaken overruns

    StereoContainerState state;

    // /////////////////////////////////////// Overload methods from SerialManager_UI
    void setSerialManager(SerialManagerBase *_sm);

    // ///////////////////////////////////////  Required methods because of SerialManager_UI
    virtual void printHelp(void);
    virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
    virtual void setFullGUIState(bool activeButtonsOnly = false); 
    // ///////// end of required methods
    virtual bool processCharacter(char c);
    

    // /////////////////////////////////////// Tympan Remote Formatter Methods

    TR_Card* addCard_chooseChan(TR_Page *page_h); //left or right
    void updateCard_chooseChan(bool activeButtonsOnly = false);
    //TR_Page* addPage_default(TympanRemoteFormatter *gui) {};
    
  protected:  
    virtual bool processCharacterTriple_self(char mode_char, char chan_char, char data_char);
    virtual bool processCharacterTriple_items(char item_ID_char, char chan_char, char data_char, int item_index);

  
};


#endif
