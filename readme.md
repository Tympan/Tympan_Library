Tympan Library
===========================

**Purpose**: This library allows you to program your own audio processing algorithms for the Tympan!  It owes a big debt to the [Teensy Audio Library](http://www.pjrc.com/teensy/td_libs_Audio.html) and all of its contributors.

Requirements
------------

**Hardware**: This library is intended to be used to program a Teensy 3.6 that is connected to a Tympan Audio Board.  If you have a Typman in its nice black case, you have all the hardware that you need!

**Arduino IDE**:  To program the Tympan, you need to download and install the [Arduino IDE](https://www.arduino.cc/en/Main/Software).  Download the acutal Arduino IDE, don't use the online web-based editor.  

**Teensyduino Add-On**:  After installing the Arduino IDE, you need to download and install the [Teensyduino Add-On](https://www.pjrc.com/teensy/td_download.html), which allows you to program the Teensy using the Arduino IDE. It'll ask you whether you want to install a bunch of libraries.  You can say "yes" to them all, or you can say "yes" to just a few (like Audio, Bounce2, FreqCount, FreqMeasure, i2c_t3, SerialFlash, Snooze, and SPIFlash).  It's your choice.

Installing the Tympan Library
------------

**Know GitHub?***:  If you know GitHub, go ahead and fork this repo and clone it to your local computer.  I strongly recommend that you clone it directly to the place on your computer where the Arduino IDE looks for libraries.  On my computer, I put them here: `C:\Users\chipaudette\Documents\Arduino\libraries\Tympan_Library`

**Don't Know GitHub?**: If you don't know GitHub, no problem!  You can download this library as a ZIP file.  Go back to the root of the Tympan library ([here](https://github.com/Tympan/Tympan_Library)) and click on the green button that says "Clone or Download".  You can then select "Download Zip".  Once the download is complete, I'd unzip the files and put them where the Arduino IDE looks for libraries.  On my computer, I put them here: `C:\Users\chipaudette\Documents\Arduino\libraries\Tympan_Library`

Try Some Examples
-------------

Once you've downloaded the Tympan library, start (or restart) the Arduino IDE.  Under the "File" menu, select "Examples".  Then, scroll way to the bottom of the list.  You should see an entry that says "Tympan_Library".  Those are all the example files for trying out different audio alorithms or features of the system.  I recommend starting with:


* **Audio Pass-Thru:**  You should start with this blog post: [Teensy Audio Board - First Audio](http://openaudio.blogspot.com/2016/10/teensy-audio-board-first-audio.html). It walks you through the process of configuring your device to pass the input audio to the output.  No processing, just an audio pass-thru.  Always start simple.  It doesn't get simpler than this.  If you don't want to walk through the whole process of using the Teensy Audio GUI, you can use my code directly.  Get it at the OpenAudio GitHub [here](https://github.com/chipaudette/OpenAudio_blog/tree/master/2016-10-23%20First%20Teensy%20Audio/Arduino/BasicLineInPassThrough).

* **Basic Gain:** As your second trial, you should try adding gain to the signal.  Go to this blog post: [A Teensy Hearing Aid](http://openaudio.blogspot.com/2016/11/a-teensy-hearing-aid.html).  In addition to introducing the hardware of the hearing aid breadboad, it points to code that adds gain to the audio signal based on the setting of the potentiometer.  It helps illustrate how the data flows through the system so that you can manipulate it.  If you don't want to read through the whole thing, the example code can be found directly at my OpenAudio GitHub [here](https://github.com/chipaudette/OpenAudio_blog/tree/master/2016-11-20%20Basic%20Hearing%20Aid/BasicGain).
