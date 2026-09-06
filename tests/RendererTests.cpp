#include "Renderer.hpp"
#include "Window.hpp"
#include <gl/GL.h>
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::vector<unsigned char> pixels(engine::SceneTexture texture) {
    std::vector<unsigned char> result(static_cast<size_t>(texture.width) * texture.height * 3);
    glBindTexture(GL_TEXTURE_2D, texture.handle);
    GLint width = 0, height = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    expect(width == static_cast<GLint>(texture.width) && height == static_cast<GLint>(texture.height),
        "Texture metadata must match allocated storage");
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, result.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return result;
}
int redWidth(const std::vector<unsigned char>& image, unsigned int width, unsigned int height) {
    int first = static_cast<int>(width), last = -1;
    for (unsigned int x = 0; x < width; ++x) {
        const auto i = (static_cast<size_t>(height / 2) * width + x) * 3;
        if (image[i] > 30 && image[i + 1] == 0 && image[i + 2] == 0) {
            first = std::min(first, static_cast<int>(x)); last = static_cast<int>(x);
        }
    }
    return last - first + 1;
}
}
int main() {
    try {
        engine::Window window;
        {
            engine::Renderer renderer(window.handle(), 0, 0);
            engine::Camera camera;
            camera.orbit(-2.55f, -0.40f);
            engine::Scene scene;
            scene.player.object.position = {100, 100, 100};
            scene.addCube("Near", {}, {1, 1, 1}, {1, 0, 0});
            scene.addCube("Far", {0, -0.32f, -4}, {1, 1, 1}, {0, 1, 0});
            expect(renderer.sceneTexture().handle == 0, "Zero initial size must expose no texture");
            renderer.drawScene(camera, scene); renderer.blitSceneToWindow(); renderer.present();
            renderer.resize(320, 240);
            // Simulate state left by a UI pass. The next scene must reset it.
            glEnable(GL_SCISSOR_TEST); glScissor(0, 0, 1, 1);
            glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            renderer.drawScene(camera, scene);
            const auto texture = renderer.sceneTexture();
            const auto original = pixels(texture);
            const auto center = (static_cast<size_t>(120) * 320 + 160) * 3;
            expect(original[center] > 30 && original[center + 1] == 0, "Near red cube must occlude the far green cube");
            std::swap(scene.cubes[0], scene.cubes[1]);
            renderer.drawScene(camera, scene);
            expect(pixels(texture) == original, "Depth visibility must be independent of cube draw order");
            renderer.blitSceneToWindow();
            std::vector<unsigned char> preview(original.size());
            glReadBuffer(GL_BACK);
            glReadPixels(0, 0, 320, 240, GL_RGB, GL_UNSIGNED_BYTE, preview.data());
            expect(preview == original, "Window preview must match offscreen color pixels");
            GLint drawBinding = -1, readBinding = -1, viewport[4]{};
            glGetIntegerv(0x8CA6, &drawBinding); glGetIntegerv(0x8CAA, &readBinding);
            glGetIntegerv(GL_VIEWPORT, viewport);
            expect(drawBinding == 0 && readBinding == 0 && viewport[2] == 320 && viewport[3] == 240,
                "Preview must return framebuffer and viewport ownership to the window");
            renderer.present();
            for (int i = 0; i < 8; ++i) renderer.resize(320, 240);
            expect(renderer.sceneTexture().handle == texture.handle && pixels(texture) == original,
                "Unchanged resize must retain the texture and its contents");
            renderer.resize(0, 240);
            expect(renderer.sceneTexture().handle == 0, "Zero width must suspend texture exposure");
            renderer.drawScene(camera, scene); renderer.blitSceneToWindow(); renderer.present();
            renderer.resize(320, 0);
            expect(renderer.sceneTexture().handle == 0, "Zero height must suspend texture exposure");
            renderer.resize(320, 240);
            expect(renderer.sceneTexture().handle == texture.handle && pixels(texture) == original,
                "Restore to the same dimensions must reuse storage");
            bool rejected = false;
            try { renderer.resize(std::numeric_limits<unsigned int>::max(), 240); }
            catch (const std::runtime_error& error) {
                rejected = std::string(error.what()).find("exceeds OpenGL limits") != std::string::npos;
            }
            expect(rejected && renderer.sceneTexture().handle == texture.handle,
                "Oversized allocation must report limits and preserve the previous target");
            renderer.resize(640, 240);
            expect(glIsTexture(texture.handle) == GL_FALSE, "Successful resize must release the old texture");
            renderer.drawScene(camera, scene);
            const auto wide = pixels(renderer.sceneTexture());
            expect(redWidth(original, 320, 240) > 0 && redWidth(original, 320, 240) == redWidth(wide, 640, 240),
                "Changing aspect with fixed height must preserve the cube's pixel width");
            for (const auto size : {240u, 480u, 300u, 320u}) {
                renderer.resize(size, 240);
                renderer.drawScene(camera, scene); renderer.blitSceneToWindow(); renderer.present();
            }
            expect(glGetError() == GL_NO_ERROR, "Renderer target operations must not leave GL errors");
        }
        expect(wglGetCurrentContext() == nullptr, "Renderer must release its context before window destruction");
        std::cout << "Offscreen texture, depth, blit, resize, zero-area and size-limit checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
