# Mouse Mover - Professional C++ Windows Application
# 
# Build system using WSL cross-compilation
#

PROJECT_NAME = mm
VERSION = 1.0

# Directories
SRC_DIR = src
BIN_DIR = bin
BUILD_DIR = build
ASSETS_DIR = assets
SCRIPTS_DIR = scripts

# Cross-compiler settings for WSL
CXX = x86_64-w64-mingw32-g++
WINDRES = x86_64-w64-mingw32-windres

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -mwindows
LDFLAGS = -lgdi32 -luser32 -lshell32 -ladvapi32 -static-libgcc -static-libstdc++

# Version defines (can be overridden from command line)
# Example: make VERSION_DEFINES="-DVERSION_MAJOR=1 -DVERSION_MINOR=2 -DVERSION_PATCH=3"
VERSION_DEFINES ?= 

# Add version defines to compiler flags
CXXFLAGS += $(VERSION_DEFINES)
WINDRES_FLAGS = $(VERSION_DEFINES)

# Source files
SOURCES = $(SRC_DIR)/main.cpp
RC_FILE = $(SRC_DIR)/resource.rc
ICON_FILE = $(ASSETS_DIR)/mouse-animal.ico

# Output files
TARGET = $(BIN_DIR)/$(PROJECT_NAME).exe
RC_OBJ = $(BUILD_DIR)/resource.o

# Default target
all: $(TARGET)

# Ensure directories exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile resources
$(RC_OBJ): $(RC_FILE) $(ICON_FILE) | $(BUILD_DIR)
	$(WINDRES) $(WINDRES_FLAGS) $(RC_FILE) -o $(RC_OBJ)

# Build main executable
$(TARGET): $(SOURCES) $(RC_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SOURCES) $(RC_OBJ) -o $(TARGET) $(LDFLAGS)
	@echo ""
	@echo "Build successful! ✓"
	@echo "Executable: $(TARGET)"
	@ls -la $(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	@echo "Clean completed"

# Install target (no external files needed - icon is embedded)
install: $(TARGET)
	@echo "Installation completed - executable is self-contained"
	@echo "Run: $(TARGET)"

# Development build with debugging
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Run the application
run: $(TARGET)
	cd $(BIN_DIR) && ./$(PROJECT_NAME).exe &

# Show project info
info:
	@echo "Mouse Mover v$(VERSION)"
	@echo "======================"
	@echo "Source:    $(SRC_DIR)/"
	@echo "Binary:    $(BIN_DIR)/"
	@echo "Assets:    $(ASSETS_DIR)/"
	@echo "Scripts:   $(SCRIPTS_DIR)/"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build application"
	@echo "  clean    - Clean build files"
	@echo "  install  - Mark installation complete"
	@echo "  run      - Build and run"
	@echo "  debug    - Debug build"

.PHONY: all clean install debug run info