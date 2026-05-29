#!/bin/bash

# Ask the user for their ESP32 board
read -p "Enter your ESP32 Port (e.g., /dev/ttyACM0): " PORT

# Compile the code with USB CDC enabled which is necessary for the ESP32-c3
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc .

# Upload the code to the board
arduino-cli upload -p $PORT --fqbn esp32:esp32:esp32c3

# Open the Serial Monitor at baudrate of 115200 for user input
arduino-cli monitor -p $PORT --config baudrate=115200