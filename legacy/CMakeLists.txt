cmake_minimum_required(VERSION 3.16)
project(MouseMover VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set Windows subsystem
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

# Add resource file for Windows
set(RESOURCE_FILE src/resource.rc)

# Create executable
add_executable(mm
    src/main.cpp
    ${RESOURCE_FILE}
)

# Set output name
set_target_properties(mm PROPERTIES
    OUTPUT_NAME "mm"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Link Windows libraries
target_link_libraries(mm
    user32
    shell32
    gdi32
    advapi32
)

# Set compile options
target_compile_options(mm PRIVATE
    /W3     # Warning level 3
    /O2     # Optimize for speed
    /EHsc   # Enable C++ exceptions
)

# Install target
install(TARGETS mm
    RUNTIME DESTINATION bin
)