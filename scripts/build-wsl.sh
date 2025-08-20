#!/bin/bash
# Build script for WSL (Cross-compilation to Windows)

set -e

echo "Mouse Mover - WSL Cross-Compilation Build Script"
echo "================================================="

# Check if we're in WSL
if [[ ! $(uname -r) =~ WSL|microsoft ]]; then
    echo "Warning: This script is designed for WSL environment"
fi

# Install dependencies if needed
echo "Checking dependencies..."

# Update package list
sudo apt update -qq

# Install MinGW-w64 cross-compiler if not present
if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "Installing MinGW-w64 cross-compiler..."
    sudo apt install -y gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64
fi

# Install windres if needed
if ! command -v x86_64-w64-mingw32-windres &> /dev/null; then
    echo "Installing windres (Windows resource compiler)..."
    sudo apt install -y binutils-mingw-w64-x86-64
fi

echo "Dependencies installed successfully!"

# Cross-compilation settings
export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++
export WINDRES=x86_64-w64-mingw32-windres

echo "Using cross-compiler: $CXX"
$CXX --version | head -1

# Build flags
CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -static-libgcc -static-libstdc++"
LDFLAGS="-lgdi32 -luser32 -lshell32 -ladvapi32 -lcomctl32"

echo ""
echo "Building Mouse Mover..."
echo "======================"

# Clean previous build
echo "Cleaning previous build..."
rm -f *.o mouse-mover.exe

# Compile source files
echo "Compiling main.cpp..."
$CXX $CXXFLAGS -c main.cpp -o main.o

echo "Compiling MouseMover.cpp..."
$CXX $CXXFLAGS -c MouseMover.cpp -o MouseMover.o

# Compile Windows resources
echo "Compiling resources..."
$WINDRES resource.rc -o resource.o

# Link executable
echo "Linking executable..."
$CXX main.o MouseMover.o resource.o -o mouse-mover.exe $LDFLAGS

# Check if build succeeded
if [[ -f mouse-mover.exe ]]; then
    echo ""
    echo "Build successful! ✓"
    echo "=================="
    ls -la mouse-mover.exe
    
    # File information
    echo ""
    echo "File information:"
    file mouse-mover.exe
    
    echo ""
    echo "Size: $(du -h mouse-mover.exe | cut -f1)"
    
    # Copy to Windows accessible location
    WINDOWS_PATH="/mnt/c/Users/de6480/OneDrive - NTT DATA Business Solutions AG/01 - Development/mm/cpp"
    if [[ -d "$WINDOWS_PATH" ]]; then
        echo ""
        echo "Copying to Windows path..."
        cp mouse-mover.exe "$WINDOWS_PATH/"
        echo "Copied to: $WINDOWS_PATH/mouse-mover.exe"
    fi
    
    echo ""
    echo "Build completed successfully!"
    echo "You can now run: ./mouse-mover.exe --help"
    
else
    echo ""
    echo "Build failed! ✗"
    exit 1
fi