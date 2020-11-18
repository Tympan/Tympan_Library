
//Rev D uses BC127: https://learn.sparkfun.com/tutorials/understanding-the-bc127-bluetooth-module/bc127-basics

void upgradeMelody_RevD(void) {
  
  // go into command mode
  USB_Serial->println("*** Switching Tympan RevD BT module into command mode...");
  myTympan.forceBTtoDataMode(false);   delay(500); echoIncomingBTSerial(); //stop forcing the device to be in data mode
//  BT_Serial->print("$");  delay(400);
  BT_Serial->print("$$$$");  delay(400);
  BT_Serial->print('\r'); delay(400);
  delay(500);
  echoIncomingBTSerial();
  USB_Serial->println("*** Could be ERROR response. That's OK here");
  USB_Serial->println("*** Now in BT <-> USB Echo mode ***");
  
}
