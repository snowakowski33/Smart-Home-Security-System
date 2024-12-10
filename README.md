# Home Security System with mbed LPC1768

A comprehensive embedded security system implementation featuring multiple sensors, real-time monitoring, and event logging capabilities.

## Overview

This security system provides professional-grade home protection using the mbed LPC1768 microcontroller. It features multiple operation modes (Home, Away, Disarmed), sensor integration, and detailed event logging to SD card.

### Key Features

- Multiple arming modes (Home/Away/Disarmed)
- Dual PIR motion detection (interior/exterior)
- Door status monitoring
- Ultrasonic window breach detection
- Real-time status display
- Event logging with timestamps
- Entry delay with countdown
- Secure keypad access
- Visual and audio alerts
- Status indication via RGB LED

## Hardware Requirements

### Core Components
- mbed LPC1768 microcontroller
- uLCD-144-G2 display module
- 4x4 matrix keypad
- MCP23S17 port expander
- DS3231 RTC module
- SD card module

### Sensors
- 2× PIR motion sensors
- Magnetic door sensor
- HC-SR04 ultrasonic sensor

### Other Components
- RGB LED
- Buzzer
- SD card (FAT32 formatted)
- Various resistors and connecting wires

## Pin Connections

### Display
- TX → p13
- RX → p14
- RST → p11

### MCP23S17 (Keypad Interface)
- MOSI → p5
- MISO → p6
- SCK → p7
- CS → p12

### Sensors
- External PIR → p17
- Internal PIR → p16
- Door Sensor → p18
- Ultrasonic TRIG → p19
- Ultrasonic ECHO → p20

### Other
- RGB LED: R→p22, G→p23, B→p24
- Buzzer → p21
- RTC: SDA→p9, SCL→p10
- SD Card: MOSI→p5, MISO→p6, SCK→p7, CS→p8

## Software Setup

1. Clone this repository
2. Install Mbed Studio
3. Import the project
4. Ensure all required libraries are available
5. Build and flash to your mbed LPC1768

## Usage

### Default Security Code
The default security code is set to "2580". This can be changed in the code.

### Operation Modes

#### Disarmed
- System is inactive
- Door openings are logged but no alerts
- Status LED shows green

#### Armed (Home)
- Internal motion detection disabled
- External sensors active
- Status LED shows purple
- Suitable when occupants are home

#### Armed (Away)
- All sensors active
- Entry delay enabled
- Status LED shows blue
- Full security mode

### Controls
- A: Arm (Home Mode)
- B: Arm (Away Mode)
- C: Disarm
- D: Panic Button
- *: Clear Entry
- #: Confirm Code

## Event Logging

All events are logged to the SD card with timestamps in the format:

YYYY-MM-DD HH:MM:SS - Event Description

Events logged include:
- System state changes
- Sensor triggers
- Door openings
- Invalid code attempts
- Alarm activations

## Development

### Build Requirements
- Mbed OS 6
- FAT Filesystem support
- SPI and I2C capabilities

### Library Dependencies
- mbed.h
- SDBlockDevice
- FATFileSystem
- uLCD_4DGL

## Contributing

Feel free to submit issues and enhancement requests. Pull requests are welcome with:
- Clear description of changes
- Testing evidence
- Documentation updates

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- ECE 4180 course staff for guidance
- Mbed community for library support
- Hardware component manufacturers for documentation

## Contact

Stanislaw Nowakowski - staszek1206@gmail.com


