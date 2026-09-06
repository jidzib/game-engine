#include "Editor.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include <imgui.h>
#include <GL/gl.h>
#include <iostream>
#include <stdexcept>
#include <vector>
void expect(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::vector<unsigned char> pixels(engine::SceneTexture t) {
    std::vector<unsigned char> data(size_t(t.width) * t.height * 4);
    glBindTexture(GL_TEXTURE_2D, t.handle);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return data;
}
int main() {
    try {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        engine::Window window;
        {
            engine::Renderer renderer(window.handle(), 0, 0);
            engine::Camera camera;
            engine::Scene scene;
            scene.addCube("Read-only cube", {3, 0, 0});
            {
                editor::Editor ui(window);
                auto frame = [&] {
                    expect(window.poll(), "Window remains open");
                    ui.beginFrame(scene, renderer);
                    renderer.drawScene(camera, scene);
                    renderer.prepareWindow(window.width(), window.height());
                    ui.render(); renderer.present();
                };
                frame();
                expect(ui.input().viewportVisible, "Initial viewport visible");
                auto initial = renderer.sceneTexture();
                expect(initial.width < window.width() && initial.height < window.height(), "Panel content determines drawable size");
                auto original = pixels(initial);
                frame();
                expect(pixels(renderer.sceneTexture()) == original, "Scene unchanged after actual ImGui pass");
                ImGui::SetWindowSize("Viewport", {480, 360}); frame();
                expect(renderer.sceneTexture().width < initial.width, "Panel resizing resizes texture");
                ImGui::SetWindowCollapsed("Viewport", true); frame();
                expect(!ui.input().viewportVisible && !renderer.sceneTexture().handle, "Collapsed viewport skips scene");
                ImGui::SetWindowCollapsed("Viewport", false); frame();
                expect(ui.input().viewportVisible && renderer.sceneTexture().handle, "Restore resumes scene");
            }
            expect(glGetError() == GL_NO_ERROR, "UI teardown without GL errors");
            expect(ImGui::GetCurrentContext() == nullptr, "UI context released");
            SendMessageW(window.handle(), WM_CLOSE, 0, 0);
            expect(!window.poll() && IsWindow(window.handle()), "Close preserves HWND for renderer teardown");
        }
        expect(wglGetCurrentContext() == nullptr, "GL context released before window");
        std::cout << "Editor sizing, collapse/restore, GL state and teardown passed.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
