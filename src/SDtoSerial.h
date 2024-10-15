
#ifndef _SDtoSerial_h
#define _SDtoSerial_h


#include <SdFat.h>  //included in Teensy install as of Teensyduino 1.54-bete3
#include <Print.h>   //frpm Arduino cores

class Print_ReplaceChar; //forward declare
class Print_RemoveChar;  //forward declare

class SDtoSerial {
	public:
		SDtoSerial(void) :                        sd_ptr(NULL), serial_ptr(&Serial) { init(); }
		SDtoSerial(SdFs * _sd) :                  sd_ptr(_sd),  serial_ptr(&Serial) { init(); }
		SDtoSerial(Stream *ser_ptr) :             sd_ptr(NULL), serial_ptr(ser_ptr) { init(); }
		SDtoSerial(SdFs * _sd, Stream *ser_ptr) : sd_ptr(_sd),  serial_ptr(ser_ptr) { init(); }
		
		enum SD_STATE {SD_STATE_BEGUN=0, SD_STATE_NOT_BEGUN=99};
		enum READ_STATE {READ_STATE_NORMAL=1, READ_STATE_FILE_EMPTY};
		
		void init(void) { state = SD_STATE_NOT_BEGUN; }
 		virtual void begin(void);                                          //begin the SD card
		virtual bool sendFilenames(void) { return sendFilenames('\n'); };  //list files in the current directory
		virtual bool sendFilenames(const char separator);                  //list files in the current directory
		virtual bool open(const String &filename) { return open(filename.c_str()); }  //returns true if successful
		virtual bool open(const char *filename);                                      //returns true if successful
		virtual bool isFileOpen(void) { if ((serial_ptr != nullptr) && file.isOpen()) { return true; } return false; }
		virtual void close(void) { file.close(); }
		
		virtual uint64_t getFileSize(void) { if (file.isOpen()) {return file.fileSize();} return 0; } //return size of file in bytes
		virtual bool sendFileSize(void); //send size of the currently-open file in bytes

		virtual SdFs* setSdPtr(SdFs *ptr) { return sd_ptr = ptr; }
		virtual SdFs* getSdPtr(void)		  { return sd_ptr; }
		unsigned int setTransferBlockSize(unsigned int _block_size) { return transfer_block_size = _block_size; }
		unsigned int getTransferBlockSize(void) { return transfer_block_size; }
		unsigned int setBlockDelay_msec(unsigned int foo_msec) { return block_delay_msec = foo_msec; }
		unsigned int getBlockDelay_msec(void)                  { return block_delay_msec; }
		
		virtual uint64_t sendSizeThenFile(const String &filename);  //this is the most likely method to be used
		virtual uint64_t sendFile(const String &filename) { return sendFile(filename.c_str()); }
		virtual uint64_t sendFile(char *filename);    //open the file, send it, close it
		virtual uint64_t sendFile(void);              //sends in blocks.  returns the number of bytes sent.  Uses sendOneBlock()
		unsigned int sendOneBlock(void);              //returns the number of bytes sent.  Uses readBytesFromSD()
		unsigned int readBytesFromSD(uint8_t* buffer, const unsigned int n_bytes_to_read); //returns the number of bytes read.  Closes file if it runs out of data


	protected:
		SdFs *sd_ptr;
		Stream *serial_ptr;
		SdFile file;
		
		unsigned int transfer_block_size = 1024;  //default value
		unsigned int block_delay_msec = 1;        //how much delay to put between each block when transmitting
		
		uint8_t state;
		uint8_t state_read;
		
};


class Print_ReplaceChar : public Print {
	public:
		Print_ReplaceChar(Print *ptr, const char _targ_char, const char _replacement_char) : print_ptr(ptr) { 
			targ_char = _targ_char; 
			replacement_char = _replacement_char;
			isActive = true;
		}
		virtual size_t write(uint8_t orig_char) {
			if (!print_ptr) return 0;
			if ((orig_char == targ_char) && (isActive)) return print_ptr->write(replacement_char);
			return print_ptr->write(orig_char);
		}
		bool isActive = false;

	private:
		Print *print_ptr;
		char targ_char = 'A', replacement_char = 'A';
};

class Print_RemoveChar : public Print {
	public:
		Print_RemoveChar(Print *ptr, const char _targ_char) : print_ptr(ptr) { 
			targ_char = _targ_char;
			isActive=true;
		}
		virtual size_t write(uint8_t orig_char) {
			if (!print_ptr) return 0;
			if ((orig_char == targ_char) && (isActive)) return 0;  //skip this character
			return print_ptr->write(orig_char);
		}
		bool isActive = false;

	private:
		Print *print_ptr;
		char targ_char = 'A';  //default, will be overwritten in constructor
};



#endif