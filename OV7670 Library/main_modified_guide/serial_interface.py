#!/usr/local/bin/python3

import serial
import time

print("Running")
 # open serial port
ser = serial.Serial('COM4', 115200)
print("Running")

LINES = 160
PIXELS = 120
file = open("INPUTFILE.yuv", "wb")

# Take an image
ser.write(b'i')
time.sleep(1)
# ser.write(b'o')
print("Starting loop")
time.sleep(1)
ser.write(b's')
time.sleep(1)
ser.reset_input_buffer()

for j in range(LINES):
    for i in range(PIXELS):
        file.write(ser.read(1))
        file.write(ser.read(1))

print("End loop")
file.close()
