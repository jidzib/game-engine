#pragma once
#include <Windows.h>
#include <memory>
#include <engine/Camera.hpp>
#include <engine/Scene.hpp>

namespace engine {
class Renderer {
public:
    Renderer(HWND window, unsigned int width, unsigned int height);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void resize(unsigned int width, unsigned int height);
    // Clear and draw the scene to the default framebuffer, leaving it ready for composition.
    void drawScene(const Camera& camera, const Scene& scene);
    // Present the completed frame once and apply the configured frame pacing.
    void present();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
