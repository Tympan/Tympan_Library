Tympan Library
===========================

![](https://travis-ci.org/Tympan/Tympan_Library.svg?branch=master)

**Purpose**: This library allows you to program your own audio processing algorithms for the Tympan!  It owes a big debt to the [Teensy Audio Library](http://www.pjrc.com/teensy/td_libs_Audio.html) and all of its contributors.

**Note**: These instructions were based on the *Arduino IDE 1.8.15* and *Teensyduino 1.54*. If you have different versions of either of these or have downloaded different versions, the program may or may not function as expected. You may revert or upgrade to the stated versions or check for solutions on the [Tympan Forum](https://forum.tympan.org/).

Requirements
------------

**Hardware**: All versions of the Tympan are, at their heart, compatible with the Teensy brand of microcontrollers.  Hence, many of the instructions below relate to Teensy.  For reference, Tympan RevD is Teensy 3.6 whereas the Tympan RevE is Teensy 4.1.

**Arduino IDE (Windows/Linux)**:  To program the Tympan, you need to download and install the 1.8.x version of the [Arduino IDE](https://www.arduino.cc/en/Main/Software).  Scroll down and download the 1.8.x version, do not use the newer 2.x version nor the web-based editor. After installing the Arduino IDE, open the shortcut so that it fully initializes.  Then, you can close it down and move to the next step, which is...

**Teensyduino Add-On (Windows/Linux, MASTER branch)**:  After installing the Arduino IDE, you need to download and install the [Teensyduino Add-On](https://www.pjrc.com/teensy/td_download.html), which allows you to program Teensy devices (including Tympan) using the Arduino IDE. It'll ask you whether you want to install a bunch of libraries.  You can say "yes" to them all, or you can say "yes" to these few: Audio, Bounce2, FreqCount, FreqMeasure, i2c_t3, SerialFlash, Snooze, and SPIFlash.  

**Teensyduino (Mac, MASTER branch)**: Recent versions of OS X do not support the Teensyduino Add-On installer, so there is a full [Teensyduino IDE](https://www.pjrc.com/teensy/td_download.html) available from the Teensy project which just needs to be downloaded and extracted from its ZIP file.


Installing the Tympan Library
------------

**Know GitHub?**:  If you know GitHub, go ahead and fork this repo and clone it to your local computer.  I strongly recommend that you clone it directly to the place on your computer where the Arduino IDE looks for libraries.  On my computer, I put them here: `C:\Users\chipaudette\Documents\Arduino\libraries\Tympan_Library`

**Don't Know GitHub?**: If you don't know GitHub, no problem!  You can download this library as a ZIP file.  Go back to the root of the Tympan library ([here](https://github.com/Tympan/Tympan_Library)) and click on the green button that says "Clone or Download".  You can then select "Download Zip".  Once the download is complete, I'd unzip the files and put them where the Arduino IDE looks for libraries.  On my computer, I put them here: `C:\Users\chipaudette\Documents\Arduino\libraries\Tympan_Library`


Connect the Tympan Audio Board
------------------

Open or restart the Arduino IDE. Before you can compile programs, you will need to tell the Arduino IDE that you want to program for Teensy. Under the "Tools" menu, choose "Board" and then "Teensy 4.1" for Tympan RevE (choose "Teensy 3.6" for Tympan RevD).  Also, under the "Tools" menu, choose "USB Type" and select "Serial + MIDI + Audio". 

If you haven't already, plug in the Tympan Audio Board to your computer using the micro USB cable. You will see orange lights. Power the unit ON using the white switch on the side and you will see the blue blinking light which indicates power. If you already had the Tympan Audio Board plugged in, turn the power off and then on again to power cycle prior to proceeding. Make sure you see the message that the device drivers are being installed.

Once your Tympan is connected to the computer via the micro USB cable and powered on, you can select the device by choosing "Port" under the "Tools" menu and and select the one that is associated with the Teensy (on Windows, it'll say "Teensy" in its name).

Try Some Examples
-------------

In the Arduino IDE, under the "File" menu, select "Examples".  Then, scroll way to the bottom of the list.  You should see an entry that says "Tympan_Library".  Those are all the example files for trying out different audio algorithms or features of the system.  I recommend starting with the basic examples:

* **Audio Pass-Thru:**  You should start with this example.  No processing, just an audio pass-thru from input (microphone, pink socket) to output (headset or earbuds, black socket).  You should always start as simple as possible.  It doesn't get simpler than this.  Since it doesn't apply any gain, it'll be quiet.

Once the example has been selected, it needs to be compiled or uploaded. The Arduino IDE's compile button is the one with the Arrow facing to the right. Note: take headphones off your ears when uploading a new program or changing modes to avoid any sharp pops, clicks, or other unpleasant noises.

Once the basic audio pass-thru program works, you can try the next example:

* **Basic Gain:** As your second trial, you try this example, which adds gain to the audio.  The blue volume knob adjust the amount of gain.  Easy!  

While the Tympan is connected to your computer (via USB) and running the Basic Gain example, you can communicate with your Tympan to find out what it's doing.  Under the Arduino IDE's "Tools" menu, select "Port" and select the one that is associated with the Teensy (on Windows, it'll say "Teensy" in its name).  Once you've selected the Teensy's port, you can open the "Serial Monitor" from the "Tools" menu.  On the Tympan, you can turn its blue volume knob.  Looking at the Serial Monitor window, you can see that it reports the volume setting it's now using.  In the other example programs, the Serial Monitor is used extensively to monitor what the Tympan is doing.

Once this works, you can work your way down the examples list to see how they add complexity and capability.  Have fun!

Releasing Tympan_Library to Arduino Library Manager
-------------
The Arduino "Library Manager" is a handy way for some users to keep up-to-date with the Tympan_Library.  For those of us used to using Git/GitHub, be aware that users do not get the latest state of the repository.  Instead, the Arduino Library Manager will only people's libraries if we do a new "Release" here on GitHub.  For Arduino to accept the release, however, you must do the release correctly:

* Make your desired changes to the Tympan_Library:
  * Commit your software changes to the repo and push them up here to GitHub
* Arduino-Specific Changes:
  * In the file [library.properties](https://github.com/Tympan/Tympan_Library/releases/new), update the version number to whatever new version number you intend to use.  Commit the change and push the change up here to GitHub.  If you forget to do this, Arduino will ignore the release.
* Do the GitHub release in the usual way:
  * Go to [Draft a New Release](https://github.com/Tympan/Tympan_Library/releases/new)
  * Under "Choose a Tag", start typing a new name for the Tag and Release ("V3.0.6") and be sure to also click "Create New Tag on Publish"
  * Fill in field for "Release Name" and add some text describing what is in the release
  * Click the button "Generate Release Notes", which puts a listing of all the commits at the bottom of the release text
  * Click "Publish Release"

To check on the status of the Tympan_Library being accepted by the Arduino Library Manager, see the Arduino processing log here: https://downloads.arduino.cc/libraries/logs/github.com/Tympan/Tympan_Library/ 
