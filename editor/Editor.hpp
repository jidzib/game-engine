#pragma once
#include <Windows.h>
#include "Navigation.hpp"
#include "Operations.hpp"
namespace engine { class Window; class Renderer; struct Scene; }
namespace editor {
struct InputState {
    bool viewportVisible = false;
    bool viewportFocused = false;
    bool viewportHovered = false;
    bool viewportInteractionStarted = false;
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
    void beginFrame(engine::Scene& scene, engine::Renderer& renderer);
    void render();
    void navigate(float seconds);
    const engine::Camera& camera() const { return navigation_.camera(); }
    const InputState& input() const { return input_; }
    Operations& operations() { return operations_; }
private:
    void inspector(engine::Scene& scene);
    Operations operations_;
    std::string validationError_;
    static LRESULT handleEvent(HWND, UINT, WPARAM, LPARAM);
    engine::Window& window_;
    InputState input_;
    float dpiScale_ = 0;
    Navigation navigation_;
    RECT viewportRect_{}; // Screen coordinates for event-time wheel hit testing.
    float wheel_ = 0;
    bool interrupted_ = false;
};
}
