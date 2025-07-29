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
#include "input_i2s_F32.h"  //for AudioInputI2SBase_F32
//include "input_i2s_quad_F32.h"
//include "input_i2s_hex_F32.h"
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
    virtual int setNumWriteChannels(int n) {
      return numWriteChannels = max(1, min(n, AUDIOSDWRITER_MAX_CHAN));  //can be 1, 2 or 4 (3 might work but its behavior is unknown)
    }
    virtual int getNumWriteChannels(void) {
      return numWriteChannels;
    }
		virtual String getCurrentFilename(void) { return current_filename; }

		virtual void begin(void) { prepareSDforRecording(); };  //begins SD card
    virtual void prepareSDforRecording(void) = 0;
    virtual int startRecording(void) = 0;
    virtual int startRecording(const char *) = 0;
		//virtual int startRecording_noOverwrite(void) = 0;
    virtual void stopRecording(void) = 0;
		virtual void end(void) = 0;
		virtual int serviceSD(void) = 0;

  protected:
	  SdFs * sd = nullptr;
    STATE current_SD_state = STATE::UNPREPARED;
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
		  end();
		  //stopRecording();
		  delete buffSDWriter;
		}

		void setup(void) {
			buffSDWriter = new BufferedSDWriter(getOrAllocateSD());
			setWriteDataType(AudioSDWriter_F32::WriteDataType::INT16); //in SDWriter.h
		}
		void setup(Print *_serial_ptr) {
			buffSDWriter = new BufferedSDWriter(getOrAllocateSD(), _serial_ptr);
			setSerial(_serial_ptr);
			setWriteDataType(AudioSDWriter_F32::WriteDataType::INT16);  //in SDWriter.h
		}
		void setup(Print *_serial_ptr, const int _writeSizeBytes) {
			buffSDWriter = new BufferedSDWriter(getOrAllocateSD(), _serial_ptr, _writeSizeBytes);
			setSerial(_serial_ptr);
			setWriteDataType(AudioSDWriter_F32::WriteDataType::INT16);  //in SDWriter.h
			setWriteSizeBytes(_writeSizeBytes);      
		}
		SdFs* getOrAllocateSD(void) { if (sd == nullptr) { return sd = new SdFs(); } else { return sd; } }

		void setSerial(Print *_serial_ptr) {  serial_ptr = _serial_ptr;  }
		enum class WriteDataType { INT16=(int)SDWriter::WriteDataType::INT16, FLOAT32=(int)SDWriter::WriteDataType::FLOAT32 };

		virtual int setWriteDataType(AudioSDWriter_F32::WriteDataType type);
		//virtual int setWriteDataType(AudioSDWriter_F32::WriteDataType type, Print* serial_ptr, const int writeSizeBytes, const int bufferLength_bytes=-1);
		
		void setWriteSizeBytes(const int n) {  //512Bytes is most efficient for SD
			if (buffSDWriter) buffSDWriter->setWriteSizeBytes(n);
		}
		uint32_t getWriteSizeBytes(void) {  //512Bytes is most efficient for SD
			if (buffSDWriter) return buffSDWriter->getWriteSizeBytes();
			return 0;
		}
		int setNumWriteChannels(int n) {
			n = AudioSDWriter::setNumWriteChannels(n);
			if (buffSDWriter) return buffSDWriter->setNChanWAV(n);
			return n;
		}
		float setSampleRate_Hz(float fs_Hz) {
			if ((fs_Hz > 44116.0) && (fs_Hz < 44118.0)) fs_Hz = 44100.0;  //special case for Teensy 3.x...44117 is intended to be 44100
			if (buffSDWriter) return buffSDWriter->setSampleRateWAV(fs_Hz);
			return fs_Hz;
		}


		//if you want to set the audio buffer size yourself, call this method before
		//calling startRecording().
		int allocateBuffer(const uint32_t nBytes) {
			stopRecording();
			//Serial.println("AudioSDWriter_F32: buffer = " + String((int)buffSDWriter) + ", allocating buffer..." + String(nBytes));
			if (buffSDWriter) return buffSDWriter->allocateBuffer(nBytes);
			return -2;     
		}
		int allocateBuffer(void) {  // this ends up using the default buffer size
			//Serial.println("AudioSDWriter_F32: allocateBuffer: buffer = " + String((int)buffSDWriter) + ", allocating buffer()");
			if (buffSDWriter) return buffSDWriter->allocateBuffer(); //use default buffer size
			return -2;
		}
		void freeBuffer(void) {
			//Serial.println("AudioSDWriter_F32: buffer = " + String((int)buffSDWriter) + ", freeing buffer...");
			if (buffSDWriter) buffSDWriter->freeBuffer(); // free memory allocated to buffer
		}

		void prepareSDforRecording(void) override; //you can call this explicitly, or startRecording() will call it automatcally
		void end(void) override;

		int startRecording(void) override;    //call this to start a WAV recording...automatically generates a filename
		int startRecording(const char* fname) override; //or call this to specify your own filename.
		//int startRecording_noOverwrite(void);
		void stopRecording(void) override;    //call this to stop recording
		int deleteAllRecordings(void);  //clears all AUDIOxxx.wav files from the SD card

		//update is called by the Audio processing ISR.  This update function should
		//only service the recording queues so as to buffer the audio data.
		//The acutal SD writing should occur in the loop() as invoked by a service routine
		void update(void) override;

		//this method is used by update() to stuff audio into the memory buffer for writing
		void copyAudioToWriteBuffer(audio_block_f32_t *audio_blocks[], const int numChan);

		//In the loop(), the user must call serviceSD regularly so that the system will actually
		//write the audio to the SD.  This is the routine htat  pulls data from the buffer and 
		//sends to SD for writing.  If you don't call this routine, the audio will just pile up
		//in the buffer and then overflow
		int serviceSD(void) override {
			if (buffSDWriter) return buffSDWriter->writeBufferedData();
			return false;
		}
		int serviceSD_withWarnings(void);
		int serviceSD_withWarnings(AudioInputI2SBase_F32 &i2s_in);
		//int serviceSD_withWarnings(AudioInputI2S_F32 &i2s_in);
		//int serviceSD_withWarnings(AudioInputI2SQuad_F32 &i2s_in);
		//int serviceSD_withWarnings(AudioInputI2SHex_F32 &i2s_in);
		void checkMemoryI2S(AudioInputI2SBase_F32 &i2s_in);
		//void checkMemoryI2S(AudioInputI2S_F32 &i2s_in);
		//void checkMemoryI2S(AudioInputI2SQuad_F32 &i2s_in);
		//void checkMemoryI2S(AudioInputI2SHex_F32 &i2s_in);

		bool isFileOpen(void) {
			if (buffSDWriter) return buffSDWriter->isFileOpen();
			return false;
		}
		uint32_t getNumUnfilledSamplesInBuffer(void) { 
			if (buffSDWriter) return (buffSDWriter->getNumUnfilledBytesInBuffer())/(buffSDWriter->getBytesPerSample());  //how much of the buffer is empty, in samples
			return 0; 
		}  
		uint32_t getNumUnfilledSamplesInBuffer_msec(void) { 
			if (buffSDWriter) return buffSDWriter->getNumUnfilledBytesInBuffer_msec();   //how much of the buffer is empty, in msec
			return 0; 
		}
		unsigned long getStartTimeMillis(void) { return t_start_millis; };
		unsigned long setStartTimeMillis(void) { return t_start_millis = millis(); };
		SdFs * getSdPtr(void) { 
			if (buffSDWriter) return buffSDWriter->getSdPtr(); 
			return sd;
		}
			

	protected:
		audio_block_f32_t *inputQueueArray[AUDIOSDWRITER_MAX_CHAN]; //up to four input channels
		BufferedSDWriter *buffSDWriter = 0;
		Print *serial_ptr = &Serial;
		unsigned long t_start_millis = 0;

		bool openAsWAV(const char *fname) {
			if (buffSDWriter) return buffSDWriter->openAsWAV(fname);
			return false;
		}
		bool open(const char *fname) {
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
		void printHelp(void) override;
		//bool processCharacter(char c) override; //no used here
		bool processCharacterTriple(char mode_char, char chan_char, char data_char) override;
		void setFullGUIState(bool activeButtonsOnly = false) override;  
		// /////////////////////////////////	
		
		//create the button sets for the TympanRemote's GUI
		virtual TR_Card* addCard_sdRecord(TR_Page *page_h);
		virtual TR_Card* addCard_sdRecord(TR_Page *, String);
		virtual TR_Page* addPage_sdRecord(TympanRemoteFormatter *gui);
		virtual TR_Page* addPage_default(TympanRemoteFormatter *gui) { return addPage_sdRecord(gui); };
		virtual void setSDRecordingButtons(bool activeButtonsOnly = false);
};



#endif

