# Mouse Mover (mm.exe)

A lightweight Windows utility that prevents screen lock by automatically moving the mouse cursor. Runs silently in the system tray with minimal resource usage.

## Features

- **Native Windows application** - No console window, runs in system tray
- **Minimal footprint** - Only ~1-2MB memory usage
- **Zero dependencies** - Single standalone executable
- **Smart detection** - Pauses when user is active
- **Easy controls** - Right-click tray icon for options
- **Configurable** - Adjust timing and movement distance

![Mouse Mover Icon](assets/mouse-animal.ico)

---

## 🎯 Usage

### Quick Start
1. **Download** the latest `mm.exe` from releases
2. **Run** the executable - it will appear in your system tray
3. **Right-click** the mouse icon in system tray for options
4. **Double-click** the tray icon to quickly pause/resume

### Requirements

- Windows 10 or Windows 11
- No additional runtime dependencies

### Default Behavior
- **Movement**: 5 pixels every 5 seconds
- **User Detection**: Pauses 30 seconds after keyboard/mouse activity
- **Movement Pattern**: Alternates between horizontal, vertical, and diagonal

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
│   ├── main.cpp           # Main application
│   ├── resource.rc        # Windows resources & version info
│   ├── resource.h         # Resource definitions
│   └── mm.manifest        # Application manifest
├── bin/
│   ├── Debug/             # Debug builds
│   └── Release/           # Release builds
├── assets/
│   └── mouse-animal.ico   # Application icon
├── legacy/                # Previous MinGW-based code
│   ├── main.cpp           # Legacy source
│   └── assets/            # Legacy assets
├── mm.sln                 # Visual Studio solution
├── mm.vcxproj            # Visual Studio project
├── CLAUDE.md             # Development instructions
└── README.md             # This file
```

### Development Environment

Built with Visual Studio 2022 and the Windows SDK:
- **MSVC compiler** for native Windows binaries
- **Static linking** ensures no runtime dependencies
- **Full Unicode support** for proper Windows text handling
- **Embedded manifest** for Windows compatibility

### Prerequisites
- **Visual Studio 2022** (Community/Professional/Enterprise)
- **Windows 10/11 SDK**
- **MSVC v143 toolset**

### Build Commands
```cmd
# In Visual Studio:
Build > Build Solution     (Ctrl+Shift+B)
Debug > Start Debugging    (F5)
Build > Rebuild Solution   (Ctrl+Alt+F7)

# Command line (Developer Command Prompt):
msbuild mm.sln /p:Configuration=Release /p:Platform=x64
```

### Build Configurations
- **Debug**: Full debug symbols, unoptimized, console output
- **Release**: Optimized, static runtime linking, minimal size

### Technical Details
- **Language**: C++17 with Win32 API
- **Threading**: std::thread for mouse movement
- **Resources**: Icon embedded via Windows Resource System
- **Memory**: ~1-2MB runtime usage
- **Dependencies**: Statically linked, no runtime dependencies
- **Build System**: Visual Studio 2022 with MSVC compiler

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


### Contributing
1. Fork repository and create feature branch
2. Follow existing code style and patterns  
3. Test thoroughly on Windows 10/11
4. Build successful with no warnings
5. Submit pull request with clear description


### Release Process
1. Update version in `src/resource.rc` (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)
2. Build release: `msbuild mm.sln /p:Configuration=Release /p:Platform=x64`
3. Test executable on Windows 10/11
4. Create GitHub release with `bin/Release/mm.exe`

---

## License

This project is released under the MIT License. See LICENSE file for details.

## Credits

Inspired by [domax/mouse-mover](https://github.com/domax/mouse-mover).