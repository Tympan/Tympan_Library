
// One can use "MTP" to access the BatTact SD card via USB from your PC.
//
// To do this, you need:
//   * Arduino IDE 1.8.15 or later
//   * Teensyduino 1.58 or later
//   * MTP_Teensy library from https://github.com/KurtE/MTP_Teensy
//   * In the Arduino IDE, under the "Tools" menu, choose "USB Type" and then "Serial + MTP Disk (Experimental)"

uint32_t MTP_store_num = 0; //state variable used by code below.  Don't change this.
bool is_MTP_setup = false;  //state variable used by code below.  Don't change this.
bool use_MTP = false;       //state variable used by code below.  Don't change this.

#if defined(USE_MTPDISK) || defined(USB_MTPDISK_SERIAL)  //detect whether "MTP Disk" or "Serial + MTP Disk" were selected in the Arduino IDEA
 
  #include <SD.h>
  #include <MTP_Teensy.h>  //from https://github.com/KurtE/MTP_Teensy, use Teensyduino 1.58 or later
  
  void start_MTP(void) {
    use_MTP = true;
  }
  
  void setup_MTP(void) {
    //audioSDWriter.end();  //stops any recordings (hopefully) and closes SD card object (hopefully)
    MTP.begin();  //required
    SD.begin(BUILTIN_SDCARD);  // Start SD Card (hopefully this fails gracefully if it is already started by by audio code)
    MTP_store_num = MTP.addFilesystem(SD, "SD Card"); // add the file system (hopefully this fails gracefully if it is already started)
    is_MTP_setup=true;
  }
  
  void service_MTP(void) {
    // has the MTP already been setup?
    if (is_MTP_setup == false) { setup_MTP(); }

    // now we can service the MTP
    MTP.loop();
  }

  //Feb 7, 2024: Stopping the MTP doesn't really seem to work.  Sorry
  void stop_MTP(void) {
    //SD.stop();
    if (is_MTP_setup) {
      (MTP.storage())->removeFilesystem(MTP_store_num); 
      is_MTP_setup = false;
    }
    use_MTP = false;
  }
   
#else
  //here are place-holder definitions for when "MTP Disk" isn't selected in the IDE's "USB Type" menu
  void start_MTP(void) {};
  void setup_MTP(void) {
    Serial.println("setup_MTP: *** ERROR ***: ");
    Serial.println("   : When compiling, you must set Tools -> USB Type -> Serial + MTP Disk (Experimental)");
  }
  void service_MTP(void) {};
  void stop_MTP(void) { };

#endif
