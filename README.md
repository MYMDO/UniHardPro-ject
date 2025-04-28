## This Universal Hardware Programmer is designed to work with several types of memory chips:

- NAND Flash Memory - Used in SSDs, USB drives, and embedded storage
- SPI Flash Memory - Common in BIOS chips, IoT devices, and microcontrollers
- I2C EEPROM - Frequently used for storing configuration data and small datasets

## The code is written for Arduino-compatible hardware with the following key features:

- Memory Type Selection: Switch between different memory interfaces
- Read Operations: Read device ID, read memory contents, and check status registers
- Write Operations: Write data to specific memory addresses
- Erase Functions: Perform sector, block, or chip erase operations

## Hardware Requirements:

- Arduino board (Uno, Mega, etc.)
- Level shifters if interfacing with 3.3V devices while using a 5V Arduino
- Appropriate socket or connection method for your target chips

## Usage Instructions:

- Upload the code to your Arduino
- Connect to the Arduino via the Serial Monitor (115200 baud)
- Select the appropriate memory type using the menu
- Use the available commands to read, write, or erase data

The programmer handles the specific protocol details for each memory type, including timing requirements and command sequences. You can easily extend it to support additional memory types or customize the pin assignments to match your hardware setup.
