#include "SecuritySystem.h"

// Define the system security code - code needed to arm/disarm system
const char SecuritySystem::SECURITY_CODE[] = "2580";

// Constructor - Initializes all hardware components and system variables
SecuritySystem::SecuritySystem() :
    // Initialize all hardware interfaces with their respective pins
    lcd(p13, p14, p11),         // uLCD display (TX, RX, RST)
    buzzer(p21),                // Piezo buzzer
    ledRed(p22),                // RGB LED components
    ledGreen(p23),
    ledBlue(p24),
    pirSensor1(p17),            // External motion sensor
    pirSensor2(p16),            // Internal motion sensor
    doorSensor(p18),            // Magnetic door sensor
    trigPin(p19),               // Ultrasonic sensor trigger
    echoPin(p20),               // Ultrasonic sensor echo
    spi(p5, p6, p7),           // SPI bus (MOSI, MISO, SCK)
    cs(p12),                    // MCP23S17 chip select
    sdCard(p5, p6, p7, p8),    // SD card interface (shares SPI bus)
    i2c(p9, p10),              // I2C bus for RTC (SDA, SCL)
    // Initialize system state variables
    currentState(DISARMED),
    codeIndex(0),
    lastCommand(0),
    entryDelayActive(false),
    entryDelayStart(0),
    lastDoorState(false),
    lastUltrasonicAlert(0)
{
    // Initialize system memory and configuration
    memset(inputCode, 0, sizeof(inputCode));    // Clear security code buffer
    doorSensor.mode(PullUp);                    // Enable internal pullup for door sensor
    echoPin.mode(PullDown);                     // Configure echo pin with pulldown
    trigPin = 0;                                // Ensure trigger starts LOW
    ultrasonicTimer.reset();                    // Reset ultrasonic timer
    
    // Configure SPI interface for keypad controller
    spi.format(8, 0);                          // 8-bit mode, clock polarity 0
    spi.frequency(1000000);                    // Set SPI clock to 1MHz
    cs = 1;                                    // Deselect MCP23S17
    
    // Set I2C frequency for RTC communication
    i2c.frequency(100000);                     // 100kHz I2C clock
}

// Main system initialization
void SecuritySystem::initialize() {
    ThisThread::sleep_for(500ms);  // Initial delay for system stabilization
    
    // Initialize all major system components
    initializeLCD();     // Setup LCD display
    initializeKeypad();  // Configure keypad interface
    initRTC();          // Initialize real-time clock
    initMCP();          // Setup MCP23S17 port expander

    // Initialize SD card logging
    if (!sdCard.initialize()) {
        showStatus("SD Init Failed!");
        ThisThread::sleep_for(2s);
    } else {
        logEvent("System Started");  // Log system startup
    }

    runStartupSequence();  // Run system startup tests
}

// RTC initialization
void SecuritySystem::initRTC() {
    // Time can be set by uncommenting and modifying this line
    //setTime(10, 51, 17, 3, 4, 12, 23);  // Format: sec, min, hour, day, date, month, year
}

// LCD display initialization
void SecuritySystem::initializeLCD() {
    lcd.cls();                          // Clear screen
    lcd.background_color(BLACK);        // Set background color
    lcd.textbackground_color(BLACK);    // Set text background
    lcd.color(WHITE);                   // Set text color
    lcd.text_width(1);                  // Set text width
    lcd.text_height(2);                 // Set text height
}

// Keypad initialization - placeholder for any future keypad setup
void SecuritySystem::initializeKeypad() {
}

// System startup test sequence
void SecuritySystem::runStartupSequence() {
    showStatus((const char*)"Starting...");
    
    // Test RGB LED components
    ledRed = 1.0;   ThisThread::sleep_for(200ms); ledRed = 0.0;
    ledGreen = 1.0; ThisThread::sleep_for(200ms); ledGreen = 0.0;
    ledBlue = 1.0;  ThisThread::sleep_for(200ms); ledBlue = 0.0;
    
    // Test buzzer with two tones
    playTone(440.0, 0.1);  // Lower pitch
    ThisThread::sleep_for(100ms);
    playTone(880.0, 0.1);  // Higher pitch
    
    // Complete startup sequence
    ThisThread::sleep_for(500ms);
    showStatus((const char*)"Ready");
    ThisThread::sleep_for(1s);
    updateLED(DISARMED);    // Set initial LED state
    showStatus((const char*)"DISARMED");
}

// Main system operation loop
void SecuritySystem::run() {
    static uint32_t lastTimeUpdate = 0;  // Track last time display update
    
    while(1) {
        // Check keypad for input
        char key = scanKeypad();
        if(key) {
            handleKeypress(key);
        }

        // Monitor sensors if system is armed
        if(currentState != DISARMED && currentState != ALARM) {
            checkSensors();
        }
        
        // Update time display every second
        uint32_t currentTime = Kernel::get_ms_count();
        if(currentTime - lastTimeUpdate >= 1000) {
            lcd.locate(1,1);
            lcd.puts(getTimeStr());
            lastTimeUpdate = currentTime;
        }

        ThisThread::sleep_for(50ms);  // Prevent CPU overload
    }
}

// Sets the time in the DS3231 RTC
// Parameters represent each time component in standard format
void SecuritySystem::setTime(int sec, int min, int hour, int day, int date, int month, int year) {
    char data[8];
    data[0] = DS3231_REG_TIME;          // First byte is the register address
    data[1] = decToBcd(sec) & 0x7F;     // Seconds (clear CH bit)
    data[2] = decToBcd(min) & 0x7F;     // Minutes
    data[3] = decToBcd(hour) & 0x3F;    // Hours (24hr mode)
    data[4] = decToBcd(day) & 0x07;     // Day of week (1-7)
    data[5] = decToBcd(date) & 0x3F;    // Date (1-31)
    data[6] = decToBcd(month) & 0x1F;   // Month (1-12)
    data[7] = decToBcd(year);           // Year (00-99)
    
    // Write time data to RTC via I2C
    i2c.write((DS3231_ADDRESS << 1), data, 8);
}

// Reads the current time from the DS3231 RTC
// Parameters are passed by reference to return all time components
void SecuritySystem::getTime(int &sec, int &min, int &hour, int &day, int &date, int &month, int &year) {
    char reg = DS3231_REG_TIME;    // Register address to start reading from
    char data[7];                  // Buffer for time data
    
    // Read time data from RTC using repeated start I2C operation
    i2c.write((DS3231_ADDRESS << 1), &reg, 1, true);  // Send register address
    i2c.read((DS3231_ADDRESS << 1), data, 7);         // Read time data
    
    // Convert BCD values to decimal and store in reference parameters
    sec = bcdToDec(data[0] & 0x7F);
    min = bcdToDec(data[1] & 0x7F);
    hour = bcdToDec(data[2] & 0x3F);
    day = bcdToDec(data[3] & 0x07);
    date = bcdToDec(data[4] & 0x3F);
    month = bcdToDec(data[5] & 0x1F);
    year = bcdToDec(data[6]);
}

// Returns formatted time string for display
// Format: HH:MM:SS
char* SecuritySystem::getTimeStr() {
    static char timeStr[32];    // Static buffer for formatted time string
    int sec, min, hour, day, date, month, year;
    
    // Get current time from RTC
    getTime(sec, min, hour, day, date, month, year);
    
    // Format time into string
    char temp[16];
    sprintf(temp, "%02d:%02d:%02d", hour, min, sec);
    strcpy(timeStr, temp);
    
    return timeStr;
}

// Converts Binary Coded Decimal (BCD) to standard decimal
uint8_t SecuritySystem::bcdToDec(uint8_t val) {
    return (val/16*10) + (val%16);    // Convert tens and ones
}

// Converts standard decimal to Binary Coded Decimal (BCD)
uint8_t SecuritySystem::decToBcd(uint8_t val) {
    return (val/10*16) + (val%10);    // Convert to tens and ones in BCD
}

// Handles all keypad input and processes system commands
void SecuritySystem::handleKeypress(char key) {
    playTone(1000.0, 0.05);  // Key press feedback tone

    // Special handling for entry delay during ARMED_AWAY state
    if(currentState == ARMED_AWAY && entryDelayActive) {
        // Handle numeric input during entry delay
        if(key >= '0' && key <= '9' && codeIndex < 4) {
            inputCode[codeIndex++] = key;
            // Update code display
            lcd.locate(1,5);
            for(int i = 0; i < 4; i++) {
                lcd.putc(i < codeIndex ? '*' : '_');
            }
        }
        // Handle code confirmation
        else if(key == '#' && codeIndex == 4) {
            if(validateCode()) {
                entryDelayActive = false;
                currentState = DISARMED;
                showStatus((const char*)"DISARMED");
                updateLED(DISARMED);
                playTone(440.0, 0.1);  // Success tone
            } else {
                // Handle invalid code
                clearDisplay();
                showStatus((const char*)"Wrong Code!");
                playTone(220.0, 0.5);  // Error tone
                ThisThread::sleep_for(1s);
                
                // Reset code entry
                codeIndex = 0;
                memset(inputCode, 0, sizeof(inputCode));
                clearDisplay();
            }
        }
        // Handle backspace
        else if(key == '*' && codeIndex > 0) {
            inputCode[--codeIndex] = 0;
            lcd.locate(1,5);
            for(int i = 0; i < 4; i++) {
                lcd.putc(i < codeIndex ? '*' : '_');
            }
        }
        return;  // Skip normal processing during entry delay
    }
    
    // Process regular keypad input
    switch(key) {
        case 'A': { // ARM HOME mode
            if(currentState != ALARM) {
                showStatus((const char*)"Enter Code:");
                codeIndex = 0;
                memset(inputCode, 0, sizeof(inputCode));
                lastCommand = 'A';
            }
            break;
        }
            
        case 'B': { // ARM AWAY mode
            if(currentState != ALARM) {
                showStatus((const char*)"Enter Code:");
                codeIndex = 0;
                memset(inputCode, 0, sizeof(inputCode));
                lastCommand = 'B';
            }
            break;
        }
            
        case 'C': { // DISARM system
            if(currentState != DISARMED) {
                showStatus((const char*)"Enter Code:");
                codeIndex = 0;
                memset(inputCode, 0, sizeof(inputCode));
                lastCommand = 'C';
            }
            break;
        }
            
        case 'D': { // Emergency/Panic button
            currentState = ALARM;
            logEvent("Panic Button Pressed");
            handleAlarm();
            break;
        }
            
        case '*': { // Backspace
            if(codeIndex > 0) {
                codeIndex--;
                inputCode[codeIndex] = 0;
                showInputCode();
            }
            break;
        }
            
        case '#': { // Enter/Confirm code
            if(codeIndex == 4) {
                if(validateCode()) {
                    // Process valid code based on last command
                    if(lastCommand == 'A') {
                        resetEntryDelay();
                        currentState = ARMED_HOME;
                        showStatus((const char*)"ARMED HOME");
                        logEvent("System Armed - Home Mode");
                    } 
                    else if(lastCommand == 'B') {
                        resetEntryDelay();
                        currentState = ARMED_AWAY;
                        showStatus((const char*)"ARMED AWAY");
                        logEvent("System Armed - Away Mode");
                    }
                    else if(lastCommand == 'C') {
                        resetEntryDelay();
                        currentState = DISARMED;
                        showStatus((const char*)"DISARMED");
                        logEvent("System Disarmed");
                    }
                    updateLED(currentState);
                    playTone(880.0, 0.1);  // Success tone
                } else {
                    // Handle invalid code
                    showStatus((const char*)"Wrong Code!");
                    playTone(220.0, 0.5);
                    ThisThread::sleep_for(1s);
                    
                    // Restore previous state display
                    switch(currentState) {
                        case DISARMED: 
                            showStatus((const char*)"DISARMED"); 
                            break;
                        case ARMED_HOME: 
                            showStatus((const char*)"ARMED HOME"); 
                            break;
                        case ARMED_AWAY: 
                            showStatus((const char*)"ARMED AWAY"); 
                            break;
                        case ALARM: 
                            showStatus((const char*)"! ALARM !"); 
                            break;
                    }
                }
                codeIndex = 0;
                memset(inputCode, 0, sizeof(inputCode));
            }
            break;
        }
            
        default: { // Handle numeric keys
            if(key >= '0' && key <= '9' && codeIndex < 4) {
                inputCode[codeIndex++] = key;
                showInputCode();
            }
            break;
        }
    }
}

// Scans the 4x4 keypad matrix and returns the pressed key
char SecuritySystem::scanKeypad() {
    // Keypad layout matrix
    static const char keys[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };
    
    // Scan each row
    for(int row = 0; row < 4; row++) {
        setKeypadRow(row);                  // Activate current row
        ThisThread::sleep_for(1ms);         // Debounce delay
        
        uint8_t cols = readKeypadCols();    // Read all columns
        if(cols != 0x0F) {                  // If any column is active (low)
            // Check each column in the row
            for(int col = 0; col < 4; col++) {
                if(!(cols & (1 << col))) {
                    // Wait for key release with debounce
                    while(!(readKeypadCols() & (1 << col))) {
                        ThisThread::sleep_for(1ms);
                    }
                    ThisThread::sleep_for(50ms);  // Additional debounce
                    return keys[row][col];        // Return the pressed key
                }
            }
        }
    }
    return 0;  // No key pressed
}

// Initialize the MCP23S17 port expander for keypad operation
void SecuritySystem::initMCP() {
    // Configure Port B for keypad matrix
    writeMCP(IODIRB, 0xF0);  // Lower 4 bits output (rows), upper 4 bits input (columns)
    writeMCP(0x0D, 0xF0);    // Enable pull-ups on column pins (GPPUB register)
}

// Write data to a register in the MCP23S17
void SecuritySystem::writeMCP(uint8_t reg, uint8_t data) {
    cs = 0;                        // Select MCP23S17
    spi.write(MCP_ADDR);          // Send write command
    spi.write(reg);               // Send register address
    spi.write(data);              // Send data
    cs = 1;                       // Deselect MCP23S17
}

// Read data from a register in the MCP23S17
uint8_t SecuritySystem::readMCP(uint8_t reg) {
    cs = 0;                        // Select MCP23S17
    spi.write(MCP_ADDR | 0x01);   // Send read command
    spi.write(reg);               // Send register address
    uint8_t data = spi.write(0x00); // Read data (send dummy byte)
    cs = 1;                       // Deselect MCP23S17
    return data;
}

// Set the active row for keypad scanning
void SecuritySystem::setKeypadRow(int row) {
    // Create row pattern (active low)
    uint8_t rowPattern = ~(1 << row) & 0x0F;  // Invert bit for selected row, mask to 4 bits
    // Preserve column inputs while updating row outputs
    writeMCP(GPIOB, (readMCP(GPIOB) & 0xF0) | rowPattern);
}

// Read the state of all keypad columns
uint8_t SecuritySystem::readKeypadCols() {
    // Read and extract column bits (upper 4 bits)
    return (readMCP(GPIOB) >> 4) & 0x0F;
}

// Validate entered security code
bool SecuritySystem::validateCode() {
    return (strcmp(inputCode, SECURITY_CODE) == 0);
}

// Main sensor monitoring function - checks all sensors and handles their states
void SecuritySystem::checkSensors() {
    // Check PIR motion sensors
    if (pirSensor1.read() == 1 || pirSensor2.read() == 1) {
        if (currentState == ARMED_AWAY) {
            // Any motion triggers alarm in AWAY mode
            handleMotionDetected("Motion Detected!");
        }
        else if (currentState == ARMED_HOME && pirSensor1.read() == 1) {
            // Only external sensor (PIR1) triggers in HOME mode
            handleMotionDetected("Outside Motion!");
        }
    }
    
    // Check magnetic door sensor
    bool currentDoorState = doorSensor.read();
    if(currentDoorState == 1 && lastDoorState == 0) {  // Door opening detected
        switch(currentState) {
            case ARMED_HOME:
                handleDoorOpen("Door Opened");
                break;
            case ARMED_AWAY:
                handleDoorOpen("Entry Started");
                break;
            default:
                break;
        }
    }
    lastDoorState = currentDoorState;
    
    // Check ultrasonic sensor for proximity/window breach
    if(currentState != DISARMED && !entryDelayActive) {
        float distance = measureDistance();
        static bool inAlertZone = false;
        
        // Check if object is within alert threshold (10cm)
        if(distance < 10.0f && !inAlertZone) {
            inAlertZone = true;
            if(currentState == ARMED_HOME) {
                handleUltrasonicAlert("Proximity Alert");
            } else if(currentState == ARMED_AWAY) {
                handleUltrasonicAlert("Window Breach!");
            }
        } else if(distance > 12.0f) {  // Reset alert with 2cm hysteresis
            inAlertZone = false;
        }
    }
    
    // Process entry delay countdown if active
    if(currentState == ARMED_AWAY && entryDelayActive) {
        processEntryDelay();
    }
}

// Function to measure distance using ultrasonic sensor
float SecuritySystem::measureDistance() {
    static uint32_t lastMeasureTime = 0;
    uint32_t currentTime = Kernel::get_ms_count();
    
    // Rate limit measurements to prevent sensor flooding
    if (currentTime - lastMeasureTime < 60) {  // Minimum 60ms between measurements
        return 400.0f;  // Return max range if too soon
    }
    lastMeasureTime = currentTime;
    
    // Generate trigger pulse
    trigPin = 0;
    wait_us(2);
    trigPin = 1;
    wait_us(10);
    trigPin = 0;
    
    Timer pulseTimer;
    pulseTimer.reset();
    pulseTimer.start();
    
    // Wait for echo to start with timeout
    while(echoPin == 0) {
        if(pulseTimer.elapsed_time().count() > 10000) {  // 10ms timeout
            return 400.0f;
        }
    }
    
    pulseTimer.reset();
    
    // Measure echo pulse width with timeout
    while(echoPin == 1) {
        if(pulseTimer.elapsed_time().count() > 25000) {  // 25ms timeout
            return 400.0f;
        }
    }
    
    auto duration = pulseTimer.elapsed_time();
    pulseTimer.stop();
    
    // Calculate distance in cm using speed of sound (343m/s)
    float distance_cm = (duration.count() * 34300.0f) / (2.0f * 1000000.0f);
    
    // Validate reading range
    if (distance_cm < 2.0f || distance_cm > 400.0f) {
        return 400.0f;
    }
    
    return distance_cm;
}

// Main alarm handler - manages alarm state and user response
void SecuritySystem::handleAlarm() {
    showStatus((const char*)"! ALARM !");
    updateLED(ALARM);
    logEvent("ALARM TRIGGERED");  // Log alarm event
    bool codeEntryMode = false;   // Track if in code entry mode
    
    // Continue alarm until system is disarmed
    while(currentState == ALARM) {
        if (!codeEntryMode) {
            // Generate alarm sound and flash LED
            ledRed = 1.0;
            playTone(1760.0, 0.1);    // High-pitched alarm tone
            ThisThread::sleep_for(100ms);
            ledRed = 0.0;
            ThisThread::sleep_for(100ms);
            
            // Check for disarm attempt (C button)
            char tempKey = scanKeypad();
            if(tempKey == 'C') {
                codeEntryMode = true;
                codeIndex = 0;
                memset(inputCode, 0, sizeof(inputCode));
                lastCommand = 'C';
                showStatus((const char*)"Enter Code:");
                ThisThread::sleep_for(100ms);
            }
        } else {
            // Handle code entry while alarm is active
            ledRed = !ledRed;  // Continue LED flashing
            playTone(1760.0, 0.1);
            
            char key = scanKeypad();
            if(key) {
                if(key >= '0' && key <= '9' && codeIndex < 4) {
                    // Process numeric input
                    inputCode[codeIndex++] = key;
                    showInputCode();
                    playTone(1000.0, 0.05);  // Key feedback
                }
                else if(key == '#' && codeIndex == 4) {
                    // Validate entered code
                    if(validateCode()) {
                        resetEntryDelay();
                        currentState = DISARMED;
                        logEvent("Alarm Disarmed");
                        showStatus((const char*)"DISARMED");
                        updateLED(DISARMED);
                        break;
                    } else {
                        // Handle invalid code
                        logEvent("Wrong Code Entry During Alarm");
                        showStatus((const char*)"Wrong Code!");
                        ThisThread::sleep_for(1s);
                        showStatus((const char*)"! ALARM !");
                        codeEntryMode = false;
                        codeIndex = 0;
                        memset(inputCode, 0, sizeof(inputCode));
                    }
                }
                else if(key == '*') {
                    // Backspace functionality
                    if(codeIndex > 0) {
                        codeIndex--;
                        inputCode[codeIndex] = 0;
                        showInputCode();
                    }
                }
            }
            ThisThread::sleep_for(100ms);
        }
    }
}

// Handles motion detection events
void SecuritySystem::handleMotionDetected(const char* motionMsg) {
    if (currentState != ALARM) {  // Prevent multiple alarms
        resetEntryDelay();
        currentState = ALARM;
        showStatus(motionMsg);
        logEvent(motionMsg);
        handleAlarm();
    }
}

// Processes door sensor triggers
void SecuritySystem::handleDoorOpen(const char* msg) {
    showStatus(msg);
    logEvent(msg);
    
    if(currentState == DISARMED) {
        // Just show brief notification in disarmed state
        ThisThread::sleep_for(1s);
        showStatus((const char*)"DISARMED");
    }
    else if(currentState == ARMED_HOME) {
        // Visual and audible alert in home mode
        for(int i = 0; i < 3; i++) {
            ledBlue = 0.0;
            playTone(880.0, 0.1);
            ThisThread::sleep_for(200ms);
            ledBlue = 0.7;
            ThisThread::sleep_for(200ms);
        }
        showStatus((const char*)"ARMED HOME");
    }
    else if(currentState == ARMED_AWAY) {
        // Start entry delay sequence
        entryDelayActive = true;
        entryDelayStart = Kernel::get_ms_count();
        codeIndex = 0;
        memset(inputCode, 0, sizeof(inputCode));
    }
}

// Handles ultrasonic sensor alerts
void SecuritySystem::handleUltrasonicAlert(const char* alertMsg) {
    uint32_t currentTime = Kernel::get_ms_count();
    
    // Rate limit alerts to prevent rapid triggering
    if(currentTime - lastUltrasonicAlert < 1000) {
        return;
    }
    
    lastUltrasonicAlert = currentTime;
    
    if(currentState == ARMED_HOME) {
        logEvent(alertMsg);
        // Visual and audible proximity warning
        for(int i = 0; i < 2; i++) {
            showStatus(alertMsg);
            ledBlue = 0.0;
            playTone(880.0, 0.05);
            ThisThread::sleep_for(100ms);
            ledBlue = 0.7;
            ThisThread::sleep_for(100ms);
        }
        showStatus((const char*)"ARMED HOME");
    }
    else if(currentState == ARMED_AWAY) {
        logEvent(alertMsg);
        // Trigger full alarm for potential breach
        currentState = ALARM;
        showStatus(alertMsg);
        handleAlarm();
    }
}

// Process entry delay countdown when system is ARMED_AWAY and door is opened
void SecuritySystem::processEntryDelay() {
    if(!entryDelayActive) return;  // Exit if entry delay isn't active
    
    static uint32_t lastUpdate = 0;
    uint32_t currentTime = Kernel::get_ms_count();
    
    // Calculate remaining time in entry delay
    uint32_t elapsed = (currentTime - entryDelayStart) / 1000;  // Convert to seconds
    uint32_t remaining = 30 - elapsed;  // 30-second entry delay
    
    // Update display once per second
    if(currentTime - lastUpdate >= 1000 || lastUpdate == 0) {
        lastUpdate = currentTime;
        
        if(remaining > 0) {
            // Update display with countdown and code entry prompt
            if(lastUpdate == currentTime) {
                clearDisplay();
                
                // Show countdown timer
                lcd.locate(9,1);
                char countMsg[32];
                sprintf(countMsg, "Time: %ds", remaining);
                lcd.puts(countMsg);
                
                // Show code entry prompt
                lcd.locate(1,3);
                lcd.puts("Enter code:");
                
                // Show code entry progress
                lcd.locate(1,5);
                for(int i = 0; i < 4; i++) {
                    lcd.putc(i < codeIndex ? '*' : '_');
                }
            }
        } else {
            // Time expired - trigger alarm
            entryDelayActive = false;
            currentState = ALARM;
            handleAlarm();
        }
    }
}

// Reset entry delay timer and flags
void SecuritySystem::resetEntryDelay() {
    entryDelayActive = false;
    entryDelayStart = 0;
}

// Clear the LCD screen
void SecuritySystem::clearDisplay() {
    lcd.cls();
}

// Display system status with time
void SecuritySystem::showStatus(const char* msg) {
    clearDisplay();

    // Show current time at top
    lcd.locate(1,1);
    char* timeStr = getTimeStr();
    lcd.puts(timeStr);

    // Show status message
    lcd.locate(1,4);
    
    // Convert message to writable buffer (LCD requires non-const char*)
    char buffer[32];
    strncpy(buffer, msg, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';  // Ensure null termination
    
    lcd.puts(buffer);
}

// Display code entry interface
void SecuritySystem::showInputCode() {
    clearDisplay();
    // Show prompt
    lcd.locate(1,3);
    lcd.puts("Enter Code:");
    // Show code entry progress with asterisks
    lcd.locate(1,5);
    for(int i = 0; i < 4; i++) {
        lcd.putc(i < codeIndex ? '*' : '_');
    }
}

// Generate tones for audio feedback
void SecuritySystem::playTone(float frequency, float duration) {
    buzzer.period(1.0/frequency);  // Set tone frequency
    buzzer = 0.5;                  // 50% duty cycle
    ThisThread::sleep_for(duration * 1000);  // Hold tone
    buzzer = 0.0;                  // Silence
}

// Update RGB LED based on system state
void SecuritySystem::updateLED(SystemState state) {
    // Turn off all LED components
    ledRed = 0.0;
    ledGreen = 0.0;
    ledBlue = 0.0;
    
    // Set appropriate color for current state
    switch(state) {
        case DISARMED:
            ledGreen = 1.0;        // Green for disarmed
            break;
        case ARMED_HOME:
            ledBlue = 1.0;         // Purple for armed home
            ledRed = 1.0;
            break;
        case ARMED_AWAY:
            ledBlue = 1.0;         // Blue for armed away
            break;
        case ALARM:
            ledRed = 1.0;          // Red for alarm
            break;
    }
}

// Log an event with timestamp to SD card
void SecuritySystem::logEvent(const char* event) {
    int sec, min, hour, day, date, month, year;
    
    // Get current time from RTC
    getTime(sec, min, hour, day, date, month, year);
    
    // Format log entry with timestamp and event description
    // Format: YYYY-MM-DD HH:MM:SS - Event Message\n
    snprintf(eventBuffer, sizeof(eventBuffer), 
             "%04d-%02d-%02d %02d:%02d:%02d - %s\n",
             2000 + year,    // Convert 2-digit year to 4-digit
             month, 
             date,           // Day of month
             hour, 
             min, 
             sec, 
             event);        // Event description
    
    // Write the formatted log entry to SD card
    sdCard.writeData(eventBuffer, strlen(eventBuffer));
}