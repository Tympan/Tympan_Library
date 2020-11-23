
//Rev D uses BC127: https://learn.sparkfun.com/tutorials/understanding-the-bc127-bluetooth-module/bc127-basics

void enterCommandMode_RevD(void) {
  int timer = 0;
  boolean ok = false;
  // go into command mode
  USB_Serial->println("*** Switching Tympan RevD BT module into command mode...");
//  myTympan.forceBTtoDataMode(false);   delay(500); echoIncomingBTSerial(); //stop forcing the device to be in data mode
//  BT_Serial->print("$");  delay(400);
  BT_Serial->print("$$$$");  //delay(500);
  while(!ok){ 
    USB_Serial->print("."); 
    delay(20); 
    timer++;
    if(timer > 200){
      USB_Serial->println("\nEnter Command Mode Timed Out!");
      return;
    } 
    if(timer%30 == 0){
      USB_Serial->println();
    }
    ok = BT_Serial->find("OK");
  }
  if(ok){ 
    USB_Serial->println("\n*** Now in Command mode ***"); 
  } else {
    USB_Serial->println("*** Error Entering Command Mode ***");
  }
 
}

void enterDataMode_RevD(void){
  int timer = 0;
  BT_Serial->print("ENTER_DATA\r");
  while(BT_Serial->available() == 0){ 
    USB_Serial->print("."); 
    delay(20); 
    timer++;
    if(timer > 20){
      USB_Serial->println("\nEnter Data Mode timed out");
      return;
    }
  }
  response = BT_Serial->readStringUntil('\n');
  if(response.indexOf("OK") > 0){
    USB_Serial->println("\n*** Now in Data mode ***");
  } else if(response.indexOf("ERROR") > 0){
    USB_Serial->println("\n*** Error Entering Data Mode ***");
  }
}
