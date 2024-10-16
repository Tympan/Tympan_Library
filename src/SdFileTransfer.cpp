
// SDFileTransfer.cpp
//
// Created: Chip Audette, OpenAudio, Oct 2024
//
// MIT License

#include "SDFileTransfer.h"

#define SD_CONFIG SdioConfig(FIFO_SDIO)

//Start the SD card, if not already started
void SdFileTransfer::begin(void) { 
  //initialize the SD card, if not already begun
  if (state == SD_STATE_NOT_BEGUN) {
    if (sd_ptr == nullptr) sd_ptr = new SdFs();  //creating new SdFs (because none was given)
    if (!(sd_ptr->begin(SD_CONFIG))) { Serial.println("SdFileTransfer: *** ERROR ***: cannot open SD.");  return; }
  }
  state = SD_STATE_BEGUN;
  state_write = WRITE_STATE_NONE;
  state_read = READ_STATE_NONE;
} 

//send string of all filenames at the root of the SD card
bool SdFileTransfer::sendFilenames(const char separator) {
  if (serial_ptr == nullptr) return false;
  if (state == SD_STATE_NOT_BEGUN) begin();
  Print_ReplaceChar print_ReplaceChar(serial_ptr, '\n', separator); //this will translate newlines into commas
  Print_RemoveChar print_RemoveChar(&print_ReplaceChar, '\r'); //this will remove carriage returns
  bool ret_val = sd_ptr->ls(&print_RemoveChar);  //add LS_R to make recursive.  otherwise, just does the local directory
  serial_ptr->println(); //finish with a newline character
  return ret_val;
}

//Set the filename for use in reading or writing to the SD
String SdFileTransfer::setFilename(const String &given_fname) {
  if (isFileOpen()) file.close();  //close any file that is open
  cur_filename.remove(0); //clear the current filename String
  return cur_filename += given_fname; //append the given string and return
 } 

//receive a string as the filename for use in reading or writing to the SD
String SdFileTransfer::receiveFilename(const char EOL_char) {
  if (serial_ptr == nullptr) return String("");  //not ready to receive bytes from Serial
  return setFilename((serial_ptr->readStringUntil(EOL_char)).trim());
} 

// /////////////////////////////////////////////// Methods to READ from SD and send over Serial


bool SdFileTransfer::openRead(void)
{
  if (state == SD_STATE_NOT_BEGUN) begin();
  if (state == SD_STATE_NOT_BEGUN) return false;  //failed to open SD
  if (cur_filename.length() == 0) return false;    //no filename	
  file.close(); //make sure that any file is closed
  
  file.open(cur_filename.c_str(),O_READ);   //open for reading
  if (!isFileOpen()) return false;
  
  state_read = READ_STATE_NORMAL;
  return true;
}

//send the file size as a character string (with end-of-line)
bool SdFileTransfer::sendNumBytesToReadFromSD(void) { 
  if (file.isOpen() && (serial_ptr != nullptr)) { 
    serial_ptr->println(getNumBytesToReadFromSD()); 
    return true; 
  } 
  return false; 
}

//automates the transfer process by sending requests and receiving requests over serial		
uint64_t SdFileTransfer::sendFile_interactive(void) {
	if (serial_ptr == nullptr) {
		Serial.println("SdFileTransfer: sendFile_interactive: *** ERROR ***: Serial pointer not provided. Exiting.");
		return 0;
	}

  //get file name from the user
  Serial.println("SdFileTransfer: sendFile_interactive: send filename to read from (end with newline)");
	Serial.setTimeout(10000);  //set timeout in milliseconds...make longer for human interaction
  String fname = receiveFilename('\n'); //also sets the filename
	Serial.setTimeout(1000);  //change the Serial timeout time back to default
    
  //open the file
  bool isOpen = openRead(); //opens using the current filename
  if (!isOpen) {
    Serial.println("SdFileTransfer: sendFile_interactive: *** ERROR ***: Cannot open file " + String(fname) + " for reading. Exiting.");
    return 0;
  }
	
	//send the file size in bytes
	Serial.println("SdFileTransfer: sending file size in bytes (followed by newline)");
	uint64_t bytes_to_send = getNumBytesToReadFromSD();
	bool success = sendNumBytesToReadFromSD();
	if (!success) {
		Serial.println("SdFileTransfer: sendFile_interactive: *** ERROR ***: Could not send file size. Exiting.");
    return 0;
	}	
	
	//send the file itself
	Serial.println("SdFileTransfer: sendFile_interactive: Sending the file bytes (after this newline)");	
	uint64_t bytes_sent = sendFile();
  if (bytes_sent != bytes_to_send) {
      Serial.println("SdFileTransfer: sendFile_interactive: Only sent " + String(bytes_sent) + " bytes.  File transfer has stopped.");
  } else {
    Serial.println("SdFileTransfer: sendFile_interactive: Successfully sent " + String(bytes_sent) + " bytes from " + fname + ".  File transfer complete.");
  }	
	
	return bytes_sent;
}

//If the file is already open, send it (shoud close automatically once it is empty)
uint64_t SdFileTransfer::sendFile(void) {  //sends the file that is currently open
  uint64_t total_bytes = 0;
  unsigned int bytes; 
  while ((bytes = sendOneBlock())) { 
    total_bytes += bytes;
    delay(block_delay_msec); //delay a bit between blocks
  }
  return total_bytes;
}

unsigned int SdFileTransfer::sendOneBlock(void) {  //returns number of bytes sent
  if (serial_ptr == nullptr) return 0;  //if no serial has been specified, return
  uint8_t buffer[transfer_block_size];            //create buffer
  if (buffer == nullptr) return 0;       //if could not create buffer, return
  unsigned int bytes_read = readBytesFromSD(buffer, transfer_block_size); //read the bytes from the SD
  if (bytes_read > 0) {
    return serial_ptr->write(buffer, bytes_read);
  }
  return 0;
};

unsigned int SdFileTransfer::readBytesFromSD(uint8_t* buffer, const unsigned int n_bytes_to_read) {
  if (isFileOpen() == false) return 0;
  unsigned int bytes_read = file.read(buffer, n_bytes_to_read);

  //did the file run out of data?
  if (n_bytes_to_read != bytes_read){
    file.close();
    state_read = READ_STATE_FILE_EMPTY;
  }
  return bytes_read;
}


// //////////////////////////////////////////// Methods to WRITE to SD after receiving from Serial

bool SdFileTransfer::openWrite(void)
{
  if (state == SD_STATE_NOT_BEGUN) begin();
  if (state == SD_STATE_NOT_BEGUN) return false;  //failed to open SD
  if (cur_filename.length() == 0) return false;    //no filename
  file.close();  //close any file that happens to be open
  file.open(cur_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC);   //open for writing
  if (!isFileOpen()) return false;  //failed to open file
  
  //we have an open file, so prepare the variables to receive more info
  setNumBytesToWriteToSD(0); //clear
  state_write = WRITE_STATE_PREPARING;
  return true;
}

//how many bytes will be sent from the PC here to the SD
uint64_t SdFileTransfer::setNumBytesToWriteToSD(const uint64_t num_bytes) { 
  total_file_length = num_bytes;
  if (isFileOpen() && (total_file_length > 0)) state_write = WRITE_STATE_READY;
  return total_file_length;
} 

uint64_t SdFileTransfer::receiveNumBytesToWriteToSD(const char EOL_char) {
  if (serial_ptr == nullptr) return 0;  //not ready to receive bytes from Serial
  int num_bytes = ((serial_ptr->readStringUntil(EOL_char)).trim()).toInt();
  return setNumBytesToWriteToSD(max(0,num_bytes)); //returning zero size is a kind of error
}

//automate the file receiving process by interacting with the user over the serial
//link to (1) get the filename, (2) get the file size, (3) acknowledge ready to receive bytes
uint64_t SdFileTransfer::receiveFile_interactive(void) {
  if (serial_ptr == nullptr) {
    Serial.println("SdFileTransfer: receiveFile_interactive: *** ERROR ***: Serial pointer not provided. Exiting.");
    return 0;
  }

  //for human interaction, increase the serial timeout time
  serial_ptr->setTimeout(10000);  //set timeout in milliseconds

  //get file name from the user
  Serial.println("SdFileTransfer: receiveFile_interactive: send filename to write to (end with newline)");
  String fname = receiveFilename('\n'); //also sets the filename

  //open the file
  bool isOpen = openWrite(); //opens using the current filename
  if (!isOpen) {
    Serial.println("SdFileTransfer: receiveFile_interactive: *** ERROR ***: Cannot open file " + String(fname) + " for writing. Exiting.");
    serial_ptr->setTimeout(1000);  //change the Serial timeout time back to default
    return 0;
  }

  //get the file size
  Serial.println("SdFileTransfer: receiveFile_interactive: Opened " + fname + " for writing. Send file size in bytes (end with newline)");
  uint64_t bytes_to_receive = receiveNumBytesToWriteToSD('\n');
  if (bytes_to_receive==0) {
    Serial.println("SdFileTransfer: receiveFile_interactive: *** ERROR ***: Received invalid filesize. Exiting.");
    serial_ptr->setTimeout(1000);  //change the Serial timeout time back to default
    return 0;
  }

  //check to see if we're ready
  if ((state_write != WRITE_STATE_READY) && isFileOpen()) {
    Serial.println("SdFileTransfer: receiveFile_interactive: *** ERROR ***: Write state not ready. Exiting.");
    serial_ptr->setTimeout(1000);  //change the Serial timeout time back to default
    return 0;
  }

  //receive the file itself
  Serial.println("SdFileTransfer: receiveFile_interactive: Send the " + String(bytes_to_receive) + " bytes now");
  uint64_t n_bytes_received = receiveFile();
  if (n_bytes_received != bytes_to_receive) {
      Serial.println("SdFileTransfer: receiveFile_interactive: Only received " + String(n_bytes_received) + " bytes.  File transfer has stopped.");
  } else {
    Serial.println("SdFileTransfer: receiveFile_interactive: Successfully received " + String(n_bytes_received) + " bytes into " + fname + ".  File transfer complete.");
  }
    
  //change the Serial timeout time back to original
  serial_ptr->setTimeout(1000);

  return n_bytes_received;
}

uint64_t SdFileTransfer::receiveFile(void) {
  if (state_write != WRITE_STATE_READY) return 0;  // not ready to receive bytes to SD
  if (serial_ptr == nullptr) return 0;  //not ready to receive bytes from Serial
  uint64_t total_bytes_received = 0;
  unsigned int bytes_received; 
  //Serial.setTimeout(100); //set timeout in milliseconds

  bool done = false;
  while (!done) {
    unsigned int bytes_to_receive = min(transfer_block_size, total_file_length - total_bytes_received);
    bytes_received = receiveOneBlock(bytes_to_receive);
    total_bytes_received += bytes_received;
    if (total_bytes_received == total_file_length) done = true;
    if (bytes_received != bytes_to_receive) done = true;       //we didn't get all the bytes.  Assume a problem.
  }
  close(); //close the file
  return total_bytes_received;
}

uint64_t SdFileTransfer::receiveOneBlock(const unsigned int bytes_to_receive) {  //returns number of bytes sent
  if (serial_ptr == nullptr) return 0;      //if no serial has been specified, return
  //if (receive_buffer == nullptr) return 0;  //if could not create buffer, return
  unsigned int bytes_received = serial_ptr->readBytes(receive_buffer,bytes_to_receive);
  if (bytes_received > 0) return writeBufferedBytesToSD(receive_buffer, bytes_received);

  return 0;
};

uint64_t SdFileTransfer::writeBufferedBytesToSD(uint8_t* buffer, const unsigned int n_bytes_to_write) {
  if (state_write != WRITE_STATE_READY) return 0;
  if (isFileOpen() == false) return 0;
  uint64_t bytes_written = file.write(buffer, n_bytes_to_write);

  //did the file run out of data?
  if (n_bytes_to_write != bytes_written){
    close();
    state_write = WRITE_ERROR;
  }
  return bytes_written;
}
