#include "SDWriter.h"

// Writes WAV header, including an info chunk that can hold metadata.
bool SDWriter::openAsWAV(const char *fname, const InfoKeyVal_t &infoKeyVal) {
	bool returnVal = open(fname);

	if (isFileOpen()) { //true if file is open
		flag__fileIsWAV = true;
		wavHeaderInt16(WAV_sampleRate_Hz, WAV_nchan, WAV_HEADER_NO_METADATA_NUM_BYTES, infoKeyVal);

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
char* SDWriter::wavHeaderInt16(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize, const InfoKeyVal_t &infoKeyVal) {
	
	constexpr uint16_t BitsPerSamp = 16;
	
	// Clear wavHeader
	wavHeader.clear();
	pWavHeader = nullptr; 
	WAVheader_bytes = 0; 

	// Check input arguments
	if ( (sampleRate_Hz>0) && (nchan>0) && (fileSize>8) ) {

		// Create Riff chunk and append to WAV header.
		Riff_Header_u riffChunk;
		riffChunk.Riff_s.chunkLenBytes	= fileSize - 8;  // File length (in bytes) - 8bytes
		std::copy(&riffChunk.byteStream[0], &riffChunk.byteStream[0] + sizeof(Riff_Header_u), std::back_inserter(wavHeader));

		// Create fmt chunk and append to WAV header.
		Fmt_Header_u fmtChunk;
		fmtChunk.Fmt_s.numChan 			= (uint16_t) nchan;								// # of audio channels
		fmtChunk.Fmt_s.sampleRate_Hz 	= (uint32_t) sampleRate_Hz;						// Sample Rate
		fmtChunk.Fmt_s.byteRate 		= (uint32_t) (sampleRate_Hz * nchan * BitsPerSamp/8u);  // SampleRate * NumChannels * BitsPerSample/8
		fmtChunk.Fmt_s.blockAlign		= (uint16_t) ( nchan * BitsPerSamp / sizeof(uint8_t) );	 // NumChannels * BitsPerSample/8
		std::copy(&fmtChunk.byteStream[0], &fmtChunk.byteStream[0] + sizeof(Fmt_Header_u), std::back_inserter(wavHeader));

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
		}

		// Create data chunk (with no data), and assign to WAV header.
		Data_Header_u dataChunk;
		dataChunk.Data_s.chunkLenBytes 	= (uint32_t)( 0 * nchan * BitsPerSamp / sizeof(uint8_t) ); 	// Number of audio bytes: NumSamples * NumChannels * BitsPerSample/8
		std::copy(&dataChunk.byteStream[0], &dataChunk.byteStream[0] + sizeof(Data_Header_u), std::back_inserter(wavHeader));

		// Update pointer to wavHeader and # of bytes.
		pWavHeader = wavHeader.data();
		WAVheader_bytes = wavHeader.size();
	}

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
			float32_t val_f32 = ptr_audio[Ichan][Isamp];  //float value, scaled -1.0 to +1.0
	
			//add dithering, if desired
			if (ditheringMethod > 0) val_f32 += generateDitherNoise(Ichan,ditheringMethod);
	
			//convert to INT16 datatype and put in the write buffer
			write_buffer[bufferWriteInd++] = (int16_t) max(-32767.0,min(32767.0,(val_f32*32767.0f))); //truncation, with saturation
			//write_buffer[bufferWriteInd++] = (int16_t) max(-32767.0,min(32767.0,(val_f32*32767.0f + 0.5f))); //round, with saturation
		}
	}

	//handle the case where we just wrote past the read index.  Push the read index ahead.
	if (flag_moveReadIndexToEndOfWrite) bufferReadInd = bufferWriteInd;
}

//write buffered data if enough has accumulated
int BufferedSDWriter::writeBufferedData(void) {
	const int max_writeSizeSamples = 8*writeSizeSamples;  //was 8
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