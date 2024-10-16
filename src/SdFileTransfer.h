//
// SdFileTransfer
//
// Created: Chip Audette, OpenAudio, Oct 2024
//
// Purpose: Enable the user to transfer files between their SD Card
//   and their PC using the basic Serial link.  Be aware that this
//   simply sends/receives the bytes blindly over the Serial.  It
//   does not error checking nor does it try to throttle the speed
//   of the transfer in response to congestion on the serial link.
//
// Typical Use Cases:
//
// Use Case A) Tympan file to PC (Tympan is "sending")
//    1) [Optional] User asks Tympan to send a list of filenames on the SD
//      * Use SdFileTransfer::sendFilenames(','); //comma-separated namespace
//    2) User commands the tympan to send a file
//      * Use SdFileTransfer::sendFile_interactive();
//    3) User follows the prompts sent by the Tympan
//      * User sends the filename that they want from the Tympan (as text, ending with newline)
//      * User receives from the Tympan the number of bytes in the file (as text, ending with newline)
//      * User receives prompt that the Tympan will be sending the file's bytes (as text, ending with newline)
//      * User receives all the bytes from the file and writes the bytes locally to their PC
//
// Use Case B) PC file to Tympan SD (Tympan is "receiving")
//    1) User commands the tympan to receive a file
//      * Use SdFileTransfer::receiveFile_interactive();
//    2) User follows the prompts sent by the Tympan
//      * User sends the filename that they want from the Tympan (as text, ending with newline)
//      * User sends the number of bytes that they will be sending (as text, ending with newline)
//      * User receives prompt that the Tympan is ready to receive file's bytes (as text, ending with newline)
//      * User sends all the bytes from the file
//      * Tympan acknowledges how many bytes it received
//
// MIT License.  Use at your own risk.

#ifndef _SdFileTransfer_h
#define _SdFileTransfer_h

#include        "SdFat.h"  // included as part of the Teensy installation

class Print_ReplaceChar; //forward declare
class Print_RemoveChar;  //forward declare

class SdFileTransfer {
  public:
		SdFileTransfer(void) :                        sd_ptr(nullptr), serial_ptr(&Serial) { init(); }
		SdFileTransfer(SdFs * _sd) :                  sd_ptr(_sd),  serial_ptr(&Serial) { init(); }
		SdFileTransfer(Stream *ser_ptr) :             sd_ptr(nullptr), serial_ptr(ser_ptr) { init(); }
		SdFileTransfer(SdFs * _sd, Stream *ser_ptr) : sd_ptr(_sd),  serial_ptr(ser_ptr) { init(); }
		
		enum SD_STATE {SD_STATE_BEGUN=0, SD_STATE_NOT_BEGUN=99};
 		enum READ_STATE {READ_STATE_NONE=0, READ_STATE_NORMAL=1, READ_STATE_FILE_EMPTY};
		enum WRITE_STATE {WRITE_STATE_NONE=0, WRITE_STATE_PREPARING, WRITE_STATE_READY, WRITE_ERROR=99};

		void init(void) { state = SD_STATE_NOT_BEGUN; }
 		virtual void begin(void);                                          //begin the SD card

  	virtual bool sendFilenames(void) { return sendFilenames('\n'); };  //list files in the current directory on the SD
		virtual bool sendFilenames(const char separator);                  //list files in the current directory on the SD

    virtual String setFilename(const String &given_fname);              //close any open files and set a new filename
    virtual String getFilename(void) { return cur_filename; }           //return the current filename
    virtual String receiveFilename(void) { return receiveFilename('\n'); };   //receive the filename over the serial link
    virtual String receiveFilename(const char EOL_char);                      //receive the filename over the serial link

    virtual bool isFileOpen(void) { if ((serial_ptr != nullptr) && file.isOpen()) { return true; } return false; }
		virtual void close(void) { file.close(); state_write = WRITE_STATE_NONE; state_read = READ_STATE_NONE; }

    //unsigned int setTransferBlockSize(unsigned int _block_size) { return transfer_block_size = _block_size; }
		unsigned int getTransferBlockSize(void) { return transfer_block_size; }



    // ////////// Methods for reading off the SD and pushing to serial
    virtual bool openRead(const String &filename) { setFilename(filename); return openRead(); }  //returns true if successful
		virtual bool openRead(const char *filename) { return openRead(String(filename)); }           //returns true if successful
		virtual bool openRead(void);
		

    virtual uint64_t getNumBytesToReadFromSD(void) { if (file.isOpen()) {return file.fileSize();} return 0; } //return size of file in bytes
		virtual bool     sendNumBytesToReadFromSD(void); //send over serial the size of the currently-open file in bytes

		virtual uint64_t sendFile_interactive(void);  //automates the transfer process by sending requests and receiving requests over serial
		virtual uint64_t sendFile(void);              //sends in blocks.  returns the number of bytes sent.  Uses sendOneBlock()
		unsigned int sendOneBlock(void);              //returns the number of bytes sent.  Uses readBytesFromSD()
		unsigned int readBytesFromSD(uint8_t* buffer, const unsigned int n_bytes_to_read); //returns the number of bytes read.  Closes file if it runs out of data
 
		unsigned int setBlockDelay_msec(unsigned int foo_msec) { return block_delay_msec = foo_msec; }
		unsigned int getBlockDelay_msec(void)                  { return block_delay_msec; }
	


    // ////////// Methods for writing to the SD based on data received from the Serial
    virtual bool openWrite(const String &filename) { setFilename(filename); return openWrite(); }  //returns true if successful
		virtual bool openWrite(const char *filename) { return openWrite(String(filename)); }           //returns true if successful
		virtual bool openWrite(void);                                           //open a file using the currently-specified filname

    virtual uint64_t setNumBytesToWriteToSD(const uint64_t num_bytes);     //how many bytes will be sent
    //virtual uint64_t getNumBytesToWriteToSD(void) { return total_file_length; }
    virtual uint64_t receiveNumBytesToWriteToSD(void) { return receiveNumBytesToWriteToSD('\n'); };  //receive file size over the serial link
    virtual uint64_t receiveNumBytesToWriteToSD(const char EOL_char);  //receive file size over the serial link
	
    virtual uint64_t receiveFile_interactive(void);  //automates the transfer process by sending requests and receiving requests over serial
    virtual uint64_t receiveFile(void);  //receives the bytes of the file *after* already being given the filename and filesize
		virtual uint64_t receiveOneBlock(const unsigned int bytes_to_receive);              //returns the number of bytes sent.  Uses readBytesFromSD()
		virtual uint64_t writeBufferedBytesToSD(uint8_t* buffer, const unsigned int n_bytes_to_write); //returns the number of bytes read.  Closes file if it runs out of data

  private:
    SdFs *sd_ptr = nullptr;
		Stream *serial_ptr = nullptr;
		SdFile file;

    //data members that are common
    uint64_t total_file_length; //bytes to be transfered over Serial and eventually written
		static constexpr unsigned int transfer_block_size = 1024;  //default value
		uint8_t state; //state of the SD
    String cur_filename = String(""); //current filename
	  
    //data members for reading off the SD and pushing to serial
    uint8_t state_read;

    //data members for writing to SD for data read from Serial
		uint8_t receive_buffer[transfer_block_size];    //create buffer
		uint8_t state_write;
    unsigned int block_delay_msec = 1;        //how much delay to put between each block when transmitting
	
};

// //////////////////// Helper classes for processing the text stream produced by the SdFat methods


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
		Print *print_ptr = nullptr;
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
		Print *print_ptr = nullptr;
		char targ_char = 'A';  //default, will be overwritten in constructor
};



#endif

