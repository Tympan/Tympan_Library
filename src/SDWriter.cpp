#include "SDWriter.h"
#include <algorithm>


/*
#include "SD.h"  //used for isSdCardPresent()
int SDWriter::isSdCardPresent(void) {
	if (!hasSdBegun) hasSdBegun = SD.begin(BUILTIN_SDCARD);  //this will cause problems if someone else has already started the SD
	return SD.mediaPresent();
}
*/


/**
 * @brief Add a metadata comment under the LIST<INFO><ICMT> tag, to be written when startRecording() is called.
 * \note If tag exists, appends string.  To clear, call ClearMetadata()
 * @param comment Comment to add
 */

void SDWriter::AddMetadata(const String &comment) {
	std::string commentStr( comment.c_str() );
	AddMetadata(Info_Tags::ICMT, commentStr);
}


 /**
 * @brief Add a metadata tagname and string, to be written when startRecording() is called.
 * \note If tag exists, appends string.  To clear, call ClearMetadata()
 * @param infoTag Tag to add the comment under
 * @param infoString  Comment to add
 */
void SDWriter::AddMetadata(const Info_Tags &infoTag, const std::string &infoString) {
	if( !infoString.empty() ) {
		// If  key exists append String
		if ( infoKeyVal.count(infoTag)>0 ) {
			infoKeyVal[infoTag] += infoString;
		
		// Else insert comment as new key
		} else {
			infoKeyVal.insert({infoTag, infoString});
		}
	} else {
		Serial.println("SDWriter::AddMetadata(): Error. infoTag not found.");
	}
}


/**
 * @brief Clear all tags from metadata buffer that startRecording uses to write metadata.
 * 
 */
void SDWriter::ClearMetadata(void) {
	infoKeyVal.clear();
}

/**
 * @brief Clear specfic tag from the metadata buffer.
 * 
 */
void SDWriter::ClearMetadata(const Info_Tags &infoTag) {
	if ( infoKeyVal.count(infoTag)>0 ) {
		infoKeyVal.erase(infoTag);
	}
}


/**
 * @brief Selece to write metadata before or after the audio data.
 * \note Some WAV players have a preference to where metadata is stored, 
 * though technically, the RIFF standard says the INFO<LIST> chunk can go
 * anywhere after the fmt chunk * 
 * @param metadataLoc Selects location
 */
void SDWriter::SetMetadataLocation(List_Info_Location metadataLoc) {
	listInfoLoc = metadataLoc;
}


bool SDWriter::openAsWAV(const char *fname) {
	bool returnVal = open(fname);

	if (isFileOpen()) { //true if file is open
		flag__fileIsWAV = true;

		// Build WAV header buffer and update pWavHeader pointer
		makeWavHeader(WAV_sampleRate_Hz, WAV_nchan, 0);  // Set file size to 0 to automatically calculate header length

		// Check that WAV Header is valid
		if ( pWavHeader && (WAVheader_bytes>0) ){
			file.write(pWavHeader, WAVheader_bytes); // Write WAV header assuming no audio data
		}

		// Mark start of data chunk to later calculate numSamples.
		filePosAudioData = file.curPosition();
	}
	return returnVal;
}

/**
 * @brief Open file on SD card.  If file already exists, delete it. 
 * 
 * @param fname File to open
 * @return true Success
 * @return false Failed to delete existing file, or failed to open file
 */
bool SDWriter::open(const char *fname) {
	bool okayFlag = true;

	if (sd->exists(fname)) {  //maybe this isn't necessary when using the O_TRUNC flag below
		// The SD library writes new data to the end of the file, so to start
		//a new recording, the old file must be deleted before new data is written.
		okayFlag = sd->remove(fname);
	}
	
	if (okayFlag) {
		__disable_irq();
			okayFlag = file.open(fname, O_RDWR | O_CREAT | O_TRUNC);
			//file.createContiguous(fname, PRE_ALLOCATE_SIZE); //alternative to the line above
		__enable_irq();

		if (!okayFlag) {
			Serial.println( String("Error: SDWriter.open() failed to open file: ") + String(fname) );
		}
	} else {
		Serial.println("Error: SDWriter.open() failed to delete existing file.");
	}
	return ( okayFlag && isFileOpen() );
}


/**
 * @brief Updates the WAV header for fields that rely on numSamples. The closes the file.
 * 
 * @return int: 0: Success; -1: Failure.  (Prior to Aug 2025, always returned 0)
 */
int SDWriter::close(void) {
	//file.truncate(); 
	bool okayFlag = true;
	uint32_t numTotalSamples = 0;

	// Record total number of samples (samples x numChannels)
	if (file.curPosition() > filePosAudioData) {
		numTotalSamples = (file.curPosition() - filePosAudioData) / (uint32_t) GetBitsPerSampType();
	// Else start of audio data was not recorded
	} else {
		okayFlag = false;
	}

	// Clear wavHeader buffer
	wavHeader.clear();
	WAVheader_bytes = 0;

	// If data chunk ends on an odd byte, then pad with 0
	if ( file && file.isOpen()) {
		if ( file.size()%2 != 0 ) {									// If not on word boundary, ...
			if ( file.write( (uint8_t)0 ) != sizeof(uint8_t) ) {	// Pad with zero
				Serial.print ("Error padding WAV header");	
				okayFlag = false;
			} // else success
		} // else file ends in even byte, pass
	}

	// Write data chunk length
	if (okayFlag) {
		if (!UpdateHeaderDataChunk(file) ) {
			Serial.println("Error writing WAV Header data chunk length.");
			okayFlag = false;
		}
	}

	// If F32 audio update "fact" chunk for # of samples
	if (okayFlag && writeDataType==WriteDataType::FLOAT32) {
		if( !UpdateHeaderFactChunk(file, numTotalSamples) ) {
			Serial.println("Error updating fact chunk.");
			okayFlag = false;
		}
	} 

	// Write LIST<INFO> metadata chunk (if it is not empty)
	if ( okayFlag && ( !infoKeyVal.empty() ) ) {			
		List_Header_u listChunk;	// temporary buffer to store LIST chunk header

		// Seek to end of file
		if ( !file.seekSet( file.fileSize() ) ) {
			okayFlag = false;
			Serial.print("Error seeking end of file.");
		} else {
			// Calculate size of LIST chunk 
			listChunk.S.chunkLenBytes = 4;	// Add 4 bytes for "INFO"

			// Add up all the info tag names and strings.  
			for (auto &keyVal:infoKeyVal) {
				//If string is odd length, pad with 0
				if ( (keyVal.second).size()%2!=0 ){
					(keyVal.second).push_back('\0');
				}

				// Add length of Key ID (4), Key Len (4) and len of string
				listChunk.S.chunkLenBytes += 8 + (keyVal.second).size();	
			} 

			// Store chunk ID, len and subchunk ID, len in header buffer
			std::copy(&listChunk.byteStream[0], &listChunk.byteStream[0] + sizeof(List_Header_u), std::back_inserter(wavHeader));

			// Append info key and strings
			for (const auto &keyVal:infoKeyVal) {
				// Append tagname (without null terminator)
				wavHeader.insert( 
					wavHeader.end(), 
					InfoTagToStr(keyVal.first).data(), 
					InfoTagToStr(keyVal.first).data() + InfoTagToStr(keyVal.first).size() );
				
				// Append tag size
				uint32_t tagLenBytes = (keyVal.second).size();	// use size of string
				
				wavHeader.insert( wavHeader.end(), (char*) &tagLenBytes, (char*) &tagLenBytes + sizeof(tagLenBytes) );

				// Append tag string (without null terminator)
				wavHeader.insert( wavHeader.end(), (keyVal.second).data(), (keyVal.second).data() + (keyVal.second).size() );
			}

			// Write WAV header to file
			if ( wavHeader.size()>0 ) {
				size_t bytesWritten = file.write( wavHeader.data(), wavHeader.size() ); // Write WAV header assuming no audio data

				if ( bytesWritten != wavHeader.size() ) {
					okayFlag = false;
					Serial.println("Error writing WAV header LIST chunk"); 
					Serial.println( String("Wrote ")+String(bytesWritten)+String(" bytes; Expected: ")+String(wavHeader.size())+String(" bytes") );
				} // else success
			} else {
				okayFlag = false;
				Serial.println( String("Error building WAV header LIST<INFO> chunk. numBytes = ") + String(wavHeader.size()) );
			}
		}
		
		// Clear the WAV metadata member (regardless of error)
		infoKeyVal.clear();
	}
	
	// Update RIFF chunk len
	if (okayFlag) {
		okayFlag = UpdateHeaderRiffChunk(file);
	}

	// Close file
	if ( file && file.isOpen() ) {
		file.close();
		flag__fileIsWAV = false;
	}

	if (okayFlag) {
		return 0;
	} else {
		return -1;
	}
}


//This "write" is for compatibility with the Print interface.  Writing one
//byte at a time is EXTREMELY inefficient and shouldn't be done
size_t SDWriter::write(uint8_t foo)  {
	size_t return_val = 0;
	if (file.isOpen()) {

		// write one audio byte.  Very inefficient.
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
size_t SDWriter::write(const uint8_t *buff, int nbytes) {
	size_t return_val = 0;
	if (file.isOpen()) {
		if (flagPrintElapsedWriteTime) { usec = 0; }
		file.write((byte *)buff, nbytes); return_val = nbytes;

		//write elapsed time only to USB serial (because only that is fast enough)
		if (flagPrintElapsedWriteTime) { Serial.print("SD, us="); Serial.println(usec); }
	}
	return return_val;
}


/**
 * @brief Build char buffer with WAV header and return a pointer to the buffer
 * 
 * @param sampleRate_Hz Sample rate of the recording
 * @param nchan # of audio channels
 * @param fileSize Size of entire file.  If unknown, then set to 0 to use the calculated size of this header.
 * @return char* pointer to the WAV header character buffer, for writing to file.
 */
char* SDWriter::makeWavHeader(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize) {
	//Serial.println("SDWriter: makeWavHeader: fileSize = " + String(fileSize) + ", writeDataType = " + String((int)writeDataType)); Serial.flush(); delay(100);
	
	// Initialize chunks (some of which may not be used)
	Riff_Header_u riffChunk;
	Fmt_Pcm_Header_u fmtPcm;
	Fmt_Ieee_Header_u fmtIeee;
	Fact_Header_u factChunk;
	List_Header_u listChunk;
	Data_Header_u dataChunk;

	uint16_t bitsPerSamp = GetBitsPerSampType();  // Set bits based on writeDataType

	// Clear wavHeader
	wavHeader.clear();		// Empty char vector
	pWavHeader = nullptr; 	// Set null pointer
	WAVheader_bytes = 0; 	// Reset num bytes in header buffer

	// --- Riff chunk ---
	riffChunk.S.chunkLenBytes = sizeof(riffChunk.S.format);

	// --- fmt chunk --- 
	// Format depends on data type
	switch (writeDataType) {
		// For integer data types
		case SDWriter::WriteDataType::INT16:
		case SDWriter::WriteDataType::INT24: {
			fmtPcm.S.numChan 			= (uint16_t) nchan;											// # of audio channels
			fmtPcm.S.sampleRate_Hz 		= (uint32_t) sampleRate_Hz;									// Sample Rate
			fmtPcm.S.byteRate 			= (uint32_t) (sampleRate_Hz * nchan * (bitsPerSamp/8ul) );  // SampleRate * NumChannels * BitsPerSample/8
			fmtPcm.S.blockAlign			= (uint16_t) ( nchan * (bitsPerSamp / sizeof(uint8_t)) );	// NumChannels * BitsPerSample/8
			fmtPcm.S.bitsPerSample		= bitsPerSamp;

			// update fmt chunk size
			riffChunk.S.chunkLenBytes += fmtPcm.S.chunkLenBytes;
			break;
		}
		// For Float 32 data type
		case SDWriter::WriteDataType::FLOAT32:
		default: {								// Default to Float32 type, though really this is ambigiuous
			fmtIeee.S.numChan 			= (uint16_t) nchan;											// # of audio channels
			fmtIeee.S.sampleRate_Hz 	= (uint32_t) sampleRate_Hz;									// Sample Rate
			fmtIeee.S.byteRate 		= (uint32_t) (sampleRate_Hz * nchan * (bitsPerSamp/8ul) );  // SampleRate * NumChannels * BitsPerSample/8
			fmtIeee.S.blockAlign		= (uint16_t) ( nchan * (bitsPerSamp / sizeof(uint8_t)) );	// NumChannels * BitsPerSample/8
			fmtIeee.S.bitsPerSample	= bitsPerSamp;

			// Update Fact Chunk
			factChunk.S.numTotalSamp		= 0; // Update later on closing file

			// update fmt chunk size
			riffChunk.S.chunkLenBytes += fmtIeee.S.chunkLenBytes;
			break;
		}
	}

	// --- INFO Chunk --- If Info tag specified, build a LIST.. INFO chunk and append to WAV header. 
	if ( !infoKeyVal.empty() && (listInfoLoc==List_Info_Location::Before_Data) ) {
		// Calculate size of LIST chunk 
		listChunk.S.chunkLenBytes = 4;	// Add 4 bytes for "INFO"

		// Add up all the info tag names and strings
		for (auto &keyVal:infoKeyVal) {
			//If string is odd length, pad with 0
			if ( (keyVal.second).size()%2!=0 ){
				(keyVal.second).push_back('\0');
			}

			// Add length of Key ID (4), Key Len (4) and len of string
			listChunk.S.chunkLenBytes += 8 + (keyVal.second).size();	
		} 

		// update fmt chunk size
		riffChunk.S.chunkLenBytes += listChunk.S.chunkLenBytes;

	} // else no info tag, so pass


	// --- data chunk --- 
	// Contains no data, just the beginning chunk ID and length
	dataChunk.S.chunkLenBytes 	= (uint32_t)( 0 * nchan * bitsPerSamp / sizeof(uint8_t) ); 	// Number of audio bytes: NumSamples * NumChannels * BitsPerSample/8
	
	// update fmt chunk size
	riffChunk.S.chunkLenBytes += dataChunk.S.chunkLenBytes;

	// If filesize specified, then override the calculated size of the RIFF Chunk
	if (fileSize > 0) {
		riffChunk.S.chunkLenBytes = std::max(fileSize, 8UL) - 8;  // File length (in bytes) - 8bytes
	}  // else leave the fileSize as specified.


	// --- WRITE CHUNKS TO BUFFER --- 
	// Write RIFF chunk
	std::copy(&riffChunk.byteStream[0], &riffChunk.byteStream[0] + sizeof(Riff_Header_u), std::back_inserter(wavHeader));

	// Write fmt subchunk
	switch (writeDataType) {
		// For integer data types
		case SDWriter::WriteDataType::INT16:
		case SDWriter::WriteDataType::INT24: {
			std::copy(&fmtPcm.byteStream[0], &fmtPcm.byteStream[0] + sizeof(Fmt_Pcm_Header_u), std::back_inserter(wavHeader));
			break;
		}
		// For Float 32 data type
		case SDWriter::WriteDataType::FLOAT32:
		default: {								// Default to Float32 type, though really this is ambigiuous			
			std::copy(&fmtIeee.byteStream[0], &fmtIeee.byteStream[0] + sizeof(Fmt_Ieee_Header_u), std::back_inserter(wavHeader));
			std::copy(&factChunk.byteStream[0], &factChunk.byteStream[0] + sizeof(Fact_Header_u), std::back_inserter(wavHeader));
			break;
		}
	}

	// Write INFO chunk (if it is not empty and we want it before the data chunk)
	if ( (!infoKeyVal.empty()) && (listInfoLoc==List_Info_Location::Before_Data) ) {
		// Append chunk ID, len and subchunk ID, len
		std::copy(&listChunk.byteStream[0], &listChunk.byteStream[0] + sizeof(List_Header_u), std::back_inserter(wavHeader));

		// Append info key and strings
		for (const auto &keyVal:infoKeyVal) {
			// Append tagname (without null terminator)
			wavHeader.insert( 
				wavHeader.end(), 
				InfoTagToStr(keyVal.first).data(), 
				InfoTagToStr(keyVal.first).data() + InfoTagToStr(keyVal.first).size() );
			
			// Append tag size
			uint32_t tagLenBytes = (keyVal.second).size();	// use size of string
			
			wavHeader.insert( wavHeader.end(), (char*) &tagLenBytes, (char*) &tagLenBytes + sizeof(tagLenBytes) );

			// Append tag string (without null terminator)
			wavHeader.insert( wavHeader.end(), (keyVal.second).data(), (keyVal.second).data() + (keyVal.second).size() );

			// If tag string is odd # of bytes, append 0
			if ( (tagLenBytes % 2)!=0 ) {
				wavHeader.push_back('\0');
			}
		}

		// Clear the INFO key so it won't be written after the data
		infoKeyVal.clear();
	}

	// Write data chunk
	std::copy(&dataChunk.byteStream[0], &dataChunk.byteStream[0] + sizeof(Data_Header_u), std::back_inserter(wavHeader));

	// Update pointer to wavHeader and # of bytes.
	pWavHeader = wavHeader.data();
	WAVheader_bytes = wavHeader.size();

	return pWavHeader;
}


/**
 * @brief Update length of data subchunk in WAV Header
 * Returns to file position before method was called.
 * \note Current file pos must be at the end of the audio data subchunk
 * @param file file handle (to open WAV file)
 * @return true Success	
 * @return false Error
 */
bool SDWriter::UpdateHeaderDataChunk(SdFile &file) {
	size_t dataLen = 0;
	size_t dataEndPos = 0;
	bool okayFlag = true;

	// Store current position at the end of the data subchunk; then seek to start of file
	if ( file and file.isOpen() ) {
		dataEndPos = file.curPosition();
		
		// Seek to start of file
		if( !file.seekSet( sizeof(uint32_t) ) ) {
			okayFlag = false;
			Serial.println("Error seeking WAV file position.");
		}
	
	// Else error with file handle
	} else {
		okayFlag = false;
		Serial.print("Error: Invalid WAV file handle.");
	}

	// Seek to data chunk
	if (okayFlag) {
		const std::vector<char> dataPattern = {'d', 'a', 't', 'a'};	// Chunk ID 
		
		if ( !SeekFileToPattern(file, dataPattern, false) ){
			// Error: "data" chunk id not found
			Serial.println("Error: Wave Header data chunk not found.");
			okayFlag = false;
		} 
	}

	// Check that current position leaves room for the chunk ID and len
	if (okayFlag) {
		if ( dataEndPos > file.curPosition() + 8 ) {
			// Record data len
			dataLen = dataEndPos - file.curPosition() - 8;  // exclude 8 bytes for "data" and data len
			
			// Seek to after the "data" chunk ID
			if(!file.seekCur( sizeof(uint32_t) ) ){
				Serial.println("Error seeking WAV file position.");
				okayFlag = false;
			}
	
		// ELSE Error. Call this function at the end of writing audio data
		} else {
			Serial.println("Error: Before calling this method, set file to end of audio 'data' chunk");
			Serial.println( String("\tchunk ID addr: ") + String( file.curPosition() ) );
			Serial.println( String("\tpresumed end of chunk: ") + String(dataEndPos) );
			okayFlag = false;
		}
	}

	// Write "data" subchunk len
	if ( okayFlag && file.write(&dataLen, sizeof(uint32_t)) != sizeof(uint32_t) ) {
		Serial.println("Error: Failed to write WAV Header RIFF Len");
		okayFlag = false;
	}

	// Return to file pos before executing this method.
	if ( file && file.isOpen() ) {
		if (!file.seekSet(dataEndPos) ) {		// Don't overwrite error
			okayFlag = false;
			Serial.println("Error seeking WAV file position.");
		}
	}

	return okayFlag;
}


/**
 * @brief Write number of samples to Fact Chunk in WAV Header.
 * 
 * @param file file handle (to open WAV file)
 * @param numSamples # of samples (across all channels)
 * @return true Success	
 * @return false Error
 */
bool SDWriter::UpdateHeaderFactChunk(SdFile &file, const uint32_t &numSamples) {
	const std::vector<char> dataPattern = {'f', 'a', 'c', 't'};	// Chunk ID 
	bool okayFlag = true;

	// Seek to "fact" chunk ID
	if ( SeekFileToPattern(file, dataPattern, true) ) {
		
		// Seek to numSampPerChan
		if (file.seekCur( 2*sizeof(uint32_t) ) ) {
			// Write # of samples
			if (file.write(&numSamples, sizeof(uint32_t)) != sizeof(uint32_t) ) {
				Serial.println("Error: Failed to write to WAV Header fact chunk");
				okayFlag = false;
			} // else success

		} else {
			// Error seeking file position
			Serial.println("Error seeking WAV file position.");
			okayFlag = false;
		}
	
	// Else chunk ID "fact" not found
	} else {
		// Error: "fact" chunk id not found
		Serial.println("Error: Wave Header data chunk not found.");
		okayFlag = false;
	}

	return okayFlag;
}




/**
 * @brief Writes the len of the RIFF chunk to the WAV Header
 * Seeks to "RIFF" chunk at position-4, then write file size - 8 bytes.
 * @param file file handle (to open WAV file)
 * @return true Success	
 * @return false Error 
 */
bool SDWriter::UpdateHeaderRiffChunk(SdFile &file) {
	bool okayFlag = true;
	uint32_t chunkLen = 0;
	size_t startPos = 0;

	// Store current position and get file size
	if ( file && file.isOpen() ) {
		startPos = file.curPosition();
		
		chunkLen = file.fileSize();

		if( chunkLen > 8) {	// minimum 8 bytes for "RIFF" and RIFF len
			chunkLen -= 8;
		} else {	
			// Error with file size
			okayFlag = false;
		} // else success

	// Else error with file handle
	} else {
		okayFlag = false;
	}

	// Write RIFF len
	if (okayFlag) {
		// Seek to Riff chunk len, which is after the chunk ID (4 bytes)
		if ( file.seekSet( sizeof(uint32_t) ) ) {	
			
			// Write RIFF len; check num bytes written
			if ( file.write(&chunkLen, sizeof(uint32_t)) != sizeof(uint32_t) ) {		
				// ELSE Error writing RIFF len
				okayFlag = false;
			}
		
		// Else error with file seek
		} else {
			okayFlag = false;
		}
	}

	// Seek to file position when method was entered.
	if ( file && file.isOpen() ) {
		file.seekSet(startPos);  // Don't overwrite error
	}

	return(okayFlag);
}


/**
 * @brief Seeks to start of pattern. Search begins at current file position. File must be open.
 * 
 * @param openFileH 
 * @param pattern 
 * @return bool True:pattern found
 */
bool SDWriter::SeekFileToPattern(SdFile &openFileH, const std::vector<char> &pattern, const bool &fromBeginningFlag ) {
	size_t totalBytesRead = 0;					// Tracks total number of bytes read from file.
	constexpr size_t bufferLen = 512;
	std::vector<char> buffer(bufferLen);
	size_t startPosFileBuffer = 0;
	size_t bytesRead = 0;
	bool okayFlag = true;
	bool foundFlag = false;

	// If desired, start from beginning of file
	if( file && file.isOpen() && fromBeginningFlag && !foundFlag ) {
		okayFlag = file.seekSet(0);
	}

	if (okayFlag) {
		// If file open...
		while ( file && file.isOpen()  && okayFlag && !foundFlag ){
			// Store current file position
			startPosFileBuffer = file.curPosition();

			// Read in section of data 
			bytesRead = file.read(buffer.data(), bufferLen);

			if (bytesRead != bufferLen ) {
				Serial.println("Error reading SD file.");
				okayFlag = false;
			}

			// Check for EOF
			if (bytesRead != 0) {
				// Search for pattern
				auto idx = std::search(buffer.begin(), buffer.begin() + bytesRead,
					pattern.begin(), pattern.end());
				
				// Success if returned idx is not the last byte that was read
				if (idx != buffer.begin() + bytesRead) {

					// Seek file to start of pattern
					if ( !file.seekSet( startPosFileBuffer + idx - buffer.begin() ) ) {
						okayFlag = false;
						Serial.print("Error: Unable to seek to desired WAV Header position.");
					} else {
						foundFlag = true;
					}
				}
				// Store total # of bytes read
				totalBytesRead += bytesRead;

			// Else EOF reached.  Abort.
			} else {
				Serial.println(" Error: end of file reached before pattern was matched.");
				okayFlag = false;
			}
		} // for loop
	} // else existing errpr
	return foundFlag;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

bool BufferedSDWriter::sync(void) {
	if (isFileOpen()) {
		file.FsBaseFile::flush();
		return file.sync();
	} else {
		return false;
	}
}

//here is how you send data to this class.  this doesn't write any data, it just stores data
void BufferedSDWriter::copyToWriteBuffer(float32_t *ptr_audio[], const int nsamps, const int numChan) {
	if (!write_buffer) {  //try to allocate buffer, return if it doesn't work
		//Serial.println("BufferedSDWriter: copyToWriteBuffer: write_buffer = " + String((int)write_buffer) + " so trying to allocate default size");
		if (!allocateBuffer()) {
			Serial.println("BufferedSDWriter: copyToWriteBuffer: *** ERROR ***");
			Serial.println("    : could not allocateBuffer()");
			return;
		}
	}
	

	//how much data will we write?
	uint32_t estFinalWriteInd_bytes = bufferWriteInd_bytes + (numChan * nsamps * nBytesPerSample);

	//will we pass by the read index?
	bool flag_moveReadIndexToEndOfWrite = false;
	if ((bufferWriteInd_bytes < bufferReadInd_bytes) && (estFinalWriteInd_bytes >= bufferReadInd_bytes)) { //exclude starting at the same index but include ending at the same index
		Serial.println("BufferedSDWriter: copyToWriteBuffer: WARNING1: writing past the read index. Likely hiccup in WAV.");
		flag_moveReadIndexToEndOfWrite = true;
	}

	//is there room to put the data into the buffer or will we hit the end
	if ( estFinalWriteInd_bytes >= bufferLengthBytes) { //is there room?
		//no there is not room
		if (bufferReadInd_bytes > bufferWriteInd_bytes) {
			Serial.println("BufferedSDWriter: copyToWriteBuffer: setting end of buffer (" + String(bufferWriteInd_bytes) + ") shorter than read index (" + String(bufferReadInd_bytes));
		}
		//if (bufferWriteInd_bytes != bufferEndInd_bytes) Serial.println("BufferedSDWriter: copyToWriteBuffer: setting end of buffer to " + String(bufferWriteInd_bytes) + " vs max = " + String(bufferLengthBytes));
		bufferEndInd_bytes = bufferWriteInd_bytes; //save the end point of the written data
		bufferWriteInd_bytes = 0;  //reset to beginning of the buffer

		//recheck to see if we're going to pass by the read buffer index
		estFinalWriteInd_bytes = bufferWriteInd_bytes + (numChan * nsamps * nBytesPerSample);
		if ((bufferWriteInd_bytes < bufferReadInd_bytes) && (estFinalWriteInd_bytes >= bufferReadInd_bytes)) {  //exclude starting at the same index but include ending at the same index
			Serial.println("BufferedSDWriter: copyToWriteBuffer: WARNING2: writing past the read index. Likely hiccup in WAV.");
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

	//now scale and interleave the data into the buffer
	uint32_t foo_bufferWriteInd = bufferWriteInd_bytes / nBytesPerSample;
	for (int Isamp = 0; Isamp < nsamps; Isamp++) {
		for (int Ichan = 0; Ichan < numChan; Ichan++) {
			float32_t val_f32 = ptr_audio[Ichan][Isamp];  //float value, scaled -1.0 to +1.0
			if (ditheringMethod > 0) val_f32 += generateDitherNoise(Ichan,ditheringMethod); //add dithering, if desired
	
			if (writeDataType == SDWriter::WriteDataType::INT16) {
				//convert to INT16 datatype and put in the write buffer
				((int16_t*)write_buffer)[foo_bufferWriteInd++] = (int16_t) max(-32767.0,min(32767.0,(val_f32*32767.0f))); //truncation, with saturation
				//write_buffer[bufferWriteInd++] = (int16_t) max(-32767.0,min(32767.0,(val_f32*32767.0f + 0.5f))); //round, with saturation
			} else {
				//simply copy
				((float32_t*)write_buffer)[foo_bufferWriteInd++] = val_f32; //copy
			}
		}
	}
	bufferWriteInd_bytes = foo_bufferWriteInd * nBytesPerSample; //new write index (bytes) for next time through
	
	//check to see if we're using more of the avialable buffer than previously being used
	if (bufferWriteInd_bytes > bufferEndInd_bytes) {
		//Serial.println("BufferedSDWriter: copyToWriteBuffer: extending end from " + String(bufferEndInd_bytes) + " to " + bufferWriteInd_bytes + " (vs max length of " + String(bufferLengthBytes) + ")");
		bufferEndInd_bytes = max(bufferEndInd_bytes,bufferWriteInd_bytes);
	}
	
	//handle the case where we just wrote past the read index.  Push the read index ahead.
	if (flag_moveReadIndexToEndOfWrite) bufferReadInd_bytes = bufferWriteInd_bytes;
}

//write buffered data if enough has accumulated
int BufferedSDWriter::writeBufferedData(void) {
	uint32_t writeSizeBytes = getWriteSizeBytes();
	//const uint32_t max_writeSizeBytes = 8*writeSizeBytes;  //try writing in larger sizes than the given writeSizeBytes
	const uint32_t max_writeSizeBytes = 1*writeSizeBytes;  //no benefit to larger than 512 bytes (https://forum.pjrc.com/index.php?threads/best-teensy-for-a-datalogger.73723/#post-332766)
	if (!write_buffer) return -1;

	//static uint32_t busy_count = 0;
	if (file.isOpen() && file.isBusy()) {  //https://forum.pjrc.com/index.php?threads/best-teensy-for-a-datalogger.73723/
		//busy_count++;
		//if ((busy_count % 100) == 1) {
		//	Serial.println("BufferedSDWriter: writeBuferedData: file is busy! busy_cout = " + String(busy_count));
		//}
		return -2; 
	}

	int return_val = 0;

	//if the write pointer has wrapped around, write the data
	if (bufferWriteInd_bytes < bufferReadInd_bytes) { //if the buffer has wrapped around
		//Serial.println("BuffSDI16: writing to end of buffer");
		//return_val += write((byte *)(write_buffer+bufferReadInd),
		//    (bufferEndInd-bufferReadInd)*sizeof(write_buffer[0]));
		//bufferReadInd = 0;  //jump back to beginning of buffer

		uint32_t bytesAvail = bufferEndInd_bytes - bufferReadInd_bytes;
		if (bytesAvail > 0) {
			uint32_t bytesToWrite = min(bytesAvail, max_writeSizeBytes);
			if (bytesToWrite >= writeSizeBytes) {
				bytesToWrite = ((uint32_t)bytesToWrite / writeSizeBytes) * writeSizeBytes; //truncate to nearest whole number
				//bytesToWrite =  min(bytesToWrite, writeSizeBytes); //limit the bytes to write
			}
			//if (bytesToWrite == 0) {
			//  Serial.print("SD Writer: writeBuff1: bytesAvail, to Write: ");
			//  Serial.print(bytesAvail); Serial.print(", ");
			//  Serial.println(bytesToWrite);
			//}
			//return_val += write((byte *)(write_buffer + bufferReadInd), samplesToWrite * sizeof(write_buffer[0]));
			return_val += write((byte *)(write_buffer + bufferReadInd_bytes), bytesToWrite);
			//if (return_val == 0) {
			//  Serial.print("SDWriter: writeBuff1: samps to write, bytes written: "); Serial.print(samplesToWrite);
			//  Serial.print(", "); Serial.println(return_val);
			//}
			bufferReadInd_bytes += bytesToWrite;  //jump back to beginning of buffer
			if (bufferReadInd_bytes == bufferEndInd_bytes) bufferReadInd_bytes = 0;
		} else { 
			//the read pointer is at the end of the buffer, so loop it back
			bufferReadInd_bytes = 0; 
		}
		
	} else {
		
		//do we have enough data to write again?  If so, write the whole thing
		uint32_t bytesAvail = bufferWriteInd_bytes - bufferReadInd_bytes;
		if (bytesAvail >= writeSizeBytes) {
			//Serial.println("BuffSDI16: writing buffered data");
			//return_val = write((byte *)(write_buffer+buffer, writeSizeSamples * sizeof(write_buffer[0]));

			//return_val += write((byte *)(write_buffer+bufferReadInd),
			//  (bufferWriteInd-bufferReadInd)*sizeof(write_buffer[0]));
			//bufferReadInd = bufferWriteInd;  //increment to end of where it wrote

			uint32_t bytesToWrite = min(bytesAvail, max_writeSizeBytes);
			bytesToWrite = ((uint32_t)(bytesToWrite / writeSizeBytes)) * writeSizeBytes; //truncate to nearest whole number
			//bytesToWrite = min(bytesToWrite, writeSizeBytes); //limit the bytes to write
			
			if (bytesToWrite == 0) {
				Serial.print("SD Writer: writeBuff2: bytesAvail, to Write: ");
				Serial.print(bytesAvail); Serial.print(", ");
				Serial.println(bytesToWrite);
			}
			return_val += write((byte *)(write_buffer + bufferReadInd_bytes), bytesToWrite);
			if (return_val == 0) {
				Serial.print("SDWriter: writeBuff2: bytes to write, bytes written: "); Serial.print(bytesToWrite);
				Serial.print(", "); Serial.println(return_val);
			}
			bufferReadInd_bytes += bytesToWrite;  //increment to end of where it wrote
		}
	}
	return return_val;
}

float32_t BufferedSDWriter::generateDitherNoise(const int &Ichan, const int &method) {
	//see http://www.robertwannamaker.com/writings/rw_phd.pdf
	
	//do dithering to spread out quantization noise (avoid spurious harmonics of pure tones)
	float32_t rand_f32 = ((float32_t)random(65534))/65534.0f; //uniform distribution, scaled for 0-1.0
	switch (method) {
		case 1:
			//do nothing special.  leave as uniform distribution
			rand_f32 = 2.0f*(rand_f32-0.5f);  //scale for -1.0 to +1.0
			break;
/* 			case 1:
			//try to smartly change uniform to triangular distribution, midpoint at 0.5...I think that this works right (WEA Mar 6, 2024)
			if (rand_f32 <= 0.5f) {
				rand_f32 = sqrtf(rand_f32*0.5f);
			} else {
				rand_f32 = 1.0f - sqrtf( (1.0f - rand_f32)*0.5f);
			}
			rand_f32 = 2.0f*(rand_f32-0.5f);  //scale for -1.0 to +1.0
			break; */
		case 2:
			//make it 2DOF random (triangle?).  There's surely a better way
			{
				//make a 2nd random signal
				float32_t rand2_f32 = ((float32_t)random(65534))/65534.0f; //uniform distribution, scaled for 0-1.0
				
				//make zero mean
				rand_f32 = 2.0f*(rand_f32-0.5f);  //scale for -1.0 to +1.0
				rand2_f32 = 2.0f*(rand2_f32-0.5f);  //scale for -1.0 to +1.0
				
				//join the two random values
				rand_f32 *= rand2_f32; //should now be triangular, psanning -1.0 to +1.0?

			}
			break;
	}
	rand_f32 /= 65534.0f;  //scaled to be size of last bit of int16 (but as a float where full scale is -1.0 to +1.0)
	rand_f32 *= 2.0f;      //scale it to the desired effect
	
	return rand_f32;
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