#pragma once    // Prevent multiple inclusions of this header file

// Include required libraries
#include "mbed.h"          // Core mbed functionality
#include "uLCD_4DGL.h"     // LCD display interface
#include <cstdint>         // Standard integer types
#include "SDCard.h"        // Custom SD card handling

// MCP23S17 Port Expander Register Definitions
#define IODIRA      0x00    // I/O Direction Register A - controls Port A pins as input/output
#define IODIRB      0x01    // I/O Direction Register B - controls Port B pins as input/output
#define GPIOA       0x12    // GPIO Port Register A - read/write Port A pins
#define GPIOB       0x13    // GPIO Port Register B - read/write Port B pins
#define MCP_ADDR    0x40    // Device address for MCP23S17 on SPI bus

// Enable use of chrono literals for time specifications
using namespace std::chrono;

// System State Enumeration
// Defines all possible states of the security system
enum SystemState {
    DISARMED,       // System is inactive
    ARMED_HOME,     // System is armed with interior motion detection disabled
    ARMED_AWAY,     // System is fully armed with all sensors active
    ALARM           // System is in alarm state due to trigger
};

// Main Security System Class Declaration
class SecuritySystem {
public:
    // Constructor and main control functions
    SecuritySystem();    // Initializes all hardware components
    void initialize();   // Performs system startup and initialization
    void run();         // Main system operation loop

private:
    // Hardware Components
    uLCD_4DGL lcd;          // LCD display interface (pins: TX→p13, RX→p14, RST→p11)
    PwmOut buzzer;          // Piezo buzzer for audio alerts (p21)
    PwmOut ledRed;          // Red component of RGB LED (p22)
    PwmOut ledGreen;        // Green component of RGB LED (p23)
    PwmOut ledBlue;         // Blue component of RGB LED (p24)
    DigitalIn pirSensor1;   // External PIR motion sensor (p17)
    DigitalIn pirSensor2;   // Internal PIR motion sensor (p16)
    DigitalIn doorSensor;   // Magnetic door contact sensor (p18)

    // Ultrasonic Sensor Configuration
    DigitalOut trigPin;     // Trigger pin for HC-SR04 sensor (p19)
    DigitalIn echoPin;      // Echo pin for HC-SR04 sensor (p20)
    Timer ultrasonicTimer;  // Timer for measuring ultrasonic pulse duration
    
    // SPI Interface for MCP23S17 Port Expander
    SPI spi;               // SPI bus interface (MOSI→p5, MISO→p6, SCK→p7)
    DigitalOut cs;         // Chip select for MCP23S17 (p12)

    // System State Variables
    SystemState currentState;     // Current state of the security system
    char inputCode[5];           // Buffer for storing entered security code (4 digits + null)
    int codeIndex;              // Current position in code entry buffer
    static const char SECURITY_CODE[];  // Defined security code for system access
    char lastCommand;           // Last keypad command received
    bool entryDelayActive;      // Flag for entry delay countdown
    uint32_t entryDelayStart;   // Timestamp for entry delay start
    bool lastDoorState;         // Previous state of door sensor for edge detection

    // Ultrasonic Sensor Parameters
    uint32_t lastUltrasonicAlert;  // Timestamp of last ultrasonic alert to prevent rapid retriggering

    // I2C Interface
    I2C i2c;                    // I2C bus for RTC communication (SDA→p9, SCL→p10)
    
    // RTC (DS3231) Configuration
    static const int DS3231_ADDRESS = 0x68;    // I2C address of DS3231 RTC
    static const int DS3231_REG_TIME = 0x00;   // Base register address for time data

    // SD Card Logging
    SDCard sdCard;              // SD card interface for event logging
    void logEvent(const char* event);  // Method to log events with timestamp
    char eventBuffer[512];      // Buffer for formatting log entries

    // Sensor Monitoring Methods
    void checkSensors();                    // Main sensor monitoring loop
    void handleMotionDetected();            // Handles PIR sensor triggers
    void handleMotionDetected(const char* motionMsg);  // Processes motion detection with specific message
    float measureDistance();                // Measures distance using ultrasonic sensor
    void handleUltrasonicAlert(const char* alertMsg);  // Handles ultrasonic sensor triggers

    // Component Initialization Methods
    void initializeLCD();     // Sets up LCD display parameters
    void initializeKeypad();  // Initializes keypad through MCP23S17
    void runStartupSequence(); // Performs system startup tests and animations

    // Core Security Functions
    void handleKeypress(char key);  // Processes keypad input
    char scanKeypad();              // Scans keypad matrix for pressed keys
    bool validateCode();            // Validates entered security code
    void handleAlarm();            // Manages alarm state and responses
    void handleDoorOpen(const char* msg);  // Handles door sensor triggers
    void processEntryDelay();      // Manages entry delay countdown
    void resetEntryDelay();        // Resets entry delay timer

    // Display Functions
    void clearDisplay();           // Clears LCD screen
    void showStatus(const char* msg);  // Displays system status message
    void showInputCode();          // Shows code entry interface

    // Hardware Control Functions
    void playTone(float frequency, float duration);  // Generates buzzer tones
    void updateLED(SystemState state);  // Updates RGB LED based on system state

    // MCP23S17 Port Expander Functions
    void writeMCP(uint8_t reg, uint8_t data);  // Writes to MCP23S17 registers
    uint8_t readMCP(uint8_t reg);              // Reads from MCP23S17 registers
    void initMCP();                            // Initializes MCP23S17
    void setKeypadRow(int row);                // Sets active keypad row
    uint8_t readKeypadCols();                  // Reads keypad columns

    // RTC (Real-Time Clock) Functions
    void initRTC();              // Initializes DS3231 RTC
    uint8_t bcdToDec(uint8_t val);  // Converts BCD to decimal
    uint8_t decToBcd(uint8_t val);  // Converts decimal to BCD
    
    // Time Management Functions
    void setTime(int sec, int min, int hour, int day, int date, int month, int year);  // Sets RTC time
    void getTime(int &sec, int &min, int &hour, int &day, int &date, int &month, int &year);  // Gets current time
    char* getTimeStr();          // Gets formatted time string
};