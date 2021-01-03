#!/usr/local/bin/python3

import serial
import time
from datetime import datetime
import re
import os

 # open serial port
ser = serial.Serial('COM3', 115200)
print("Running")

# For VGA Raw RGB set Lines = 640 Pixels = 480 (1 Byte)
# For QVGA YUV set Lines = 320 Pixels = 240 (2 Bytes) This appears to be more sensitive with the wires. Have to take multiple pictures in order to get a complete colour bar.
# For QQVGA YUV set Lines = 160 Pixels = 120 (2 Bytes)
VGA = 640*480*1
QVGA = 320*240*2
QQVGA = 160*120*2

ser.write(b'p')
ser.write(b's')
ser.write(b'p')
ser.write(b'c')

while(1):
    while(1):
        read = str(ser.readline())
        print(read)
        if "<<<< START RECORD PICTURE DATA >>>>" in read:
            break

    print("Creating/opening file")
    now = datetime.now()
    date_time = now.strftime("%m_%d_%Y_%H_%M_%S")
    file_name = date_time + ".yuv"
    file = open(file_name, "wb")
    byte_counter = 0

    print("Receiving Data")
    while(1):
        read_line = str(ser.readline())
        print(read_line)
        if "<<<< OUTPUT DATA START " in read_line:
            for i in range(int(re.search(r'\d+', read_line).group())):
                file.write(ser.read(1))
                byte_counter += 1
        elif "<<<< OUTPUT DATA STOP >>>>" in read_line:
            break

    file.close()
    print("Byte counter = %d", byte_counter)
    if byte_counter == QQVGA:
        os.system("ffmpeg -f rawvideo -s 160x120 -pix_fmt yuyv422 -i " + file_name + " -f image2 -vcodec png outputdir\\" + date_time + ".PNG")
        print("PNG Created")

    print("Picture Received From Trial Camera.")
