
//Rev D uses BC127: https://learn.sparkfun.com/tutorials/understanding-the-bc127-bluetooth-module/bc127-basics

void renameBT_RevD(void) {
  
  // go into command mode
  USB_Serial->println("*** Switching Tympan RevD BT module into command mode...");
  myTympan.forceBTtoDataMode(false);   delay(500); echoIncomingBTSerial(); //stop forcing the device to be in data mode
  BT_Serial->print("$");  delay(400);
  BT_Serial->print("$$$");  delay(400);
  //BT_Serial->print('\r'); delay(400);
  delay(2000);
  echoIncomingBTSerial();
  USB_Serial->println("*** Should be in command mode.");

  // clear the buffer by forcing an error
  USB_Serial->println("*** Clearing buffers.  Sending carraige return.");
  BT_Serial->print('\r'); delay(500); echoIncomingBTSerial();
  USB_Serial->println("*** Should have gotten 'ERROR', which is fine here.");

  // restore factory defaults
  USB_Serial->println("*** Restoring factory defaults...");
  BT_Serial->print("RESTORE"); BT_Serial->print('\r'); delay(500);echoIncomingBTSerial();
  BT_Serial->print("WRITE"); BT_Serial->print('\r'); delay(500);echoIncomingBTSerial();
  BT_Serial->print("RESET"); BT_Serial->print('\r'); delay(1500);echoIncomingBTSerial();
  USB_Serial->println("*** Reset should be complete.  Did it give two 'OK's, an ID blurb, and then 'READY'?");

  // get Bluetooth name (to get the hex string at the end
  USB_Serial->print("*** Begin renaming process.  Getting BT name...");
  BT_Serial->print("GET NAME"); BT_Serial->print('\r'); delay(500);  readBTResponse(); //lives in String response
  int ind = response.indexOf('\n'); if (ind > 0) response.remove(ind); //strip off trailing carraige returns, newlines, and whatnot
  USB_Serial->print("*** Response = "); USB_Serial->println(response); echoIncomingBTSerial();
  
  // process to get new Bluetooth name
  USB_Serial->println("*** Processing name info..."); 
  given_BT_name = response.substring(5); //strip off "NAME="
  BT_hex = given_BT_name.substring(13);
  new_BT_name.concat("-"); new_BT_name.concat(BT_hex);
  USB_Serial->print("*** Desired New BT Name = "); USB_Serial->println(new_BT_name);
  
  // set the new BT name
  USB_Serial->print("*** Setting New BT Name..."); USB_Serial->println(new_BT_name);
  BT_Serial->print("SET NAME="); BT_Serial->print(new_BT_name); BT_Serial->print('\r'); delay(1500); echoIncomingBTSerial();
  BT_Serial->print("WRITE"); BT_Serial->print('\r'); delay(500); echoIncomingBTSerial();
  BT_Serial->print("GET NAME"); BT_Serial->print('\r'); delay(500);  echoIncomingBTSerial();
  USB_Serial->println("*** Name setting complete.  Did it return the desired name?");

  // set the GP IO pins so that the data mode / command mode can be set by hardware
  USB_Serial->println("*** Setting GPIOCONTROL mode to OFF, which is what we need...");
  BT_Serial->print("SET GPIOCONTROL=OFF");BT_Serial->print('\r'); delay(500);echoIncomingBTSerial();
  BT_Serial->print("WRITE"); BT_Serial->print('\r'); delay(500); echoIncomingBTSerial();
  
  // go into data mode (Tympan's normal way of operating)
  USB_Serial->println("*** Changing into transparanet data mode...");
  BT_Serial->print("ENTER_DATA");BT_Serial->print('\r'); delay(500); echoIncomingBTSerial();
  myTympan.forceBTtoDataMode(true); //forcing (via hardware pin) the BT device to be in data mode
  USB_Serial->println("*** BT Setup complete.");
}
