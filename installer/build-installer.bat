@echo off
echo Building Mouse Mover Installer...

REM Ensure we're in the right directory
cd /d "%~dp0"

REM Build the main application first
echo Building main application...
msbuild ..\mm.vcxproj /p:Configuration=Release /p:Platform=x64 /v:minimal
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build main application
    exit /b 1
)

REM Build the installer
echo Building installer...
msbuild MouseMover.wixproj /p:Configuration=Release /p:Platform=x64 /v:minimal
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build installer
    exit /b 1
)

echo.
echo SUCCESS: Installer built successfully!
echo Output: bin\Release\MouseMover.msi
echo.
pause