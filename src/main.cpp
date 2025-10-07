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
#include <memory>

// Constants
namespace {
constexpr UINT kTrayIconMessage = WM_USER + 1;
constexpr UINT kTrayIconId = 1;
constexpr UINT kMenuIdExit = 1001;
constexpr UINT kMenuIdPause = 1002;

constexpr int kMinDelaySeconds = 1;
constexpr int kMaxDelaySeconds = 3600;
constexpr int kMaxLongDelaySeconds = 7200;
constexpr int kMinDistance = 1;
constexpr int kMaxDistance = 100;
constexpr int kScreenBorderMargin = 10;

constexpr const wchar_t* kWindowClassName = L"MouseMoverClass";
constexpr const wchar_t* kWindowTitle = L"Mouse Mover";
}

// Configuration structure
struct Config {
    int short_delay = 5;    // seconds between moves
    int long_delay = 30;    // seconds to wait after user activity
    int distance = 5;       // pixels to move
};

// Main application class
class MouseMoverApp {
public:
    MouseMoverApp();
    ~MouseMoverApp();
    
    int Run(HINSTANCE instance, LPWSTR cmd_line);

private:
    // Core functionality
    bool Initialize(HINSTANCE instance, const std::string& cmd_line);
    void Cleanup();
    void RunMessageLoop();
    
    // Command line parsing
    bool ParseCommandLine(const std::string& cmd_line);
    bool ParseDelayParameter(const std::string& token, int& target, int min_val, int max_val, const std::string& param_name);
    bool ParseDistanceParameter(const std::string& token);
    void ShowHelp() const;
    bool ValidateConfig() const;
    
    // Window management
    bool RegisterWindowClass(HINSTANCE instance);
    bool CreateMessageWindow(HINSTANCE instance);
    static LRESULT CALLBACK WindowProcStatic(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    
    // System tray
    void CreateTrayIcon();
    void ShowContextMenu();
    void UpdateTrayTooltip();
    
    // Mouse movement
    void MouseThreadFunc();
    void MoveMouse();
    bool ShouldPauseForUserActivity();
    
    // Member variables
    HWND hwnd_;
    NOTIFYICONDATA tray_icon_data_;
    std::atomic<bool> is_paused_{false};
    std::atomic<bool> is_running_{true};
    Config config_;
    std::mutex tray_mutex_;
    std::unique_ptr<std::thread> mouse_thread_;
    
    // Mouse movement state
    struct MouseState {
        int move_pattern = 0;  // 0=horizontal, 1=vertical, 2=diagonal
        int direction_x = 1;
        int direction_y = 1;
        POINT last_user_pos = {-1, -1};
        std::chrono::steady_clock::time_point last_user_activity = std::chrono::steady_clock::now();
        bool user_was_active = false;
    } mouse_state_;
};

// Global app instance for window procedure callback
static MouseMoverApp* g_app_instance = nullptr;

// Entry point
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    MouseMoverApp app;
    return app.Run(hInstance, lpCmdLine);
}

// MouseMoverApp implementation
MouseMoverApp::MouseMoverApp() : hwnd_(nullptr) {
    ZeroMemory(&tray_icon_data_, sizeof(tray_icon_data_));
    g_app_instance = this;
}

MouseMoverApp::~MouseMoverApp() {
    Cleanup();
    g_app_instance = nullptr;
}

int MouseMoverApp::Run(HINSTANCE instance, LPWSTR cmd_line) {
    // Convert wide string to narrow string for parsing
    int size = WideCharToMultiByte(CP_UTF8, 0, cmd_line, -1, nullptr, 0, nullptr, nullptr);
    std::string cmd_line_str(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, cmd_line, -1, &cmd_line_str[0], size, nullptr, nullptr);
    
    if (!Initialize(instance, cmd_line_str)) {
        return 1;
    }
    
    RunMessageLoop();
    return 0;
}

bool MouseMoverApp::Initialize(HINSTANCE instance, const std::string& cmd_line) {
    if (!ParseCommandLine(cmd_line)) {
        return false;
    }
    
    if (!ValidateConfig()) {
        return false;
    }
    
    if (!RegisterWindowClass(instance)) {
        MessageBoxW(nullptr, L"Failed to register window class", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    if (!CreateMessageWindow(instance)) {
        MessageBoxW(nullptr, L"Failed to create window", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    CreateTrayIcon();
    
    // Start mouse movement thread
    mouse_thread_ = std::make_unique<std::thread>(&MouseMoverApp::MouseThreadFunc, this);
    
    return true;
}

void MouseMoverApp::Cleanup() {
    is_running_ = false;
    
    if (mouse_thread_ && mouse_thread_->joinable()) {
        mouse_thread_->join();
    }
    
    Shell_NotifyIcon(NIM_DELETE, &tray_icon_data_);
    
    if (tray_icon_data_.hIcon) {
        DestroyIcon(tray_icon_data_.hIcon);
    }
}

void MouseMoverApp::RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool MouseMoverApp::ParseCommandLine(const std::string& cmd_line) {
    if (cmd_line.empty()) {
        return true; // No parameters, use defaults
    }
    
    // Check for help
    if (cmd_line.find("-h") != std::string::npos || cmd_line.find("--help") != std::string::npos) {
        ShowHelp();
        return false;
    }
    
    // Parse parameters
    std::istringstream iss(cmd_line);
    std::string token;
    
    while (iss >> token) {
        if ((token == "-s" || token == "--short-delay") && iss >> token) {
            if (!ParseDelayParameter(token, config_.short_delay, kMinDelaySeconds, kMaxDelaySeconds, "Short-delay")) {
                return false;
            }
        }
        else if ((token == "-l" || token == "--long-delay") && iss >> token) {
            if (!ParseDelayParameter(token, config_.long_delay, 0, kMaxLongDelaySeconds, "Long-delay")) {
                return false;
            }
        }
        else if ((token == "-d" || token == "--distance") && iss >> token) {
            if (!ParseDistanceParameter(token)) {
                return false;
            }
        }
    }
    
    return true;
}

bool MouseMoverApp::ParseDelayParameter(const std::string& token, int& target, int min_val, int max_val, const std::string& param_name) {
    try {
        int value = std::stoi(token);
        if (value < min_val || value > max_val) {
            std::wstring error_msg = L"" + std::wstring(param_name.begin(), param_name.end()) + 
                                   L" must be between " + std::to_wstring(min_val) + 
                                   L" and " + std::to_wstring(max_val) + L" seconds";
            MessageBoxW(nullptr, error_msg.c_str(), L"Parameter Error", MB_OK | MB_ICONERROR);
            return false;
        }
        target = value;
        return true;
    } catch (const std::exception&) {
        std::wstring error_msg = L"Invalid " + std::wstring(param_name.begin(), param_name.end()) + L" parameter";
        MessageBoxW(nullptr, error_msg.c_str(), L"Parameter Error", MB_OK | MB_ICONERROR);
        return false;
    }
}

bool MouseMoverApp::ParseDistanceParameter(const std::string& token) {
    try {
        int value = std::stoi(token);
        if (value < kMinDistance || value > kMaxDistance) {
            MessageBoxW(nullptr, L"Distance must be between 1 and 100 pixels", L"Parameter Error", MB_OK | MB_ICONERROR);
            return false;
        }
        config_.distance = value;
        return true;
    } catch (const std::exception&) {
        MessageBoxW(nullptr, L"Invalid distance parameter", L"Parameter Error", MB_OK | MB_ICONERROR);
        return false;
    }
}

bool MouseMoverApp::ValidateConfig() const {
    if (config_.short_delay > config_.long_delay) {
        MessageBoxW(nullptr, L"Short delay must be less than or equal to long delay", L"Parameter Error", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

void MouseMoverApp::ShowHelp() const {
    std::string help_text = 
        "Mouse Mover v1.0.3 - Prevents screen lock\n\n"
        "Usage: mm.exe [OPTIONS]\n\n"
        "Options:\n"
        "  -s, --short-delay SECONDS   Short delay between moves (default: 5)\n"
        "  -l, --long-delay SECONDS    Long delay after user activity (default: 30)\n"
        "  -d, --distance PIXELS       Distance in pixels to move (default: 5)\n"
        "  -h, --help                  Show this help\n\n"
        "Examples:\n"
        "  mm.exe -s 3 -l 15 -d 10\n"
        "  mm.exe --short-delay 2 --long-delay 60\n\n"
        "The application runs in the system tray.\n"
        "Right-click the tray icon for options.";
    
    // Convert to wide string for MessageBox
    int size = MultiByteToWideChar(CP_UTF8, 0, help_text.c_str(), -1, nullptr, 0);
    std::wstring w_help_text(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, help_text.c_str(), -1, &w_help_text[0], size);
    MessageBoxW(nullptr, w_help_text.c_str(), L"Mouse Mover Help", MB_OK | MB_ICONINFORMATION);
}

bool MouseMoverApp::RegisterWindowClass(HINSTANCE instance) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProcStatic;
    wc.hInstance = instance;
    wc.lpszClassName = kWindowClassName;
    
    return RegisterClassW(&wc) != 0;
}

bool MouseMoverApp::CreateMessageWindow(HINSTANCE instance) {
    hwnd_ = CreateWindowW(
        kWindowClassName, kWindowTitle,
        0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, instance, nullptr
    );
    
    return hwnd_ != nullptr;
}

LRESULT CALLBACK MouseMoverApp::WindowProcStatic(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (g_app_instance) {
        return g_app_instance->WindowProc(hwnd, msg, wparam, lparam);
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT MouseMoverApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case kTrayIconMessage:
            if (lparam == WM_RBUTTONUP) {
                ShowContextMenu();
            } else if (lparam == WM_LBUTTONDBLCLK) {
                is_paused_ = !is_paused_.load();
                UpdateTrayTooltip();
            }
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case kMenuIdPause: {
                    is_paused_ = !is_paused_.load();
                    UpdateTrayTooltip();
                    break;
                }
                case kMenuIdExit:
                    is_running_ = false;
                    PostQuitMessage(0);
                    break;
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return 0;
}

void MouseMoverApp::CreateTrayIcon() {
    ZeroMemory(&tray_icon_data_, sizeof(tray_icon_data_));
    tray_icon_data_.cbSize = sizeof(NOTIFYICONDATA);
    tray_icon_data_.hWnd = hwnd_;
    tray_icon_data_.uID = kTrayIconId;
    tray_icon_data_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    tray_icon_data_.uCallbackMessage = kTrayIconMessage;
    
    // Load embedded icon from resources
    HINSTANCE instance = GetModuleHandle(nullptr);
    tray_icon_data_.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    if (!tray_icon_data_.hIcon) {
        // Fallback to system icon if embedded icon fails
        tray_icon_data_.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    
    UpdateTrayTooltip();
    
    if (!Shell_NotifyIcon(NIM_ADD, &tray_icon_data_)) {
        MessageBoxW(nullptr, L"Failed to create system tray icon.", L"Error", MB_ICONERROR);
    }
}

void MouseMoverApp::UpdateTrayTooltip() {
    std::lock_guard<std::mutex> lock(tray_mutex_);
    
    const wchar_t* status = is_paused_.load() ? L"Mouse Mover - Paused" : L"Mouse Mover - Active";
    
    int result = swprintf_s(tray_icon_data_.szTip, sizeof(tray_icon_data_.szTip)/sizeof(wchar_t), 
                           L"%s (Move: %ds, Wait: %ds)", status, config_.short_delay, config_.long_delay);
    if (result < 0) {
        wcscpy_s(tray_icon_data_.szTip, sizeof(tray_icon_data_.szTip)/sizeof(wchar_t), status);
    }
    
    Shell_NotifyIcon(NIM_MODIFY, &tray_icon_data_);
}

void MouseMoverApp::ShowContextMenu() {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU menu = CreatePopupMenu();
    
    // Pause/Resume
    AppendMenuW(menu, MF_STRING, kMenuIdPause, is_paused_.load() ? L"Resume" : L"Pause");
    AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
    
    AppendMenuW(menu, MF_STRING, kMenuIdExit, L"Exit");
    
    SetForegroundWindow(hwnd_);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}

void MouseMoverApp::MouseThreadFunc() {
    while (is_running_.load()) {
        if (!is_paused_.load()) {
            MoveMouse();
        }
        std::this_thread::sleep_for(std::chrono::seconds(config_.short_delay));
    }
}

void MouseMoverApp::MoveMouse() {
    POINT current_pos;
    GetCursorPos(&current_pos);
    
    // Check if user moved mouse
    if (current_pos.x != mouse_state_.last_user_pos.x || current_pos.y != mouse_state_.last_user_pos.y) {
        mouse_state_.last_user_pos = current_pos;
        mouse_state_.last_user_activity = std::chrono::steady_clock::now();
        mouse_state_.user_was_active = true;
        return; // User is active, don't move
    }
    
    // Wait for user inactivity period
    if (mouse_state_.user_was_active) {
        auto time_since_activity = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - mouse_state_.last_user_activity).count();
        
        if (time_since_activity < config_.long_delay) {
            return; // Still in waiting period
        }
        mouse_state_.user_was_active = false;
    }
    
    // Prepare SendInput structure
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    
    // Calculate movement based on pattern
    switch (mouse_state_.move_pattern) {
        case 0:  // Horizontal movement
            input.mi.dx = mouse_state_.direction_x * config_.distance;
            input.mi.dy = 0;
            break;
        case 1:  // Vertical movement
            input.mi.dx = 0;
            input.mi.dy = mouse_state_.direction_y * config_.distance;
            break;
        case 2:  // Diagonal movement
            input.mi.dx = mouse_state_.direction_x * config_.distance;
            input.mi.dy = mouse_state_.direction_y * config_.distance;
            break;
    }
    
    // Boundary checks and direction changes
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    
    if (current_pos.x + input.mi.dx < kScreenBorderMargin || 
        current_pos.x + input.mi.dx > screen_width - kScreenBorderMargin) {
        mouse_state_.direction_x = -mouse_state_.direction_x;
        input.mi.dx = mouse_state_.direction_x * config_.distance;
    }
    
    if (current_pos.y + input.mi.dy < kScreenBorderMargin || 
        current_pos.y + input.mi.dy > screen_height - kScreenBorderMargin) {
        mouse_state_.direction_y = -mouse_state_.direction_y;
        input.mi.dy = mouse_state_.direction_y * config_.distance;
    }
    
    // Send the input
    SendInput(1, &input, sizeof(INPUT));
    
    // Update last position after movement
    GetCursorPos(&mouse_state_.last_user_pos);
    
    // Cycle through movement patterns
    mouse_state_.move_pattern = (mouse_state_.move_pattern + 1) % 3;
    
    // Change directions occasionally for more natural movement
    if (mouse_state_.move_pattern == 0) {
        mouse_state_.direction_x = -mouse_state_.direction_x;
        mouse_state_.direction_y = -mouse_state_.direction_y;
    }
}