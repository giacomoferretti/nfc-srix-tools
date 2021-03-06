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
mv srix-dump ../
mv srix-read ../
mv srix-reset ../
mv srix-restore ../

# Cleanup
cd ../
rm -fr "${TEMP_DIR}"
