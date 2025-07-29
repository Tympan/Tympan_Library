
#include "SDWriter.h"


bool SDWriter::openAsWAV(const char *fname) {
	bool returnVal = open(fname);
	if (isFileOpen()) { //true if file is open
		flag__fileIsWAV = true;
		char* wavHeader = makeWavHeader(0);  //Populates the WAV header.  Also sets wavHeadser_bytes
		file.write(wavHeader, WAVheader_bytes); //initialize assuming zero length
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
	if (flag__fileIsWAV) {
		//re-write the header with the correct file size
		uint32_t fileSize = file.fileSize();//SdFat_Gre_FatLib version of size();
		file.seekSet(0); //SdFat_Gre_FatLib version of seek();
		char* wavHeader = makeWavHeader(fileSize);  //Populates the WAV header.  Also sets wavHeadser_bytes
		file.write(wavHeader, WAVheader_bytes); //write header with correct length
		file.seekSet(fileSize);
	}
	file.close();
	flag__fileIsWAV = false;
	return 0;
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

/*
 char* SDWriter::wavHeaderInt16(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize) {
	//const int fileSize = bytesWritten+44;

	int fsamp = (int) sampleRate_Hz;
	int nbits = 16;   //assumes we're writing INT16 data
	int nbytes = nbits / 8;
	int nsamp = (fileSize - WAVheader_bytes) / (nbytes * nchan);

	static char wheader[48]; // 44 for wav

	//https://web.archive.org/web/20100325183246/http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
	strcpy(wheader, "RIFF");
	strcpy(wheader + 8, "WAVE");
	strcpy(wheader + 12, "fmt ");
	strcpy(wheader + 36, "data");
	*(int32_t*)(wheader + 16) = 16; // size of this format chunk: 16 or 18 or 40  (use 16 for INT16 PCM, use 18 for IEEE Float32)
	*(int16_t*)(wheader + 20) = 1; // 0x0001 = WAVE_FORMAT_PCM, 0x0003 = IEEE float
	*(int16_t*)(wheader + 22) = nchan; // numChannels (number of interleaved channels)
	*(int32_t*)(wheader + 24) = fsamp; // sample rate (number of blocks per second)
	*(int32_t*)(wheader + 28) = fsamp * nchan * nbytes; // data rate (bytes/sec)
	*(int16_t*)(wheader + 32) = nchan * nbytes; // data block size (bytes)
	*(int16_t*)(wheader + 34) = nbits; // bits per sample
	// *(int16_t*)(wheader + 36) = 0;  //size of extension of this fmt chunk...only needed for Float32
	*(int32_t*)(wheader + 40) = nsamp * nchan * nbytes;  //total number of bytes in the recording...
	*(int32_t*)(wheader + 4) = 36 + nsamp * nchan * nbytes;  //what is this?  Why 36???

	return wheader;
}
*/

char* SDWriter::makeWavHeader(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize) {
	//Serial.println("SDWriter: makeWavHeader: fileSize = " + String(fileSize) + ", writeDataType = " + String((int)writeDataType)); Serial.flush(); delay(100);

	//const int fileSize = bytesWritten+44;
	int nbits = 16;   //assumes we're writing INT16 data
	if (writeDataType == SDWriter::WriteDataType::FLOAT32) nbits = 32;

	//clear the header
	for (int i=0; i < wheader_maxlen; i++) wheader[i] = (char)0;

	int fsamp = (int) sampleRate_Hz;
	int nbytes = nbits / 8;
	//int nsamp = (fileSize - WAVheader_bytes) / (nbytes * nchan);  //beware!  WAVheader_bytes changes with INT16 vs FLOAT32!!!

	//WAV Format
	//https://web.archive.org/web/20100325183246/http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html

	//Write the bytes in order to the char* string
	strcpy(wheader, "RIFF");
	strcpy(wheader + 8, "WAVE");
	strcpy(wheader + 12, "fmt ");
	if (writeDataType == SDWriter::WriteDataType::FLOAT32) {
		*(uint32_t*)(wheader + 16) = 18; // size of this format chunk: 16 or 18 or 40  (use 16 for INT16 PCM, use 18 for IEEE Float32)
		*(uint16_t*)(wheader + 20) = 3; // 0x0001 = WAVE_FORMAT_PCM, 0x0003 = IEEE float
	} else {	
		*(uint32_t*)(wheader + 16) = 16; // size of this format chunk: 16 or 18 or 40  (use 16 for INT16 PCM, use 18 for IEEE Float32)
		*(uint16_t*)(wheader + 20) = 1; // 0x0001 = WAVE_FORMAT_PCM, 0x0003 = IEEE float
	}
	*(uint16_t*)(wheader + 22) = nchan; // numChannels (number of interleaved channels)
	*(uint32_t*)(wheader + 24) = fsamp; // sample rate (number of blocks per second)
	*(uint32_t*)(wheader + 28) = fsamp * nchan * nbytes; // data rate (bytes/sec)
	*(uint16_t*)(wheader + 32) = nchan * nbytes; // data block size (bytes)
	*(uint16_t*)(wheader + 34) = nbits; // bits per sample
	int ref_byte = 34+2;
	int index_dwSampleLength = 0;
	if (writeDataType == SDWriter::WriteDataType::FLOAT32) {
		//extra fields required for IEEE float32 format
		*(uint16_t*)(wheader + ref_byte) = 0;  //size of extension of this fmt chunk...only needed for Float32
		ref_byte = (ref_byte + 2);
		strcpy(wheader + ref_byte, "fact");        //header for this section. Only needed for Float32
		*(uint32_t*)(wheader + ref_byte + 4) = 4;  //size of this section. Only needed for Float32
		//*(uint32_t*)(wheader + ref_byte + 8) = nsamp;   //dwSampleLength = num samples (in one channel)
		index_dwSampleLength = ref_byte + 8;
		ref_byte = ref_byte + 12;
	}
	strcpy(wheader + ref_byte, "data");
	//*(uint32_t*)(wheader + ref_byte + 4) = nsamp * nchan * nbytes;  //total number of bytes in the recording...
	WAVheader_bytes = (ref_byte + 4) + 4;
	//*(uint32_t*)(wheader + 4) = ref_byte + (nsamp * nchan * nbytes);  //INT16: 4+24+(8 * nsamp*nchan*nbytes), IEE FLOAT = 4+(24+2)+12+(8 * nsamp*nchan*nbytes)


	//now that we know the header size, write all fields with the number of samples
	int nsamp = (fileSize - WAVheader_bytes) / (nbytes * nchan);  //beware!  WAVheader_bytes changes with INT16 vs FLOAT32!!!
	if (writeDataType == SDWriter::WriteDataType::FLOAT32) *(uint32_t*)(wheader + index_dwSampleLength) = nsamp;   //dwSampleLenth = num samples (in one channel)
	*(uint32_t*)(wheader + ref_byte + 4) = nsamp * nchan * nbytes;  //total number of bytes in the recording...
	*(uint32_t*)(wheader + 4) = ref_byte + (nsamp * nchan * nbytes);  //INT16: 4+24+(8 * nsamp*nchan*nbytes), IEE FLOAT = 4+(24+2)+12+(8 * nsamp*nchan*nbytes)

	//Serial.println("SDWriter: makeWavHeader: returning...wheader:"); Serial.flush(); delay(500);
	//Serial.print("Header (char):"); for (int i=0; i < wheader_maxlen; i++) Serial.print(wheader[i]);	Serial.println();
	//Serial.print("Header (HEX):"); for (int i=0; i < wheader_maxlen; i++) { Serial.print(wheader[i], HEX); Serial.println(", ");}	Serial.println();

	return wheader;
	
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
		bufferEndInd_bytes = bufferWriteInd_bytes; //save the end point of the written data
		bufferWriteInd_bytes = 0;  //reset

		//recheck to see if we're going to pass by the read buffer index
		estFinalWriteInd_bytes = bufferWriteInd_bytes + (numChan * nsamps * nBytesPerSample);
		if ((bufferWriteInd_bytes < bufferReadInd_bytes) && (estFinalWriteInd_bytes >= bufferReadInd_bytes)) {  //exclude starting at the same index but include ending at the same index
			Serial.println("BufferedSDWriter: WARNING2: writing past the read index. Likely hiccup in WAV.");
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
	bufferWriteInd_bytes = foo_bufferWriteInd * nBytesPerSample;
	bufferEndInd_bytes = max(bufferEndInd_bytes,bufferWriteInd_bytes);

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