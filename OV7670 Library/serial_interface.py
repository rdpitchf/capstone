#!/usr/local/bin/python3

import numpy as np
from PIL import Image
import serial
import time
import chardet

def generate_image(array):

    # print(array)
    for a in range(LINES*PIXELS*2):
        array[a] = int.from_bytes(array[a], "big")

    rgb565_1 = np.zeros([LINES*PIXELS,3], dtype=np.uint16)
    rgb565_2 = np.zeros([LINES*PIXELS,3], dtype=np.uint16)

    for a in range(0,LINES*PIXELS*2,2):
        first_byte = array[a]
        second_byte = array[a+1]
        r_565 = first_byte >> 3
        g_565 = ((first_byte & 7) << 3) | ((second_byte & 224) >> 5)
        b_565 = second_byte & 31

        if ((r_565 == 0) and (g_565 == 0) and (b_565 == 0) ):
            print("0 %d ERROR DETECTED ALL ZEROS", a)

        # https://stackoverflow.com/questions/2442576/how-does-one-convert-16-bit-rgb565-to-24-bit-rgb888
        # Both methods show about the same image so they probably do the same thing.
        # Method #1
        r_888_1 = int((r_565 * 255 + 15) / 31)
        g_888_1 = int((g_565 * 255 + 31) / 63)
        b_888_1 = int((b_565 * 255 + 15) / 31)

        # Method #2
        r_888_2 = ( r_565 * 527 + 23 ) >> 6
        g_888_2 = ( g_565 * 259 + 33 ) >> 6
        b_888_2 = ( b_565 * 527 + 23 ) >> 6

        # if ((r_888_1 == 0) and (g_888_1 == 0) and (b_888_1 == 0) ):
        #     print("First all zero 0")
        # if ((r_888_2 == 0) and (g_888_2 == 0) and (b_888_2 == 0) ):
        #     print("Second all zero 0")

        rgb565_1[int(a/2)]=r_888_1,g_888_1,b_888_1
        rgb565_2[int(a/2)]=r_888_2,g_888_2,b_888_2

    format_array_1 = []
    format_array_2 = []
    for a in range(PIXELS):
        line_array_1 = []
        line_array_2 = []
        for i in range(LINES):
            line_array_1.append(rgb565_1[i+a*i])
            line_array_2.append(rgb565_2[i+a*i])
        format_array_1.append(line_array_1)
        format_array_2.append(line_array_2)

    print(len(format_array_1))
    print(len(format_array_1[0]))
    print(len(format_array_1[0][0]))

    rgb888array_1 = np.array(format_array_1, dtype=np.uint16)
    rgb888array_2 = np.array(format_array_2, dtype=np.uint16)
    print("Shape = " + str(rgb888array_1.shape))

    # for a in range(PIXELS):
    #     for b in range(LINES):
    #         if ((rgb888array_1[a][b][0] == 0) and (rgb888array_1[a][b][1] == 0) and (rgb888array_1[a][b][2] == 0) ):
    #             print("1 ERROR DETECTED ALL ZEROS")
    #         if ((rgb888array_2[a][b][0] == 0) and (rgb888array_2[a][b][1] == 0) and (rgb888array_2[a][b][2] == 0) ):
    #             print("2 ERROR DETECTED ALL ZEROS")

    Image.fromarray(rgb888array_1, mode='RGB').save('result_1.png')
    Image.fromarray(rgb888array_2, mode='RGB').save('result_2.png')


    # rgb565array = np.array(array, dtype=np.bytes)
    # h, w = rgb565array.shape
    # rgb888array = np.zeros([h,w,3], dtype=np.uint8)
    # for row in range(h):
    #     for col in range(w):
    #         rgb565 = rgb565array[row,col]
    #         binary = bin(rgb565)
    #
    #         value = int.from_bytes(rgb565, byteorder='big', signed=False)
    #         print(type(binary))
    #         print(binary)
    #
    #         print("Value: " + str(value) + " rgb565: " + str(rgb565))
    #         print(value)
    #         print(rgb565 >> 11)
    #         print((value >> 11) & 0x1f)
    #         r = ((value >> 11 ) & 0x1f ) << 3
    #         # print(r)
    #         g = ((value >> 5  ) & 0x3f ) << 2
    #         b = ((value       ) & 0x1f ) << 3
    #         rgb888array[row,col]=r,g,b
    # Image.fromarray(rgb888array).save('result.png')

ser = serial.Serial('COM3', 115200)  # open serial port
print("Running")

LINES = 640
PIXELS = 480
array = []
line = []

print("Starting loop")
while True:
    a = 0
    ser.write(b's')
    array = []

    for j in range(LINES):
        for i in range(PIXELS):
            array.append(ser.read(1))
            array.append(ser.read(1))
            a += 2
            # b = a%1000
            # if(b == 0):
            #     print(a)

    print("A = " + str(a))
    generate_image(array)
    print("END")
    time.sleep(5)
