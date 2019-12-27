#!/usr/bin/env bash

# Create build folder
mkdir build
cd build
TEMP_DIR=$(mktemp -d build.XXXXXXXX)
cd "${TEMP_DIR}"

# Build
cmake ../../
make

# Copy executables
mv nfc-srix-read ../
mv print_dump ../
mv dump_eeprom ../

# Cleanup
cd ../
rm -fr "${TEMP_DIR}"