Tympan Library
===========================

![](https://travis-ci.org/Tympan/Tympan_Library.svg?branch=master)

**Purpose**: This library allows you to program your own audio processing algorithms for the Tympan!  It owes a big debt to the [Teensy Audio Library](http://www.pjrc.com/teensy/td_libs_Audio.html) and all of its contributors.

**Note**: These instructions were based on the *Arduino IDE 1.8.2* and *Teensyduino 1.36*. If you have different versions of either of these or have downloaded different versions, the program may or may not function as expected. You may revert or upgrade to the stated versions or check for solutions on the [Tympan Forum](https://forum.tympan.org/).

Requirements
------------

**Hardware**: This library is intended to be used to program a Teensy 3.6 that is connected to a Tympan Audio Board (the Tympan in the nice black case).  To fully utilize the Teensy 3.6, you will also need an external microphone, a headset or earbuds and a micro USB cable. Although the Tympan Audio Board also has an On-Board Microphone, the default programming for this device references an external microphone. If you have been provided with the 2017-04-27 Typman Getting Started.pptx, you will find it helpful to review that first for additional information related to the physical hardware setup. 

**Arduino IDE (Windows/Linux)**:  To program the Tympan, you need to download and install the [Arduino IDE](https://www.arduino.cc/en/Main/Software).  Download the actual Arduino IDE, don't use the online web-based editor.  After installing the Arduino IDE, open the shortcut so that it fully initializes.  Then, you can close it down and move to the next step, which is...

**Teensyduino Add-On (Windows/Linux)**:  After installing the Arduino IDE, you need to download and install the [Teensyduino Add-On](https://www.pjrc.com/teensy/td_download.html), which allows you to program the Teensy using the Arduino IDE. It'll ask you whether you want to install a bunch of libraries.  You can say "yes" to them all, or you can say "yes" to just a few (like Audio, Bounce2, FreqCount, FreqMeasure, i2c_t3, SerialFlash, Snooze, and SPIFlash).  It's your choice.

**Teensyduino (Mac)**: Recent versions of OS X do not support the Teensyduino Add-On installer, so there is a full [Teensyduino IDE](https://www.pjrc.com/teensy/td_download.html) available from the Teensy project which just needs to be downloaded and extracted from its ZIP file.

Installing the Tympan Library
------------

**Know GitHub?**:  If you know GitHub, go ahead and fork this repo and clone it to your local computer.  I strongly recommend that you clone it directly to the place on your computer where the Arduino IDE looks for libraries.  On my computer, I put them here: `C:\Users\chipaudette\Documents\Arduino\libraries\Tympan_Library`

**Don't Know GitHub?**: If you don't know GitHub, no problem!  You can download this library as a ZIP file.  Go back to the root of the Tympan library ([here](https://github.com/Tympan/Tympan_Library)) and click on the green button that says "Clone or Download".  You can then select "Download Zip".  Once the download is complete, I'd unzip the files and put them where the Arduino IDE looks for libraries.  On my computer, I put them here: `C:\Users\chipaudette\Documents\Arduino\libraries\Tympan_Library`


Connect the Tympan Audio Board
------------------

Open or restart the Arduino IDE. Before you can compile programs, you will need to tell the Arduino IDE that you want to program for Teensy. Under the "Tools" menu, choose "Board" and then "Teensy 3.6".  Also, under the "Tools" menu, choose "USB Type" and select "Serial + MIDI + Audio". 

If you haven't already, plug in the Tympan Audio Board to your computer using the micro USB cable. You will see orange lights. Power the unit ON using the white switch on the side and you will see the blue blinking light which indicates power. If you already had the Tympan Audio Board plugged in, turn the power off and then on again to power cycle prior to proceeding. Make sure you see the message that the device drivers are being installed.

Try Some Examples
-------------

In the Arduino IDE, under the "File" menu, select "Examples".  Then, scroll way to the bottom of the list.  You should see an entry that says "Tympan_Library".  Those are all the example files for trying out different audio algorithms or features of the system.  I recommend starting with the basic examples:

* **Audio Pass-Thru:**  You should start with this example.  No processing, just an audio pass-thru from input (microphone, pink socket) to output (headset or earbuds, black socket).  You should always start as simple as possible.  It doesn't get simpler than this.  Since it doesn't apply any gain, it'll be quiet.

Once the example has been selected, it needs to be compiled or uploaded. The Arduino IDE's compile button is the one with the Arrow facing to the right. Note: take headphones off your ears when uploading a new program or changing modes to avoid any sharp pops, clicks, or other unpleasant noises.

Once the basic audio pass-thru program works, you can try the next example:

* **Basic Gain:** As your second trial, you try this example, which adds gain to the audio.  The blue volume knob adjust the amount of gain.  Easy!  

While the Tympan is connected to your computer (via USB) and running the Basic Gain example, you can communicate with your Tympan to find out what it's doing.  Under the Arduino IDE's "Tools" menu, select "Port" and select the one that is associated with the Teensy (on Windows, it'll say "Teensy" in its name).  Once you've selected the Teensy's port, you can open the "Serial Monitor" from the "Tools" menu.  On the Tympan, you can turn its blue volume knob.  Looking at the Serial Monitor window, you can see that it has told you what volume setting it's now using.  In the other example programs, the Serial Monitor is used extensively to monitor what the Tympan is doing.

Once this works, you can work your way down the examples list to see how they add complexity and capability.  Have fun!


---
