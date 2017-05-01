Tympan Library
===========================

**Purpose**: This library allows you to program your own audio processing algorithms for the Tympan!  It owes a big debt to the [Teensy Audio Library](http://www.pjrc.com/teensy/td_libs_Audio.html) and all of its contributors.

Requirements
------------

**Hardware**: This library is intended to be used to program a Teensy 3.6 that is connected to a Tympan Audio Board.  If you have a Typman in its nice black case, you have all the hardware that you need!

**Arduino IDE**:  To program the Tympan, you need to download and install the [Arduino IDE](https://www.arduino.cc/en/Main/Software).  Download the acutal Arduino IDE, don't use the online web-based editor.  After installing the Arduino IDE, run it once so that it fully initializes.  Then, you can close it down and move to the next step, which is...

**Teensyduino Add-On**:  After installing the Arduino IDE, you need to download and install the [Teensyduino Add-On](https://www.pjrc.com/teensy/td_download.html), which allows you to program the Teensy using the Arduino IDE. It'll ask you whether you want to install a bunch of libraries.  You can say "yes" to them all, or you can say "yes" to just a few (like Audio, Bounce2, FreqCount, FreqMeasure, i2c_t3, SerialFlash, Snooze, and SPIFlash).  It's your choice.

Installing the Tympan Library
------------

**Know GitHub?**:  If you know GitHub, go ahead and fork this repo and clone it to your local computer.  I strongly recommend that you clone it directly to the place on your computer where the Arduino IDE looks for libraries.  On my computer, I put them here: `C:\Users\chipaudette\Documents\Arduino\libraries\Tympan_Library`

**Don't Know GitHub?**: If you don't know GitHub, no problem!  You can download this library as a ZIP file.  Go back to the root of the Tympan library ([here](https://github.com/Tympan/Tympan_Library)) and click on the green button that says "Clone or Download".  You can then select "Download Zip".  Once the download is complete, I'd unzip the files and put them where the Arduino IDE looks for libraries.  On my computer, I put them here: `C:\Users\chipaudette\Documents\Arduino\libraries\Tympan_Library`

Try Some Examples
-------------

Once you've downloaded the Tympan library, start (or restart) the Arduino IDE.  Under the "File" menu, select "Examples".  Then, scroll way to the bottom of the list.  You should see an entry that says "Tympan_Library".  Those are all the example files for trying out different audio alorithms or features of the system.  I recommend starting with the basic examples:

* **Audio Pass-Thru:**  You should start with this example.  No processing, just an audio pass-thru from input to output.  You should always start as simple as possible.  It doesn't get simpler than this.  Since it doesn't apply any gain, it'll be quiet.

Before you compile this example, you need to tell the Arduino IDE that you want to program for Teensy.  So, in the Arduino IDE, under the "Tools" menu, choose "Board" and then "Teensy 3.6".  Now you can connect your Teensy to your computer, you can turn on your Teensy (it has to be on!), and you can hit the Arduino IDE's compile button (the one with the Arrow facing to the right).

Once the basic audio pass-thru program works, you can try the next example:

* **Basic Gain:** As your second trial, you try this example, which adds gain to the audio.  The blue volume knob adjust the amount of gain.  Easy!  While the Tympan is connected to your computer (via USB) and running, try opening the Serial Monitor in the Arduino IDE.  Turn the volume knob.  Note that it tells you what volume setting you've changed to.  This is how you'll debug in the future.

Once this works, you can work your way down the examples list to see how they add complexity and capability.  Have fun!
