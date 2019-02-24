
//Rev C uses RN42:  https://cdn.sparkfun.com/datasheets/Wireless/Bluetooth/bluetooth_cr_UG-v1.0r.pdf

void renameBT_RevC(void) {

  //go into command mode
  USB_Serial->println("*** Switching Tympan RevC BT module to command mode...");
  audioHardware.forceBTtoDataMode(false);   delay(400); 
  BT_Serial->print("$$$");delay(200);echoIncomingBTSerial();  //should respond "CMD"
  USB_Serial->println("*** If successful, should see 'CMD'.");

  //get the BT address
  USB_Serial->println("*** Getting the device's BT address...");
  BT_Serial->println("GB");delay(1000); readBTResponse();
  int ind = response.indexOf('\n'); if (ind > 0) response.remove(ind); //strip off trailing carraige returns, newlines, and whatnot
  USB_Serial->print("*** Response = "); USB_Serial->println(response); echoIncomingBTSerial();

  // build full new name
  BT_hex = response.substring(8);
  new_BT_name.concat("-"); new_BT_name.concat(BT_hex);

  //change the BT name
  USB_Serial->print("*** Setting BT name to: "); USB_Serial->println(new_BT_name);
  BT_Serial->print("SN,");BT_Serial->println(new_BT_name); delay(200);echoIncomingBTSerial();  //should response "AOK"
  BT_Serial->println("R,1");delay(200);echoIncomingBTSerial();  //should respond "Reboot!"
  USB_Serial->println("*** If you saw 'AOK', and 'Reboot!', the renaming was successful.");
  
  USB_Serial->println("*** Bluetooth renaming complete.");
}

