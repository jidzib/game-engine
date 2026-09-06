#include "Window.hpp"
#include <stdexcept>
#include <utility>

namespace engine {
namespace { constexpr wchar_t windowClass[] = L"CppGameEngineWindow"; }

Window::Window() {
    WNDCLASSEXW description{};
    description.cbSize = sizeof(description);
    description.style = CS_OWNDC;
    description.lpfnWndProc = procedure;
    description.hInstance = instance_;
    description.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    description.lpszClassName = windowClass;
    if (!RegisterClassExW(&description)) { throw std::runtime_error("Window class registration failed."); }
    RECT bounds{0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_)};
    AdjustWindowRect(&bounds, WS_OVERLAPPEDWINDOW, FALSE);
    handle_ = CreateWindowExW(0, windowClass,
        L"C++ Game Engine - OpenGL | WASD: move player | Arrows: orbit | Wheel: zoom | R: camera reset | Esc: exit",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        bounds.right - bounds.left, bounds.bottom - bounds.top,
        nullptr, nullptr, instance_, this);
    if (!handle_) {
        UnregisterClassW(windowClass, instance_);
        throw std::runtime_error("Could not create the game window.");
    }
    ShowWindow(handle_, SW_SHOW);
}
Window::~Window() {
    if (handle_) { DestroyWindow(handle_); }
    UnregisterClassW(windowClass, instance_);
}
bool Window::poll() {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) { return false; }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return handle_ != nullptr && !closing_;
}
float Window::takeWheelDelta() { return std::exchange(wheelDelta_, 0.0f); }

LRESULT CALLBACK Window::procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        self = static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self) {
        switch (message) {
        case WM_CLOSE:
            // Release the renderer and GL context before destroying the HWND.
            self->closing_ = true;
            return 0;
        case WM_SIZE:
            self->width_ = LOWORD(lParam);
            self->height_ = HIWORD(lParam);
            return 0;
        case WM_MOUSEWHEEL:
            self->wheelDelta_ += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            return 0;
        case WM_GETMINMAXINFO: {
            auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize = {480, 320};
            return 0;
        }
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) { self->closing_ = true; return 0; }
            break;
        case WM_ERASEBKGND: return 1;
        case WM_DESTROY:
            self->handle_ = nullptr;
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
}
