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
constexpr uint32_t WAV_HEADER_NUM_BYTES_INT16 = 44;	// # of bytes of WAV Header with no metadata and no audio data.

/* WAV HEADER KEYWORDS as hex */
constexpr uint32_t RIFF_LSB               = 0x46464952;  //"RIFF" as hex
constexpr uint32_t WAVE_LSB               = 0x45564157;  //"WAVE" as hex
constexpr uint32_t FMT_LSB                = 0x20746d66;  //"fmt " as hex
constexpr uint32_t FACT_LSB               = 0x74636166;  //"fact" as hex
constexpr uint32_t DATA_LSB               = 0x61746164;  //"data" as hex

constexpr uint32_t LIST_LSB               = 0x5453494c;  //"LIST" as hex
constexpr uint32_t INFO_LSB               = 0x4F464E49;  //"INFO" as hex
constexpr uint32_t ICMT_LSB               = 0x544D4349;  //"ICMT" as hex

/* *******************************  Types  ********************************** */

/**
 * @brief Bit Rate
 */
enum Wave_Bitrate_e : uint16_t {
  Wav_Bitrate_16                           = 16,
  Wav_Bitrate_32                           = 32
};

/** 
 * @brief WAV Audio format: Currently handles PCM and IEEE
 */
enum Wave_Format_e : uint16_t {
  Wav_Format_Uninitialized                = 0x0000,       // Default condition that serves as placholder.  Not a valid format. 
  Wav_Format_Pcm                          = 0x0001,       // Pulse Code Modulation (PCM) format
  Wav_Format_A_Law                        = 0x0006,       // \note not compatible!!!
  Wav_Format_Mu_Law                       = 0x0007,       // \note not compatible!!!
  Wav_Format_Ieee_Float                   = 0x0003,       // IEEE Float format
  Wav_Format_Extensible                   = 0xFFFE        // Extensible format (\note not compatible!!!)
};


/*Structures to capture header for
  -"RIFF" chunk
    - "fmt " subchunk contains audio format such as sample rate (format depends on data type)
    - "fact" (if IEEE Float is used)
    - "LIST" subchunk (if metadata is used)
      - "INFO" sub-subchunk
    - "data" subchunk contains audio data
*/

// RIFF chunk
// Note: ensure that structures are 32-bit aligned
typedef struct __attribute__ ((packed)) {
    uint32_t chunkId;             // "RIFF" as LSB
    uint32_t chunkLenBytes;       // File length (in bytes) - 8bytes
    uint32_t format;              // "WAVE" as LSB
} Riff_s;

union Riff_Header_u {
  Riff_s S;  
  uint8_t byteStream[sizeof(Riff_s)]; // represented as byte array
  
  // Initializer
  Riff_Header_u() {
    S.chunkId = RIFF_LSB;     // "RIFF" as LSB
    S.chunkLenBytes = 0;      // File length (in bytes) - 8bytes
    S.format = WAVE_LSB;      // "WAVE" as LSB
  }
};


// RIFF's FMT subchunk for **PCM** data type (audioFmt==1) 
// Note: ensure that structures are 32-bit aligned
typedef struct __attribute__((packed)) {
  uint32_t chunkId;             // "fmt " as LSB
  uint32_t chunkLenBytes;       // Expect 16 bytes (this chunk - 4 bytes)
  uint16_t audioFmt;            // 1: PCM, otherwise is compressed
  uint16_t numChan;             // # of audio channels
  uint32_t sampleRate_Hz;       // bits per second
  uint32_t byteRate;            // SampleRate * NumChannels * BitsPerSample/8
  uint16_t blockAlign;          // NumChannels * BitsPerSample/8
  uint16_t bitsPerSample;       // 8: 8-bit;  16: 16-bit
} Fmt_Pcm_s;

union Fmt_Pcm_Header_u {
  Fmt_Pcm_s S;
  uint8_t byteStream[sizeof(Fmt_Pcm_s)];   // represented as byte array
  
  // Initializer
  Fmt_Pcm_Header_u() {
    S.chunkId         = FMT_LSB;
    S.chunkLenBytes   = sizeof(Fmt_Pcm_Header_u) - 8;   // File length (in bytes) - 8bytes
    S.audioFmt        = Wave_Format_e::Wav_Format_Pcm; // 0x01: PCM
    S.numChan         = 0; // To be updated
    S.sampleRate_Hz   = 0; // To be updated
    S.byteRate        = 0; // To be updated
    S.blockAlign      = 0; // To be updated
    S.bitsPerSample   = 0; // To be updated
    }
};

// RIFF"s FMT subchunk for IEEE Float data type (audioFmt==3)  Uses union to type pun bytestream.
// Note: ensure that structures are 32-bit aligned
typedef struct __attribute__((packed)) {
    uint32_t chunkId;             // "fmt " as LSB
    uint32_t chunkLenBytes;       // Expect 16 bytes (this chunk - 4 bytes)
    uint16_t audioFmt;            // 1: PCM, otherwise is compressed
    uint16_t numChan;             // # of audio channels
    uint32_t sampleRate_Hz;       // bits per second
    uint32_t byteRate;            // SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign;          // NumChannels * BitsPerSample/8
    uint16_t bitsPerSample;       // 8: 8-bit;  16: 16-bit
    uint16_t wExtSize;            // Size of the extension, which for this IEEE format, is 0 bytes.
  } Fmt_Ieee_s;

union Fmt_Ieee_Header_u {
  Fmt_Ieee_s S;
  uint8_t byteStream[sizeof(Fmt_Ieee_s)];   // represented as byte array
  
  // Initializer
  Fmt_Ieee_Header_u() {
    S.chunkId         = FMT_LSB;
    S.chunkLenBytes   = sizeof(Fmt_Ieee_Header_u) - 8;        // File length (in bytes) - 8bytes
    S.audioFmt        = Wave_Format_e::Wav_Format_Ieee_Float; // 0x03 IEEE Float
    S.numChan         = 0; // To be updated
    S.sampleRate_Hz   = 0; // To be updated
    S.byteRate        = 0; // To be updated
    S.blockAlign      = 0; // To be updated
    S.bitsPerSample   = 32;
    S.wExtSize        = 0x00;                                 // Size of extension, which is 0 bytes for this IEEE format
    }
};


// fact chunk uses union to type pun bytestream.  Only used if not PCM data type
// Note: ensure that structures are 32-bit aligned
typedef struct __attribute__((packed)) {
    uint32_t chunkId;                             // "fact" as LSB
    uint32_t chunkLenBytes;                       // length of fact chunk
    uint32_t numTotalSamp;                        // # of samples x # of channels
  } Fact_s;      

union Fact_Header_u {
  Fact_s S;
  uint8_t byteStream[sizeof(Fact_s)];      // represented as byte array

  // Initializer
  Fact_Header_u() {
    S.chunkId                    = FACT_LSB;
    S.chunkLenBytes              = sizeof(Fact_Header_u)-8;        
    S.numTotalSamp               = 0;        // To be updated after audio data written
  }
};


// "data" chunk.  This does not include the audio data contained in this chunk
// Note: ensure that structures are 32-bit aligned
typedef struct __attribute__((packed)) {
  uint32_t chunkId;                             // "data" as LSB
  uint32_t chunkLenBytes;                       // Number of audio bytes: NumSamples * NumChannels * BitsPerSample/8
} Data_s;      

union Data_Header_u {
  Data_s S;
  uint8_t byteStream[sizeof(Data_s)];      // represented as byte array

  // Initializer
  Data_Header_u() {
    S.chunkId                    = DATA_LSB;
    S.chunkLenBytes              = 0;
  }
};

/**
 * @brief List chunk uses union to type pun bytestream.  This does not contain the InfoKeyVal_t data contained in this chunk.
 * \note Ensure that structures are 32-bit aligned
 */
typedef struct __attribute__((packed)) {
  uint32_t chunkId;                               // "LIST" as LSB
  uint32_t chunkLenBytes;                         // Length of chunk after this value (i.e. entire chunk - 8bytes)
  uint32_t subChunkId;                            // "INFO" as LSB
} List_s;      

union List_Header_u {
  List_s S;
  uint8_t byteStream[sizeof(List_s)];         // represented as byte array

  // Initializer
  List_Header_u() {
    S.chunkId                  = LIST_LSB;
    S.chunkLenBytes            = 0;
    S.subChunkId               = INFO_LSB;
  }
};


/**
 * @brief For storing the tagID and tagLen, not that following tag string
 * 
 */
typedef struct __attribute__((packed)) {
  uint32_t tagId;                               // "LIST" as LSB
  uint32_t tagLenBytes;                         // Length of chunk after this value (i.e. entire chunk - 8bytes)
} List_Tag_s;      


union List_Tag_Header_u {
  List_Tag_s S;
  uint8_t byteStream[sizeof(List_Tag_s)];         // represented as byte array

  // Initialize to 0 as we can't prepopulate
  List_Tag_Header_u() {
    S.tagId                = 0;
    S.tagLenBytes          = 0;
  }
};


// INFO tags
enum class Info_Tags {
  ERRO, // ERROR! no tag found
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
  ITCH, // Technician. Identifies the technician who digitized the subject file. For example, “ Smith, John.”
};

using InfoKeyVal_t = std::map<Info_Tags, std::string>;

/* ******************************* Globals ********************************** */
//extern std::map<std::string, Info_Tags> StrToTagId; 
//std::string_view InfoTagToStr(Info_Tags tagName);

/* ********************************* External Functions ***********************/
//Info_Tags StrToTagId(std::string);


constexpr Info_Tags StrToTagId(std::string_view) {
    if ("ICMT") return Info_Tags::ICMT;       // Comments. Provides general comments about the file or the subject of the file. 
    else if ("IARL") return Info_Tags::IARL;  // Archival Location. Indicates where the subject of the file is archived.
    else if ("IART") return Info_Tags::IART;  // Artist. Lists the artist of the original subject of the file. For example,Info_Tags:: : “Michaelangelo.”
    else if ("ICMS") return Info_Tags::ICMS;  // Commissioned. Lists the name of the person or organization that commissioned the subject of the file. 
    else if ("IDPI") return Info_Tags::IDPI;  // Dots Per Inch. Stores dots per inch setting of the digitizer used to produce the file,Info_Tags:: : such as “ 300.”
    else if ("IENG") return Info_Tags::IENG;  // Engineer. Stores the name of the engineer who worked on the file. Separate the names by a semicolon and a blank. 
    else if ("IKEY") return Info_Tags::IKEY;  // Keywords. Provides a list of keywords that refer to the file or subject of the file. Separate multiple keywords with a semicolon
    else if ("ILGT") return Info_Tags::ILGT;  // Lightness. Describes the changes in lightness settings on the digitizer required to produce the file. Note that the format of this information depends on hardware used.
    else if ("IMED") return Info_Tags::IMED;  // Medium. Describes the original subject of the file, such as, “ computer image,” “ drawing,” “ lithograph,” and so forth.
    else if ("INAM") return Info_Tags::INAM;  // Name. Stores the title of the subject of the file, such as, “ Seattle From Above.”
    else if ("IPLT") return Info_Tags::IPLT;  // Palette Setting. Specifies the number of colors requested when digitizing an image, such as “ 256.”
    else if ("IPRD") return Info_Tags::IPRD;  // Product. Specifies the name of the title the file was originally intended for, such as “Encyclopedia of Pacific Northwest Geography.”
    else if ("ISBJ") return Info_Tags::ISBJ;  // Subject. Describes the conbittents of the file, such as “Aerial view of Seattle.”
    else if ("ISFT") return Info_Tags::ISFT;  // Software. Identifies the name of the software package used to create the file, such as “Microsoft WaveEdit.”
    else if ("ISHP") return Info_Tags::ISHP;  // Sharpness. Identifies the changes in sharpness for the digitizer required to produce the file (the format depends on the hardware used).
    else if ("ISRC") return Info_Tags::ISRC;  // Source. Identifies the name of the person or organization who supplied the original subject of the file. For example, “ Trey Research.”
    else if ("ISRF") return Info_Tags::ISRF;  //Source Form. Identifies the original form of the material that was digitized, such as “ slide,” “ paper,” “map,” and so forth. This is not necessarily the same as IMED.
    else if ("ITCH") return Info_Tags::ITCH;  // Technician. Identifies the technician who digitized the subject file. For example, “ 
    else return Info_Tags::ERRO;
  };


/**
 * @brief Converts LIST<INFO> enum tagID to char array
 * @param tagName 
 * @return constexpr std::string_view 
 */
constexpr std::string_view InfoTagToStr(Info_Tags tagName) {
    switch (tagName) {
        case Info_Tags::ICMT: return "ICMT"; // Comments. Provides general comments about the file or the subject of the file. 
        case Info_Tags::IARL: return "IARL"; // Archival Location. Indicates where the subject of the file is archived.
        case Info_Tags::IART: return "IART"; // Artist. Lists the artist of the original subject of the file. For example, “Michaelangelo.”
        case Info_Tags::ICMS: return "ICMS"; // Commissioned. Lists the name of the person or organization that commissioned the subject of the file. 
        case Info_Tags::IDPI: return "IDPI"; // Dots Per Inch. Stores dots per inch setting of the digitizer used to produce the file, such as “ 300.”
        case Info_Tags::IENG: return "IENG"; // Engineer. Stores the name of the engineer who worked on the file. Separate the names by a semicolon and a blank. 
        case Info_Tags::IKEY: return "IKEY"; // Keywords. Provides a list of keywords that refer to the file or subject of the file. Separate multiple keywords with a semicolon
        case Info_Tags::ILGT: return "ILGT"; // Lightness. Describes the changes in lightness settings on the digitizer required to produce the file. Note that the format of this information depends on hardware used.
        case Info_Tags::IMED: return "IMED"; // Medium. Describes the original subject of the file, such as, “ computer image,” “ drawing,” “ lithograph,” and so forth.
        case Info_Tags::INAM: return "INAM"; // Name. Stores the title of the subject of the file, such as, “ Seattle From Above.”
        case Info_Tags::IPLT: return "IPLT"; // Palette Setting. Specifies the number of colors requested when digitizing an image, such as “ 256.”
        case Info_Tags::IPRD: return "IPRD"; // Product. Specifies the name of the title the file was originally intended for, such as “Encyclopedia of Pacific Northwest Geography.”
        case Info_Tags::ISBJ: return "ISBJ"; // Subject. Describes the conbittents of the file, such as “Aerial view of Seattle.”
        case Info_Tags::ISFT: return "ISFT"; // Software. Identifies the name of the software package used to create the file, such as “Microsoft WaveEdit.”
        case Info_Tags::ISHP: return "ISHP"; // Sharpness. Identifies the changes in sharpness for the digitizer required to produce the file (the format depends on the hardware used).
        case Info_Tags::ISRC: return "ISRC"; // Source. Identifies the name of the person or organization who supplied the original subject of the file. For example, “ Trey Research.”
        case Info_Tags::ISRF: return "ISRF"; //Source Form. Identifies the original form of the material that was digitized, such as “ slide,” “ paper,” “map,” and so forth. This is not necessarily the same as IMED.
        case Info_Tags::ITCH: return "ITCH"; // Technician. Identifies the technician who digitized the subject file. For example, “ Smith, John.”
        default: return ("ERRO");
	}
};

#endif //Wav_Header_Fmt_h