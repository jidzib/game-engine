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
        L"C++ Game Engine - Edit | Hold RMB in viewport to navigate | Esc: exit",
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

LRESULT CALLBACK Window::procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        self = static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self) {
        const LRESULT uiResult = self->eventHandler_ ? self->eventHandler_(hwnd, message, wParam, lParam) : 0;
        switch (message) {
        case WM_DPICHANGED: {
            const auto* bounds = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(hwnd, nullptr, bounds->left, bounds->top,
                bounds->right - bounds->left, bounds->bottom - bounds->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        case WM_CLOSE:
            // Release the renderer and GL context before destroying the HWND.
            self->closing_ = true;
            return 0;
        case WM_SIZE:
            self->width_ = LOWORD(lParam);
            self->height_ = HIWORD(lParam);
            return 0;
        case WM_GETMINMAXINFO: {
            auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize = {480, 320};
            return 0;
        }
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE && !uiResult) { self->closing_ = true; return 0; }
            break;
        case WM_ERASEBKGND: return 1;
        case WM_DESTROY:
            self->handle_ = nullptr;
            PostQuitMessage(0);
            return 0;
        }
        if (uiResult) return uiResult;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
}
