
#include "SDtoSerial.h"

#define SD_CONFIG SdioConfig(FIFO_SDIO)

//Start the SD card, if no already started
void SDtoSerial::begin(void) { 
  //initialize the SD card, if not already begun
  if (state == SD_STATE_NOT_BEGUN) {
    if (sd_ptr == nullptr) sd_ptr = new SdFs();  //creating new SdFs (because none was given)
    if (!(sd_ptr->begin(SD_CONFIG))) { Serial.println("SDtoSerial: cannot open SD.");  return; }
  }
  state = SD_STATE_BEGUN;
  state_read = READ_STATE_NORMAL;
} 

//send strings of all filenames
bool SDtoSerial::sendFilenames(const char separator) {
  if (serial_ptr == nullptr) return false;
  if (state == SD_STATE_NOT_BEGUN) begin();
  Print_ReplaceChar print_ReplaceChar(serial_ptr, '\n', separator); //this will translate newlines into commas
  Print_RemoveChar print_RemoveChar(&print_ReplaceChar, '\r'); //this will remove carriage returns
  bool ret_val = sd_ptr->ls(&print_RemoveChar);  //add LS_R to make recursive.  otherwise, just does the local directory
  serial_ptr->println(); //finish with a newline character
  return ret_val;
}


bool SDtoSerial::open(const char *filename)
{
  if (state == SD_STATE_NOT_BEGUN) begin();

  file.open(filename,O_READ);   //open for reading
  if (!isFileOpen()) return false;
  
  state_read = READ_STATE_NORMAL;
  //readHeader();
		
  return true;
}


//send the file size as a character string (with end-of-line)
bool SDtoSerial::sendFileSize(void) { 
  if (file.isOpen() && serial_ptr) { 
    serial_ptr->println(getFileSize()); 
    return true; 
  } 
  return false; 
}

//opens the file, sends the size in bytes (as text), then sends the raw contents of the file, then closes the file
uint64_t SDtoSerial::sendSizeThenFile(const String &filename) {

  bool isOpen = open(filename);
  if (!isOpen) {
    Serial.println("SDtoSerial: sendSizeThenFile: *** ERROR ***: could not open " + filename + " on SD");
    return 0;
  }

  //send the size of the data from the WAV
  sendFileSize();  //sends as text.  terminated by newline. (via println())
  
  //send the file
  return sendFile(); //sends as raw bytes
}

//open the file, send it, close it
uint64_t SDtoSerial::sendFile(char *filename) { 
  uint64_t bytes_sent = 0;
  if (open(filename)) {           //tries to open the file.  Returns true if the file opens
    if (sendFileSize() > 0) {     //tries to send the file size.  Returns true if successful
      bytes_sent = sendFile();    //tries to send the file.  Returns whatever number of bytes was sent
    } 
    close();    //close the file...though it should have closed automatically when it emptied
  } 
  return bytes_sent; 
}

//If the file is already open, send it (shoud close automatically once it is empty)
uint64_t SDtoSerial::sendFile(void) {  //sends the file that is currently open
  uint64_t total_bytes = 0;
  unsigned int bytes; 
  while ((bytes = sendOneBlock())) { 
    total_bytes += bytes;
    delay(block_delay_msec); //delay a bit between blocks
  }
  return total_bytes;
}

unsigned int SDtoSerial::sendOneBlock(void) {  //returns number of bytes sent
  if (serial_ptr == nullptr) return 0;  //if no serial has been specified, return
  uint8_t buffer[transfer_block_size];            //create buffer
  if (buffer == nullptr) return 0;       //if could not create buffer, return
  unsigned int bytes_read = readBytesFromSD(buffer, transfer_block_size); //read the bytes from the SD
  if (bytes_read > 0) {
    return serial_ptr->write(buffer, bytes_read);
  }
  return 0;
};

unsigned int SDtoSerial::readBytesFromSD(uint8_t* buffer, const unsigned int n_bytes_to_read) {
  if (isFileOpen() == false) return 0;
  unsigned int bytes_read = file.read(buffer, n_bytes_to_read);

  //did the file run out of data?
  if (n_bytes_to_read != bytes_read){
    file.close();
    state_read = READ_STATE_FILE_EMPTY;
  }
  return bytes_read;
}
