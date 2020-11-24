/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

 /*
  * Adapted for float32 data type by Chip Audette, OpenAudio, December 2019.
  * Changes are MIT License.  Use at your own risk.
*/

#include <Arduino.h>
#include "AudioSDPlayer_F32.h"
//#include <spi_interrupt.h>

#define STATE_DIRECT_8BIT_MONO    0  // playing mono at native sample rate
#define STATE_DIRECT_8BIT_STEREO  1  // playing stereo at native sample rate
#define STATE_DIRECT_16BIT_MONO   2  // playing mono at native sample rate
#define STATE_DIRECT_16BIT_STEREO 3  // playing stereo at native sample rate
#define STATE_CONVERT_8BIT_MONO   4  // playing mono, converting sample rate
#define STATE_CONVERT_8BIT_STEREO 5  // playing stereo, converting sample rate
#define STATE_CONVERT_16BIT_MONO  6  // playing mono, converting sample rate
#define STATE_CONVERT_16BIT_STEREO  7  // playing stereo, converting sample rate
#define STATE_PARSE1      8  // looking for 20 byte ID header
#define STATE_PARSE2      9  // looking for 16 byte format header
#define STATE_PARSE3      10 // looking for 8 byte data header
#define STATE_PARSE4      11 // ignoring unknown chunk after "fmt "
#define STATE_PARSE5      12 // ignoring unknown chunk before "fmt "
#define STATE_STOP      13
#define STATE_NOT_BEGUN  99


unsigned long AudioSDPlayer_F32::update_counter = 0;

void AudioSDPlayer_F32::init(void) {
  state = STATE_NOT_BEGUN;
}
void AudioSDPlayer_F32::begin(void)
{
  if (state == STATE_NOT_BEGUN) {
    if (!(sd.begin())) {
      Serial.println("AudioPlaySdWAV_F32: cannot open SD.");
      return;
    }
  }
  
  state = STATE_STOP;
  state_play = STATE_STOP;
  data_length = 0;
  if (block_left_f32) {
    AudioStream_F32::release(block_left_f32);
    block_left_f32 = NULL;
  }
  if (block_right_f32) {
    AudioStream_F32::release(block_right_f32);
    block_right_f32 = NULL;
  }

  update_counter = 0;
}


bool AudioSDPlayer_F32::play(const char *filename)
{
  if (state == STATE_NOT_BEGUN) begin();
  
  stop();

  file.open(filename,O_READ);   //open for reading
  if (!isFileOpen()) return false;
  
  buffer_length = 0;
  buffer_offset = 0;
  state_play = STATE_STOP;
  data_length = 20;
  header_offset = 0;
  state = STATE_PARSE1;
  return true;
}

void AudioSDPlayer_F32::stop(void)
{
  if (state == STATE_NOT_BEGUN) begin();

  if (state != STATE_STOP) {
    audio_block_f32_t *b1 = block_left_f32;
    block_left_f32 = NULL;
    audio_block_f32_t *b2 = block_right_f32;
    block_right_f32 = NULL;
    state = STATE_STOP;

    if (b1) AudioStream_F32::release(b1);
    if (b2) AudioStream_F32::release(b2);
    file.close();

  } else {

  }
}


void AudioSDPlayer_F32::update(void)
{
  int32_t n;

  // only update if we're playing
  if ((state == STATE_STOP) || (state == STATE_NOT_BEGUN)) return;

  // allocate the audio blocks to transmit
  block_left_f32 = AudioStream_F32::allocate_f32();
  if (block_left_f32 == NULL) return;
  if (state < 8 && (state & 1) == 1) {
    // if we're playing stereo, allocate another
    // block for the right channel output
    block_right_f32 = AudioStream_F32::allocate_f32();
    if (block_right_f32 == NULL) {
      AudioStream_F32::release(block_left_f32);
      return;
    }
  } else {
    // if we're playing mono or just parsing
    // the WAV file header, no right-side block
    block_right_f32 = NULL;
  }
  block_offset = 0;

  //Serial.println("update");

  // is there buffered data?
  n = buffer_length - buffer_offset;
  if (n > 0) {
    // we have buffered data
    if (consume(n)) return; // it was enough to transmit audio
  }

  // we only get to this point when buffer[512] is empty
  if (state != STATE_STOP && file.available()) {
    // we can read more data from the file...
    readagain:
    buffer_length = file.read(buffer, 512);  //read 512 bytes  (128 samples, 2 channels, 2 bytes/sample
    if (buffer_length == 0) goto end;
    buffer_offset = 0;
    bool parsing = (state >= 8);
    bool txok = consume(buffer_length);
    if (txok) {
      if (state != STATE_STOP) return;
    } else {
      if (state != STATE_STOP) {
        if (parsing && state < 8) goto readagain;
        else goto cleanup;
      }
    }
  }
end:  // end of file reached or other reason to stop
  file.close(); 
  state_play = STATE_STOP;
  state = STATE_STOP;
cleanup:
  if (block_left_f32) {
    if (block_offset > 0) {
      for (uint32_t i=block_offset; i < ((uint32_t)audio_block_samples); i++) {
        block_left_f32->data[i] = 0.0f;
      }
      update_counter++; //let's increment the counter here to ensure that we get every ISR resulting in audio
      block_left_f32->id = update_counter; //prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
      block_left_f32->length = audio_block_samples;
      AudioStream_F32::transmit(block_left_f32, 0);
      if (state < 8 && (state & 1) == 0) {
        AudioStream_F32::transmit(block_left_f32, 1);
      }
    }
    AudioStream_F32::release(block_left_f32);
    block_left_f32 = NULL;
  }
  if (block_right_f32) {
    if (block_offset > 0) {
      for (uint32_t i=block_offset; i < (uint32_t)audio_block_samples; i++) {
        block_right_f32->data[i] = 0.0f;
      }
      block_right_f32->id = update_counter; //prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
      block_right_f32->length = audio_block_samples;
      AudioStream_F32::transmit(block_right_f32, 1);
    }
    AudioStream_F32::release(block_right_f32);
    block_right_f32 = NULL;
  }
}


// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

// Consume already buffered data.  Returns true if audio transmitted.
bool AudioSDPlayer_F32::consume(uint32_t size)
{
  uint32_t len;
  uint8_t lsb, msb;
  const uint8_t *p;
  int16_t val_int16;

  p = buffer + buffer_offset;
start:
  if (size == 0) return false;
#if 0
  Serial.print("AudioSDPlayer_F32 consume, ");
  Serial.print("size = ");
  Serial.print(size);
  Serial.print(", buffer_offset = ");
  Serial.print(buffer_offset);
  Serial.print(", data_length = ");
  Serial.print(data_length);
  Serial.print(", space = ");
  Serial.print((audio_block_samples - block_offset) * 2);
  Serial.print(", state = ");
  Serial.println(state);
#endif
  switch (state) {
    // parse wav file header, is this really a .wav file?
    case STATE_PARSE1:
    len = data_length;
    if (size < len) len = size;
    memcpy((uint8_t *)header + header_offset, p, len);
    header_offset += len;
    buffer_offset += len;
    data_length -= len;
    if (data_length > 0) return false;
    // parse the header...
    if (header[0] == 0x46464952 && header[2] == 0x45564157) {
      //Serial.println("is wav file");
      if (header[3] == 0x20746D66) {
        // "fmt " header
        if (header[4] < 16) {
          // WAV "fmt " info must be at least 16 bytes
          break;
        }
        if (header[4] > sizeof(header)) {
          // if such .wav files exist, increasing the
          // size of header[] should accomodate them...
          //Serial.println("WAVEFORMATEXTENSIBLE too long");
          break;
        }
        //Serial.println("header ok");
        header_offset = 0;
        state = STATE_PARSE2;
      } else {
        // first chuck is something other than "fmt "
        //Serial.print("skipping \"");
        //Serial.printf("\" (%08X), ", __builtin_bswap32(header[3]));
        //Serial.print(header[4]);
        //Serial.println(" bytes");
        header_offset = 12;
        state = STATE_PARSE5;
      }
      p += len;
      size -= len;
      data_length = header[4];
      goto start;
    }
    //Serial.println("unknown WAV header");
    break;

    // check & extract key audio parameters
    case STATE_PARSE2:
    len = data_length;
    if (size < len) len = size;
    memcpy((uint8_t *)header + header_offset, p, len);
    header_offset += len;
    buffer_offset += len;
    data_length -= len;
    if (data_length > 0) return false;
    if (parse_format()) {
      //Serial.println("audio format ok");
      p += len;
      size -= len;
      data_length = 8;
      header_offset = 0;
      state = STATE_PARSE3;
      goto start;
    }
    //Serial.println("unknown audio format");
    break;

    // find the data chunk
    case STATE_PARSE3: // 10
    len = data_length;
    if (size < len) len = size;
    memcpy((uint8_t *)header + header_offset, p, len);
    header_offset += len;
    buffer_offset += len;
    data_length -= len;
    if (data_length > 0) return false;
    //Serial.print("chunk id = ");
    //Serial.print(header[0], HEX);
    //Serial.print(", length = ");
    //Serial.println(header[1]);
    p += len;
    size -= len;
    data_length = header[1];
    if (header[0] == 0x61746164) {
      //Serial.print("wav: found data chunk, len=");
      //Serial.println(data_length);
      // TODO: verify offset in file is an even number
      // as required by WAV format.  abort if odd.  Code
      // below will depend upon this and fail if not even.
      leftover_bytes = 0;
      state = state_play;
      if (state & 1) {
        // if we're going to start stereo
        // better allocate another output block
        block_right_f32 = AudioStream_F32::allocate_f32();
        if (!block_right_f32) return false;
      }
      total_length = data_length;
    } else {
      state = STATE_PARSE4;
    }
    goto start;

    // ignore any extra unknown chunks (title & artist info)
    case STATE_PARSE4: // 11
    if (size < data_length) {
      data_length -= size;
      buffer_offset += size;
      return false;
    }
    p += data_length;
    size -= data_length;
    buffer_offset += data_length;
    data_length = 8;
    header_offset = 0;
    state = STATE_PARSE3;
    //Serial.println("consumed unknown chunk");
    goto start;

    // skip past "junk" data before "fmt " header
    case STATE_PARSE5:
    len = data_length;
    if (size < len) len = size;
    buffer_offset += len;
    data_length -= len;
    if (data_length > 0) return false;
    p += len;
    size -= len;
    data_length = 8;
    state = STATE_PARSE1;
    goto start;

    // playing mono at native sample rate
    case STATE_DIRECT_8BIT_MONO:
    return false;

    // playing stereo at native sample rate
    case STATE_DIRECT_8BIT_STEREO:
    return false;

    // playing mono at native sample rate
    case STATE_DIRECT_16BIT_MONO:
    if (size > data_length) size = data_length;
    data_length -= size;
    while (1) {
      lsb = *p++;
      msb = *p++;
      size -= 2;
      val_int16 = (msb << 8) | lsb;
      block_left_f32->data[block_offset++] = ((float)val_int16)/(32767.0);  //convert from int16 to float32 spanning +/-1.0
      if (block_offset >= audio_block_samples) {
        update_counter++; block_left_f32->id = update_counter;
        block_left_f32->length = audio_block_samples;
        AudioStream_F32::transmit(block_left_f32, 0);
        AudioStream_F32::transmit(block_left_f32, 1);
        AudioStream_F32::release(block_left_f32);
        block_left_f32 = NULL;
        data_length += size;
        buffer_offset = p - buffer;
        if (block_right_f32) {
          block_right_f32->id = update_counter;
          block_right_f32->length = audio_block_samples;
          AudioStream_F32::release(block_right_f32);
        }
        if (data_length == 0) state = STATE_STOP;
        return true;
      }
      if (size == 0) {
        if (data_length == 0) break;
        return false;
      }
    }
    //Serial.println("end of file reached");
    // end of file reached
    if (block_offset > 0) {
      // TODO: fill remainder of last block with zero and transmit
    }
    state = STATE_STOP;
    return false;

    // playing stereo at native sample rate
    case STATE_DIRECT_16BIT_STEREO:
    if (size > data_length) size = data_length;
    data_length -= size;
    if (leftover_bytes) {
      //block_left_f32->data[block_offset] = header[0];
      val_int16 = (int16_t)header[0];
      block_left_f32->data[block_offset++] = ((float)val_int16)/(32767.0);  //convert from int16 to float32 spanning +/-1.0
      
//PAH fix problem with left+right channels being swapped
      leftover_bytes = 0;
      goto right16;
    }
    while (1) {
      lsb = *p++;
      msb = *p++;
      size -= 2;
      if (size == 0) {
        if (data_length == 0) break;
        header[0] = (msb << 8) | lsb;
        leftover_bytes = 2;
        return false;
      }
      //block_left_f32->data[block_offset] = (msb << 8) | lsb;
      val_int16 = (int16_t)((msb << 8) | lsb);
      block_left_f32->data[block_offset++] = ((float)val_int16)/(32767.0);  //convert from int16 to float32 spanning +/-1.0
      
      right16:
      lsb = *p++;
      msb = *p++;
      size -= 2;
      //block_right_f32->data[block_offset++] = (msb << 8) | lsb;
      val_int16 = (int16_t)((msb << 8) | lsb);
      block_right_f32->data[block_offset++] = ((float)val_int16)/(32767.0);  //convert from int16 to float32 spanning +/-1.0
      
      if (block_offset >= audio_block_samples) {
        update_counter++; block_left_f32->id = update_counter;
        block_left_f32->length = audio_block_samples;
        AudioStream_F32::transmit(block_left_f32, 0);
        AudioStream_F32::release(block_left_f32);
        block_left_f32 = NULL;
        block_right_f32->id = update_counter;
        block_right_f32->length = audio_block_samples;
        AudioStream_F32::transmit(block_right_f32, 1);
        AudioStream_F32::release(block_right_f32);
        block_right_f32 = NULL;
        data_length += size;
        buffer_offset = p - buffer;
        if (data_length == 0) state = STATE_STOP;
        return true;
      }
      if (size == 0) {
        if (data_length == 0) break;
        leftover_bytes = 0;
        return false;
      }
    }
    // end of file reached
    if (block_offset > 0) {
      // TODO: fill remainder of last block with zero and transmit
    }
    state = STATE_STOP;
    return false;

    // playing mono, converting sample rate
    case STATE_CONVERT_8BIT_MONO :
    return false;

    // playing stereo, converting sample rate
    case STATE_CONVERT_8BIT_STEREO:
    return false;

    // playing mono, converting sample rate
    case STATE_CONVERT_16BIT_MONO:
    return false;

    // playing stereo, converting sample rate
    case STATE_CONVERT_16BIT_STEREO:
    return false;

    // ignore any extra data after playing
    // or anything following any error
    case STATE_STOP:
    return false;

    // this is not supposed to happen!
    //default:
    //Serial.println("AudioSDPlayer_F32, unknown state");
  }
  state_play = STATE_STOP;
  state = STATE_STOP;
  return false;
}


/*
00000000  52494646 66EA6903 57415645 666D7420  RIFFf.i.WAVEfmt 
00000010  10000000 01000200 44AC0000 10B10200  ........D.......
00000020  04001000 4C495354 3A000000 494E464F  ....LIST:...INFO
00000030  494E414D 14000000 49205761 6E742054  INAM....I Want T
00000040  6F20436F 6D65204F 76657200 49415254  o Come Over.IART
00000050  12000000 4D656C69 73736120 45746865  ....Melissa Ethe
00000060  72696467 65006461 746100EA 69030100  ridge.data..i...
00000070  FEFF0300 FCFF0400 FDFF0200 0000FEFF  ................
00000080  0300FDFF 0200FFFF 00000100 FEFF0300  ................
00000090  FDFF0300 FDFF0200 FFFF0100 0000FFFF  ................
*/





// SD library on Teensy3 at 96 MHz
//  256 byte chunks, speed is 443272 bytes/sec
//  512 byte chunks, speed is 468023 bytes/sec

//define B2M_96000 (uint32_t)((double)4294967296000.0 / 96000.f) // 97352592
//define B2M_88200 (uint32_t)((double)4294967296000.0 / 88200.f)) // 97352592
//define B2M_44100 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT) // 97352592
//define B2M_22050 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT * 2.0)
//define B2M_11025 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT * 4.0)


bool AudioSDPlayer_F32::parse_format(void)
{
  uint8_t num = 0;
  uint16_t format;
  //uint16_t channels;
  uint32_t rate; //b2m;
  //uint16_t bits;

  format = header[0];
  //Serial.print("  format = ");
  //Serial.println(format);
  if (format != 1) return false;

  rate = header[1];
  //Serial.print("  rate = ");
  //Serial.println(rate);
  
  if (rate == 96000) {
    //b2m = B2M_96000;
  } else if (rate == 88200) {
    //b2m = B2M_88200;
  } else if (rate == 44100) {
    //b2m = B2M_44100;
  } else if (rate == 22050) {
    //b2m = B2M_22050;
    num |= 4;
  } else if (rate == 11025) {
    //b2m = B2M_11025;
    num |= 4;
  } else {
    return false;
  }

  channels = header[0] >> 16;
  //Serial.print("  channels = ");
  //Serial.println(channels);
  if (channels == 1) {
  } else if (channels == 2) {
    //b2m >>= 1;
    num |= 1;
  } else {
    return false;
  }

  bits = header[3] >> 16;
  //Serial.print("  bits = ");
  //Serial.println(bits);
  if (bits == 8) {
  } else if (bits == 16) {
    //b2m >>= 1;
    num |= 2;
  } else {
    return false;
  }

  //bytes2millis = b2m;
  //bytes2millis = computeBytes2Millis();
  updateBytes2Millis();
  
  //Serial.print("  bytes2millis = ");
  //Serial.println(b2m);

  // we're not checking the byte rate and block align fields
  // if they're not the expected values, all we could do is
  // return false.  Do any real wav files have unexpected
  // values in these other fields?
  state_play = num;
  return true;
}

uint32_t AudioSDPlayer_F32::updateBytes2Millis(void)
{
  double b2m;

  //account for sample rate
  b2m = ((double)4294967296000.0 / ((double)sample_rate_Hz));

  //account for channels
  b2m = b2m / ((double)channels);

  //account for bits per second
  if (bits == 16) {
    b2m = b2m / 2;
  } else if (bits == 24) {
    b2m = b2m / 3;  //can we handle 24 bits?  I don't think that we can.
  }

  return bytes2millis = ((uint32_t)b2m);
}

bool AudioSDPlayer_F32::isPlaying(void)
{
  uint8_t s = *(volatile uint8_t *)&state;
  return (s < 8);
}



uint32_t AudioSDPlayer_F32::positionMillis(void)
{
  uint8_t s = *(volatile uint8_t *)&state;
  if (s >= 8) return 0;
  uint32_t tlength = *(volatile uint32_t *)&total_length;
  uint32_t dlength = *(volatile uint32_t *)&data_length;
  uint32_t offset = tlength - dlength;
  uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
  return ((uint64_t)offset * b2m) >> 32;
}


uint32_t AudioSDPlayer_F32::lengthMillis(void)
{
  uint8_t s = *(volatile uint8_t *)&state;
  if (s >= 8) return 0;
  uint32_t tlength = *(volatile uint32_t *)&total_length;
  uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
  return ((uint64_t)tlength * b2m) >> 32;
}

