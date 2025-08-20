#include <windows.h>
#include <shellapi.h>
#include "resource.h"
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <atomic>
#include <mutex>
#include <stdexcept>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_ICON 1
#define ID_TRAY_EXIT 1001
#define ID_TRAY_PAUSE 1002
#define ID_TRAY_AUTOSTART 1003

// Configuration structure
struct Config {
    int shortDelay = 5;    // seconds between moves
    int longDelay = 30;    // seconds to wait after user activity
    int distance = 5;      // pixels to move
};

// Global variables
HWND g_hwnd = nullptr;
NOTIFYICONDATA g_nid = {};
std::atomic<bool> g_isPaused{false};
std::atomic<bool> g_isRunning{true};
Config g_config;
std::mutex g_trayMutex;

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateTrayIcon();
void ShowContextMenu();
void MoveMouse();
bool ParseCommandLine(LPSTR cmdLine, Config& config);
void ShowHelp();
bool IsAutostartEnabled();
void SetAutostart(bool enable);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Parse command line parameters
    if (!ParseCommandLine(lpCmdLine, g_config)) {
        return 1; // Help was shown or error occurred
    }
    
    // Register window class
    const char* className = "MouseMoverClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    
    if (!RegisterClass(&wc)) {
        MessageBox(nullptr, "Failed to register window class", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Create message-only window (invisible)
    g_hwnd = CreateWindow(
        className, "Mouse Mover",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, hInstance, nullptr
    );
    
    if (!g_hwnd) {
        MessageBox(nullptr, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Create tray icon
    CreateTrayIcon();
    
    // Start mouse movement thread
    std::thread mouseThread([]() {
        while (g_isRunning.load()) {
            if (!g_isPaused.load()) {
                MoveMouse();
            }
            std::this_thread::sleep_for(std::chrono::seconds(g_config.shortDelay));
        }
    });
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    g_isRunning = false;
    if (mouseThread.joinable()) {
        mouseThread.join();
    }
    
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
    
    // Clean up icon handle
    if (g_nid.hIcon) {
        DestroyIcon(g_nid.hIcon);
    }
    
    return 0;
}

bool ParseCommandLine(LPSTR cmdLine, Config& config) {
    if (!cmdLine || strlen(cmdLine) == 0) {
        return true; // No parameters, use defaults
    }
    
    std::string cmd(cmdLine);
    
    // Check for help
    if (cmd.find("-h") != std::string::npos || cmd.find("--help") != std::string::npos) {
        ShowHelp();
        return false;
    }
    
    // Parse parameters
    std::istringstream iss(cmd);
    std::string token;
    
    while (iss >> token) {
        if ((token == "-s" || token == "--short-delay") && iss >> token) {
            try {
                int value = std::stoi(token);
                if (value < 1 || value > 3600) {
                    MessageBox(nullptr, "Short-delay must be between 1 and 3600 seconds", "Parameter Error", MB_OK | MB_ICONERROR);
                    return false;
                }
                config.shortDelay = value;
            } catch (const std::invalid_argument& e) {
                MessageBox(nullptr, "Invalid short-delay parameter: not a number", "Parameter Error", MB_OK | MB_ICONERROR);
                return false;
            } catch (const std::out_of_range& e) {
                MessageBox(nullptr, "Short-delay parameter out of range", "Parameter Error", MB_OK | MB_ICONERROR);
                return false;
            }
        }
        else if ((token == "-l" || token == "--long-delay") && iss >> token) {
            try {
                int value = std::stoi(token);
                if (value < 0 || value > 7200) {
                    MessageBox(nullptr, "Long-delay must be between 0 and 7200 seconds", "Parameter Error", MB_OK | MB_ICONERROR);
                    return false;
                }
                config.longDelay = value;
            } catch (const std::invalid_argument& e) {
                MessageBox(nullptr, "Invalid long-delay parameter: not a number", "Parameter Error", MB_OK | MB_ICONERROR);
                return false;
            } catch (const std::out_of_range& e) {
                MessageBox(nullptr, "Long-delay parameter out of range", "Parameter Error", MB_OK | MB_ICONERROR);
                return false;
            }
        }
        else if ((token == "-d" || token == "--distance") && iss >> token) {
            try {
                int value = std::stoi(token);
                if (value < 1 || value > 100) {
                    MessageBox(nullptr, "Distance must be between 1 and 100 pixels", "Parameter Error", MB_OK | MB_ICONERROR);
                    return false;
                }
                config.distance = value;
            } catch (const std::invalid_argument& e) {
                MessageBox(nullptr, "Invalid distance parameter: not a number", "Parameter Error", MB_OK | MB_ICONERROR);
                return false;
            } catch (const std::out_of_range& e) {
                MessageBox(nullptr, "Distance parameter out of range", "Parameter Error", MB_OK | MB_ICONERROR);
                return false;
            }
        }
    }
    
    // Enhanced parameter validation
    if (config.shortDelay < 1 || config.shortDelay > 3600) {
        MessageBox(nullptr, "Short delay must be between 1 and 3600 seconds", "Parameter Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    if (config.longDelay < 1 || config.longDelay > 7200) {
        MessageBox(nullptr, "Long delay must be between 1 and 7200 seconds", "Parameter Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    if (config.shortDelay > config.longDelay) {
        MessageBox(nullptr, "Short delay must be less than or equal to long delay", "Parameter Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    if (config.distance == 0 || abs(config.distance) > 100) {
        MessageBox(nullptr, "Distance must be between -100 and 100 pixels (non-zero)", "Parameter Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    return true;
}

void ShowHelp() {
    std::string helpText = 
        "Mouse Mover v1.0 - Prevents screen lock\n\n"
        "Usage: mouse-mover.exe [OPTIONS]\n\n"
        "Options:\n"
        "  -s, --short-delay SECONDS   Short delay between moves (default: 5)\n"
        "  -l, --long-delay SECONDS    Long delay after user activity (default: 30)\n"
        "  -d, --distance PIXELS       Distance in pixels to move (default: 5)\n"
        "  -h, --help                  Show this help\n\n"
        "Examples:\n"
        "  mouse-mover.exe -s 3 -l 15 -d 10\n"
        "  mouse-mover.exe --short-delay 2 --long-delay 60\n\n"
        "The application runs in the system tray.\n"
        "Right-click the tray icon for options.";
    
    MessageBox(nullptr, helpText.c_str(), "Mouse Mover Help", MB_OK | MB_ICONINFORMATION);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                ShowContextMenu();
            } else if (lParam == WM_LBUTTONDBLCLK) {
                g_isPaused = !g_isPaused.load();
                // Update tooltip
                const char* status = g_isPaused.load() ? "Mouse Mover - Paused" : "Mouse Mover - Active";
                strncpy_s(g_nid.szTip, sizeof(g_nid.szTip), status, _TRUNCATE);
                Shell_NotifyIcon(NIM_MODIFY, &g_nid);
            }
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_PAUSE: {
                    g_isPaused = !g_isPaused.load();
                    // Update tooltip
                    const char* status = g_isPaused.load() ? "Mouse Mover - Paused" : "Mouse Mover - Active";
                    strncpy_s(g_nid.szTip, sizeof(g_nid.szTip), status, _TRUNCATE);
                    Shell_NotifyIcon(NIM_MODIFY, &g_nid);
                    break;
                }
                case ID_TRAY_AUTOSTART:
                    SetAutostart(!IsAutostartEnabled());
                    break;
                case ID_TRAY_EXIT:
                    g_isRunning = false;
                    PostQuitMessage(0);
                    break;
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void CreateTrayIcon() {
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = ID_TRAY_ICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    
    // Load embedded icon from resources
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    g_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    if (!g_nid.hIcon) {
        // Fallback to system icon if embedded icon fails
        g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    
    // Create tooltip with current settings
    int result = snprintf(g_nid.szTip, sizeof(g_nid.szTip), "Mouse Mover - Active (Move: %ds, Wait: %ds)", 
                         g_config.shortDelay, g_config.longDelay);
    if (result < 0 || result >= sizeof(g_nid.szTip)) {
        strncpy_s(g_nid.szTip, sizeof(g_nid.szTip), "Mouse Mover - Active", _TRUNCATE);
    }
    
    if (!Shell_NotifyIcon(NIM_ADD, &g_nid)) {
        MessageBox(nullptr, "Failed to create system tray icon.", "Error", MB_ICONERROR);
    }
}

void ShowContextMenu() {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    
    // Pause/Resume
    AppendMenu(hMenu, MF_STRING, ID_TRAY_PAUSE, g_isPaused.load() ? "Resume" : "Pause");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    
    // Autostart option
    bool autostartEnabled = IsAutostartEnabled();
    AppendMenu(hMenu, MF_STRING, ID_TRAY_AUTOSTART, 
               autostartEnabled ? "Disable Autostart" : "Enable Autostart");
    
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");
    
    SetForegroundWindow(g_hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_hwnd, nullptr);
    DestroyMenu(hMenu);
}

bool IsAutostartEnabled() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                     "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                     0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    DWORD dwType, dwSize = 0;
    bool exists = (RegQueryValueEx(hKey, "MouseMover", nullptr, &dwType, nullptr, &dwSize) == ERROR_SUCCESS);
    
    RegCloseKey(hKey);
    return exists;
}

void SetAutostart(bool enable) {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                     0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        MessageBox(g_hwnd, "Failed to access registry", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (enable) {
        // Get current executable path
        char exePath[MAX_PATH];
        DWORD pathLength = GetModuleFileName(nullptr, exePath, MAX_PATH);
        if (pathLength == 0 || pathLength == MAX_PATH) {
            RegCloseKey(hKey);
            return;
        }
        
        // Set registry value
        if (RegSetValueEx(hKey, "MouseMover", 0, REG_SZ, 
                          (BYTE*)exePath, strlen(exePath) + 1) == ERROR_SUCCESS) {
            MessageBox(g_hwnd, "Autostart enabled successfully", "Mouse Mover", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(g_hwnd, "Failed to enable autostart", "Error", MB_OK | MB_ICONERROR);
        }
    } else {
        // Remove registry value
        if (RegDeleteValue(hKey, "MouseMover") == ERROR_SUCCESS) {
            MessageBox(g_hwnd, "Autostart disabled successfully", "Mouse Mover", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(g_hwnd, "Failed to disable autostart", "Error", MB_OK | MB_ICONERROR);
        }
    }
    
    RegCloseKey(hKey);
}

void MoveMouse() {
    static int direction = 5;
    static POINT lastUserPos = {-1, -1};
    static auto lastUserActivity = std::chrono::steady_clock::now();
    static bool userWasActive = false;
    
    POINT currentPos;
    GetCursorPos(&currentPos);
    
    // Check if user moved mouse
    if (currentPos.x != lastUserPos.x || currentPos.y != lastUserPos.y) {
        lastUserPos = currentPos;
        lastUserActivity = std::chrono::steady_clock::now();
        userWasActive = true;
        return; // User is active, don't move
    }
    
    // Wait for user inactivity period
    if (userWasActive) {
        auto timeSinceActivity = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - lastUserActivity).count();
        
        if (timeSinceActivity < g_config.longDelay) {
            return; // Still in waiting period
        }
        userWasActive = false;
    }
    
    // Move mouse by configured distance
    POINT newPos = {currentPos.x + (direction * g_config.distance), currentPos.y};
    
    // Simple boundary check
    if (newPos.x < 10 || newPos.x > GetSystemMetrics(SM_CXSCREEN) - 10) {
        direction = -direction;
        newPos.x = currentPos.x + (direction * g_config.distance);
    } else {
        // Only alternate direction if we didn't hit boundary
        direction = -direction;
    }
    
    SetCursorPos(newPos.x, newPos.y);
}