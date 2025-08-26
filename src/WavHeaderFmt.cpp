/*
 * WavHeaderFmt.h
 * 
 * Created: Eric Yuan, OpenAudio, July 2025
 * 
 * Purpose: Provide WAV file header as a union comprised of a struct and byte stream.  

   MIT License.  Use at your own risk.
*/

#include "WavHeaderFmt.h"
#include <algorithm> // Required for std::find

/**
 * @brief Converts tagId from string to enum Info_Tags
 * \note Also update InfoTagToStr() and enum Info_Tags
 * @param tagStr Info tag
 * @return enum InfoTags;  if ERRO then tag not found
 */
Info_Tags StrToTagId(std::string_view tagStr) {
    if (tagStr == "ICMT")      {return Info_Tags::ICMT;}  // Comments. Provides general comments about the file or the subject of the file. 
    else if (tagStr == "IARL") {return Info_Tags::IARL;}  // Archival Location. Indicates where the subject of the file is archived.
    else if (tagStr == "IART") {return Info_Tags::IART;}  // Artist. Lists the artist of the original subject of the file. For example,Info_Tags:: : “Michaelangelo.”
    else if (tagStr == "ICMS") {return Info_Tags::ICMS;}  // Commissioned. Lists the name of the person or organization that commissioned the subject of the file. 
    else if (tagStr == "ICOP") {return Info_Tags::ICOP;}  // // Copyright information about the file (e.g., "Copyright Some Company 2011")
    else if (tagStr == "ICRD") {return Info_Tags::ICRD;}  // The date the subject of the file was created (creation date) (e.g., "2022-12-31")
    else if (tagStr == "ICRP") {return Info_Tags::ICRP;}  // Copyright information about the file (e.g., "Copyright Some Company 2011")
    else if (tagStr == "IDIM") {return Info_Tags::IDIM;}  // The dimensions of the original subject of the file
    else if (tagStr == "IDPI") {return Info_Tags::IDPI;}  // Dots Per Inch. Stores dots per inch setting of the digitizer used to produce the file,Info_Tags:: : such as “ 300.”
    else if (tagStr == "IENG") {return Info_Tags::IENG;}  // Engineer. Stores the name of the engineer who worked on the file. Separate the names by a semicolon and a blank. 
    else if (tagStr == "IGNR") {return Info_Tags::IGNR;}  // The genre of the subject
    else if (tagStr == "IKEY") {return Info_Tags::IKEY;}  // Keywords. Provides a list of keywords that refer to the file or subject of the file. Separate multiple keywords with a semicolon
    else if (tagStr == "ILGT") {return Info_Tags::ILGT;}  // Lightness. Describes the changes in lightness settings on the digitizer required to produce the file. Note that the format of this information depends on hardware used.
    else if (tagStr == "IMED") {return Info_Tags::IMED;}  // Medium. Describes the original subject of the file, such as, “ computer image,” “ drawing,” “ lithograph,” and so forth.
    else if (tagStr == "INAM") {return Info_Tags::INAM;}  // Name. Stores the title of the subject of the file, such as, “ Seattle From Above.”
    else if (tagStr == "IPLT") {return Info_Tags::IPLT;}  // Palette Setting. Specifies the number of colors requested when digitizing an image, such as “ 256.”
    else if (tagStr == "IPRD") {return Info_Tags::IPRD;}  // Product. Specifies the name of the title the file was originally intended for, such as “Encyclopedia of Pacific Northwest Geography.”
    else if (tagStr == "ISBJ") {return Info_Tags::ISBJ;}  // Subject. Describes the conbittents of the file, such as “Aerial view of Seattle.”
    else if (tagStr == "ISFT") {return Info_Tags::ISFT;}  // Software. Identifies the name of the software package used to create the file, such as “Microsoft WaveEdit.”
    else if (tagStr == "ISHP") {return Info_Tags::ISHP;}  // Sharpness. Identifies the changes in sharpness for the digitizer required to produce the file (the format depends on the hardware used).
    else if (tagStr == "ISRC") {return Info_Tags::ISRC;}  // Source. Identifies the name of the person or organization who supplied the original subject of the file. For example, “ Trey Research.”
    else if (tagStr == "ISRF") {return Info_Tags::ISRF;}  //Source Form. Identifies the original form of the material that was digitized, such as “ slide,” “ paper,” “map,” and so forth. This is not necessarily the same as IMED.
    else if (tagStr == "ITCH") {return Info_Tags::ITCH;}  // Technician. Identifies the technician who digitized the subject file. For example, “ 
    else {
      return Info_Tags::ERRO;
    }
  };



/**
 * @brief Converts tagId from enum tagID to char array
 * \note Also update StrToTagId() and enum Info_Tags
 * @param tagName 
 * @return constexpr std::string_view 
 */
std::string_view InfoTagToStr(Info_Tags tagName) {
    switch (tagName) {
        case Info_Tags::ICMT: return "ICMT"; // Comments. Provides general comments about the file or the subject of the file. 
        case Info_Tags::IARL: return "IARL"; // Archival Location. Indicates where the subject of the file is archived.
        case Info_Tags::IART: return "IART"; // Artist. Lists the artist of the original subject of the file. For example, “Michaelangelo.”
        case Info_Tags::ICMS: return "ICMS"; // Commissioned. Lists the name of the person or organization that commissioned the subject of the file. 
        case Info_Tags::ICOP: return "ICOP"; // Copyright information about the file (e.g., "Copyright Some Company 2011")
        case Info_Tags::ICRD: return "ICRD"; // The date the subject of the file was created (creation date) (e.g., "2022-12-31")
        case Info_Tags::ICRP: return "ICRP"; // Whether and how an image was cropped
        case Info_Tags::IDIM: return "IDIM"; // The dimensions of the original subject of the file
        case Info_Tags::IDPI: return "IDPI"; // Dots Per Inch. Stores dots per inch setting of the digitizer used to produce the file, such as “ 300.”
        case Info_Tags::IENG: return "IENG"; // Engineer. Stores the name of the engineer who worked on the file. Separate the names by a semicolon and a blank. 
        case Info_Tags::IGNR: return "IGNR"; // The genre of the subject
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

/**
 * @brief Is chunk ID valid?
 * 
 * @param chunkIdU32 chunk ID as U32 (LSB)
 * @return true: valid
 * @return false: invalid
 */
bool isValidChunkId(const uint32_t &chunkIdU32) {
    // If find does not return the end of the array, then success.
    return (std::find(acceptedChunkIds.begin(), acceptedChunkIds.end(), chunkIdU32) != acceptedChunkIds.end());
}