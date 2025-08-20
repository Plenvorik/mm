#!/bin/bash
# WSL Development Environment Setup

echo "Setting up WSL Development Environment for Mouse Mover"
echo "======================================================"

# Update package list
echo "Updating package list..."
sudo apt update

# Install essential build tools
echo "Installing build tools..."
sudo apt install -y \
    build-essential \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64-x86-64 \
    binutils-mingw-w64-x86-64 \
    git \
    make \
    vim \
    nano

echo ""
echo "Installed tools:"
echo "- GCC: $(gcc --version | head -1)"
echo "- MinGW Cross-Compiler: $(x86_64-w64-mingw32-g++ --version | head -1)"
echo "- Windres: $(x86_64-w64-mingw32-windres --version | head -1)"

echo ""
echo "WSL setup complete! ✓"
echo ""
echo "Next steps:"
echo "1. cd to your project directory"
echo "2. Run: chmod +x build-wsl.sh"
echo "3. Run: ./build-wsl.sh"