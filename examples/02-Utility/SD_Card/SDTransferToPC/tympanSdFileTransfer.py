#
# tympanSdFileTransfer.py
# 
# Chip Audette, OpenAudio, Oct 2024
# MIT License 
#
# This script communicates with the Tympan over the USB Serial link.
# The purpose is to send a file to the Tympan's SD card or to 
# receive a file back from the Tympan's SD card.
#
# For Python serial communication, I'm buiding from this example: 
# https://projecthub.arduino.cc/ansh2919/serial-communication-between-python-and-arduino-663756
#

import serial  #pip install pyserial
import tympanSdFileTransferFunctions as tympanSerial


# VERY IMPORTANT: specify the COM port of your Tympan. You can Look in the Arduino IDE to find this.
my_com_port = 'COM26'    #YOUR INPUT IS NEEDED HERE!!!  


# Do you want to learn what is happening?  Or, do you need to debug?
verbose = False   #set to true for printing of helpful info


# create a serial instance for communicating to your Tympan
print("ACTION: Opening serial port...make sure the Serial Monitor is closed in Arduino IDE...")
if ('serial_to_tympan' in locals()): serial_to_tympan.close()  #close serial port if already open
wait_period_sec = 0.5  #how long before serial comms time out (could set this faster, if you want)
serial_to_tympan = serial.Serial(port=my_com_port, baudrate=115200, timeout=wait_period_sec) #baudrate doesn't matter for Tympan
print("RESULT: Serial port opened successfully")


# [Optional] let's test the connection by asking for the help menu
print();print('ACTION: Requesting the Tympan Help Menu...')
tympanSerial.sendTextToSerial(serial_to_tympan, 'h')                   #send the command to the Tympan
reply = tympanSerial.readMultipleLinesFromSerial(serial_to_tympan)     #get the mutli-line reply from the Tympan
print("REPLY:",reply.strip()); print()


# Transfer a file TO THE TYMPAN
print();print("ACTION: Starting process for transferring file TO the Tympan...")
command_sendFileToTympan = 'T'                 #This is set by the Tympan program
fname_to_read_locally = 'exampleToSend.txt'    #on the local computer, what file to send to Tympan?
fname_to_write_on_Tympan = 'exampleSent.txt'   #on Tympan, what file to receive to?
send_success = tympanSerial.sendFileToTypman(serial_to_tympan, command_sendFileToTympan, \
                    fname_to_read_locally, fname_to_write_on_Tympan, verbose=verbose)
if (send_success):
    print("RESULT: File " + fname_to_read_locally + " sent successfully!  Saved there as " + fname_to_write_on_Tympan)
else:
    print("RESULT: File " + fname_to_read_locally + " failed to be sent to the Tympan")


# Check to see what files are on the SD
print();print("ACTION: Reading the file names on the Tympan SD Card...")
tympanSerial.sendTextToSerial(serial_to_tympan, 'L')                #send the command to the Tympan
reply = tympanSerial.readLineFromSerial(serial_to_tympan)           #get the mutli-line reply from the Tympan
print("REPLY:",reply.strip())  


# Transfer a file FROM THE TYMPAN
print();print("ACTION: Receiving a file from the Tympan...")
command_getFileFromTypman = 't'                #This is set by the Tympan program
fname_to_read_on_Tympan = 'exampleSent.txt'    #on Tympan, what file to send to PC?
fname_to_write_locally = 'exampleReceived.txt' #on the local computer, what file to receive to?
fname_to_read_on_Tympan = fname_to_write_on_Tympan     #on the Tympan, what filename to read from?
receive_success = tympanSerial.receiveFileFromTympan(serial_to_tympan, command_getFileFromTypman, \
                        fname_to_read_on_Tympan,fname_to_write_locally, verbose=verbose)

if (receive_success):
    print("RESULT: File " + fname_to_read_on_Tympan + " received successfully!  Saved here as " + fname_to_write_locally)
else:
    print("RESULT: File " + fname_to_read_on_Tympan + " failed to be received from the Tympan")


# Finally, let's finish up
print(); print("ACTION: Cleaning up...")
serial_to_tympan.close()
print("Result: Serial port closed")

