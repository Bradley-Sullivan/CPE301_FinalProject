# CPE 301 Embedded Systems Design - Final Project
Final Project for CPE 301 (Fall 2022)

## Author
Bradley Sullivan (bradleysullivan@nevada.unr.edu)

## Project Overview
This project aims to implement the base functionality of the systems commonly found in evaporation cooler units. These units rely on a few systems and inputs to function correctly. This project implements the following systems:

- Temperature, humidity, and water level monitoring
- Main fan control
- Output vent adjustments
- User control (Start/Stop, Reset, Vent Control)
- Status LED output(s)
- Serial data communication (timestamped event reporting)

## Hardware Description
This project was built upon the Arduino ATMega 2560 microcontroller. Devices and sensors used (with datasheets):
- ATMega 2560
    - https://pdf1.alldatasheet.com/datasheet-pdf/view/107092/ATMEL/ATMEGA2560.html
- 16-pin LCD Display
    - https://components101.com/sites/default/files/component_datasheet/16x2%20LCD%20Datasheet.pdf
- DS3231 RTC Module
    - https://pdf1.alldatasheet.com/datasheet-pdf/view/254832/MAXIM/DS3231.html
- 28BYJ-48 5v DC Stepper Motor (w/ control board)
    - https://components101.com/sites/default/files/component_datasheet/28byj48-step-motor-datasheet.pdf
- 5v DC Motor (w/ fan blade)
- DHT11 Sensor
    - https://components101.com/sites/default/files/component_datasheet/DHT11-Temperature-Sensor.pdf
- Resistive Water Level Sensor
- 3x Axial Push Buttons
- 4x LEDs (Green, Yellow, Red, and Blue)
- 10k Potentiometer
- 1x 220 Ohm Resistor
- 4x 330 Ohm Resistors
- 3x 1K Ohm Resistors
- 2N2222 NPN Transistor
    - https://components101.com/sites/default/files/component_datasheet/2N2222%20NPN-transistor%20datasheet.PDF
- 1N4007 Diode
    - https://components101.com/sites/default/files/component_datasheet/1N4001.pdf
- 5v Powersupply
- 3x Breadboards (Large, Medium, and Small sized)
- Tons of Jumper Wires

## Schematic
Basic schematic layout for the assembled system.

![image](https://user-images.githubusercontent.com/77858921/206936560-267190f2-23aa-4e59-91a5-c63fec3c5a4b.png)

## Operation Description
This system operates within 1 of 4 states. The system can operate in the:
- `DISABLED` State
    - In this state many of the main functions are disabled.
    - Indicated by a YELLOW status LED and the `(DISABLED)` message displayed to the LCD.
    - Temperature, humidity, and water-level monitoring is disabled.
    -  The fan motor is turned OFF.
    - System will exit this state when the Start/Stop button is pressed (enters `RUNNING`).
- `IDLE` State
    - In this state most functions are enabled.
    - Indicated by a GREEN status LED and the `IDLE` message displayed to the LCD.
    - Temperature, humidity, and water-level monitoring is enabled and output to the LCD.
    - The fan motor is turned OFF.
    - System can exit this state in several ways:
        - Exits upon press of the Start/Stop button (enters `DISABLED`)
        - Exits upon temperature falling outside of the threshold (enters `RUNNING`)
        - Exits upon water-level falling outside of the threshold (enters `ERROR`)
- `ERROR` State
    - In this state most functions are disabled.
    - Indicated by a RED status LED and the `ERROR` message displayed to the LCD.
    - Temperature, humidity, and water-level monitoring is disabled.
    - The fan motor is turned OFF.
    - An error message is displayed to the LCD.
    - System can exit this tate in several ways:
        - Exits upon press of the Start/Stop button (enters `DISABLED`)
        - Exits upon press of the Reset button if water-level threshold is satisfied (enters IDLE)
- `RUNNING` State
    - In this state all functions are enabled.
    - Indicated by a BLUE status LED and the `RUNNING` message displayed to the LCD.
    - Temperature, humidity, and water-level monitoring is enabled and output to the LCD.
    - The fan motor is turned ON.
    - System can exit this tate in several ways:
        - Exits upon press of the Start/Stop button (enters `DISABLED`)
        - Exits when temperature threshold is satisfied (enters `IDLE`)
        - Exits when water-level threshold is violated (enters `ERROR`)

State transitions are reported to the serial monitor with time and date stamps.

In all states, control of the vent positioning is enabled.

## System Images
Below are a few images of the system in the various states of operation.
- DISABLED
    ![20221211_142903](https://user-images.githubusercontent.com/77858921/206932733-c10a1581-e187-4e18-bf46-669b94b86a0b.jpg)
- IDLE
    ![20221211_143042](https://user-images.githubusercontent.com/77858921/206932739-ed94518c-3d0b-4c1c-a671-015a03b79f5f.jpg)
- ERROR
    ![20221211_142923](https://user-images.githubusercontent.com/77858921/206932744-ccde95c3-66c8-4dfe-bba6-ec2353cc1c42.jpg)
- RUNNING
    ![20221211_142854](https://user-images.githubusercontent.com/77858921/206932737-e274a78b-19de-4d82-bde8-9b9964b7bac2.jpg)

Below is a screenshot of the serial output which records system state changes.

![image](https://user-images.githubusercontent.com/77858921/206932961-ebe597a3-1ccb-40cc-962f-3f4ce0e365f8.png)