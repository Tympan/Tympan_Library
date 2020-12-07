

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
#include <SdFat_Gre.h>       //originally from https://github.com/greiman/SdFat  but class names have been modified to prevent collisions with Teensy Audio/SD libraries
#include <Print.h>

//set some constants
#define maxBufferLengthBytes 150000    //size of big memroy buffer to smooth out slow SD write operations
const int DEFAULT_SDWRITE_BYTES = 512; //target size for individual writes to the SD card.  Usually 512
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
    SDWriter() {};
    SDWriter(Print* _serial_ptr) {
      setSerial(_serial_ptr);
    };
    virtual ~SDWriter() {
      if (isFileOpen()) close();
    }

    void setup(void) { init(); }
    virtual void init() {
      if (!sd.begin()) sd.errorHalt(serial_ptr, "SDWriter: begin failed");
    }

    bool openAsWAV(char *fname) {
      bool returnVal = open(fname);
      if (isFileOpen()) { //true if file is open
        flag__fileIsWAV = true;
        file.write(wavHeaderInt16(0), WAVheader_bytes); //initialize assuming zero length
      }
      return returnVal;
    }

    bool open(char *fname) {
      if (sd.exists(fname)) {  //maybe this isn't necessary when using the O_TRUNC flag below
        // The SD library writes new data to the end of the file, so to start
        //a new recording, the old file must be deleted before new data is written.
        sd.remove(fname);
      }
      file.open(fname, O_RDWR | O_CREAT | O_TRUNC);
      //file.createContiguous(fname, PRE_ALLOCATE_SIZE); //alternative to the line above
      return isFileOpen();
    }

	bool exists(char *fname) {
		return sd.exists(fname);
	}
	bool remove(char *fname) {
		return sd.remove(fname);
	}
	
    int close(void) {
      //file.truncate(); 
      if (flag__fileIsWAV) {
        //re-write the header with the correct file size
        uint32_t fileSize = file.fileSize();//SdFat_Gre_FatLib version of size();
        file.seekSet(0); //SdFat_Gre_FatLib version of seek();
        file.write(wavHeaderInt16(fileSize), WAVheader_bytes); //write header with correct length
        file.seekSet(fileSize);
      }
      file.close();
      flag__fileIsWAV = false;
      return 0;
    }

    bool isFileOpen(void) {
      if (file.isOpen()) return true;
      return false;
    }

    //This "write" is for compatibility with the Print interface.  Writing one
    //byte at a time is EXTREMELY inefficient and shouldn't be done
    virtual size_t write(uint8_t foo)  {
      size_t return_val = 0;
      if (file.isOpen()) {

        // write all audio bytes (512 bytes is most efficient)
        if (flagPrintElapsedWriteTime) { usec = 0; }
        file.write((byte *) (&foo), 1); //write one value
        return_val = 1;

        //write elapsed time only to USB serial (because only that is fast enough)
        if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec); }
      }
      return return_val;
    }

    //write Byte buffer...the lowest-level call upon which the others are built.
    //writing 512 is most efficient (ie 256 int16 or 128 float32
    virtual size_t write(const uint8_t *buff, int nbytes) {
      size_t return_val = 0;
      if (file.isOpen()) {
        if (flagPrintElapsedWriteTime) { usec = 0; }
        file.write((byte *)buff, nbytes); return_val = nbytes;

        //write elapsed time only to USB serial (because only that is fast enough)
        if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec); }
      }
      return return_val;
    }
    virtual size_t write(const char *buff, int nchar) { 
      return write((uint8_t *)buff,nchar); 
    }

    //write Int16 buffer.
    virtual size_t write(int16_t *buff, int nsamps) {
      return write((const uint8_t *)buff, nsamps * sizeof(buff[0]));
    }

    //write float32 buffer
    virtual size_t write(float32_t *buff, int nsamps) {
      return write((const uint8_t *)buff, nsamps * sizeof(buff[0]));
    }

    void setPrintElapsedWriteTime(bool flag) { flagPrintElapsedWriteTime = flag; }
    
    virtual void setSerial(Print *ptr) {  serial_ptr = ptr; }
    virtual Print* getSerial(void) { return serial_ptr;  }

    int setNChanWAV(int nchan) { return WAV_nchan = nchan;  };
    float setSampleRateWAV(float sampleRate_Hz) { return WAV_sampleRate_Hz = sampleRate_Hz; }

    //modified from Walter at https://github.com/WMXZ-EU/microSoundRecorder/blob/master/audio_logger_if.h
    char * wavHeaderInt16(const uint32_t fsize) {
      return wavHeaderInt16(WAV_sampleRate_Hz, WAV_nchan, fsize);
    }
    char* wavHeaderInt16(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize) {
      //const int fileSize = bytesWritten+44;

      int fsamp = (int) sampleRate_Hz;
      int nbits = 16;
      int nbytes = nbits / 8;
      int nsamp = (fileSize - WAVheader_bytes) / (nbytes * nchan);

      static char wheader[48]; // 44 for wav

      strcpy(wheader, "RIFF");
      strcpy(wheader + 8, "WAVE");
      strcpy(wheader + 12, "fmt ");
      strcpy(wheader + 36, "data");
      *(int32_t*)(wheader + 16) = 16; // chunk_size
      *(int16_t*)(wheader + 20) = 1; // PCM
      *(int16_t*)(wheader + 22) = nchan; // numChannels
      *(int32_t*)(wheader + 24) = fsamp; // sample rate
      *(int32_t*)(wheader + 28) = fsamp * nbytes; // byte rate
      *(int16_t*)(wheader + 32) = nchan * nbytes; // block align
      *(int16_t*)(wheader + 34) = nbits; // bits per sample
      *(int32_t*)(wheader + 40) = nsamp * nchan * nbytes;
      *(int32_t*)(wheader + 4) = 36 + nsamp * nchan * nbytes;

      return wheader;
    }
    
  protected:
    //SdFatSdio sd; //slower
    SdFatSdioEX sd; //faster
    SdFile_Gre file;
    boolean flagPrintElapsedWriteTime = false;
    elapsedMicros usec;
    Print* serial_ptr = &Serial;
    bool flag__fileIsWAV = false;
    const int WAVheader_bytes = 44;
    float WAV_sampleRate_Hz = 44100.0;
    int WAV_nchan = 2;
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
    BufferedSDWriter() : SDWriter() {
      setWriteSizeBytes(DEFAULT_SDWRITE_BYTES);
    };
    BufferedSDWriter(Print* _serial_ptr) : SDWriter(_serial_ptr) {
      setWriteSizeBytes(DEFAULT_SDWRITE_BYTES );
    };
    BufferedSDWriter(Print* _serial_ptr, const int _writeSizeBytes) : SDWriter(_serial_ptr) {
      setWriteSizeBytes(_writeSizeBytes);
    };
    ~BufferedSDWriter(void) {
      delete ptr_zeros;
      delete write_buffer;
    }

    //how many bytes should each write event be?  Set it here
    void setWriteSizeBytes(const int _writeSizeBytes) {
      setWriteSizeSamples(_writeSizeBytes / nBytesPerSample);
    }
    void setWriteSizeSamples(const int _writeSizeSamples) {
      writeSizeSamples = max(2, 2 * int(_writeSizeSamples / 2));//ensure even number >= 2
    }
    int getWriteSizeBytes(void) { return (getWriteSizeSamples() * nBytesPerSample); }
    int getWriteSizeSamples(void) { return writeSizeSamples;  }


    //allocate the buffer for storing all the samples between write events
    int allocateBuffer(const int _nBytes = maxBufferLengthBytes) {
      bufferLengthSamples = max(4,min(_nBytes,maxBufferLengthBytes) / nBytesPerSample);
      if (write_buffer != 0) delete write_buffer;  //delete the old buffer
      write_buffer = new int16_t[bufferLengthSamples];
      resetBuffer();
      return (int)write_buffer;
    }
    void resetBuffer(void) { bufferReadInd = 0; bufferWriteInd = 0;  }
 
    //here is how you send data to this class.  this doesn't write any data, it just stores data
    virtual void copyToWriteBuffer(float32_t *ptr_audio[], const int nsamps, const int numChan) {
      if (!write_buffer) {  //try to allocate buffer, return if it doesn't work
		if (!allocateBuffer()) {
			Serial.println("BufferedSDWriter: copyToWriteBuffer: *** ERROR ***");
			Serial.println("    : could not allocateBuffer()");
			return;
		}
	  }

      //how much data will we write?
      int estFinalWriteInd = bufferWriteInd + (numChan * nsamps);

      //will we pass by the read index?
      bool flag_moveReadIndexToEndOfWrite = false;
      if ((bufferWriteInd < bufferReadInd) && (estFinalWriteInd > bufferReadInd)) {
        Serial.println("BufferedSDWriter_I16: WARNING: writing past the read index.");
        flag_moveReadIndexToEndOfWrite = true;
      }

      //is there room to put the data into the buffer or will we hit the end
      if ( estFinalWriteInd >= bufferLengthSamples) { //is there room?
        //no there is not room
        bufferEndInd = bufferWriteInd; //save the end point of the written data
        bufferWriteInd = 0;  //reset

        //recheck to see if we're going to pass by the read buffer index
        estFinalWriteInd = bufferWriteInd + (numChan * nsamps);
        if ((bufferWriteInd < bufferReadInd) && (estFinalWriteInd > bufferReadInd)) {
          Serial.println("BufferedSDWriter_I16: WARNING: writing past the read index.");
          flag_moveReadIndexToEndOfWrite = true;
        }
      }

      //make sure no null arrays
      for (int Ichan=0; Ichan < numChan; Ichan++) {
        if (!(ptr_audio[Ichan])) {
          if (ptr_zeros == NULL) { ptr_zeros = new float32_t[nsamps](); } //creates and initializes to zero
          ptr_audio[Ichan] = ptr_zeros;
        }
      }

      //now interleave the data into the buffer
      for (int Isamp = 0; Isamp < nsamps; Isamp++) {
        for (int Ichan = 0; Ichan < numChan; Ichan++) {
            //convert the F32 to Int16 and interleave
            write_buffer[bufferWriteInd++] = (int16_t)(ptr_audio[Ichan][Isamp]*32767.0);
        }
      }

      //handle the case where we just wrote past the read index.  Push the read index ahead.
      if (flag_moveReadIndexToEndOfWrite) bufferReadInd = bufferWriteInd;
    }

    //write buffered data if enough has accumulated
    virtual int writeBufferedData(void) {
      const int max_writeSizeSamples = 8*writeSizeSamples;
      if (!write_buffer) return -1;
      int return_val = 0;

      //if the write pointer has wrapped around, write the data
      if (bufferWriteInd < bufferReadInd) { //if the buffer has wrapped around
        //Serial.println("BuffSDI16: writing to end of buffer");
        //return_val += write((byte *)(write_buffer+bufferReadInd),
        //    (bufferEndInd-bufferReadInd)*sizeof(write_buffer[0]));
        //bufferReadInd = 0;  //jump back to beginning of buffer

        int samplesAvail = bufferEndInd - bufferReadInd;
        if (samplesAvail > 0) {
          int samplesToWrite = min(samplesAvail, max_writeSizeSamples);
          if (samplesToWrite >= writeSizeSamples) {
            samplesToWrite = ((int)samplesToWrite / writeSizeSamples) * writeSizeSamples; //truncate to nearest whole number
          }
          //if (samplesToWrite == 0) {
          //  Serial.print("SD Writer: writeBuff1: samplesAvail, to Write: ");
          //  Serial.print(samplesAvail); Serial.print(", ");
          //  Serial.println(samplesToWrite);
          //}
          return_val += write((byte *)(write_buffer + bufferReadInd), samplesToWrite * sizeof(write_buffer[0]));
          //if (return_val == 0) {
          //  Serial.print("SDWriter: writeBuff1: samps to write, bytes written: "); Serial.print(samplesToWrite);
          //  Serial.print(", "); Serial.println(return_val);
          //}
          bufferReadInd += samplesToWrite;  //jump back to beginning of buffer
          if (bufferReadInd == bufferEndInd) bufferReadInd = 0;
        } else { 
          //the read pointer is at the end of the buffer, so loop it back
          bufferReadInd = 0; 
        }
      } else {
        
        //do we have enough data to write again?  If so, write the whole thing
        int samplesAvail = bufferWriteInd - bufferReadInd;
        if (samplesAvail >= writeSizeSamples) {
          //Serial.println("BuffSDI16: writing buffered data");
          //return_val = write((byte *)(write_buffer+buffer, writeSizeSamples * sizeof(write_buffer[0]));
  
          //return_val += write((byte *)(write_buffer+bufferReadInd),
          //  (bufferWriteInd-bufferReadInd)*sizeof(write_buffer[0]));
          //bufferReadInd = bufferWriteInd;  //increment to end of where it wrote
  
          int samplesToWrite = min(samplesAvail, max_writeSizeSamples);
          samplesToWrite = ((int)(samplesToWrite / writeSizeSamples)) * writeSizeSamples; //truncate to nearest whole number
          if (samplesToWrite == 0) {
            Serial.print("SD Writer: writeBuff2: samplesAvail, to Write: ");
            Serial.print(samplesAvail); Serial.print(", ");
            Serial.println(samplesToWrite);
          }
          return_val += write((byte *)(write_buffer + bufferReadInd), samplesToWrite * sizeof(write_buffer[0]));
          if (return_val == 0) {
            Serial.print("SDWriter: writeBuff2: samps to write, bytes written: "); Serial.print(samplesToWrite);
            Serial.print(", "); Serial.println(return_val);
          }
          bufferReadInd += samplesToWrite;  //increment to end of where it wrote
        }
      }
      return return_val;
    }

//    virtual int interleaveAndWrite(int16_t *chan1, int16_t *chan2, int nsamps) {
//      //Serial.println("BuffSDI16: interleaveAndWrite given I16...");
//      Serial.println("BuffSDI16: interleave and write (I16 inputs).  UPDATE ME!!!");
//      if (!write_buffer) return -1;
//      int return_val = 0;
//
//      //interleave the data and write whenever the write buffer is full
//      //Serial.println("BuffSDI16: buffering given I16...");
//      for (int Isamp = 0; Isamp < nsamps; Isamp++) {
//        write_buffer[bufferWriteInd++] = chan1[Isamp];
//        write_buffer[bufferWriteInd++] = chan2[Isamp];
//
//        //do we have enough data to write our block to SD?
//        if (bufferWriteInd >= writeSizeSamples) {
//          //Serial.println("BuffSDI16: writing given I16...");
//          return_val = write((byte *)write_buffer, writeSizeSamples * sizeof(write_buffer[0]));
//          bufferWriteInd = 0;  //jump back to beginning of buffer
//        }
//      }
//      return return_val;
//    }

//    virtual int interleaveAndWrite(float32_t *chan1, float32_t *chan2, int nsamps) {
//      //Serial.println("BuffSDI16: interleaveAndWrite given F32...");
//      if (!write_buffer) return -1;
//      int return_val = 0;
//
//      //interleave the data into the buffer
//      //Serial.println("BuffSDI16: buffering given F32...");
//      if ( (bufferWriteInd + nsamps) >= bufferLengthSamples) { //is there room?
//        bufferEndInd = bufferWriteInd; //save the end point of the written data
//        bufferWriteInd = 0;  //reset
//      }
//      for (int Isamp = 0; Isamp < nsamps; Isamp++) {
//        //convert the F32 to Int16 and interleave
//        write_buffer[bufferWriteInd++] = (int16_t)(chan1[Isamp] * 32767.0);
//        write_buffer[bufferWriteInd++] = (int16_t)(chan2[Isamp] * 32767.0);
//      }
//
//      //if the write pointer has wrapped around, write the data
//      if (bufferWriteInd < bufferReadInd) { //if the buffer has wrapped around
//        //Serial.println("BuffSDI16: writing given F32...");
//        return_val = write((byte *)(write_buffer + bufferReadInd),
//                           (bufferEndInd - bufferReadInd) * sizeof(write_buffer[0]));
//        bufferReadInd = 0;  //jump back to beginning of buffer
//      }
//
//      //do we have enough data to write again?  If so, write the whole thing
//      if ((bufferWriteInd - bufferReadInd) >= writeSizeSamples) {
//        //Serial.println("BuffSDI16: writing given F32...");
//        //return_val = write((byte *)(write_buffer+buffer, writeSizeSamples * sizeof(write_buffer[0]));
//        return_val = write((byte *)(write_buffer + bufferReadInd),
//                           (bufferWriteInd - bufferReadInd) * sizeof(write_buffer[0]));
//        bufferReadInd = bufferWriteInd;  //increment to end of where it wrote
//      }
//      return return_val;
//    }
//
//    //write one channel of int16 as int16
//    virtual int writeOneChannel(int16_t *chan1, int nsamps) {
//      if (write_buffer == 0) return -1;
//      int return_val = 0;
//
//      if (nsamps == writeSizeSamples) {
//        //special case where everything is the right size...it'll be fast!
//        return write((byte *)chan1, writeSizeSamples * sizeof(chan1[0]));
//      } else {
//        //do the buffering and write when the buffer is full
//
//        for (int Isamp = 0; Isamp < nsamps; Isamp++) {
//          //convert the F32 to Int16 and interleave
//          write_buffer[bufferWriteInd++] = chan1[Isamp];
//
//          //do we have enough data to write our block to SD?
//          if (bufferWriteInd >= writeSizeSamples) {
//            return_val = write((byte *)write_buffer, writeSizeSamples * sizeof(write_buffer[0]));
//            bufferWriteInd = 0;  //jump back to beginning of buffer
//          }
//        }
//        return return_val;
//      }
//      return return_val;
//    }
//
//    //write one channel of float32 as int16
//    virtual int writeOneChannel(float32_t *chan1, int nsamps) {
//      if (write_buffer == 0) return -1;
//      int return_val = 0;
//
//      //interleave the data and write whenever the write buffer is full
//      for (int Isamp = 0; Isamp < nsamps; Isamp++) {
//        //convert the F32 to Int16 and interleave
//        write_buffer[bufferWriteInd++] = (int16_t)(chan1[Isamp] * 32767.0);
//
//        //do we have enough data to write our block to SD?
//        if (bufferWriteInd >= writeSizeSamples) {
//          return_val = write((byte *)write_buffer, writeSizeSamples * sizeof(write_buffer[0]));
//          bufferWriteInd = 0;  //jump back to beginning of buffer
//        }
//      }
//      return return_val;
//    }

  protected:
    int writeSizeSamples = 0;
    int16_t* write_buffer = 0;
    int32_t bufferWriteInd = 0;
    int32_t bufferReadInd = 0;
    const int nBytesPerSample = 2;
    int32_t bufferLengthSamples = maxBufferLengthBytes / nBytesPerSample;
    int32_t bufferEndInd = maxBufferLengthBytes / nBytesPerSample;
    float32_t *ptr_zeros;

};


#endif

