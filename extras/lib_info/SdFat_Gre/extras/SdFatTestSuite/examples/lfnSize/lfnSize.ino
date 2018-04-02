// Program to compare size of SdFat_Gre with the SD.h library.
#include <SPI.h>
// Select the test library by commenting out one of the following two lines.
// #include <SD.h>
#include <SdFat_Gre.h>

// SD chip select pin.
const uint8_t SD_CS_PIN = SS;

#ifdef __SD_H__
File_Gre file;
#else  // __SD_H__
SdFat_Gre SD;
SdFile_Gre file;
#endif // __SD_H__

void setup() {
  Serial.begin(9600);
  while (!Serial) {}  // wait for Leonardo

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("begin failed");
    return;
  }
  #ifdef __SD_H__
  file = SD.open("SFN_file.txt", FILE_WRITE);  
  #else  // __SD_H__
  file.open("LFN_file.txt", O_RDWR | O_CREAT);
  #endif  // __SD_H__
  
  file.println("Hello");
  file.close();
  Serial.println("Done");
}
//------------------------------------------------------------------------------
void loop() {}