# Capstone Project
Programming code related to Engineering 4th Year Design Capstone Project. Using mBed software to run on NUCLEO-L4R5ZI with other peripherals like OV7670 camera and SX1262 RF module.

# OV7670 Library:
## Description:
- OV7670_SCCB.cpp => Is used to read and write to registers on OV7670 i2c. This is working correctly because we can write and read new values from registers.
- AL422_FIFO.cpp => Is used to read data from the AL422 buffer. This has not been confirmed to work (how to determine if data is valid?).
- CAMERA.cpp => Is used to set the settings with OV7670_SCCB.cpp as well as collect image data from AL422_FIFO.cpp
- OV7670_Register.h => Is usedto define the registers that will be used to read and write using OV7670_SCCB.cpp
- main.cpp => Is used to run the files and only print out data from image if an 's' character is sent over serial port.
- serial_interface.py => Is used to receive image data over serial port and save as png. There are a few methods to convert from rgb565 tp rgb888 so different images like result_1.png and result_2.png are created.
## Possible Issues:
- AL422_FIFO.cpp has not been verified whether it is working or not.
- CAMERA.cpp is not setting the right register settings with OV7670_SCCB.cpp
- serial_interface.py could be saving the image incorrectly or converting from rgb565 to rgb888 incorrectly.
