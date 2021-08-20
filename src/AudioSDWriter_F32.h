/*
 * AudioSDWriter_F32
 * 
 * Created: Chip Audette, OpenAudio, Apr 2019
 * 
 * Purpose: This class tries to simplify writing audio from the Tympan/OpenAudio/Teensy
 *   Audio processing paradigm to the SD card.  

   MIT License.  Use at your own risk.
*/

#ifndef _AudioSDWriter_F32_h
#define _AudioSDWriter_F32_h

#include <Arduino.h>  //for Serial
#include "AudioSettings_F32.h"
#include "AudioStream_F32.h"

#include "SDWriter.h"
#include "SerialManager_UI.h"
#include "TympanRemoteFormatter.h"


//variables to control printing of warnings and timings and whatnot
#define PRINT_FULL_SD_TIMING 0    //set to 1 to print timing information of *every* write operation.  Great for logging to file.  Bad for real-time human reading.

//how many channels do we want to allow for?
#define AUDIOSDWRITER_MAX_CHAN 4

//AudioSDWriter: A class to write data from audio blocks as part of the 
//   Teensy/Tympan audio processing paradigm.  The AudioSDWriter class is 
//   just a virtual Base class.  Use AudioSDWriter_F32 further down.
class AudioSDWriter {
  public:
 	AudioSDWriter(void) {};
    AudioSDWriter(SdFs * _sd) {
		sd = _sd;
	};
    enum class STATE { UNPREPARED = -1, STOPPED, RECORDING };
    STATE getState(void) {
      return current_SD_state;
    };
    enum class WriteDataType { INT16 }; //in the future, maybe add FLOAT32
    virtual int setNumWriteChannels(int n) {
      return numWriteChannels = max(1, min(n, AUDIOSDWRITER_MAX_CHAN));  //can be 1, 2 or 4 (3 might work but its behavior is unknown)
    }
    virtual int getNumWriteChannels(void) {
      return numWriteChannels;
    }
	virtual String getCurrentFilename(void) { return current_filename; }

    virtual void prepareSDforRecording(void) = 0;
    virtual int startRecording(void) = 0;
    virtual int startRecording(char *) = 0;
	//virtual int startRecording_noOverwrite(void) = 0;
    virtual void stopRecording(void) = 0;

  protected:
	SdFs * sd;
    STATE current_SD_state = STATE::UNPREPARED;
    WriteDataType writeDataType = WriteDataType::INT16;
    int recording_count = 0;
    int numWriteChannels = 2;
	String current_filename = String("Not Recording");
};

//AudioSDWriter_F32: A class to write data from audio blocks as part
//   of the Teensy/Tympan audio processing paradigm.  For this class, the
//   audio is given as float32 and written as int16
class AudioSDWriter_F32 : public AudioSDWriter, public AudioStream_F32 {
  //GUI: inputs:4, outputs:0 //this line used for automatic generation of GUI node
  public:
    AudioSDWriter_F32(void) :
      AudioSDWriter(),
      AudioStream_F32(AUDIOSDWRITER_MAX_CHAN, inputQueueArray)
    { 
      setup();
    }
    AudioSDWriter_F32(const AudioSettings_F32 &settings) :
      AudioSDWriter(),
      AudioStream_F32(AUDIOSDWRITER_MAX_CHAN, inputQueueArray)
    { 
      setup(); 
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    AudioSDWriter_F32(const AudioSettings_F32 &settings, Print* _serial_ptr) :
      AudioSDWriter(),
      AudioStream_F32(AUDIOSDWRITER_MAX_CHAN, inputQueueArray)
    { 
      setup(_serial_ptr);
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    AudioSDWriter_F32(const AudioSettings_F32 &settings, Print* _serial_ptr, const int _writeSizeBytes) :
      AudioSDWriter(),
      AudioStream_F32(AUDIOSDWRITER_MAX_CHAN, inputQueueArray)
    { 
      setup(_serial_ptr, _writeSizeBytes); 
      setSampleRate_Hz(settings.sample_rate_Hz); 
    }
    AudioSDWriter_F32(SdFs * _sd) :
      AudioSDWriter(_sd),
      AudioStream_F32(AUDIOSDWRITER_MAX_CHAN, inputQueueArray)
    { 
      setup();
    }
    AudioSDWriter_F32(SdFs * _sd,const AudioSettings_F32 &settings) :
      AudioSDWriter(_sd),
      AudioStream_F32(AUDIOSDWRITER_MAX_CHAN, inputQueueArray)
    { 
      setup(); 
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    AudioSDWriter_F32(SdFs * _sd,const AudioSettings_F32 &settings, Print* _serial_ptr) :
      AudioSDWriter(_sd),
      AudioStream_F32(AUDIOSDWRITER_MAX_CHAN, inputQueueArray)
    { 
      setup(_serial_ptr);
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    AudioSDWriter_F32(SdFs * _sd,const AudioSettings_F32 &settings, Print* _serial_ptr, const int _writeSizeBytes) :
      AudioSDWriter(_sd),
      AudioStream_F32(AUDIOSDWRITER_MAX_CHAN, inputQueueArray)
    { 
      setup(_serial_ptr, _writeSizeBytes); 
      setSampleRate_Hz(settings.sample_rate_Hz); 
    }
    ~AudioSDWriter_F32(void) {
      stopRecording();
      delete buffSDWriter;
    }

    void setup(void) {
      setWriteDataType(WriteDataType::INT16, &Serial, DEFAULT_SDWRITE_BYTES);
    }
    void setup(Print *_serial_ptr) {
      setSerial(_serial_ptr);
      setWriteDataType(WriteDataType::INT16, _serial_ptr, DEFAULT_SDWRITE_BYTES);
    }
    void setup(Print *_serial_ptr, const int _writeSizeBytes) {
      setSerial(_serial_ptr);
      setWriteDataType(WriteDataType::INT16, _serial_ptr, _writeSizeBytes);
    }

    void setSerial(Print *_serial_ptr) {  serial_ptr = _serial_ptr;  }
	
    int setWriteDataType(WriteDataType type);
	int setWriteDataType(WriteDataType type, Print* serial_ptr, const int writeSizeBytes, const int bufferLength_samps=-1);
	
    void setWriteSizeBytes(const int n) {  //512Bytes is most efficient for SD
      if (buffSDWriter) buffSDWriter->setWriteSizeBytes(n);
    }
    int getWriteSizeBytes(void) {  //512Bytes is most efficient for SD
      if (buffSDWriter) return buffSDWriter->getWriteSizeBytes();
      return 0;
    }
    int setNumWriteChannels(int n) {
		n = AudioSDWriter::setNumWriteChannels(n);
      if (buffSDWriter) return buffSDWriter->setNChanWAV(n);
      return n;
    }
    float setSampleRate_Hz(float fs_Hz) {
		if ((fs_Hz > 44116.0) && (fs_Hz < 44118.0)) fs_Hz = 44100.0;  //special case for Teensy...44117 is intended to be 44100
      if (buffSDWriter) return buffSDWriter->setSampleRateWAV(fs_Hz);
      return fs_Hz;
    }

	
    //if you want to set the audio buffer size yourself, call this method before
	//calling startRecording().
    int allocateBuffer(const int nBytes) {
		if (buffSDWriter) return buffSDWriter->allocateBuffer(nBytes);
		return -2;     
    }
    int allocateBuffer(void) {  // this ends up using the default buffer size
		if (buffSDWriter) return buffSDWriter->allocateBuffer(); //use default buffer size
		return -2;
    }

    void prepareSDforRecording(void); //you can call this explicitly, or startRecording() will call it automatcally
    int startRecording(void);    //call this to start a WAV recording...automatically generates a filename
    int startRecording(char* fname); //or call this to specify your own filename.
	//int startRecording_noOverwrite(void);
    void stopRecording(void);    //call this to stop recording
	int deleteAllRecordings(void);  //clears all AUDIOxxx.wav files from the SD card

    //update is called by the Audio processing ISR.  This update function should
    //only service the recording queues so as to buffer the audio data.
    //The acutal SD writing should occur in the loop() as invoked by a service routine
    void update(void);

	//this method is used by update() to stuff audio into the memory buffer for writing
    virtual void copyAudioToWriteBuffer(audio_block_f32_t *audio_blocks[], const int numChan);
    
    //In the loop(), the user must call serviceSD regularly so that the system will actually
	//write the audio to the SD.  This is the routine htat  pulls data from the buffer and 
	//sends to SD for writing.  If you don't call this routine, the audio will just pile up
	//in the buffer and then overflow
    int serviceSD(void) {
      if (buffSDWriter) return buffSDWriter->writeBufferedData();
      return false;
    }
	int serviceSD_withWarnings(void);

	 bool isFileOpen(void) {
      if (buffSDWriter) return buffSDWriter->isFileOpen();
      return false;
    }

	
  unsigned long getStartTimeMillis(void) { return t_start_millis; };
  unsigned long setStartTimeMillis(void) { return t_start_millis = millis(); };
  SdFs * getSdPtr(void) { 
	if (!buffSDWriter) return buffSDWriter->getSdPtr(); 
	return sd;
  }
		

  protected:
    audio_block_f32_t *inputQueueArray[AUDIOSDWRITER_MAX_CHAN]; //up to four input channels
    BufferedSDWriter *buffSDWriter = 0;
    Print *serial_ptr = &Serial;
    unsigned long t_start_millis = 0;

    bool openAsWAV(char *fname) {
      if (buffSDWriter) return buffSDWriter->openAsWAV(fname);
      return false;
    }
    bool open(char *fname) {
      if (buffSDWriter) return buffSDWriter->open(fname);
      return false;
    }

    int close(void) {
      if (buffSDWriter) return buffSDWriter->close();
      return 0;
    }
};


// ////////////////////////////////////////////////////////////////////////////////////////////////
//
// UI Versions of the Classes
//
// These versions of the classes add no signal processing functionality.  Instead, they add to the
// classes simply to make it easier to add a menu-based or App-based interface to configure and 
// control the audio-processing classes above.
//
// If you want to add a GUI, you might consider using the classes below instead of the classes above.
// Again, the signal processing is exactly the same either way.
//
// ////////////////////////////////////////////////////////////////////////////////////////////////

// Here's a class that wraps AudioSDWriter_F32 with some functions to interact
// via a serial menu and via the Tympan_Remote App.
class AudioSDWriter_F32_UI : public AudioSDWriter_F32, public SerialManager_UI {
	//GUI: inputs:4, outputs:0 //this line used for automatic generation of GUI node
	public:
		//copy all of the constructors from AudioSDWriter_F32...just pass through to the underlying constructors...nothing new being done here
		AudioSDWriter_F32_UI(void) : 
			AudioSDWriter_F32(), SerialManager_UI() {};
			
		AudioSDWriter_F32_UI(const AudioSettings_F32 &settings) : 
			AudioSDWriter_F32(settings), SerialManager_UI() {};
			
		AudioSDWriter_F32_UI(const AudioSettings_F32 &settings, Print* _serial_ptr) : 
			AudioSDWriter_F32(settings, _serial_ptr), SerialManager_UI() {};
			
		AudioSDWriter_F32_UI(const AudioSettings_F32 &settings, Print* _serial_ptr, const int _writeSizeBytes) : 
			AudioSDWriter_F32(settings, _serial_ptr, _writeSizeBytes), SerialManager_UI() {};
			
		AudioSDWriter_F32_UI(SdFs * _sd) : 
			AudioSDWriter_F32(_sd), SerialManager_UI() {};
			
		AudioSDWriter_F32_UI(SdFs * _sd, const AudioSettings_F32 &settings) :
			AudioSDWriter_F32( _sd, settings), SerialManager_UI() {};
			
		AudioSDWriter_F32_UI(SdFs * _sd, const AudioSettings_F32 &settings, Print* _serial_ptr) : 
			AudioSDWriter_F32(_sd, settings, _serial_ptr), SerialManager_UI() {};
			
		AudioSDWriter_F32_UI(SdFs * _sd,const AudioSettings_F32 &settings, Print* _serial_ptr, const int _writeSizeBytes) :
			AudioSDWriter_F32(settings, _serial_ptr, _writeSizeBytes), SerialManager_UI() {};
			
		 // ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void);
		//virtual bool processCharacter(char c); //no used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
		virtual void setFullGUIState(bool activeButtonsOnly = false);  
		// /////////////////////////////////	
		
		//create the button sets for the TympanRemote's GUI
		virtual TR_Card* addCard_sdRecord(TR_Page *page_h);
		virtual TR_Card* addCard_sdRecord(TR_Page *, String);
		virtual TR_Page* addPage_sdRecord(TympanRemoteFormatter *gui);
		virtual TR_Page* addPage_default(TympanRemoteFormatter *gui) { return addPage_sdRecord(gui); };
		virtual void setSDRecordingButtons(bool activeButtonsOnly = false);
};



#endif

