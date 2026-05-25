#!/bin/bash
set -e

# get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# ask the user which project to compile
echo "Select project to compile:"
echo "1) Rover (ESP32-C3)"
echo "2) Controller (ESP32)"
read -p "Enter choice (1 or 2): " CHOICE

# set project path and board based on choice
if [ "$CHOICE" == "1" ]; then
  PROJECT_PATH="$PROJECT_ROOT/rover"
  FQBN="esp32:esp32:esp32c3:CDCOnBoot=cdc"
  UPLOAD_FQBN="esp32:esp32:esp32c3"
  echo "Selected: Rover (ESP32-C3)"
elif [ "$CHOICE" == "2" ]; then
  PROJECT_PATH="$PROJECT_ROOT/controller"
  FQBN="esp32:esp32:esp32c3:CDCOnBoot=cdc"
  UPLOAD_FQBN="esp32:esp32:esp32c3"
  echo "Selected: Controller (ESP32)"
else
  echo "Invalid choice"
  exit 1
fi

# ask for port
read -p "Enter your ESP32 Port (e.g., /dev/ttyACM0): " PORT

# compile the code
arduino-cli compile \
  --fqbn "$FQBN" \
  --build-property "build.extra_flags=-I$PROJECT_PATH/include" \
  "$PROJECT_PATH"

# upload the code to the board
arduino-cli upload \
  -p "$PORT" \
  --fqbn "$UPLOAD_FQBN" \
  "$PROJECT_PATH"

# open the Serial Monitor
arduino-cli monitor \
  -p "$PORT" \
  --config baudrate=115200