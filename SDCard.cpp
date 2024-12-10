#include "SDCard.h"

// Constructor implementation
SDCard::SDCard(PinName mosi, PinName miso, PinName sck, PinName cs) :
    _bd(mosi, miso, sck, cs),    // Initialize SD block device with provided pins
    _fs("fs"),                    // Initialize filesystem with mount point name "fs"
    _mounted(false)               // Start with filesystem unmounted
{
}

// Destructor implementation
SDCard::~SDCard() {
    // Clean up resources if filesystem is mounted
    if(_mounted) {
        _fs.unmount();    // Unmount the filesystem
        _bd.deinit();     // Deinitialize the SD card
    }
}

// Initialize SD card and filesystem
bool SDCard::initialize() {
    printf("Initializing SD card...\n");
    
    // Initialize the SD card hardware
    int err = _bd.init();
    if(err != 0) {
        printf("SD init error: %d\n", err);
        return false;
    }
    
    // Mount the filesystem on the SD card
    err = _fs.mount(&_bd);
    if(err != 0) {
        printf("Mount failed: %d\n", err);
        _bd.deinit();
        return false;
    }
    
    // Mark filesystem as mounted
    _mounted = true;
    
    // Try to open events file to verify filesystem access
    FILE* fp = fopen("/fs/events.txt", "a");
    if(fp == NULL) {
        printf("Could not open events.txt\n");
        _fs.unmount();
        _bd.deinit();
        _mounted = false;
        return false;
    }
    fclose(fp);
    
    printf("SD card initialized successfully\n");
    return true;
}

// Write data to the events log file
bool SDCard::writeData(const char* data, uint32_t length) {
    // Check if filesystem is mounted
    if(!_mounted) return false;
    
    // Open events file in append mode
    FILE* fp = fopen("/fs/events.txt", "a");
    if(fp == NULL) return false;
    
    // Write data to file
    size_t written = fwrite(data, 1, length, fp);
    fflush(fp);    // Ensure data is written to disk
    fclose(fp);    // Close the file
    
    // Return true if all bytes were written
    return (written == length);
}