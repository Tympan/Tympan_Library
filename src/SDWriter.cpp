#include "SDWriter.h"

// Constants for writing WAV Header INFO metadata
constexpr size_t riffChunkSizeIdx = 4;

bool SDWriter::openAsWAV(const char *fname) {
	bool returnVal = open(fname);
	if (isFileOpen()) { //true if file is open
		flag__fileIsWAV = true;
		wavHeaderInt16(44);	// Build WAV header with no audio data. This results in a header of 44 bytes.
		if ( pWavHeader && (WAVheader_bytes>0) ){
			file.write(pWavHeader, WAVheader_bytes); //initialize assuming zero length
		} 
	}
	return returnVal;
}

// Writes WAV header, including an info chunk that can hold metadata.
bool SDWriter::openAsWAV(const char *fname, const InfoKeyVal_t &infoKeyVal) {
	bool returnVal = open(fname);

	if (isFileOpen()) { //true if file is open
		flag__fileIsWAV = true;
		wavHeaderInt16(44);	// Build WAV header with no audio data. This results in a header of 44 bytes.
		insertWavInfoComment(infoKeyVal);
		if ( pWavHeader && (WAVheader_bytes>0) ){
			file.write(pWavHeader, WAVheader_bytes); //initialize assuming zero length
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

char* SDWriter::wavHeaderInt16(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize) {
	//const int fileSize = bytesWritten+44;

	int fsamp = (int) sampleRate_Hz;
	int nbits = 16;   //assumes we're writing INT16 data
	int nbytes = nbits / 8;
	int nsamp = (fileSize - WAVheader_bytes) / (nbytes * nchan);

	static char wheader[44] = {0};	\\ \todo Use a single std::vector instead, to accomodate growing it for metadata

	strcpy(wheader, "RIFF");
	strcpy(wheader + 8, "WAVE");
	strcpy(wheader + 12, "fmt ");
	strcpy(wheader + 36, "data");
	*(int32_t*)(wheader + 16) = 16; // chunk_size   //is this related to assuming INT16 data type being written to file?
	*(int16_t*)(wheader + 20) = 1; // PCM
	*(int16_t*)(wheader + 22) = nchan; // numChannels
	*(int32_t*)(wheader + 24) = fsamp; // sample rate
	*(int32_t*)(wheader + 28) = fsamp * nchan * nbytes; // byte rate (updated 10/14/2024) 
	*(int16_t*)(wheader + 32) = nchan * nbytes; // block align
	*(int16_t*)(wheader + 34) = nbits; // bits per sample
	*(int32_t*)(wheader + 40) = nsamp * nchan * nbytes;
	*(int32_t*)(wheader + 4) = 36 + nsamp * nchan * nbytes;  //what is this?  Why 36???

	// Assign pointer to WAV header
	pWavHeader = wheader;

	// Update len of WAV Header
	WAVheader_bytes = 44;

	return pWavHeader;
}


// Insert INFO ICMT comment into WAV header stored in this class (not on SD card)
char* SDWriter::insertWavInfoComment(const InfoKeyVal_t &infoKeyVal) {
	static std::vector<char> newHeader;		// Static member to store header
	uint32_t listLenBytes = 0;
	std::string chunkName;							// Temporary storage of chunk names
	char* pData = nullptr;
	bool okayFlag = true;

	// if WAV header does not exist, create one.
	if ( (!pWavHeader) || (WAVheader_bytes <=0) ) {
		wavHeaderInt16(44);
	}

	// Search for "data" string in header
	if (WAVheader_bytes > 0) {
		pData = strstr(pWavHeader, "data");
	}

	// Check that header exists and there are Info tags to write
	if ( (pData) && !infoKeyVal.empty() ) {
		
		// Calculate size of LIST chunk
		listLenBytes = 4;							// Start with 4bytes for "INFO"
		for (const auto &keyVal:infoKeyVal) {
			listLenBytes += 4 + (keyVal.second).size();	// (4bytes for key) + len of value
		} 
	
	// Else error, empty infoTagArray
	} else {
		okayFlag = false;
	}

	// Add Info metadata
	if ( okayFlag ) {
		// Copy first section of existing header into dynamic array
		newHeader.assign(pWavHeader, pData); // uses strlen to omit null character

		// Append "LIST"
		chunkName.assign("LIST");
		newHeader.insert(newHeader.end(), chunkName.c_str(), chunkName.c_str() + sizeof(uint32_t) );

		// Append LIST size
		newHeader.resize( newHeader.size() + sizeof(uint32_t), '\n');					// expand length by 4 bytes
		memcpy( &newHeader.back()-sizeof(int32_t), &listLenBytes, sizeof(int32_t) ); 	// copy List length to newHeader

		// Append "INFO"
		chunkName.assign("INFO");
		newHeader.insert(newHeader.end(), chunkName.c_str(), chunkName.c_str() + sizeof(uint32_t) );

		// Append INFO tagname, size, and string
		for (const auto &keyVal:infoKeyVal) {
			// Append tagname (without null)
			chunkName.assign( InfoTagToStr(keyVal.first) );
			newHeader.insert( newHeader.end(), chunkName.c_str(), chunkName.c_str() + sizeof(uint32_t) );

			//Append tag size
			uint32_t tagLenBytes = (keyVal.second).size();								// use size of string.
			newHeader.resize( newHeader.size() + sizeof(uint32_t), 0);					// expand length and fill with 0's
			memcpy( &newHeader.back()-sizeof(int32_t), &tagLenBytes, sizeof(int32_t) );	// copy size of string.

			// Append tag string
			newHeader.insert( newHeader.end(), (keyVal.second).c_str(), (keyVal.second).c_str() + tagLenBytes);
		}

		// Insert the data chunk (pData to end of WAV header, which excludes null byte)
		newHeader.insert(newHeader.end(), pData, pWavHeader + WAVheader_bytes); // uses strlen to omit null character

		// Assign pointer to WAV header
		pWavHeader = newHeader.data();

		// Update len of WAV Header
		WAVheader_bytes = newHeader.size();

	// Else error.  Set pointer to header to nullptr and len to 0 bytes.
	} else {
		pWavHeader = nullptr;
		WAVheader_bytes = 0;
	}

	return pWavHeader;
}


// Access WAV file metadata tagname for the INFO chunk
constexpr std::string_view SDWriter::InfoTagToStr(InfoTags tagName) {
      switch (tagName) {
 		case InfoTags::ICMT: return "ICMT"; // Comments. Provides general comments about the file or the subject of the file. 
   		case InfoTags::IARL: return "IARL"; // Archival Location. Indicates where the subject of the file is archived.
   		case InfoTags::IART: return "IART"; // Artist. Lists the artist of the original subject of the file. For example, “Michaelangelo.”
   		case InfoTags::ICMS: return "ICMS"; // Commissioned. Lists the name of the person or organization that commissioned the subject of the file. 
   		case InfoTags::IDPI: return "IDPI"; // Dots Per Inch. Stores dots per inch setting of the digitizer used to produce the file, such as “ 300.”
   		case InfoTags::IENG: return "IENG"; // Engineer. Stores the name of the engineer who worked on the file. Separate the names by a semicolon and a blank. 
   		case InfoTags::IKEY: return "IKEY"; // Keywords. Provides a list of keywords that refer to the file or subject of the file. Separate multiple keywords with a semicolon
   		case InfoTags::ILGT: return "ILGT"; // Lightness. Describes the changes in lightness settings on the digitizer required to produce the file. Note that the format of this information depends on hardware used.
   		case InfoTags::IMED: return "IMED"; // Medium. Describes the original subject of the file, such as, “ computer image,” “ drawing,” “ lithograph,” and so forth.
   		case InfoTags::INAM: return "INAM"; // Name. Stores the title of the subject of the file, such as, “ Seattle From Above.”
   		case InfoTags::IPLT: return "IPLT"; // Palette Setting. Specifies the number of colors requested when digitizing an image, such as “ 256.”
   		case InfoTags::IPRD: return "IPRD"; // Product. Specifies the name of the title the file was originally intended for, such as “Encyclopedia of Pacific Northwest Geography.”
   		case InfoTags::ISBJ: return "ISBJ"; // Subject. Describes the conbittents of the file, such as “Aerial view of Seattle.”
   		case InfoTags::ISFT: return "ISFT"; // Software. Identifies the name of the software package used to create the file, such as “Microsoft WaveEdit.”
   		case InfoTags::ISHP: return "ISHP"; // Sharpness. Identifies the changes in sharpness for the digitizer required to produce the file (the format depends on the hardware used).
   		case InfoTags::ISRC: return "ISRC"; // Source. Identifies the name of the person or organization who supplied the original subject of the file. For example, “ Trey Research.”
   		case InfoTags::ISRF: return "ISRF"; //Source Form. Identifies the original form of the material that was digitized, such as “ slide,” “ paper,” “map,” and so forth. This is not necessarily the same as IMED.
   		case InfoTags::ITCH: return "ITCH"; // Technician. Identifies the technician who digitized the subject file. For example, “ Smith, John.”
		default: return "";
	  }
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