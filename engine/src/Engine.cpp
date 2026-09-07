#include <engine/Engine.hpp>
#include <engine/Camera.hpp>
#include "Window.hpp"
#include "Renderer.hpp"
#include <Editor.hpp>
#include <algorithm>
#include <chrono>

namespace engine {
void Engine::run(unsigned int frameLimit) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Window window;
    Renderer renderer(window.handle(), window.width(), window.height());
    editor::Editor editor(window);
    using Clock = std::chrono::steady_clock;
    auto previous = Clock::now();
    unsigned int frames = 0;
    while (window.poll()) {
        const auto now = Clock::now();
        const float delta = std::clamp(std::chrono::duration<float>(now - previous).count(), 0.0f, 0.05f);
        previous = now;
        if (window.minimized()) {
            if (frameLimit != 0) { if (++frames >= frameLimit) break; }
            else WaitMessage();
            continue;
        }
        editor.beginFrame(scene, renderer);
        // Start in Edit mode. Gameplay movement/collision remains available to
        // a future Play mode, but authored scene data is never simulated here.
        editor.navigate(delta);
        renderer.drawScene(editor.camera(), scene);
        renderer.prepareWindow(window.width(), window.height());
        editor.render();
        renderer.present();
        if (frameLimit != 0 && ++frames >= frameLimit) { break; }
    }
}
}
