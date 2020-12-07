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

#include "SDWriter.h"
#include "AudioSettings_F32.h"
#include "AudioStream_F32.h"

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
    int setWriteDataType(WriteDataType type) {
      Print *serial_ptr = &Serial1;
      int write_nbytes = DEFAULT_SDWRITE_BYTES;

      //get info from previous objects
      if (buffSDWriter) {
        serial_ptr = buffSDWriter->getSerial();
        write_nbytes = buffSDWriter->getWriteSizeBytes();
      }

      //make the full method call
      return setWriteDataType(type, serial_ptr, write_nbytes);
    }
	int setWriteDataType(WriteDataType type, Print* serial_ptr, const int writeSizeBytes, const int bufferLength_samps=-1) {
		stopRecording();
		writeDataType = type;
		if (!buffSDWriter) {
			Serial.println("AudioSDWriter_F32: setWriteDataType: creating buffSDWriter...");
			buffSDWriter = new BufferedSDWriter(serial_ptr, writeSizeBytes);
			if (buffSDWriter) {
				buffSDWriter->setNChanWAV(numWriteChannels);
				if (bufferLength_samps >= 0) {
					allocateBuffer(bufferLength_samps); //leave empty for default buffer size
				} else {
					//if we don't allocateBuffer() here, it simply lets BufferedSDWrite create it last-minute
				}
			} else {
				Serial.print("AudioSDWriter_F32: setWriteDataType: *** ERROR *** Could not create buffered SD writer.");
			}
		}
		if (buffSDWriter == NULL) { return -1; } else { return 0; };
	}
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

	 bool isFileOpen(void) {
      if (buffSDWriter) return buffSDWriter->isFileOpen();
      return false;
    }

	
  unsigned long getStartTimeMillis(void) { return t_start_millis; };
  unsigned long setStartTimeMillis(void) { return t_start_millis = millis(); };

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

#endif

