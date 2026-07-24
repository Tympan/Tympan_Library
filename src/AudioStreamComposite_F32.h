/*
  AudioStreamComposite_F32.h

  AudioStreamComposite_F32 base class.

  Created by Chip Audette, 26-Sep-2024 for Tympan_Sandbox/OpenHearing/TestSwitchedConnections_composite
  https://github.com/Tympan/Tympan_Sandbox/blob/master/OpenHearing/TestSwitchedConnections_composite
*/


#ifndef AudioStreamComposite_F32_h
#define AudioStreamComposite_F32_h

#include <Tympan_Library.h>
#include <AudioSwitchMatrix_F32.h>
#include <vector>
#include <Audio.h>  //for AudioNoInterrupts() and AudioInterrupts()

// AudioStreamComposite_F32:
//
// This is a base class intended to provide an interface (and some common functionality) for derived AudioStreamComposite_F32
// classes that are actually employed by the user.
//
// The idea is that an AudioStreamComposite_F32 is a composite class that instantiates and manages a whole chain of 
// AudioStream_F32 audio-processing objects.  One can simply instantiate an AudioStreamComposite_F32 and all of the constituant
// objects and connections are created.  The only thing left to do is connect the final output.
//
class AudioStreamComposite_F32: public AudioStream_F32 {
  public:
    AudioStreamComposite_F32(void) : 
      AudioStream_F32(4, inputQueueArray) {}
    AudioStreamComposite_F32(const AudioSettings_F32 &_audio_settings) : 
      AudioStream_F32(4, inputQueueArray) {}
    
    // FUTURE IMPLEMENTATION OF DESTRUCTOR:
    // ====================================
    // //Destructor: note that destroying Audio Objects is not a good idea because AudioStream (from Teensy)
    // //does not currently support (as of Sept 9, 2024) having instances of AudioStream be destroyed.  The
    // //problem is that AudioStream keeps pointers to all AudioStream instances ever created.  If an AudioStream
    // //instance is deleted, AudioStream does not remove the instance from its lists.  So, it has stale pointers
    // //that eventually take down the system.
    // //So, it is *not* recommended that you destroy AudioStream objects or, therefore, AudioPath objects
    // virtual ~AudioStreamComposite_F32() {
    //   for (int i=(int)patchCords.size()-1; i>=0; i--) delete patchCords[i]; //destroy all the Audio Connections (in reverse order...maybe less memroy fragmentation?)
    //   Serial.println("AudioStreamComposite_F32: Destructor: *** WARNING ***: destroying AudioStream instances, which is not supported by Teensy's AudioStream.");
    //   for (int i=(int)audioObjects.size()-1; i>=0; i--) delete audioObjects[i];       //destroy all the audio class instances (in reverse order...maybe less memroy fragmentation?)
    // }

    //Define the required update() method.
    //Goal: move the received audo blocks from this AudioStreamComposite's inputs to the inputs of our start node.
    //Note 1: The update methods for the constituant classes should be called automatically by AudioStream.
    //Note 2: The update method defined here for the AudioStreamComposite_F32 should NOT have to be overriden by
    //derived classes of AudioStreamComposite, unless something new is specifically desired by your derived class.
    virtual void update(void) {
      //loop over each possible input to this composite AudioStream object
      for (int Iinput = 0; Iinput < num_inputs_f32; Iinput++) {
        //get any audio block waiting for us
        audio_block_f32_t *block = receiveReadOnly_f32(Iinput);
        //see if the block is valid
        if (block) {
          //see if we have a valid startNode
          if (startNode) { //make sure it's not NULL
            //transmit the block to the startNode, which following along with the behavior from AudioStream_F32::transmit(),
            //is to just set the destinations inputQueue.  The desitation is the first node (startNode) so go ahead and move
            //the audio block to the start node's ipnut queue
            startNode->putBlockInInputQueue(block, Iinput);
          }
        }
        //release the block, thereby releasing ownership of the block by this composite AudiosStream_F32 object.  (decreements ref_count)
        AudioStream_F32::release(block);
      }
    }

    //Declare a reset method to restore all components to their desired initial values
    virtual void reset(void) {};

    //set the hardware, if configuring the hardware is needed by your derived class
    virtual void setTympanHardware(Tympan *ptr) { tympan_ptr = ptr; }
    void setTympanHardware(Tympan *ptr1, AICShield *ptr2) { tympan_ptr = ptr1; shield_ptr = ptr2; }
    
    //expose the starting and ending nodes to enable audio connections between this AudioPath and the rest of the world
    virtual AudioSwitchMatrix4_F32* getStartNode(void) { if (startNode == NULL) Serial.println("AudioStreamComposite_F32 (" + instanceName + "): getStartNode: *** WARNING ***: Returning NULL pointer."); return startNode; }
    virtual AudioForwarder4_F32* getEndNode(void)   { if (endNode == NULL)   Serial.println("AudioStreamComposite_F32 (" + instanceName + "): getEndNode: *** WARNING ***: Returning NULL pointer.");   return endNode;   }

    //Loop through each audio object and set them all to the desired state of active or inactive.
    //Per the rules of AudioStream::update_all(), active = false should mean that the audio object
    //is not invoked, thereby saving CPU.
    bool setActive(bool _active) override { return setActive(_active, false); }
    bool setActive(bool _active, int flag_moreSetup) override {
      bool interrupt_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE);
      if (interrupt_enabled) {
        AudioNoInterrupts();  //stop the audio processing in order to change all the active states (from Audio library)
      } else {
        // Already disabled.
      }

      //call the default AudioStream_F32::setActive, which will activate this AudioStreamComposite too
      AudioStream_F32::setActive(_active, flag_moreSetup); //do the default actions (which, I think, ignores flag_setupHardware) and sets active = _active
      
      //iterate over each constituant Audio object and call its setActive method.
      for (auto & audioObject : audioObjects) { //iterate over each audio object in the audioObjects vector
        audioObject->setActive(_active, false);   // do not forward the flag_setupHardware as only the top-most composite should have control over the hardware...override this setActive method, if you disagree.
      }

      if (interrupt_enabled) {
        AudioInterrupts();  //restart the audio processing in order to change all the active states
      } else {
        // Interrupt was already disabled. Do not enable it.
      }

      //call any additional setup
      if (_active && flag_moreSetup) setup_fromSetActive();

      return active;
    }
    virtual void setup_fromSetActive(void) {}  //override this as desired in your derived class
  
    // ///////////////////////////////////////////////// Convenince methods for easing interaction with main program

    //Interface to allow for any slower main-loop updates
    virtual int serviceMainLoop(void) { return 0; }  //Do nothing by default.  You can override in your derived class, if you want to do something in the main loop.
    //Interface to ease use with SerialMonitor
    virtual void printHelp(void) { Serial.println("    : (none)"); };  //print default message
    virtual void respondToByte(char c) { };                           //default do nothing

  protected:
		const int max_n_IO_chan = 4;
    audio_block_f32_t *inputQueueArray[4];
    AudioSwitchMatrix4_F32 *startNode = NULL; //instantiate as the first audio class in your AudioPath (even if you have no inputs)
    AudioForwarder4_F32 *endNode = NULL;   //instantiate as the last audio class in your AudioPath (even if you have no outputs)
    Tympan *tympan_ptr = NULL;
    AICShield *shield_ptr = NULL;
    std::vector<AudioConnection_F32 *> patchCords;
    std::vector<AudioStream_F32 *> audioObjects;
};


#endif
