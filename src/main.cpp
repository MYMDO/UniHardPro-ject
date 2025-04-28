/**
 * Universal Hardware Programmer
 * 
 * Supports:
 * - NAND Flash
 * - SPI Flash
 * - I2C EEPROM
 * 
 * Designed to run on Arduino-compatible hardware with appropriate level shifters
 * for interfacing with various memory chips (3.3V/5V logic).
 */

 #include <Arduino.h>
 #include <SPI.h>
 #include <Wire.h>
 
 // Define pin configurations
 #define SPI_CS_PIN      10  // SPI Chip Select
 #define NAND_CLE_PIN    A0  // NAND Command Latch Enable
 #define NAND_ALE_PIN    A1  // NAND Address Latch Enable
 #define NAND_WE_PIN     A2  // NAND Write Enable
 #define NAND_RE_PIN     A3  // NAND Read Enable
 #define NAND_CE_PIN     A4  // NAND Chip Enable
 #define NAND_RB_PIN     A5  // NAND Ready/Busy
 
 // Debug settings
 #define DEBUG_MODE      1   // Set to 0 to disable debug messages
 #define SERIAL_BAUD     115200
 
 // Commands for SPI Flash
 #define SPI_CMD_WRITE_ENABLE      0x06
 #define SPI_CMD_WRITE_DISABLE     0x04
 #define SPI_CMD_READ_STATUS       0x05
 #define SPI_CMD_WRITE_STATUS      0x01
 #define SPI_CMD_READ_DATA         0x03
 #define SPI_CMD_FAST_READ         0x0B
 #define SPI_CMD_PAGE_PROGRAM      0x02
 #define SPI_CMD_SECTOR_ERASE      0x20
 #define SPI_CMD_BLOCK_ERASE_32K   0x52
 #define SPI_CMD_BLOCK_ERASE_64K   0xD8
 #define SPI_CMD_CHIP_ERASE        0xC7
 #define SPI_CMD_READ_ID           0x9F
 
 // Common NAND commands
 #define NAND_CMD_READ_ID          0x90
 #define NAND_CMD_READ_STATUS      0x70
 #define NAND_CMD_READ             0x00
 #define NAND_CMD_READ_CONFIRM     0x30
 #define NAND_CMD_PROGRAM          0x80
 #define NAND_CMD_PROGRAM_CONFIRM  0x10
 #define NAND_CMD_ERASE            0x60
 #define NAND_CMD_ERASE_CONFIRM    0xD0
 #define NAND_CMD_RESET            0xFF
 
 // Memory interface types
 enum MemoryType {
   MEM_UNKNOWN,
   MEM_NAND_FLASH,
   MEM_SPI_FLASH,
   MEM_I2C_EEPROM
 };
 
 // Global variables
 MemoryType currentMemoryType = MEM_UNKNOWN;
 byte i2cAddress = 0x50;  // Default I2C EEPROM address

 // Function prototypes
 void hexDump(byte (*readFunc)(), unsigned int numBytes);
 void printMenu();
 void handleCommand(char cmd);
 void setMemoryType(MemoryType type);
 void readDeviceID();
 void nandReadID();
 void spiReadID();
 void identifySPIFlash(byte manufacturerID, byte deviceID1, byte deviceID2);
 void i2cDetect();
 void readData();
 void nandReadData(unsigned long address, unsigned int numBytes);
 void spiReadData(unsigned long address, unsigned int numBytes);
 void i2cReadData(unsigned long address, unsigned int numBytes);
 void writeData();
 void nandWriteData(unsigned long address, byte* data, unsigned int numBytes);
 void spiWriteData(unsigned long address, byte* data, unsigned int numBytes);
 void spiWritePage(unsigned long address, byte* data, unsigned int numBytes);
 void i2cWriteData(unsigned long address, byte* data, unsigned int numBytes);
 void eraseMemory();
 void nandErase(char option, unsigned long address);
 void spiErase(char option, unsigned long address);
 void i2cErase(char option, unsigned long address);
 void readStatus();
 void nandReadStatus();
 void spiReadStatus();
 void i2cReadStatus();
 byte nandReadByte();
 void nandWriteByte(byte data);
 void nandReset();
 void waitForNandReady();
 bool waitForSpiReady();
 void setI2CAddress();
 unsigned long readHexValue();
 unsigned int readDecValue();
 
 void setup() {
   // Initialize serial communication
   Serial.begin(SERIAL_BAUD);
   while (!Serial && millis() < 3000); // Wait for serial port to connect (max 3 seconds)
   
   Serial.println(F("\nUniversal Hardware Programmer"));
   Serial.println(F("v1.0 - NAND/SPI/I2C Memory"));
   
   // Configure SPI
   SPI.begin();
   pinMode(SPI_CS_PIN, OUTPUT);
   digitalWrite(SPI_CS_PIN, HIGH);
   
   // Configure NAND pins
   pinMode(NAND_CLE_PIN, OUTPUT);
   pinMode(NAND_ALE_PIN, OUTPUT);
   pinMode(NAND_WE_PIN, OUTPUT);
   pinMode(NAND_RE_PIN, OUTPUT);
   pinMode(NAND_CE_PIN, OUTPUT);
   pinMode(NAND_RB_PIN, INPUT);
   
   // Initialize pins to safe state
   digitalWrite(NAND_CLE_PIN, LOW);
   digitalWrite(NAND_ALE_PIN, LOW);
   digitalWrite(NAND_WE_PIN, HIGH);
   digitalWrite(NAND_RE_PIN, HIGH);
   digitalWrite(NAND_CE_PIN, HIGH);
   
   // Initialize I2C
   Wire.begin();
   
   Serial.println(F("Hardware initialized\n"));
   printMenu();
 }
 
 void loop() {
   if (Serial.available()) {
     char cmd = Serial.read();
     handleCommand(cmd);
   }
 }
 
 void printMenu() {
   Serial.println(F("==== COMMANDS ===="));
   Serial.println(F("1: Set NAND Flash mode"));
   Serial.println(F("2: Set SPI Flash mode"));
   Serial.println(F("3: Set I2C EEPROM mode"));
   Serial.println(F("i: Read device ID"));
   Serial.println(F("r: Read data"));
   Serial.println(F("w: Write data"));
   Serial.println(F("e: Erase"));
   Serial.println(F("s: Read status"));
   Serial.println(F("a: Set I2C address (EEPROM mode)"));
   Serial.println(F("h: Show this menu"));
   Serial.println();
 }
 
 void handleCommand(char cmd) {
   switch (cmd) {
     case '1':
       setMemoryType(MEM_NAND_FLASH);
       break;
     case '2':
       setMemoryType(MEM_SPI_FLASH);
       break;
     case '3':
       setMemoryType(MEM_I2C_EEPROM);
       break;
     case 'i':
       readDeviceID();
       break;
     case 'r':
       readData();
       break;
     case 'w':
       writeData();
       break;
     case 'e':
       eraseMemory();
       break;
     case 's':
       readStatus();
       break;
     case 'a':
       setI2CAddress();
       break;
     case 'h':
       printMenu();
       break;
     case '\n':
     case '\r':
       // Ignore newlines
       break;
     default:
       Serial.println(F("Unknown command. Type 'h' for help."));
   }
 }
 
 void setMemoryType(MemoryType type) {
   currentMemoryType = type;
   
   // Initialize the selected interface
   switch (type) {
     case MEM_NAND_FLASH:
       Serial.println(F("NAND Flash mode selected"));
       nandReset();
       break;
     case MEM_SPI_FLASH:
       Serial.println(F("SPI Flash mode selected"));
       break;
     case MEM_I2C_EEPROM:
       Serial.println(F("I2C EEPROM mode selected"));
       Serial.print(F("Current I2C address: 0x"));
       Serial.println(i2cAddress, HEX);
       break;
     default:
       Serial.println(F("Unknown memory type!"));
   }
 }
 
 // ===== DEVICE ID FUNCTIONS =====
 
 void readDeviceID() {
   if (currentMemoryType == MEM_UNKNOWN) {
     Serial.println(F("Please select memory type first!"));
     return;
   }
   
   Serial.println(F("Reading device ID..."));
   
   switch (currentMemoryType) {
     case MEM_NAND_FLASH:
       nandReadID();
       break;
     case MEM_SPI_FLASH:
       spiReadID();
       break;
     case MEM_I2C_EEPROM:
       i2cDetect();
       break;
     default:
       Serial.println(F("Unknown memory type!"));
   }
 }
 
 void nandReadID() {
   // Select the chip
   digitalWrite(NAND_CE_PIN, LOW);
   
   // Send READ ID command (0x90)
   digitalWrite(NAND_CLE_PIN, HIGH);
   nandWriteByte(NAND_CMD_READ_ID);
   digitalWrite(NAND_CLE_PIN, LOW);
   
   // Send address 0x00
   digitalWrite(NAND_ALE_PIN, HIGH);
   nandWriteByte(0x00);
   digitalWrite(NAND_ALE_PIN, LOW);
   
   // Read ID bytes (typically 5 bytes)
   Serial.print(F("Manufacturer ID: 0x"));
   Serial.println(nandReadByte(), HEX);
   
   Serial.print(F("Device ID: 0x"));
   Serial.println(nandReadByte(), HEX);
   
   Serial.print(F("Third ID byte: 0x"));
   Serial.println(nandReadByte(), HEX);
   
   Serial.print(F("Fourth ID byte: 0x"));
   Serial.println(nandReadByte(), HEX);
   
   Serial.print(F("Fifth ID byte: 0x"));
   Serial.println(nandReadByte(), HEX);
   
   // Deselect the chip
   digitalWrite(NAND_CE_PIN, HIGH);
 }
 
 void spiReadID() {
   digitalWrite(SPI_CS_PIN, LOW);
   SPI.transfer(SPI_CMD_READ_ID);  // JEDEC ID command
   
   byte manufacturerID = SPI.transfer(0);
   byte deviceID1 = SPI.transfer(0);
   byte deviceID2 = SPI.transfer(0);
   
   digitalWrite(SPI_CS_PIN, HIGH);
   
   Serial.print(F("Manufacturer ID: 0x"));
   Serial.println(manufacturerID, HEX);
   
   Serial.print(F("Device ID: 0x"));
   Serial.print(deviceID1, HEX);
   Serial.println(deviceID2, HEX);
   
   // Try to identify common chips
   identifySPIFlash(manufacturerID, deviceID1, deviceID2);
 }
 
 void identifySPIFlash(byte manufacturerID, byte deviceID1, byte deviceID2) {
   Serial.print(F("Device: "));
   
   switch (manufacturerID) {
     case 0x01:  // Spansion/Cypress
       Serial.print(F("Spansion/Cypress "));
       break;
     case 0x20:  // Micron/Numonyx/ST
       Serial.print(F("Micron/ST "));
       break;
     case 0xEF:  // Winbond
       Serial.print(F("Winbond "));
       if (deviceID1 == 0x40) {
         if (deviceID2 == 0x14) Serial.println(F("W25Q80 (8Mbit)"));
         else if (deviceID2 == 0x15) Serial.println(F("W25Q16 (16Mbit)"));
         else if (deviceID2 == 0x16) Serial.println(F("W25Q32 (32Mbit)"));
         else if (deviceID2 == 0x17) Serial.println(F("W25Q64 (64Mbit)"));
         else if (deviceID2 == 0x18) Serial.println(F("W25Q128 (128Mbit)"));
         else Serial.println(F("Unknown W25Q series"));
       } else {
         Serial.println(F("Unknown model"));
       }
       break;
     case 0xC2:  // Macronix
       Serial.print(F("Macronix "));
       break;
     case 0xBF:  // SST
       Serial.print(F("SST "));
       break;
     default:
       Serial.println(F("Unknown manufacturer"));
   }
 }
 
 void i2cDetect() {
   Serial.println(F("Scanning I2C bus for devices..."));
   
   byte count = 0;
   for (byte addr = 0x08; addr < 0x78; addr++) {
     Wire.beginTransmission(addr);
     byte error = Wire.endTransmission();
     
     if (error == 0) {
       Serial.print(F("Device found at address 0x"));
       if (addr < 16) Serial.print("0");
       Serial.print(addr, HEX);
       
       // Check if it's likely an EEPROM (most common at 0x50-0x57)
       if (addr >= 0x50 && addr <= 0x57) {
         Serial.println(F(" (likely EEPROM)"));
       } else {
         Serial.println();
       }
       count++;
     }
   }
   
   if (count == 0) {
     Serial.println(F("No I2C devices found!"));
   }
 }
 
 // ===== DATA READ/WRITE FUNCTIONS =====
 
 void readData() {
   if (currentMemoryType == MEM_UNKNOWN) {
     Serial.println(F("Please select memory type first!"));
     return;
   }
   
   Serial.println(F("Enter start address (in hex):"));
   unsigned long startAddr = readHexValue();
   
   Serial.println(F("Enter number of bytes to read:"));
   unsigned int numBytes = readDecValue();
   
   if (numBytes > 256) {
     Serial.println(F("Warning: Limiting to 256 bytes"));
     numBytes = 256;
   }
   
   Serial.print(F("Reading "));
   Serial.print(numBytes);
   Serial.print(F(" bytes from address 0x"));
   Serial.println(startAddr, HEX);
   
   switch (currentMemoryType) {
     case MEM_NAND_FLASH:
       nandReadData(startAddr, numBytes);
       break;
     case MEM_SPI_FLASH:
       spiReadData(startAddr, numBytes);
       break;
     case MEM_I2C_EEPROM:
       i2cReadData(startAddr, numBytes);
       break;
     default:
       Serial.println(F("Unknown memory type!"));
   }
 }
 
 void nandReadData(unsigned long address, unsigned int numBytes) {
   // Basic implementation for small page NAND
   // For modern NAND, more complex ECC would be needed
   
   unsigned int pageSize = 512;  // Adjust based on your NAND flash
   unsigned int page = address / pageSize;
   unsigned int offset = address % pageSize;
   
   // Select the chip
   digitalWrite(NAND_CE_PIN, LOW);
   
   // Send READ command
   digitalWrite(NAND_CLE_PIN, HIGH);
   nandWriteByte(NAND_CMD_READ);
   digitalWrite(NAND_CLE_PIN, LOW);
   
   // Send address bytes
   digitalWrite(NAND_ALE_PIN, HIGH);
   nandWriteByte(offset & 0xFF);        // Column address low byte
   nandWriteByte((offset >> 8) & 0xFF); // Column address high byte (if needed)
   nandWriteByte(page & 0xFF);          // Page address low byte
   nandWriteByte((page >> 8) & 0xFF);   // Page address high byte
   nandWriteByte((page >> 16) & 0xFF);  // Page address highest byte (if needed)
   digitalWrite(NAND_ALE_PIN, LOW);
   
   // Send READ confirm command
   digitalWrite(NAND_CLE_PIN, HIGH);
   nandWriteByte(NAND_CMD_READ_CONFIRM);
   digitalWrite(NAND_CLE_PIN, LOW);
   
   // Wait for the device to be ready
   waitForNandReady();
   
   // Read and display data
   hexDump(nandReadByte, numBytes);
   
   // Deselect the chip
   digitalWrite(NAND_CE_PIN, HIGH);
 }
 
 void spiReadData(unsigned long address, unsigned int numBytes) {
   digitalWrite(SPI_CS_PIN, LOW);
   
   // Send Fast Read command
   SPI.transfer(SPI_CMD_FAST_READ);
   
   // Send 24-bit address (MSB first)
   SPI.transfer((address >> 16) & 0xFF);
   SPI.transfer((address >> 8) & 0xFF);
   SPI.transfer(address & 0xFF);
   
   // Dummy byte for fast read
   SPI.transfer(0);
   
   // Read and display data
   byte buffer[16];
   for (unsigned int i = 0; i < numBytes; i++) {
     if (i % 16 == 0) {
       // Print address at start of line
       if (i > 0) {
         Serial.println();
       }
       Serial.print("0x");
       if ((address + i) < 0x1000) Serial.print("0");
       if ((address + i) < 0x100) Serial.print("0");
       if ((address + i) < 0x10) Serial.print("0");
       Serial.print(address + i, HEX);
       Serial.print(": ");
     }
     
     // Read byte
     buffer[i % 16] = SPI.transfer(0);
     
     // Print hex value
     if (buffer[i % 16] < 0x10) Serial.print("0");
     Serial.print(buffer[i % 16], HEX);
     Serial.print(" ");
     
     // Print ASCII representation at end of line
     if ((i % 16 == 15) || (i == numBytes - 1)) {
       // Pad with spaces if not a full line
       for (int j = (i % 16) + 1; j < 16; j++) {
         Serial.print("   ");
       }
       
       Serial.print(" | ");
       
       // Print ASCII chars if printable
       for (int j = 0; j <= (i % 16); j++) {
         if (buffer[j] >= 32 && buffer[j] <= 126) {
           Serial.write(buffer[j]);
         } else {
           Serial.print(".");
         }
       }
     }
   }
   
   digitalWrite(SPI_CS_PIN, HIGH);
   Serial.println();
 }
 
 void i2cReadData(unsigned long address, unsigned int numBytes) {
   // Check if device is present
   Wire.beginTransmission(i2cAddress);
   byte error = Wire.endTransmission();
   
   if (error != 0) {
     Serial.print(F("Error: Device at address 0x"));
     Serial.print(i2cAddress, HEX);
     Serial.println(F(" not responding"));
     return;
   }
   
   // For larger EEPROMs, handle 16-bit addressing
   bool use16bitAddr = (address > 0xFF); 
   
   // Read data in chunks (Wire library typically has a 32-byte buffer)
   const byte chunkSize = 16;
   byte buffer[chunkSize];
   
   for (unsigned int i = 0; i < numBytes; i += chunkSize) {
     unsigned int bytesToRead = min(chunkSize, numBytes - i);
     unsigned long currentAddr = address + i;
     
     // Start write operation to set address pointer in EEPROM
     Wire.beginTransmission(i2cAddress);
     
     if (use16bitAddr) {
       Wire.write((currentAddr >> 8) & 0xFF); // MSB
     }
     
     Wire.write(currentAddr & 0xFF); // LSB
     Wire.endTransmission();
     
     // Read data
     Wire.requestFrom(i2cAddress, bytesToRead);
     
     for (unsigned int j = 0; j < bytesToRead && Wire.available(); j++) {
       buffer[j] = Wire.read();
     }
     
     // Print data
     for (unsigned int j = 0; j < bytesToRead; j++) {
       if ((i + j) % 16 == 0) {
         if (i + j > 0) Serial.println();
         Serial.print("0x");
         if ((address + i + j) < 0x1000) Serial.print("0");
         if ((address + i + j) < 0x100) Serial.print("0");
         if ((address + i + j) < 0x10) Serial.print("0");
         Serial.print(address + i + j, HEX);
         Serial.print(": ");
       }
       
       if (buffer[j] < 0x10) Serial.print("0");
       Serial.print(buffer[j], HEX);
       Serial.print(" ");
       
       if ((i + j) % 16 == 15 || (i + j == numBytes - 1)) {
         // Pad for alignment
         for (int k = ((i + j) % 16) + 1; k < 16; k++) {
           Serial.print("   ");
         }
         
         Serial.print(" | ");
         
         // Print ASCII chars
         for (int k = 0; k <= ((i + j) % 16); k++) {
           if (k < bytesToRead) {
             if (buffer[k] >= 32 && buffer[k] <= 126) {
               Serial.write(buffer[k]);
             } else {
               Serial.print(".");
             }
           }
         }
       }
     }
   }
   
   Serial.println();
 }
 
 void writeData() {
   if (currentMemoryType == MEM_UNKNOWN) {
     Serial.println(F("Please select memory type first!"));
     return;
   }
   
   Serial.println(F("Enter start address (in hex):"));
   unsigned long startAddr = readHexValue();
   
   Serial.println(F("Enter data (hex bytes separated by spaces, max 32 bytes):"));
   
   // Wait for input
   while (!Serial.available()) {
     delay(100);
   }
   
   // Parse hex bytes from input
   String input = Serial.readStringUntil('\n');
   input.trim();
   
   // Convert string to bytes
   byte data[32];
   unsigned int numBytes = 0;
   
   char* token = strtok((char*)input.c_str(), " ,");
   while (token != NULL && numBytes < 32) {
     // Convert hex string to byte
     data[numBytes++] = strtol(token, NULL, 16);
     token = strtok(NULL, " ,");
   }
   
   Serial.print(F("Writing "));
   Serial.print(numBytes);
   Serial.print(F(" bytes to address 0x"));
   Serial.println(startAddr, HEX);
   
   switch (currentMemoryType) {
     case MEM_NAND_FLASH:
       nandWriteData(startAddr, data, numBytes);
       break;
     case MEM_SPI_FLASH:
       spiWriteData(startAddr, data, numBytes);
       break;
     case MEM_I2C_EEPROM:
       i2cWriteData(startAddr, data, numBytes);
       break;
     default:
       Serial.println(F("Unknown memory type!"));
   }
 }
 
 void nandWriteData(unsigned long address, byte* data, unsigned int numBytes) {
   // Basic implementation for small page NAND
   unsigned int pageSize = 512;  // Adjust based on your NAND flash
   unsigned int page = address / pageSize;
   unsigned int offset = address % pageSize;
   
   // Check if write crosses page boundary
   if (offset + numBytes > pageSize) {
     Serial.println(F("Error: Write crosses page boundary!"));
     return;
   }
   
   // Select the chip
   digitalWrite(NAND_CE_PIN, LOW);
   
   // Send PROGRAM command
   digitalWrite(NAND_CLE_PIN, HIGH);
   nandWriteByte(NAND_CMD_PROGRAM);
   digitalWrite(NAND_CLE_PIN, LOW);
   
   // Send address bytes
   digitalWrite(NAND_ALE_PIN, HIGH);
   nandWriteByte(offset & 0xFF);        // Column address low byte
   nandWriteByte((offset >> 8) & 0xFF); // Column address high byte (if needed)
   nandWriteByte(page & 0xFF);          // Page address low byte
   nandWriteByte((page >> 8) & 0xFF);   // Page address high byte
   nandWriteByte((page >> 16) & 0xFF);  // Page address highest byte (if needed)
   digitalWrite(NAND_ALE_PIN, LOW);
   
   // Write data
   for (unsigned int i = 0; i < numBytes; i++) {
     nandWriteByte(data[i]);
   }
   
   // Send PROGRAM confirm command
   digitalWrite(NAND_CLE_PIN, HIGH);
   nandWriteByte(NAND_CMD_PROGRAM_CONFIRM);
   digitalWrite(NAND_CLE_PIN, LOW);
   
   // Wait for the device to be ready
   waitForNandReady();
   
   // Check for program status
   digitalWrite(NAND_CLE_PIN, HIGH);
   nandWriteByte(NAND_CMD_READ_STATUS);
   digitalWrite(NAND_CLE_PIN, LOW);
   
   byte status = nandReadByte();
   
   // Deselect the chip
   digitalWrite(NAND_CE_PIN, HIGH);
   
   // Check if program was successful (bit 0 should be 0)
   if (status & 0x01) {
     Serial.println(F("Program failed!"));
   } else {
     Serial.println(F("Program successful"));
   }
 }
 
 void spiWriteData(unsigned long address, byte* data, unsigned int numBytes) {
   // Check if we're crossing page boundary (typically 256 bytes)
   unsigned int pageSize = 256;
   unsigned int offset = address % pageSize;
   
   if (offset + numBytes > pageSize) {
     Serial.println(F("Warning: Write crosses page boundary!"));
     
     // Calculate bytes that fit in first page
     unsigned int firstPageBytes = pageSize - offset;
     
     // Write first page
     spiWritePage(address, data, firstPageBytes);
     
     // Write remaining bytes in next page
     spiWritePage(address + firstPageBytes, data + firstPageBytes, numBytes - firstPageBytes);
   } else {
     // Write fits in a single page
     spiWritePage(address, data, numBytes);
   }
 }
 
 void spiWritePage(unsigned long address, byte* data, unsigned int numBytes) {
   // Enable write operations
   digitalWrite(SPI_CS_PIN, LOW);
   SPI.transfer(SPI_CMD_WRITE_ENABLE);
   digitalWrite(SPI_CS_PIN, HIGH);
   delay(1);
   
   // Send Page Program command
   digitalWrite(SPI_CS_PIN, LOW);
   SPI.transfer(SPI_CMD_PAGE_PROGRAM);
   
   // Send 24-bit address (MSB first)
   SPI.transfer((address >> 16) & 0xFF);
   SPI.transfer((address >> 8) & 0xFF);
   SPI.transfer(address & 0xFF);
   
   // Send data
   for (unsigned int i = 0; i < numBytes; i++) {
     SPI.transfer(data[i]);
   }
   
   digitalWrite(SPI_CS_PIN, HIGH);
   
   // Wait for write to complete
   waitForSpiReady();
   
   Serial.println(F("Write complete"));
 }
 
 void i2cWriteData(unsigned long address, byte* data, unsigned int numBytes) {
   // Check if device is present
   Wire.beginTransmission(i2cAddress);
   byte error = Wire.endTransmission();
   
   if (error != 0) {
     Serial.print(F("Error: Device at address 0x"));
     Serial.print(i2cAddress, HEX);
     Serial.println(F(" not responding"));
     return;
   }
   
   // For larger EEPROMs, handle 16-bit addressing
   bool use16bitAddr = (address > 0xFF);
   
   // EEPROM page size (typically 8, 16, 32, or 64 bytes)
   unsigned int pageSize = 8;  // Adjust based on your EEPROM
   
   // Write data in page-sized chunks (or smaller)
   unsigned int bytesWritten = 0;
   
   while (bytesWritten < numBytes) {
     // Calculate current address and offset within page
     unsigned long currentAddr = address + bytesWritten;
     unsigned int pageOffset = currentAddr % pageSize;
     
     // Calculate bytes to write in this page (don't cross page boundary)
     unsigned int bytesToWrite = min(pageSize - pageOffset, numBytes - bytesWritten);
     
     // Begin write operation
     Wire.beginTransmission(i2cAddress);
     
     // Write address bytes
     if (use16bitAddr) {
       Wire.write((currentAddr >> 8) & 0xFF); // MSB
     }
     Wire.write(currentAddr & 0xFF); // LSB
     
     // Write data bytes
     for (unsigned int i = 0; i < bytesToWrite; i++) {
       Wire.write(data[bytesWritten + i]);
     }
     
     // Send data to device
     Wire.endTransmission();
     
     // Wait for write cycle to complete (typically 5ms)
     delay(5);
     
     bytesWritten += bytesToWrite;
   }
   
   Serial.println(F("Write complete"));
 }
 
 // ===== ERASE FUNCTIONS =====
 
 void eraseMemory() {
   if (currentMemoryType == MEM_UNKNOWN) {
     Serial.println(F("Please select memory type first!"));
     return;
   }
   
   Serial.println(F("Erase options:"));
   Serial.println(F("1. Sector erase"));
   Serial.println(F("2. Block erase"));
   Serial.println(F("3. Chip erase"));
   
   while (!Serial.available()) {
     delay(100);
   }
   
   char option = Serial.read();
   
   unsigned long address = 0;
   
   if (option == '1' || option == '2') {
     Serial.println(F("Enter start address (in hex):"));
     address = readHexValue();
   }

   String confirmation = ""; // Declare outside the switch to avoid bypassing initialization
   switch (option) {
     case '1':
       Serial.print(F("Erasing sector at 0x"));
       Serial.println(address, HEX);
       break;
     case '2':
       Serial.print(F("Erasing block at 0x"));
       Serial.println(address, HEX);
       break;
     case '3':
       Serial.println(F("WARNING: This will erase the entire chip!"));
       Serial.println(F("Type 'YES' to confirm:"));
       
       while (!Serial.available()) {
         delay(100);
       }
       
       confirmation = Serial.readStringUntil('\n');
       confirmation.trim();
       
       if (confirmation != "YES") {
         Serial.println(F("Erase aborted!"));
         return;
       }
       
       Serial.println(F("Erasing entire chip..."));
       break;
     default:
       Serial.println(F("Invalid option"));
       return;
   }
   
   switch (currentMemoryType) {
     case MEM_NAND_FLASH:
       nandErase(option, address);
       break;
     case MEM_SPI_FLASH:
       spiErase(option, address);
       break;
     case MEM_I2C_EEPROM:
       i2cErase(option, address);
       break;
    default:
      Serial.println(F("Unknown memory type!"));
  }
}

void nandErase(char option, unsigned long address) {
  // Select the chip
  digitalWrite(NAND_CE_PIN, LOW);
  
  // Send ERASE command
  digitalWrite(NAND_CLE_PIN, HIGH);
  nandWriteByte(NAND_CMD_ERASE);
  digitalWrite(NAND_CLE_PIN, LOW);
  
  if (option == '1' || option == '2') {
    // For NAND, we erase blocks (no sector erase)
    unsigned int blockSize = 16 * 1024; // 16KB blocks, adjust as needed
    unsigned int block = address / blockSize;
    
    // Send row address for the block
    digitalWrite(NAND_ALE_PIN, HIGH);
    nandWriteByte(block & 0xFF);          // Block address low byte
    nandWriteByte((block >> 8) & 0xFF);   // Block address high byte
    nandWriteByte((block >> 16) & 0xFF);  // Block address highest byte (if needed)
    digitalWrite(NAND_ALE_PIN, LOW);
  }
  
  // Send ERASE confirm command
  digitalWrite(NAND_CLE_PIN, HIGH);
  nandWriteByte(NAND_CMD_ERASE_CONFIRM);
  digitalWrite(NAND_CLE_PIN, LOW);
  
  // Wait for the device to be ready
  waitForNandReady();
  
  // Check for erase status
  digitalWrite(NAND_CLE_PIN, HIGH);
  nandWriteByte(NAND_CMD_READ_STATUS);
  digitalWrite(NAND_CLE_PIN, LOW);
  
  byte status = nandReadByte();
  
  // Deselect the chip
  digitalWrite(NAND_CE_PIN, HIGH);
  
  // Check if erase was successful (bit 0 should be 0)
  if (status & 0x01) {
    Serial.println(F("Erase failed!"));
  } else {
    Serial.println(F("Erase successful"));
  }
}

void spiErase(char option, unsigned long address) {
  // Enable write operations
  digitalWrite(SPI_CS_PIN, LOW);
  SPI.transfer(SPI_CMD_WRITE_ENABLE);
  digitalWrite(SPI_CS_PIN, HIGH);
  delay(1);
  
  digitalWrite(SPI_CS_PIN, LOW);
  
  switch (option) {
    case '1': // Sector erase (4KB)
      SPI.transfer(SPI_CMD_SECTOR_ERASE);
      SPI.transfer((address >> 16) & 0xFF);
      SPI.transfer((address >> 8) & 0xFF);
      SPI.transfer(address & 0xFF);
      break;
    case '2': // Block erase (64KB)
      SPI.transfer(SPI_CMD_BLOCK_ERASE_64K);
      SPI.transfer((address >> 16) & 0xFF);
      SPI.transfer((address >> 8) & 0xFF);
      SPI.transfer(address & 0xFF);
      break;
    case '3': // Chip erase
      SPI.transfer(SPI_CMD_CHIP_ERASE);
      break;
  }
  
  digitalWrite(SPI_CS_PIN, HIGH);
  
  // Wait for erase to complete
  unsigned long startTime = millis();
  Serial.print(F("Erasing"));
  
  while (waitForSpiReady()) {
    if (millis() - startTime > 500) {
      Serial.print(".");
      startTime = millis();
    }
  }
  
  Serial.println(F("\nErase complete"));
}

void i2cErase(char option, unsigned long address) {
  if (option == '3') {
    // Chip erase for EEPROM - fill with 0xFF
    unsigned int pageSize = 8;  // Adjust based on your EEPROM
    unsigned int maxSize = 32 * 1024; // Maximum size to erase (32KB)
    
    byte eraseData[8]; // Page-sized buffer of 0xFF
    memset(eraseData, 0xFF, sizeof(eraseData));
    
    Serial.print(F("Erasing EEPROM"));
    
    for (unsigned int i = 0; i < maxSize; i += pageSize) {
      i2cWriteData(i, eraseData, pageSize);
      
      if (i % 256 == 0) {
        Serial.print(".");
      }
    }
    
    Serial.println(F("\nErase complete"));
  } else {
    // Sector/Block erase - For EEPROM, we're just writing 0xFF
    unsigned int eraseSize = (option == '1') ? 256 : 4096; // Sector: 256B, Block: 4KB
    
    byte eraseData[8]; // Buffer of 0xFF
    memset(eraseData, 0xFF, sizeof(eraseData));
    
    Serial.print(F("Erasing"));
    
    for (unsigned int i = 0; i < eraseSize; i += sizeof(eraseData)) {
      i2cWriteData(address + i, eraseData, sizeof(eraseData));
      
      if (i % 64 == 0) {
        Serial.print(".");
      }
    }
    
    Serial.println(F("\nErase complete"));
  }
}

// ===== STATUS FUNCTIONS =====

void readStatus() {
  if (currentMemoryType == MEM_UNKNOWN) {
    Serial.println(F("Please select memory type first!"));
    return;
  }
  
  Serial.println(F("Reading status register..."));
  
  switch (currentMemoryType) {
    case MEM_NAND_FLASH:
      nandReadStatus();
      break;
    case MEM_SPI_FLASH:
      spiReadStatus();
      break;
    case MEM_I2C_EEPROM:
      i2cReadStatus();
      break;
    default:
      Serial.println(F("Unknown memory type!"));
  }
}

void nandReadStatus() {
  // Select the chip
  digitalWrite(NAND_CE_PIN, LOW);
  
  // Send READ STATUS command
  digitalWrite(NAND_CLE_PIN, HIGH);
  nandWriteByte(NAND_CMD_READ_STATUS);
  digitalWrite(NAND_CLE_PIN, LOW);
  
  // Read status byte
  byte status = nandReadByte();
  
  // Deselect the chip
  digitalWrite(NAND_CE_PIN, HIGH);
  
  // Display status information
  Serial.print(F("Status: 0x"));
  Serial.println(status, HEX);
  
  Serial.print(F("Program/Erase Failed: "));
  Serial.println((status & 0x01) ? "Yes" : "No");
  
  Serial.print(F("Ready/Busy: "));
  Serial.println((status & 0x40) ? "Ready" : "Busy");
  
  Serial.print(F("Write Protected: "));
  Serial.println((status & 0x80) ? "Yes" : "No");
}

void spiReadStatus() {
  digitalWrite(SPI_CS_PIN, LOW);
  
  SPI.transfer(SPI_CMD_READ_STATUS);
  byte status = SPI.transfer(0);
  
  digitalWrite(SPI_CS_PIN, HIGH);
  
  // Display status information
  Serial.print(F("Status Register: 0x"));
  Serial.println(status, HEX);
  
  Serial.print(F("Busy: "));
  Serial.println((status & 0x01) ? "Yes" : "No");
  
  Serial.print(F("Write Enable Latch: "));
  Serial.println((status & 0x02) ? "Enabled" : "Disabled");
  
  Serial.print(F("Block Protection: "));
  Serial.println((status >> 2) & 0x0F, BIN);
  
  Serial.print(F("Write Protect Enable: "));
  Serial.println((status & 0x80) ? "Yes" : "No");
}

void i2cReadStatus() {
  // For I2C EEPROMs, there's typically no status register to read
  // Instead, we check if the device responds
  
  Wire.beginTransmission(i2cAddress);
  byte error = Wire.endTransmission();
  
  Serial.print(F("Device present: "));
  Serial.println((error == 0) ? "Yes" : "No");
  
  if (error == 0) {
    // Try to read one byte to see if the device is busy
    // When busy with internal write cycle, the device won't ACK
    Wire.beginTransmission(i2cAddress);
    Wire.write(0); // Address 0
    error = Wire.endTransmission();
    
    Serial.print(F("Device ready: "));
    Serial.println((error == 0) ? "Yes" : "No");
  }
}

// ===== UTILITY FUNCTIONS =====

byte nandReadByte() {
  // Configure Arduino pins as inputs for reading data
  DDRD &= 0x03; // Set pins 2-7 as inputs (D2-D7)
  DDRB &= 0xFC; // Set pins 8-9 as inputs (D8-D9)
  
  // Assert read enable
  digitalWrite(NAND_RE_PIN, LOW);
  delayMicroseconds(1);
  
  // Read data from pins
  byte data = 0;
  data |= ((PIND >> 2) & 0x3F);    // Bits 0-5 from pins D2-D7
  data |= ((PINB & 0x03) << 6);    // Bits 6-7 from pins D8-D9
  
  // De-assert read enable
  digitalWrite(NAND_RE_PIN, HIGH);
  delayMicroseconds(1);
  
  return data;
}

void nandWriteByte(byte data) {
  // Configure Arduino pins as outputs for writing data
  DDRD |= 0xFC; // Set pins 2-7 as outputs (D2-D7)
  DDRB |= 0x03; // Set pins 8-9 as outputs (D8-D9)
  
  // Output data to pins
  PORTD = (PORTD & 0x03) | ((data & 0x3F) << 2); // Bits 0-5 to pins D2-D7
  PORTB = (PORTB & 0xFC) | ((data >> 6) & 0x03); // Bits 6-7 to pins D8-D9
  
  // Assert write enable
  digitalWrite(NAND_WE_PIN, LOW);
  delayMicroseconds(1);
  
  // De-assert write enable
  digitalWrite(NAND_WE_PIN, HIGH);
  delayMicroseconds(1);
}

void nandReset() {
  // Select the chip
  digitalWrite(NAND_CE_PIN, LOW);
  
  // Send RESET command
  digitalWrite(NAND_CLE_PIN, HIGH);
  nandWriteByte(NAND_CMD_RESET);
  digitalWrite(NAND_CLE_PIN, LOW);
  
  // Wait for the device to be ready
  waitForNandReady();
  
  // Deselect the chip
  digitalWrite(NAND_CE_PIN, HIGH);
  
  Serial.println(F("NAND Flash reset complete"));
}

void waitForNandReady() {
  // Wait for the RB pin to go high
  unsigned long startTime = millis();
  
  while (digitalRead(NAND_RB_PIN) == LOW) {
    if (millis() - startTime > 1000) {
      Serial.println(F("Warning: NAND Flash timeout"));
      return;
    }
  }
}

bool waitForSpiReady() {
  digitalWrite(SPI_CS_PIN, LOW);
  SPI.transfer(SPI_CMD_READ_STATUS);
  byte status = SPI.transfer(0);
  digitalWrite(SPI_CS_PIN, HIGH);
  
  // Return true if busy (WIP bit set)
  return (status & 0x01);
}

void setI2CAddress() {
  Serial.println(F("Enter I2C address (in hex, e.g. 50 for 0x50):"));
  
  while (!Serial.available()) {
    delay(100);
  }
  
  String input = Serial.readStringUntil('\n');
  input.trim();
  
  byte newAddress = strtol(input.c_str(), NULL, 16);
  
  if (newAddress >= 0x08 && newAddress <= 0x77) {
    i2cAddress = newAddress;
    Serial.print(F("I2C address set to 0x"));
    Serial.println(i2cAddress, HEX);
  } else {
    Serial.println(F("Invalid I2C address! Valid range is 0x08-0x77"));
  }
}

unsigned long readHexValue() {
  while (!Serial.available()) {
    delay(100);
  }
  
  String input = Serial.readStringUntil('\n');
  input.trim();
  
  // Handle 0x prefix if present
  if (input.startsWith("0x") || input.startsWith("0X")) {
    input = input.substring(2);
  }
  
  return strtoul(input.c_str(), NULL, 16);
}

unsigned int readDecValue() {
  while (!Serial.available()) {
    delay(100);
  }
  
  String input = Serial.readStringUntil('\n');
  input.trim();
  
  return input.toInt();
}

// Read bytes using a function pointer and display as hex dump
void hexDump(byte (*readFunc)(), unsigned int numBytes) {
  byte buffer[16];
  
  for (unsigned int i = 0; i < numBytes; i++) {
    if (i % 16 == 0) {
      // Print address at start of line
      if (i > 0) {
        Serial.println();
      }
      Serial.print("0x");
      if (i < 0x1000) Serial.print("0");
      if (i < 0x100) Serial.print("0");
      if (i < 0x10) Serial.print("0");
      Serial.print(i, HEX);
      Serial.print(": ");
    }
    
    // Read byte
    buffer[i % 16] = readFunc();
    
    // Print hex value
    if (buffer[i % 16] < 0x10) Serial.print("0");
    Serial.print(buffer[i % 16], HEX);
    Serial.print(" ");
    
    // Print ASCII representation at end of line
    if ((i % 16 == 15) || (i == numBytes - 1)) {
      // Pad with spaces if not a full line
      for (int j = (i % 16) + 1; j < 16; j++) {
        Serial.print("   ");
      }
      
      Serial.print(" | ");
      
      // Print ASCII chars if printable
      for (int j = 0; j <= (i % 16); j++) {
        if (buffer[j] >= 32 && buffer[j] <= 126) {
          Serial.write(buffer[j]);
        } else {
          Serial.print(".");
        }
      }
    }
  }
  
  Serial.println();
}
