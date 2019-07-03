
/*
   Chip Audette, OpenAudio, Mar 2018

   MIT License.  Use at your own risk.
*/

#ifndef _SDAudioWriter_SdFAT
#define _SDAudioWriter_SdFAT

//include <SdFat_Gre.h>       //originally from https://github.com/greiman/SdFat  but class names have been modified to prevent collisions with Teensy Audio/SD libraries
#include <Tympan_Library.h>  //for data types float32_t and int16_t and whatnot
#include <AudioStream.h>     //for AUDIO_BLOCK_SAMPLES


// Preallocate 40MB file.
const uint64_t PRE_ALLOCATE_SIZE = 40ULL << 20;

#define MAXFILE 100

//char *header = 0;
#define MAX_AUDIO_BUFF_LEN (2*AUDIO_BLOCK_SAMPLES)  //Assume stereo (for the 2x).  AUDIO_BLOCK_SAMPLES is from Audio_Stream.h, which should be max of Audio_Stream_F32 as well.

class SDAudioWriter_SdFat
{
  public:
    SDAudioWriter_SdFat() {};
    ~SDAudioWriter_SdFat() {
      if (isFileOpen()) {
        close();
      }
    }
    void setup(void) {
      init();
    };
    void init() {
      if (!sd.begin()) sd.errorHalt("sd.begin failed");
    }
    //write two F32 channels as int16
    int writeF32AsInt16(float32_t *chan1, float32_t *chan2, int nsamps) {
      const int buffer_len = 2 * nsamps; //it'll be stereo, so 2*nsamps
      int count = 0;
      if (file.isOpen()) {
        for (int Isamp = 0; Isamp < nsamps; Isamp++) {
          //convert the F32 to Int16 and interleave
          write_buffer[count++] = (int16_t)(chan1[Isamp] * 32767.0);
          write_buffer[count++] = (int16_t)(chan2[Isamp] * 32767.0);
        }

        // write all audio bytes (512 bytes is most efficient)
        if (flagPrintElapsedWriteTime) { usec = 0; }
        file.write((byte *)write_buffer, buffer_len * sizeof(write_buffer[0]));
        nBlocksWritten++;
        if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec);  }
      }
      return 0;
    }

    bool openAsWAV(char *fname) {
      bool returnVal = open(fname);
      if (isFileOpen()) { //true if file is open
        flag__fileIsWAV = true;
        file.write(wavHeaderInt16(0),WAVheader_bytes); //initialize assuming zero length
      }
      return returnVal;
    }

    bool open(char *fname) {
      if (sd.exists(fname)) {  //maybe this isn't necessary when using the O_TRUNC flag below
        // The SD library writes new data to the end of the
        // file, so to start a new recording, the old file
        // must be deleted before new data is written.
        sd.remove(fname);
      }
      
      #if 1
        file.open(fname,O_RDWR | O_CREAT | O_TRUNC);
      #else
        file.createContiguous(fname,PRE_ALLOCATE_SIZE);
      #endif 
 
      return isFileOpen();
    }

    int close(void) {
      //file.truncate();
      if (flag__fileIsWAV) {
        //re-write the header with the correct file size
        uint32_t fileSize = file.fileSize();//SdFat_Gre_FatLib version of size();
        file.seekSet(0); //SdFat_Gre_FatLib version of seek();
        file.write(wavHeaderInt16(fileSize),WAVheader_bytes); //write header with correct length
        file.seekSet(fileSize);
      }
      file.close();
      flag__fileIsWAV = false;
      return 0;
    }
    
    bool isFileOpen(void) {
      if (file.isOpen()) {
        return true;
      } else {
        return false;
      }
    }
//    char* makeFilename(char * filename)
//    { static int ifl = 0;
//      ifl++;
//      if (ifl > MAXFILE) return 0;
//      sprintf(filename, "File%04d.raw", ifl);
//      //Serial.println(filename);
//      return filename;
//    }

    void enablePrintElapsedWriteTime(void) {
      flagPrintElapsedWriteTime = true;
    }
    void disablePrintElapseWriteTime(void) {
      flagPrintElapsedWriteTime = false;
    }
    unsigned long getNBlocksWritten(void) {
      return nBlocksWritten;
    }
    void resetNBlocksWritten(void) {
      nBlocksWritten = 0;
    }

    float setSampleRateWAV(float fs_Hz) { return WAV_sampleRate_Hz = fs_Hz; }

   //modified from Walter at https://github.com/WMXZ-EU/microSoundRecorder/blob/master/audio_logger_if.h
    char * wavHeaderInt16(const uint32_t fsize) { return wavHeaderInt16(WAV_sampleRate_Hz, WAV_nchan, fsize);  }
    char* wavHeaderInt16(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize) {
      //const int fileSize = bytesWritten+44;
      
      int fsamp = (int) sampleRate_Hz;
      int nbits=16;
      int nbytes=nbits/8;
      int nsamp=(fileSize-WAVheader_bytes)/(nbytes*nchan);
      
      static char wheader[48]; // 44 for wav
    
      strcpy(wheader,"RIFF");
      strcpy(wheader+8,"WAVE");
      strcpy(wheader+12,"fmt ");
      strcpy(wheader+36,"data");
      *(int32_t*)(wheader+16)= 16;// chunk_size
      *(int16_t*)(wheader+20)= 1; // PCM 
      *(int16_t*)(wheader+22)=nchan;// numChannels 
      *(int32_t*)(wheader+24)= fsamp; // sample rate 
      *(int32_t*)(wheader+28)= fsamp*nbytes; // byte rate
      *(int16_t*)(wheader+32)=nchan*nbytes; // block align
      *(int16_t*)(wheader+34)=nbits; // bits per sample 
      *(int32_t*)(wheader+40)=nsamp*nchan*nbytes; 
      *(int32_t*)(wheader+4)=36+nsamp*nchan*nbytes; 
    
      return wheader;
    }


  private:
    //SdFatSdio sd; //slower
    SdFatSdioEX sd; //faster
    SdFile_Gre file;
    int16_t write_buffer[MAX_AUDIO_BUFF_LEN];
    boolean flagPrintElapsedWriteTime = false;
    elapsedMicros usec;
    unsigned long nBlocksWritten = 0;
    bool flag__fileIsWAV = false;
    const int WAVheader_bytes = 44;
    float WAV_sampleRate_Hz = 44100.0;
    int WAV_nchan = 2;

};


#endif
