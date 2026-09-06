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
    Camera camera;
    using Clock = std::chrono::steady_clock;
    auto previous = Clock::now();
    unsigned int frames = 0;
    while (window.poll()) {
        const auto now = Clock::now();
        const float delta = std::clamp(std::chrono::duration<float>(now - previous).count(), 0.0f, 0.05f);
        previous = now;
        if (window.minimized()) {
            window.takeWheelDelta();
            if (frameLimit != 0) { if (++frames >= frameLimit) break; }
            else WaitMessage();
            continue;
        }
        editor.beginFrame(scene, renderer);
        const float wheel = window.takeWheelDelta();
        const auto& input = editor.input();
        if (window.active() && !input.wantCaptureKeyboard) {
            const auto held = [](int key) { return (GetAsyncKeyState(key) & 0x8000) != 0 ? 1.0f : 0.0f; };
            camera.orbit((held(VK_LEFT) - held(VK_RIGHT)) * delta * 1.5f,
                (held(VK_UP) - held(VK_DOWN)) * delta * 1.2f);
            std::vector<const GameObject*> obstacles;
            for (const auto& cube : scene.cubes) obstacles.push_back(&cube);
            scene.player.move(held('A') - held('D'), held('W') - held('S'), held(VK_SPACE) - held(VK_LSHIFT), delta, obstacles);
            
            if (held('R') != 0.0f) { camera.reset(); }
        }
        camera.setTarget(scene.player.object.position);
        // The image owns wheel input; other UI surfaces keep scrolling exclusively.
        if (window.active() && input.viewportHovered)
            camera.zoom(-wheel * 0.6f);
        renderer.drawScene(camera, scene);
        renderer.prepareWindow(window.width(), window.height());
        editor.render();
        renderer.present();
        if (frameLimit != 0 && ++frames >= frameLimit) { break; }
    }
}
}
