#pragma once
#include <Windows.h>
namespace engine { class Window; class Renderer; struct Scene; }
namespace editor {
struct InputState {
    bool viewportVisible = false;
    bool viewportFocused = false;
    bool viewportHovered = false;
    bool wantCaptureKeyboard = false;
    bool wantCaptureMouse = false;
};
// Destroy before the renderer (current GL context) and native window.
class Editor {
public:
    explicit Editor(engine::Window& window);
    ~Editor();
    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;
    void beginFrame(const engine::Scene& scene, engine::Renderer& renderer);
    void render();
    const InputState& input() const { return input_; }
private:
    static LRESULT handleEvent(HWND, UINT, WPARAM, LPARAM);
    engine::Window& window_;
    InputState input_;
    float dpiScale_ = 0;
};
}
