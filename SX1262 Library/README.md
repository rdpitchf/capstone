# SX1262 Library
Information related to SX1262 mbed shield programming.

## Issues:
- When uploading to board, the board will begin to flash LED1 in a pattern to indicate an error.
	- If error message is "pinmap not found for peripheral" this means that one of the pin assignments for radio() is not correct.
	- Each mbed board comes with their own pinmaps.
		- Example: spi uses mosi pin, if assigned a non-mosi pin from the board.
		- Example: AnalogIn uses an analog pin, if assigned a non-analog pin from the board. (Change from PF_7 to PC_5).

# Notes:
- main_sx1262_data_request_example is working with lora.
