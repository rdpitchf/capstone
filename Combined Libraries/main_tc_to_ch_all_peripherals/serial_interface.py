#!/usr/local/bin/python3

import serial
import time
from datetime import datetime
import re
import os
import json


def check_for_negative(integer_value, size):
    d = bin(integer_value)
    a = str(d)
    data = 0
    # Reverse ones compliment
    if len(a) == (size + 2) and a[3] == "1":
        for i in a[3:]:
            if i is "1":
                data = data*2
            else:
                data = data*2 + 1
        aa = ~(data)
        return aa
    return integer_value
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

date_time = None

while(1):
    status = 0
    while(1):
        read = str(ser.readline())
        print(read)
        if "<<<< START RECORD PICTURE DATA >>>>" in read:
            status = 0
            break
        if "<<<< OUTPUT POSITION TIME DATE 23 Bytes >>>>" in read:
            status = 1
            break
    if status == 1:
        cam_id = (int.from_bytes(ser.read(1), "little")) + (int.from_bytes(ser.read(1), "little") * (2**8))
        bat_en = (int.from_bytes(ser.read(1), "little"))
        bat_status = (int.from_bytes(ser.read(1), "little"))
        picture_size = (int.from_bytes(ser.read(1), "little"))

        lat = (int.from_bytes(ser.read(1), "little")) + (int.from_bytes(ser.read(1), "little") * (2**8)) + (int.from_bytes(ser.read(1), "little") * (2**16)) + (int.from_bytes(ser.read(1), "little") * (2**24))
        lat = check_for_negative(lat,32)
        lon = (int.from_bytes(ser.read(1), "little")) + (int.from_bytes(ser.read(1), "little") * (2**8)) + (int.from_bytes(ser.read(1), "little") * (2**16)) + (int.from_bytes(ser.read(1), "little") * (2**24))
        lon = check_for_negative(lon,32)
        alt = (int.from_bytes(ser.read(1), "little")) + (int.from_bytes(ser.read(1), "little") * (2**8)) + (int.from_bytes(ser.read(1), "little") * (2**16)) + (int.from_bytes(ser.read(1), "little") * (2**24))
        alt = check_for_negative(alt,32)
        tow = (int.from_bytes(ser.read(1), "little")) + (int.from_bytes(ser.read(1), "little") * (2**8)) + (int.from_bytes(ser.read(1), "little") * (2**16)) + (int.from_bytes(ser.read(1), "little") * (2**24))
        Year = int.from_bytes(ser.read(1), "little") + (int.from_bytes(ser.read(1), "little") * (2**8))
        Month = int.from_bytes(ser.read(1), "little")
        Day = int.from_bytes(ser.read(1), "little")
        Hour = int.from_bytes(ser.read(1), "little")
        Minute = int.from_bytes(ser.read(1), "little")
        Second = int.from_bytes(ser.read(1), "little")

        lora = int.from_bytes(ser.read(1), "little")
        bw = (int.from_bytes(ser.read(1), "little")) + (int.from_bytes(ser.read(1), "little") * (2**8)) + (int.from_bytes(ser.read(1), "little") * (2**16)) + (int.from_bytes(ser.read(1), "little") * (2**24))
        bps = (int.from_bytes(ser.read(1), "little")) + (int.from_bytes(ser.read(1), "little") * (2**8)) + (int.from_bytes(ser.read(1), "little") * (2**16)) + (int.from_bytes(ser.read(1), "little") * (2**24))
        f_dev = (int.from_bytes(ser.read(1), "little")) + (int.from_bytes(ser.read(1), "little") * (2**8)) + (int.from_bytes(ser.read(1), "little") * (2**16)) + (int.from_bytes(ser.read(1), "little") * (2**24))
        tx_p = int.from_bytes(ser.read(1), "little")
        central_hub_id = int.from_bytes(ser.read(1), "little")

        now = datetime.now()
        date_time = now.strftime("%m_%d_%Y_%H_%M_%S")
        file_name = "outputdir/" + date_time + ".txt"
        file = open(file_name, "w")
        json_message = {
            "Trail Camera":{
                "Position": {
                    "Latitude (degrees)":lat,
                    "Longitude (degrees)":lon,
                    "Altitude":alt,
                },
                "Date":{
                    "Year":Year,
                    "Month":Month,
                    "Day":Day,
                },
                "Time":{
                    "Hour":Hour,
                    "Minute":Minute,
                    "Second":Second,
                },
                "Battery Enable":bat_en,
                "Battery Level (%)":bat_status,
                "Camera ID":cam_id,
                "Lora":lora,
                "Bandwidth":bw,
                "BPS":bps,
                "Frequency Deviation":f_dev,
                "TX Power":tx_p,
            },
            "Central Hub":{
                "Central Hub ID":central_hub_id,
            }
        }
        y = json.dumps(json_message)
        file.write(y)
        file.close()
        print("GPS: ", lat, lon, alt, tow, Year, Month, Day, Hour, Minute, Second)

    else:
        print("Creating/opening file")

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

        print("Picture Received From Trial Camer")
