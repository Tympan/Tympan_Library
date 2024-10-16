#
# getFileFromTympan.py
# 
# Chip Audette, OpenAudio, Oct 2024
# MIT License 
#
# This script communicates with the Tympan over the USB Serial link.
# The purpose is to get a file from the Tympan's SD card over to the
# PC and saved on the PC's local disk
#
# I'm using this as a Serial communication example: 
# https://projecthub.arduino.cc/ansh2919/serial-communication-between-python-and-arduino-663756
#

import serial  #pip install pyserial
import time 
import codecs
import numpy as np


# ##################### Functions 

# send text over the serial link with an EOL character in the format that is expected for Tympan
def sendTextToSerial(serial_to_tympan, text, EOL='\n'):
    serial_to_tympan.write(bytes(text + EOL, 'utf-8'))  #send the text 
    time.sleep(0.05)   #give some time for the device to respond

# receive a line of text from the serial link (must end in newline)
def readLineFromSerial(serial_to_tympan):
    return codecs.decode(serial_to_tympan.readline(),encoding='utf-8')

# keeping receiving lines of text until enough time has passed that we exceed the wait period
def readMultipleLinesFromSerial(serial_to_tympan, wait_period_sec=0.5):
    all_lines = readLineFromSerial(serial_to_tympan)
    last_reply_time  = time.time()
    while (time.time() < (last_reply_time + wait_period_sec)):
        new_readline = readLineFromSerial(serial_to_tympan)
        all_lines += new_readline
        if len(new_readline) > 0:
            last_reply_time = time.time()
    return all_lines

# receive raw btes from the serial until we have receive the number of bytes
# specified or until the serial link times out
def readBytesFromSerial(serial_to_tympan, n_bytes_to_receive, blocksize=1024):  # blocksize specifies how many bytes to try to read from the serial port at a time
    all_data = bytearray()  #initialize empty
    bytesleft = n_bytes_to_receive
    while (bytesleft > 0):
        bytes_to_read= min(blocksize, bytesleft)
        raw_bytes = serial_to_tympan.read(bytes_to_read) #this will timeout (if needed) according to the serial port timeout parameter
        if (len(raw_bytes) == bytes_to_read):
            # we read the whole block.  Great.
            bytesleft = bytesleft - bytes_to_read
        else:
            # We got too few bytes.  Assume no more bytes are coming
            print("getRawReply: recieved " + str(len(raw_bytes)) + " but expected " + str(bytes_to_read))
            bytesleft = 0  #assume no more bytes are coming.  this will break out of the while loop
        #
        all_data += raw_bytes
    #
    return all_data

# given a comman-delimited string of file names, parse out the file names
# and return as a list of strings
def processLineIntoFilenames(line):
    # Assumes we are given one line that contains comma-seperated filenames.  
    # The line of text might contain preamble text, which will end with a colon.
    # So, find the last colon and only keep the text after the last colon.
    line = line.split(':')  #split into sections
    line = line[-1]  #get everything after the last split
    #
    #Now, split the text at the colons
    names = line.split(',')
    all_fnames = []
    for name in names:
        name = name.strip()
        if len(name) > 0:
            all_fnames.append(name.strip())  #remove whitespace before and after and save
        #
    #
    return all_fnames


# ####################################################################
# ######################## Here is the Main ##########################
# ####################################################################



#specify the COM port for your Tympan...every one is different!
my_com_port = 'COM9'    #Look in the Arduino IDE! 



# ################ Let's set up the serial communication to the Tympan

#release the comm port from any previous runs of this Python script
if 'serial_to_tympan' in locals():
    serial_to_tympan.close()

# create a serial instance for communicating to your Tympan
print("Opening serial port...make sure the Serial Monitor is closed in Arduino IDE")
wait_period_sec = 0.5  #how long before serial comms time out (could set this faster, if you want)
serial_to_tympan = serial.Serial(port=my_com_port, baudrate=115200, timeout=wait_period_sec) #baudrate doesn't matter for Tympan

# let's test the connection by asking for the help menu
sendTextToSerial(serial_to_tympan, 'h')                   #send the command to the Tympan
reply = readMultipleLinesFromSerial(serial_to_tympan)     #get the mutli-line reply from the Tympan
print("REPLY:",reply)                                     #print the lines to the screen here in Python



# ############# If you don't have any files on the SD, create one!
if 1:  #set this to zero to skip this step
    sendTextToSerial(serial_to_tympan, 'd')               #send the command to the Tympan
    reply = readMultipleLinesFromSerial(serial_to_tympan) #get the one-line reply from the Tympan
    print("REPLY:",reply)                                 #print the line to the screen here in Python



# ############# Now, let's follow the typical download procedure

# First, let's ask for a list of files on the Tympan's SD card
sendTextToSerial(serial_to_tympan, 'L')                   #send the command to the Tympan
reply = readLineFromSerial(serial_to_tympan)              #get the one-line reply from the Tympan
print("REPLY:",reply.strip())                             #print the line to the screen here in Python

# let's break up the full text reply into the filenames
fnames = processLineIntoFilenames(reply)                  #parse out the filenames
print("RESULT: Files on Tympan SD:", fnames)              #print the line to the screen here in Python

# choose the file that you want to download to the PC
if len(fnames) > 0:
    fname = fnames[-1] #load the last (ie, the most recent?)
    print("ACTION: Asking for file:",fname)
else:
    print("ERROR: no filenames were received from the Tympan!")

# Second, start to open the file on the Tympan
sendTextToSerial(serial_to_tympan, 'f')                   #send the command to the Tympan
reply = readLineFromSerial(serial_to_tympan)              #get the one-line reply from the Tympan
print("REPLY:",reply.strip())                             #print the line to the screen here in Python

# send the filename that we want
sendTextToSerial(serial_to_tympan, fname)                 #send the command to the Tympan
reply = readLineFromSerial(serial_to_tympan)              #get the one-line reply from the Tympan
print("REPLY:",reply.strip())                             #print the line to the screen here in Python

# Third, get the file size in bytes
sendTextToSerial(serial_to_tympan, 'b')                   #send the command to the Tympan
reply = readLineFromSerial(serial_to_tympan)              #get the one-line reply from the Tympan
print("REPLY:",reply.strip())                             #print the line to the screen here in Python
n_bytes = int(reply)                                      #interpret the value as an integer
print("RESULT: value=",n_bytes)                           #print the value to the screen here in Python

# Fourth, transfer the file itself
sendTextToSerial(serial_to_tympan, 't')                   #send the command to the Tympan
reply = readBytesFromSerial(serial_to_tympan,n_bytes)     #get the one-line reply from the Tympan
print("REPLY:",reply)                                     #print the bytes to the screen here in Pyton



# ############################# Finally, let's finish up

# save to local file
print("ACTION: Writing bytes to file here on PC:", fname)
with open(fname,'wb') as file:
    file.write(reply)

# close the serial port
print("ACTION: Closing serial port...")
serial_to_tympan.close()


