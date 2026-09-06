#pragma once
#include <Windows.h>
#include <memory>
#include <engine/Camera.hpp>
#include <engine/Scene.hpp>

namespace engine {
// Borrowed GL texture, valid until a successful size change or renderer destruction.
// Empty while the drawable has zero area. Use only with this renderer's context.
struct SceneTexture {
    unsigned int handle = 0;
    unsigned int width = 0, height = 0;
};
class Renderer {
public:
    Renderer(HWND window, unsigned int width, unsigned int height);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void resize(unsigned int width, unsigned int height);
    void resizeScene(unsigned int width, unsigned int height);
    void prepareWindow(unsigned int width, unsigned int height);
    // Owns GL state; no caller state is preserved. Clears/draws the offscreen target,
    // then leaves framebuffer 0, program 0 and VAO 0 bound. Skips zero-area drawables.
    void drawScene(const Camera& camera, const Scene& scene);
    SceneTexture sceneTexture() const noexcept;
    // Full-window preview for the sandbox. Leaves framebuffer 0 and the window
    // viewport ready for UI, with depth/culling/blending/scissor disabled and writes enabled.
    void blitSceneToWindow();
    // Present the completed frame once and apply the configured frame pacing.
    void present();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
