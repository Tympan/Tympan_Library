
/*
 * WavHeaderFmt.h
 * 
 * Created: Eric Yuan, OpenAudio, July 2025
 * 
 * Purpose: Provide WAV file header as a union comprised of a struct and byte stream.  

   MIT License.  Use at your own risk.
*/

#ifndef Wav_Header_Fmt_h
#define Wav_Header_Fmt_h

/* **************************** Include Files ******************************* */
#include <cstdint>
#include <map>
#include <string>

/* ******************************* Constexpr ********************************** */
constexpr uint32_t WAV_HEADER_NO_METADATA_NUM_BYTES = 44;	// # of bytes of WAV Header with no metadata and no audio data.

/* WAV HEADER KEYWORDS as hex */
constexpr uint32_t RIFF_LSB               = 0x46464952;  //"RIFF" as hex
constexpr uint32_t WAVE_LSB               = 0x45564157;  //"WAVE" as hex
constexpr uint32_t FMT_LSB                = 0x20746d66;  //"fmt " as hex

constexpr uint32_t DATA_LSB               = 0x61746164;  //"data" as hex

constexpr uint32_t LIST_LSB               = 0x5453494c;  //"LIST" as hex
constexpr uint32_t INFO_LSB               = 0x4F464E49;  //"INFO" as hex
constexpr uint32_t ICMT_LSB               = 0x544D4349;  //"ICMT" as hex

constexpr uint16_t WAVE_FORMAT_PCM        = 0x01;        //Microsoft Pulse Code Modulation (PCM) format


/* *******************************  Types  ********************************** */
/*Structures to capture header for
  -"RIFF" chunk
    - "fmt " subchunk contains audio format such as sample rate
    - "LIST" subchunk (if metadata is used)
      - "INFO" sub-subchunk
    - "data" subchunk contains audio data
*/

// RIFF chunk uses union to type pun bytestream.
// Note: ensure that structures are 32-bit aligned
union Riff_Header_u {
  struct {
    uint32_t chunkId;             // "RIFF" as LSB
    uint32_t chunkLenBytes;       // File length (in bytes) - 8bytes
    uint32_t format;              // "WAVE" as LSB
  } Riff_s;      
  uint8_t byteStream[sizeof(Riff_s)]; // represented as byte array
  
  // Initializer
  Riff_Header_u() {
    Riff_s.chunkId = RIFF_LSB;     // "RIFF" as LSB
    Riff_s.chunkLenBytes = 0;      // File length (in bytes) - 8bytes
    Riff_s.format = WAVE_LSB;      // "WAVE" as LSB
  }
};

// FMT chunk uses union to type pun bytestream.
// Note: ensure that structures are 32-bit aligned
union Fmt_Header_u {
  struct {
    uint32_t chunkId;             // "fmt " as LSB
    uint32_t chunkLenBytes;       // Expect 16 bytes (this chunk - 4 bytes)
    uint16_t audioFmt;            // 1: PCM, otherwise is compressed
    uint16_t numChan;             // # of audio channels
    uint32_t sampleRate_Hz;       // bits per second
    uint32_t byteRate;            // SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign;          // NumChannels * BitsPerSample/8
    uint16_t bitsPerSample;       // 8: 8-bit;  16: 16-bit
  } Fmt_s;
  uint8_t byteStream[sizeof(Fmt_s)] = {0};   // represented as byte array
  
  // Initializer
  Fmt_Header_u() {
    Fmt_s.chunkId         = FMT_LSB;
    Fmt_s.chunkLenBytes   = sizeof(Fmt_Header_u) - 8;   // File length (in bytes) - 8bytes
    Fmt_s.audioFmt        = WAVE_FORMAT_PCM;            // 1: PCM
    Fmt_s.numChan         = 0;
    Fmt_s.sampleRate_Hz   = 0;
    Fmt_s.byteRate        = 0;
    Fmt_s.blockAlign      = 0;
    Fmt_s.bitsPerSample   = 0;
    }
};

// data chunk uses union to type pun bytestream.  This does not include the audio data contained in this chunk
// Note: ensure that structures are 32-bit aligned
union Data_Header_u {
  struct {
    uint32_t chunkId;                             // "data" as LSB
    uint32_t chunkLenBytes;                       // Number of audio bytes: NumSamples * NumChannels * BitsPerSample/8
  } Data_s;      
  uint8_t byteStream[sizeof(Data_s)]  = {0};      // represented as byte array

  // Initializer
  Data_Header_u() {
    Data_s.chunkId                    = DATA_LSB;
    Data_s.chunkLenBytes              = 0;
  }
};

// List chunk uses union to type pun bytestream.  This does not contain the InfoKeyVal_t data contained in this chunk.
// Note: ensure that structures are 32-bit aligned
union List_Header_u {
  struct {
    uint32_t chunkId;                               // "LIST" as LSB
    uint32_t chunkLenBytes;                         // Length of chunk (including InfoKeyVal_t data) - 8bytes
    uint32_t subChunkId;                            // "INFO" as LSB
    uint32_t subChunkLenBytes;                      // Length of subChunk (including InfoKeyVal_t) - 8bytes    
  } List_s;      
  uint8_t byteStream[sizeof(List_s)] = {0};         // represented as byte array

  // Initializer
  List_Header_u() {
    List_s.chunkId                  = LIST_LSB;
    List_s.chunkLenBytes            = 0;
    List_s.subChunkId               = INFO_LSB;
    List_s.subChunkLenBytes         = 0;
  }
};

// INFO tags
enum class InfoTags {
  ICMT, // Comments. Provides general comments about the file or the subject of the file. 
  IARL, // Archival Location. Indicates where the subject of the file is archived.
  IART, // Artist. Lists the artist of the original subject of the file. For example, “Michaelangelo.”
  ICMS, // Commissioned. Lists the name of the person or organization that commissioned the subject of the file. 
  IDPI, // Dots Per Inch. Stores dots per inch setting of the digitizer used to produce the file, such as “ 300.”
  IENG, // Engineer. Stores the name of the engineer who worked on the file. Separate the names by a semicolon and a blank. 
  IKEY, // Keywords. Provides a list of keywords that refer to the file or subject of the file. Separate multiple keywords with a semicolon
  ILGT, // Lightness. Describes the changes in lightness settings on the digitizer required to produce the file. Note that the format of this information depends on hardware used.
  IMED, // Medium. Describes the original subject of the file, such as, “ computer image,” “ drawing,” “ lithograph,” and so forth.
  INAM, // Name. Stores the title of the subject of the file, such as, “ Seattle From Above.”
  IPLT, // Palette Setting. Specifies the number of colors requested when digitizing an image, such as “ 256.”
  IPRD, // Product. Specifies the name of the title the file was originally intended for, such as “Encyclopedia of Pacific Northwest Geography.”
  ISBJ, // Subject. Describes the conbittents of the file, such as “Aerial view of Seattle.”
  ISFT, // Software. Identifies the name of the software package used to create the file, such as “Microsoft WaveEdit.”
  ISHP, // Sharpness. Identifies the changes in sharpness for the digitizer required to produce the file (the format depends on the hardware used).
  ISRC, // Source. Identifies the name of the person or organization who supplied the original subject of the file. For example, “ Trey Research.”
  ISRF, //Source Form. Identifies the original form of the material that was digitized, such as “ slide,” “ paper,” “map,” and so forth. This is not necessarily the same as IMED.
  ITCH // Technician. Identifies the technician who digitized the subject file. For example, “ Smith, John.”
};

using InfoKeyVal_t = std::map<InfoTags, std::string>;

// Access WAV file metadata tagname for the INFO chunk
constexpr std::string_view InfoTagToStr(InfoTags tagName) {
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
#endif //Wav_Header_Fmt_h