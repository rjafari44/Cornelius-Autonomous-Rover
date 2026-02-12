#!/bin/bash

# Ask the user for their arduino board
read -p "Enter your Arduino Port (e.g., /dev/ttyACM0): " PORT

# Compile the code
arduino-cli compile --fqbn arduino:avr:nano --build-property "compiler.cpp.extra_flags=-Iinclude" Ultrasonic_Rover.ino

# Upload the code to the board
arduino-cli upload -p $PORT --fqbn arduino:avr:nano