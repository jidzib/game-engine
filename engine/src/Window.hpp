#pragma once
#include <Windows.h>

namespace engine {
class Window {
public:
    Window();
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    bool poll();
    float takeWheelDelta();
    using EventHandler = LRESULT (*)(HWND, UINT, WPARAM, LPARAM);
    void setEventHandler(EventHandler handler) { eventHandler_ = handler; }
    [[nodiscard]] HWND handle() const { return handle_; }
    [[nodiscard]] unsigned int width() const { return width_; }
    [[nodiscard]] unsigned int height() const { return height_; }
    [[nodiscard]] bool minimized() const { return width_ == 0 || height_ == 0; }
    [[nodiscard]] bool active() const { return GetForegroundWindow() == handle_; }
private:
    static LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    HWND handle_ = nullptr;
    HINSTANCE instance_ = GetModuleHandleW(nullptr);
    unsigned int width_ = 1280;
    unsigned int height_ = 720;
    float wheelDelta_ = 0.0f;
    bool closing_ = false;
    EventHandler eventHandler_ = nullptr;
};
}
