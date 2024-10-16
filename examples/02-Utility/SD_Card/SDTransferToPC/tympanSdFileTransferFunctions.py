
#
# tympanSerialdFileTransferFunctions.py
#
# Created: Chip Audette, OpenAudio, Oct 2024
#
# Purpose: Provide a library of helper functions for communicating
#     over the USB Serial link to the Tympan
#
# MIT License
#

import serial  #pip install pyserial
import time 
import codecs
import os


# ####################################### Define Low-Level Functions 

class HaltException(Exception): 
    pass

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

def sendFileAsBytesToSerial(local_fname, serial_to_tympan, blocksize=1024):
    with open(local_fname,'rb') as file:
        byte_count = 0
        byte = file.read(1)
        while byte:
            byte_count = byte_count+1
            serial_to_tympan.write(byte)
            if ((byte_count % blocksize) == 0):
                time.sleep(0.005) #slow it down after every block
            byte = file.read(1)
    return byte_count

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

# Filter the filenames to keep those whose extension is of the desired type (WAV, TXT, etc)
def keepFilenamesOfType(all_fnames,targ_types=['wav']):
    out_fnames=[]
    for fname in all_fnames:
        pieces = fname.split('.')
        if (pieces[-1].lower() in targ_types): #is the trailing extension the same as our target?
            out_fnames.append(fname)          #if so, keep the filename!
        #
    #
    return out_fnames

# ##################################### Define High-Level Functions

# Here is the script for working with the Tympan to send a file to be saved on its SD card
def sendFileToTypman(serial_to_tympan, command_char, fname_to_read_locally, fname_to_write_on_Tympan, verbose=False):
    try:
        # Step 1: Initiate the file transfer process (PC to Tympan)
        if (verbose): print("ACTION: Initiating process of file transfer to Tympan")
        sendTextToSerial(serial_to_tympan, command_char)        #send the command to the Tympan
        reply = readLineFromSerial(serial_to_tympan)            #get the one-line reply from the Tympan
        if (verbose): print("REPLY:",reply.strip())                           #print the line to the screen here in Python
        if ("*** ERROR ***" in reply):                          #check for an error code
            raise HaltException()                               #jump to the end if error
        
        #Step 2: Send the filename to write on Tympan
        if (verbose): print("ACTION: Sending filename to Tympan:",fname_to_write_on_Tympan)
        sendTextToSerial(serial_to_tympan, fname_to_write_on_Tympan)  #adds an newline at end
        reply = readLineFromSerial(serial_to_tympan)            #get the one-line reply from the Tympan
        if (verbose): print("REPLY:",reply.strip())                           #print the line to the screen here in Python
        if ("*** ERROR ***" in reply):                          #check for an error code
            raise HaltException()                               #jump to the end if error

        #Step 3: Send the number of bytes to be transferred
        file_size = os.path.getsize(fname_to_read_locally)      #get the file size from the OS
        if (verbose): print("ACTION: Sending number of bytes to Tympan:",file_size)
        sendTextToSerial(serial_to_tympan, str(file_size))      #send the size to the Tympan
        reply = readLineFromSerial(serial_to_tympan)            #get the one-line reply from the Tympan
        if (verbose): print("REPLY:",reply.strip())                           #print the line to the screen here in Python
        if ("*** ERROR ***" in reply):                          #check for an error code
            raise HaltException()                               #jump to the end if error

        #Step 4: Send the bytes of the file
        if (verbose): print("ACTION: Sending the file to the Tympan:", fname_to_read_locally)
        sendFileAsBytesToSerial(fname_to_read_locally, serial_to_tympan)  #send the local file to the Tympan
        
        #Step 5: (Optional) Read the final reply from the Tympan
        if (verbose): print("ACTION: Reading the confirmation message sent by the Tympan")
        reply = readLineFromSerial(serial_to_tympan)            #get the one-line reply from the Tympan
        if (verbose): print("REPLY:",reply.strip())                           #print the line to the screen here in Python
        if ("*** ERROR ***" in reply):                          #check for an error code
            raise HaltException()                               #jump to the end if error

        if (verbose): print("SUCCESSS: File was successfully transferred to the Tympan")
        return True

    except HaltException as h:
        print("FAIL: File was NOT successfully transferred to the Tympan")
        return False


# Here is the script for working with the Tympan to have it send a file from its SD card
def receiveFileFromTympan(serial_to_tympan, command_char, fname_to_read_on_Tympan, fname_to_write_locally, verbose=False):
    try:
        # Step 1: Initiate the file transfer process (PC to Tympan)
        if (verbose):print("ACTION: Initiating process of file transfer from Tympan")
        sendTextToSerial(serial_to_tympan, command_char)        #send the command to the Tympan
        reply = readLineFromSerial(serial_to_tympan)            #get the one-line reply from the Tympan
        if (verbose):print("REPLY:",reply.strip())                           #print the line to the screen here in Python
        if ("*** ERROR ***" in reply):                          #check for an error code
            raise HaltException()                               #jump to the end if error
        
        # Step 2: Send the filename to read on Tympan
        if (verbose):print("ACTION: Sending filename to Tympan:",fname_to_read_on_Tympan)
        sendTextToSerial(serial_to_tympan, fname_to_read_on_Tympan)  #adds an newline at end
        reply = readLineFromSerial(serial_to_tympan)            #get the one-line reply from the Tympan
        if (verbose):print("REPLY:",reply.strip())                           #print the line to the screen here in Python
        if ("*** ERROR ***" in reply):                          #check for an error code
            raise HaltException()                               #jump to the end if error

        # Step 3: Read the number of bytes to be sent by the Tympan.
        if (verbose):print("ACTION: Reading number of bytes to be sent by the Tympan")
        reply = readLineFromSerial(serial_to_tympan)            #get the one-line reply from the Tympan
        if (verbose):print("REPLY:",reply.strip())                           #print the line to the screen here in Python
        if ("*** ERROR ***" in reply):                          #check for an error code
            raise HaltException()   
        bytes_to_receive = int(reply.strip())

        #Step 4: Read the prompt that the Tympan is starting the transfer (message ends in newline)
        if (verbose):print("ACTION: Reading the start prompt provided by the Tympan")
        reply = readLineFromSerial(serial_to_tympan)            #get the one-line reply from the Tympan
        if (verbose):print("REPLY:",reply.strip())                           #print the line to the screen here in Python
        if ("*** ERROR ***" in reply):                          #check for an error code
            raise HaltException()   
        
        #Step 5: Read the in-coming bytes
        received_bytes = readBytesFromSerial(serial_to_tympan,bytes_to_receive) #get the one-line reply from the Tympan
        if (len(received_bytes)<300):
            if (verbose):print("RESULT: The",len(received_bytes),"received bytes are:", received_bytes)
        else:
           if (verbose): print("RESULTS: ",len(received_bytes),"bytes were received")
        
        #Step 6: (Optional) Read the final reply from the Tympan
        if (verbose):print("ACTION: Reading the confirmation message sent by the Tympan")
        reply = readLineFromSerial(serial_to_tympan)            #get the one-line reply from the Tympan
        if (verbose):print("REPLY:",reply.strip())                           #print the line to the screen here in Python
        if ("*** ERROR ***" in reply):                          #check for an error code
            raise HaltException()                               #jump to the end if error

        #Step 7: Save the received bytes to a local file here on the PC
        with open(fname_to_write_locally,'wb') as file:
            if (verbose):print("ACTION: Writing bytes to local file:",fname_to_write_locally)
            file.write(received_bytes)
            if (verbose):print("RESULT: Writing to local file was successful.")
            if (verbose):print("SUCCESS: File was successfully transfererd from the Tympan")
            return True
    
        raise HaltException()

    except HaltException as h:
        print("FAIL: File was NOT successfully transferred from the Tympan")
        return False

