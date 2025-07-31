

/*
 * SDWriter.h containing SDWriter and BufferedSDWriter
 * 
 * Created: Chip Audette, OpenAudio, Apr 2019
 * Purpose: These classes wrap lower-level SD libraries.  The purpose of these
 *    wrapping classes is to provide functionality such as (1) writing audio in
 *    WAV format, (2) writing in efficient 512B blocks, and (3) providing a big
 *    memory buffer to the SD writing to help mitigate the occasional slow SD
 *    writing operation.
 *    
 *    For a wrapper that works more directly with the Tympan/OpenAudio library,    
 *    see the class AudioSDWriter in AudioSDWriter.h
 *
 * MIT License.  Use at your own risk.
*/


#ifndef _SDWriter_h
#define _SDWriter_h

#include <arm_math.h>        //possibly only used for float32_t definition?
//#include <SdFat_Gre.h>       //originally from https://github.com/greiman/SdFat  but class names have been modified to prevent collisions with Teensy Audio/SD libraries
//#include "SD.h" // was using this but we should be using sdfat
#include <SdFat.h>  //included in Teensy install as of Teensyduino 1.54-bete3
#include <Print.h>

//set some constants
#define SDWRITER_MAX_BUFFER_LENGTH_BYTES 150000    //size of big memroy buffer to smooth out slow SD write operations
#define SD_CONFIG SdioConfig(FIFO_SDIO)

const int DEFAULT_SDWRITE_BYTES = (512); //target size for individual writes to the SD card.  Usually 512
//const uint64_t PRE_ALLOCATE_SIZE = 40ULL << 20;// Preallocate 40MB file.  Not used.

//SDWriter:  This is a class to write blocks of bytes, chars, ints or floats to
//  the SD card.  It will write blocks of data of whatever the size, even if it is not
//  most efficient for the SD card.  This is a base class upon which other classes
//  can inheret.  The derived classes can then do things more smartly, like write
//  more efficient blocks of 512 bytes.
//
//  To handle the interleaving of multiple channels and to handle conversion to the
//  desired write type (float32 -> int16) and to handle buffering so that the optimal
//  number of bytes are written at once, use one of the derived classes such as
//  BufferedSDWriter
class SDWriter : public Print
{
  public:
    SDWriter(SdFs * _sd) { sd = _sd; };
    SDWriter(SdFs * _sd, Print* _serial_ptr) { sd = _sd; setSerial(_serial_ptr); };
    virtual ~SDWriter() { end(); }

    virtual void setup(void) { init(); }
    virtual void init() { if (!sd->begin(SD_CONFIG)) sd->errorHalt(serial_ptr, "SDWriter: begin failed"); }
   		
		virtual void end() {
			if (isFileOpen()) close();
			sd->end();
		}
		
		enum class WriteDataType { INT16=0, INT24, FLOAT32 }; //not all of these are necessarily supported
    
    virtual bool openAsWAV(const char *fname);
    virtual bool open(const char *fname);
    virtual int close(void);
		bool exists(const char *fname) { return sd->exists(fname); }
		bool remove(const char *fname) { return sd->remove(fname); }
		
    bool isFileOpen(void) {
      if (file.isOpen()) return true;
      return false;
    }

    //This "write" is for compatibility with the Print interface.  Writing one
    //byte at a time is EXTREMELY inefficient and shouldn't be done
    size_t write(uint8_t foo) override;

    //write Byte buffer...the lowest-level call upon which the others are built.
    //writing 512 is most efficient (ie 256 int16 or 128 float32
    virtual size_t write(const uint8_t *buff, int nbytes);
    virtual size_t write(const char *buff, int nchar) { return write((uint8_t *)buff,nchar);  }
    virtual size_t write(int16_t *buff, int nsamps) { return write((const uint8_t *)buff, nsamps * sizeof(buff[0]));}

    //write float32 buffer
    virtual size_t write(float32_t *buff, int nsamps) {
      return write((const uint8_t *)buff, nsamps * sizeof(buff[0]));
    }

    void setPrintElapsedWriteTime(bool flag) { flagPrintElapsedWriteTime = flag; }
    
    virtual void setSerial(Print *ptr) {  serial_ptr = ptr; }
    virtual Print* getSerial(void) { return serial_ptr;  }

    int setNChanWAV(int nchan) { return WAV_nchan = nchan;  };
		int getNChanWAV(void) { return WAV_nchan; }
		
    float setSampleRateWAV(float sampleRate_Hz) { return WAV_sampleRate_Hz = sampleRate_Hz; }
		float getSampleRateWAV(void) { return WAV_sampleRate_Hz; }

    //modified from Walter at https://github.com/WMXZ-EU/microSoundRecorder/blob/master/audio_logger_if.h
    //char* wavHeaderInt16(const uint32_t fsize) { return wavHeaderInt16(WAV_sampleRate_Hz, WAV_nchan, fsize); }
    //char* wavHeaderInt16(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize);
		char* makeWavHeader(const uint32_t fsize) { return makeWavHeader(WAV_sampleRate_Hz, WAV_nchan, fsize); }
    char* makeWavHeader(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize);
    
		SdFs * getSdPtr(void) { return sd; }

		virtual int setWriteDataType(WriteDataType type) { 
			//Serial.println("SDWriter: setWriteDataType: type = " + String((int)type));
			if (writeDataType != type) {
				if (isFileOpen()) Serial.println("SDWriter: *** WARNING ***: Changing data type but WAV file is already open!");
			}
			return (int)(writeDataType = type); 
		}

		virtual int isSdCardPresent(void);

  protected:
    //SdFatSdio sd; //slower
		SdFs * sd = nullptr; //faster
    SdFile file;
    boolean flagPrintElapsedWriteTime = false;
    elapsedMicros usec;
    Print* serial_ptr = &Serial;
    bool flag__fileIsWAV = false;
    int WAVheader_bytes = 44; //default, for Int16 data type
    float WAV_sampleRate_Hz = 44100.0;
    int WAV_nchan = 2;  //default
    WriteDataType writeDataType = SDWriter::WriteDataType::INT16; // default to INT16 data in WAV files
		char wheader[58];  //36+8=44 is big enough for INT16 header, 36+2+12+8=58 is big enough for IEEE FLOAT32 header
		const int wheader_maxlen = 58;
};

//BufferedSDWriter:  This is a drived class from SDWriter.  This class assumes that
//  you want to write Int16 data to the SD card.  You may give this class data that is
//  either Int16 or Float32.  This class will also handle interleaving of several input
//  channels.  This class will also buffer the data until the optimal (or desired) number
//  of samples have been accumulated, which makes the SD writing more efficient.
//
//  Note that "writeSizeBytes" is the size of an individual write operation to the SD
//  card, which should normally be (I think) 512B or some multiple thereof.  This class
//  also provides a big memory buffer for mitigating the effect of the occasional slow
//  SD write operation.  The size of this buffer defaults to a very large size, as set
//  by 
class BufferedSDWriter : public SDWriter
{
  public:
    BufferedSDWriter(SdFs * _sd) : SDWriter(_sd) {
      setWriteSizeBytes(DEFAULT_SDWRITE_BYTES);
    };
    BufferedSDWriter(SdFs * _sd, Print* _serial_ptr) : SDWriter(_sd, _serial_ptr) {
      setWriteSizeBytes(DEFAULT_SDWRITE_BYTES );
    };
    BufferedSDWriter(SdFs * _sd, Print* _serial_ptr, const int _writeSizeBytes) : SDWriter(_sd, _serial_ptr) {
      setWriteSizeBytes(_writeSizeBytes);
    };
    ~BufferedSDWriter(void) { delete[] ptr_zeros; delete[] write_buffer; }
		
		bool sync(void);

    //how many bytes should each write event be?  Set it here.  [July 28, 2025: Is this actually used anywhere?]
    void setWriteSizeBytes(const int _writeSizeBytes) {
      writeSizeBytes = max(4, 4 * int(_writeSizeBytes/4)); //ensure at least 4 bytes long
    }
    void setWriteSizeSamples(const int _writeSizeSamples) {
			setWriteSizeBytes(_writeSizeSamples * nBytesPerSample);
    }
    uint getWriteSizeBytes(void) { return writeSizeBytes; }
    uint getWriteSizeSamples(void) { return (getWriteSizeBytes() / nBytesPerSample);  }


    //allocate the buffer for storing all the samples between write events
    int allocateBuffer(const int _nBytes = SDWRITER_MAX_BUFFER_LENGTH_BYTES) {
			if (write_buffer != 0) delete[] write_buffer;  //delete the old buffer
  		bufferLengthBytes = max(4,_nBytes);
			write_buffer = new (std::nothrow) uint8_t[bufferLengthBytes];
			resetBuffer();
			//Serial.println("BufferedSDWriter: allocateBuffer: allocated " + String(bufferLengthBytes) + " after requesting " + String(_nBytes) + ", write_buffer = " + String((int)write_buffer));
      return (int)write_buffer;
    }
    void freeBuffer(void) { delete[] write_buffer; write_buffer = nullptr; resetBuffer(); }
    void resetBuffer(void) { bufferReadInd_bytes = 0; bufferWriteInd_bytes = 0;  }
 
    //here is how you send data to this class.  this doesn't write any data, it just stores data
    virtual void copyToWriteBuffer(float32_t *ptr_audio[], const int nsamps, const int numChan);
    
    //write buffered data if enough has accumulated
    virtual int writeBufferedData(void);

		virtual float32_t generateDitherNoise(const int &Ichan, const int &method);
		int enableDithering(bool enable) { if (enable) { return setDitheringMethod(0); } else { return setDitheringMethod(2); } }
		int setDitheringMethod(int val) { return ditheringMethod = val; }
		int getDitheringMethod(void) { return ditheringMethod; }
		
		//int32_t getLengthOfBuffer(void) { return bufferLengthSamples; }
		uint32_t getLengthOfBuffer_bytes(void) { return bufferLengthBytes; }
		//int32_t getNumSampsInBuffer(void) {
		//	if (bufferReadInd <= bufferWriteInd) return bufferWriteInd-bufferReadInd;
		//	return getLengthOfBuffer() - bufferReadInd + bufferWriteInd;
		//}
		uint32_t getNumBytesInBuffer(void) {
			if (bufferReadInd_bytes <= bufferWriteInd_bytes) return bufferWriteInd_bytes-bufferReadInd_bytes;
			return getLengthOfBuffer_bytes() - bufferReadInd_bytes + bufferWriteInd_bytes;
		}
		//int32_t getNumUnfilledSamplesInBuffer(void) { return getLengthOfBuffer() - getNumSampsInBuffer(); }  //how much of the buffer is empty, in samples
		uint32_t getNumUnfilledBytesInBuffer(void) { return getLengthOfBuffer_bytes() - getNumBytesInBuffer(); }  //how much of the buffer is empty, in samples
		//uint32_t getNumUnfilledSamplesInBuffer_msec(void) {
		//	int32_t available_buffer_samples = getNumUnfilledSamplesInBuffer();
		//	float samples_per_msec = (WAV_sampleRate_Hz*WAV_nchan) / 1000.0f;  //these data memersare in the SDWriter class
		//	return (uint32_t)((float)available_buffer_samples/samples_per_msec + 0.5f); //the "+0.5"rounds to the nearest millisec
		//}
		uint32_t getNumUnfilledBytesInBuffer_msec(void) {
			int32_t available_buffer_bytes = getNumUnfilledBytesInBuffer();
			float bytes_per_msec = (WAV_sampleRate_Hz * WAV_nchan * nBytesPerSample) / 1000.0f;  //these data memersare in the SDWriter class
			return (uint32_t)((float)available_buffer_bytes/bytes_per_msec + 0.5f); //the "+0.5"rounds to the nearest millisec
		}
	
		int setWriteDataType(SDWriter::WriteDataType type) override { 
			int return_val = SDWriter::setWriteDataType(type); 
			resetBuffer(); 
			if (type == SDWriter::WriteDataType::INT16) {
				nBytesPerSample = 16/8;
			} else {
				nBytesPerSample = 32/8;
			}
			return return_val;
		}
		
		uint getBytesPerSample(void) { return nBytesPerSample; }
		
		
  protected:
    uint32_t writeSizeBytes = DEFAULT_SDWRITE_BYTES;  //default
    uint8_t* write_buffer = 0; //generic array of bytes
    uint32_t bufferWriteInd_bytes = 0;
    uint32_t bufferReadInd_bytes = 0;
    uint32_t nBytesPerSample = 2;
    //int32_t bufferLengthSamples = SDWRITER_MAX_BUFFER_LENGTH_BYTES / nBytesPerSample;
    uint32_t bufferLengthBytes = 0;
		uint32_t bufferEndInd_bytes = 0 / nBytesPerSample;
    float32_t *ptr_zeros = nullptr;
		int ditheringMethod = 0;  //default 0 is off
};


#endif

