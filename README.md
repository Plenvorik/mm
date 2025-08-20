# Mouse Mover (mm.exe)

## 💡 Motivation

This project was inspired by [domax/mouse-mover](https://github.com/domax/mouse-mover), a Java-based utility that prevents screen lock by automatically moving the mouse cursor. While the original project works well, I noticed some drawbacks that motivated me to create this C++ alternative:

**Issues with the Java version:**
- Always keeps a **console window open** (distracting)
- **High memory usage** due to JVM overhead (~100MB+)
- Requires **Java Runtime Environment** installation
- Slower startup time due to JVM initialization

**My improvements:**
- **Native Windows application** with no visible windows
- **Minimal memory footprint** (~1-2MB runtime usage)
- **No dependencies** - single executable, no JRE required
- **System tray integration** with context menu controls
- **Embedded resources** - no external files needed
- **Faster startup** and lower CPU usage

**Developer Background:**
I primarily come from the **Linux world** and prefer developing in **WSL**, so this is one of my first native Windows applications. If you spot any Windows-specific issues or improvements, I'd greatly appreciate your feedback and contributions!

![Mouse Mover Icon](assets/mouse-animal.ico)

---

## 🎯 For Users

### Quick Start
1. **Download** the latest `mm.exe` from releases
2. **Run** the executable - it will appear in your system tray
3. **Right-click** the mouse icon in system tray for options
4. **Double-click** the tray icon to quickly pause/resume

### Features
- **🖱️ Smart Movement** - Moves cursor 5px every 5 seconds
- **⏸️ Auto-Pause** - Detects user activity and pauses for 30 seconds
- **👻 Hidden Operation** - No windows, runs silently in background
- **🎛️ System Tray Controls** - Right-click for Pause/Resume/Autostart/Exit
- **🔒 Secure** - No network access, no data collection, fully offline
- **💾 Lightweight** - Only 2.3MB, minimal memory usage (~1-2MB runtime)

### Comparison with Java Version

| Feature | Java Version | This C++ Version |
|---------|-------------|------------------|
| Runtime Memory | ~100MB+ (JVM) | ~1-2MB |
| Dependencies | Java 8+ Required | None (standalone) |
| Startup Time | Slow (JVM init) | Instant |
| Console Window | Always visible | Hidden (system tray) |
| File Size | ~10MB+ | 2.3MB |
| User Interface | Console only | System tray + context menu |

### Configuration
- **Movement**: 5 pixels horizontal every 5 seconds (configurable)
- **User Detection**: Pauses 30 seconds after keyboard/mouse activity
- **Autostart**: Optional Windows startup integration via registry

### Command Line Options
```cmd
mm.exe [options]
  -s, --short-delay SECONDS   Movement interval (1-3600, default: 5)
  -l, --long-delay SECONDS    Pause after activity (0-7200, default: 30)
  -d, --distance PIXELS       Movement distance (1-100, default: 5)
  -h, --help                  Show help information
```

### Troubleshooting
- **Icon missing**: Embedded in executable - no external files needed
- **Not working**: Run as administrator or check antivirus settings
- **Teams status**: Keep Teams window minimized, not closed

---

## 🛠️ For Developers

### Project Structure
```
mm/
├── src/                    # Source code
│   ├── main.cpp           # Main application (single file)
│   ├── resource.rc        # Windows resources & icon
│   └── resource.h         # Resource definitions
├── bin/
│   └── mm.exe             # Compiled executable (2.3MB)
├── assets/
│   └── mouse-animal.ico   # Icon asset for build
├── scripts/               # Build scripts
│   ├── setup-wsl.sh      # WSL environment setup
│   └── build-wsl.sh      # Build script
├── Makefile              # Build system
├── CLAUDE.md             # Development instructions
└── README.md             # This file
```

### Why WSL Cross-Compilation?

As a Linux developer, I prefer the WSL environment for development:
- **Familiar toolchain** - GCC, Make, shell scripts
- **Better terminal** experience than Windows Command Prompt
- **Cross-compilation** - develop on Linux, target Windows
- **Package management** - easy MinGW-w64 installation
- **Consistent build environment** across different machines

### Prerequisites
- **WSL2** with Ubuntu 24.04+
- **MinGW-w64** cross-compiler: `x86_64-w64-mingw32-g++`
- Setup: `./scripts/setup-wsl.sh`

### Build Commands
```bash
make all        # Build mm.exe
make clean      # Clean build artifacts
make run        # Build and run
make debug      # Debug build with -g -DDEBUG
make info       # Show project information
```

### Manual Build Process
```bash
# Compile Windows resources
x86_64-w64-mingw32-windres src/resource.rc -o build/resource.o

# Compile application with static linking
x86_64-w64-mingw32-g++ -std=c++17 -mwindows \
    src/main.cpp build/resource.o \
    -o bin/mm.exe \
    -lgdi32 -luser32 -lshell32 -ladvapi32 \
    -static-libgcc -static-libstdc++
```

### Technical Architecture
- **Language**: C++17 with Win32 API
- **Threading**: std::thread for mouse movement
- **Resources**: Icon embedded via Windows Resource System
- **Memory**: ~1-2MB runtime usage (vs ~100MB+ Java)
- **Dependencies**: Statically linked, no runtime dependencies
- **Build**: Cross-compiled from WSL to Windows x64

### Key Components
1. **System Tray Integration** - Custom icon with context menu
2. **Mouse Movement Engine** - Thread-based cursor manipulation
3. **User Activity Detection** - Monitors for keyboard/mouse input
4. **Registry Integration** - Windows autostart functionality
5. **Command Line Parser** - Parameter validation and help

### Code Quality & Security
- **Input validation** - All command-line parameters validated
- **Buffer overflow protection** - Safe string handling
- **Thread safety** - Atomic operations for shared state
- **Resource cleanup** - Proper Windows handle management
- **Error handling** - Comprehensive error checking

### Windows Development Notes

Since I'm primarily a Linux developer, I welcome feedback on:
- **Win32 API usage** - Are there better/more modern approaches?
- **Windows conventions** - File paths, registry usage, etc.
- **Security considerations** - Windows-specific security best practices
- **Performance optimizations** - Windows-specific improvements
- **Compatibility** - Testing across Windows versions (10, 11)

### Contributing
1. Fork repository and create feature branch
2. Follow existing code style and patterns  
3. Test thoroughly on Windows 10/11
4. Build successful with no warnings
5. Submit pull request with clear description

**Special thanks to Linux/Windows cross-platform developers for guidance!**

### Release Process
1. Update version in `Makefile` and `resource.rc`
2. Build release: `make clean && make all`
3. Test executable thoroughly on multiple Windows versions
4. Create GitHub release with `bin/mm.exe`

---

## 🙏 Acknowledgments

- **[domax/mouse-mover](https://github.com/domax/mouse-mover)** - Original inspiration and concept
- **WSL Team** - Making cross-platform development seamless
- **MinGW-w64 Project** - Excellent cross-compilation toolchain
- **Windows API Documentation** - Comprehensive Win32 reference

---

**Built with WSL cross-compilation | Pure Windows API | Linux developer's first Windows app**