#pragma once  // Prevents multiple inclusion of this header file

// Include necessary Mbed libraries
#include "mbed.h"
#include "SDBlockDevice.h"    // For SD card block device operations
#include "FATFileSystem.h"    // For FAT filesystem operations

// Class to manage SD card operations
class SDCard {
public:
    // Constructor initializes SD card with specified pins
    // Parameters:
    //   mosi: Master Out Slave In pin for SPI
    //   miso: Master In Slave Out pin for SPI
    //   sck:  Serial Clock pin for SPI
    //   cs:   Chip Select pin for SD card
    SDCard(PinName mosi, PinName miso, PinName sck, PinName cs);
    
    // Destructor ensures proper cleanup of SD card resources
    ~SDCard();
    
    // Initializes the SD card and mounts the filesystem
    // Returns: true if successful, false if any step fails
    bool initialize();
    
    // Writes data to the SD card's event log file
    // Parameters:
    //   data:   Pointer to the data to write
    //   length: Length of the data in bytes
    // Returns: true if write successful, false otherwise
    bool writeData(const char* data, uint32_t length);
    
private:
    SDBlockDevice _bd;        // Block device interface for SD card
    FATFileSystem _fs;        // FAT filesystem interface
    bool _mounted;            // Tracks if filesystem is currently mounted
};