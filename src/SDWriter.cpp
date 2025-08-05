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
 * 
 */
void SDWriter::AddMetadata(const String &comment) {
	std::string commentStr( comment.c_str() );
	AddMetadata(InfoTags::ICMT, commentStr);
}

/**
 * @brief Add a metadata tagname and string, to be written when startRecording() is called.
 * 
 */
void SDWriter::AddMetadata(const InfoTags &infoTag, const std::string &infoString) {
	infoKeyVal.insert({infoTag, infoString});
}

/**
 * @brief Clear the metadata buffer that startRecording uses to write metadata.
 * 
 */
void SDWriter::ClearMetadata(void) {
	infoKeyVal.clear();
}


bool SDWriter::openAsWAV(const char *fname) {
	bool returnVal = open(fname);

	if (isFileOpen()) { //true if file is open
		flag__fileIsWAV = true;
		// wavHeaderInt16(WAV_sampleRate_Hz, WAV_nchan, WAV_HEADER_NO_METADATA_NUM_BYTES); to write 16-bit data
		makeWavHeader(WAV_HEADER_NO_METADATA_NUM_BYTES);

		// Check that WAV Header is valid
		if ( pWavHeader && (WAVheader_bytes>0) ){
			file.write(pWavHeader, WAVheader_bytes); // Write WAV header assuming no audio data
		}
	}
	return returnVal;
}

bool SDWriter::open(const char *fname) {
	if (sd->exists(fname)) {  //maybe this isn't necessary when using the O_TRUNC flag below
		// The SD library writes new data to the end of the file, so to start
		//a new recording, the old file must be deleted before new data is written.
		sd->remove(fname);
	}
	__disable_irq();
		file.open(fname, O_RDWR | O_CREAT | O_TRUNC);
	__enable_irq();
	//file.createContiguous(fname, PRE_ALLOCATE_SIZE); //alternative to the line above
	return isFileOpen();
}
		
int SDWriter::close(void) {
	//file.truncate(); 
	updateWavFileSizeOnSD();  // ignore returned error;
	file.close();
	flag__fileIsWAV = false;
	return 0;
}

// Updates file size specified in WAV header of open SD file
bool SDWriter::updateWavFileSizeOnSD(void) {
	bool okayFlag = false;  // init to error
	uint32_t riffChunkSize = 0;

	if (flag__fileIsWAV) {
		// Write the RIFF chunk size specified in the header
		riffChunkSize = file.fileSize();//SdFat_Gre_FatLib version of size();

		// Check that file is bigger than just the RIFF + RIFF chunk Size header
		if (riffChunkSize>8) { 
			riffChunkSize -= 8;

			// Seek to RIFF chunk size index; Catches file not open and file too small
			if ( file.seekSet(riffChunkSize) ) { //SdFat_Gre_FatLib version of seek(); 
				
				// Write RIFF chunk size
				if ( file.write(&riffChunkSize, sizeof(uint32_t)) == sizeof(uint32_t) ) { //write header with correct length
					
					// Seek to end of file
					if ( file.seekSet(file.fileSize() ) ) {
						okayFlag = true;
					}
				}
			}
		}
	}

	return okayFlag;
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

// Build WAV header.  If error, returns nullptr and sets WAVheader_bytes==0
char* SDWriter::makeWavHeader(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize) {
	//Serial.println("SDWriter: makeWavHeader: fileSize = " + String(fileSize) + ", writeDataType = " + String((int)writeDataType)); Serial.flush(); delay(100);
	uint16_t bitsPerSamp = GetBitsPerSampType();  // Set bits based on writeDataType

	// Clear wavHeader
	wavHeader.clear();		// Empty char vector
	pWavHeader = nullptr; 	// Set null pointer
	WAVheader_bytes = 0; 	// Reset num bytes in header buffer

	// Create Riff chunk and append to WAV header.
	Riff_Header_u riffChunk;
	riffChunk.Riff_s.chunkLenBytes	= std::max(fileSize, 8UL) - 8;  // File length (in bytes) - 8bytes

	std::copy(&riffChunk.byteStream[0], &riffChunk.byteStream[0] + sizeof(Riff_Header_u), std::back_inserter(wavHeader));

	// Create fmt chunk (depending on selected data type)
	switch (writeDataType) {
		// For integer data types
		case SDWriter::WriteDataType::INT16:
		case SDWriter::WriteDataType::INT24: {
			Fmt_Pcm_Header_u fmtPcm;
			fmtPcm.Fmt_Pcm_s.numChan 			= (uint16_t) nchan;								// # of audio channels
			fmtPcm.Fmt_Pcm_s.sampleRate_Hz 		= (uint32_t) sampleRate_Hz;						// Sample Rate
			fmtPcm.Fmt_Pcm_s.byteRate 			= (uint32_t) (sampleRate_Hz * nchan * (bitsPerSamp/8ul) );  // SampleRate * NumChannels * BitsPerSample/8
			fmtPcm.Fmt_Pcm_s.blockAlign			= (uint16_t) ( nchan * (bitsPerSamp / sizeof(uint8_t)) );	 // NumChannels * BitsPerSample/8
			fmtPcm.Fmt_Pcm_s.bitsPerSample		= bitsPerSamp;
			
			// Append to header buffer
			std::copy(&fmtPcm.byteStream[0], &fmtPcm.byteStream[0] + sizeof(fmtPcm), std::back_inserter(wavHeader));
			break;
		}
		// For Float 32 data type
		case SDWriter::WriteDataType::FLOAT32:
		default: {								// Default to Float32 type, though really this is ambigiuous
			Fmt_Ieee_Header_u fmtIeee;
			fmtIeee.Fmt_Ieee_s.numChan 			= (uint16_t) nchan;								// # of audio channels
			fmtIeee.Fmt_Ieee_s.sampleRate_Hz 	= (uint32_t) sampleRate_Hz;						// Sample Rate
			fmtIeee.Fmt_Ieee_s.byteRate 		= (uint32_t) (sampleRate_Hz * nchan * (bitsPerSamp/8ul) );  // SampleRate * NumChannels * BitsPerSample/8
			fmtIeee.Fmt_Ieee_s.blockAlign		= (uint16_t) ( nchan * (bitsPerSamp / sizeof(uint8_t)) );	 // NumChannels * BitsPerSample/8
			fmtIeee.Fmt_Ieee_s.bitsPerSample	= bitsPerSamp;

			Fact_Header_u factChunk;			// Fact chunk parameters will be updated after closing file.
			
			// Append to header buffer
			std::copy(&fmtIeee.byteStream[0], &fmtIeee.byteStream[0] + sizeof(fmtIeee), std::back_inserter(wavHeader));
			std::copy(&factChunk.byteStream[0], &factChunk.byteStream[0] + sizeof(factChunk), std::back_inserter(wavHeader));
			break;
		}
	}

	// If Info tag specified, build a LIST.. INFO chunk and append to WAV header. 
	if ( !infoKeyVal.empty() ) {
		List_Header_u listChunk;

		// Calculate size of INFO subchunk by iterating through each info key
		for (const auto &keyVal:infoKeyVal) {
			listChunk.List_s.subChunkLenBytes += 4 + (keyVal.second).size()-1;	// (4bytes for key) + len of string
		} 
		// Assign List chunk length
		listChunk.List_s.chunkLenBytes = listChunk.List_s.subChunkLenBytes + 8;	// Add 8 bytes for "INFO" and info subchunk len

		// Append chunk ID, len and subchunk ID, len
		std::copy(&listChunk.byteStream[0], &listChunk.byteStream[0] + sizeof(List_Header_u), std::back_inserter(wavHeader));

		// Append info key and strings
		for (const auto &keyVal:infoKeyVal) {
			// Append tagname (without null terminator)
			wavHeader.insert( 
				wavHeader.end(), 
				InfoTagToStr(keyVal.first).data(), 
				InfoTagToStr(keyVal.first).data() + InfoTagToStr(keyVal.first).size() );
				
			//Append tag size
			uint32_t tagLenBytes = (keyVal.second).size();								// use size of string.
			wavHeader.insert( wavHeader.end(), (char*) &tagLenBytes, (char*) &tagLenBytes + sizeof(tagLenBytes) );

			// Append tag string (without null terminator)
			wavHeader.insert( wavHeader.end(), (keyVal.second).data(), (keyVal.second).data() + (keyVal.second).size() );
		}
	} // else no info tag, so pass

	// Create data chunk (with no data), and assign to WAV header.
	Data_Header_u dataChunk;
	dataChunk.Data_s.chunkLenBytes 	= (uint32_t)( 0 * nchan * bitsPerSamp / sizeof(uint8_t) ); 	// Number of audio bytes: NumSamples * NumChannels * BitsPerSample/8
	std::copy(&dataChunk.byteStream[0], &dataChunk.byteStream[0] + sizeof(Data_Header_u), std::back_inserter(wavHeader));

	// Update pointer to wavHeader and # of bytes.
	pWavHeader = wavHeader.data();
	WAVheader_bytes = wavHeader.size();

	return pWavHeader;
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