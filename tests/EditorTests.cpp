#include "Editor.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include <imgui.h>
#include <imgui_internal.h>
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
            engine::Scene scene;
            scene.addCube("Read-only cube", {3, 0, 0});
            {
                editor::Editor ui(window);
                bool injectPointer = false;
                ImVec2 pointer;
                bool showTextEditor = false, focusText = false;
                char text[64] = "Editable field";
                auto frame = [&] {
                    expect(window.poll(), "Window remains open");
                    if (injectPointer) ImGui::GetIO().AddMousePosEvent(pointer.x, pointer.y);
                    ui.beginFrame(scene, renderer);
                    if (showTextEditor) {
                        ImGui::SetNextWindowPos({20, 80});
                        ImGui::SetNextWindowSize({200, 100});
                        ImGui::Begin("Input regression");
                        if (focusText) { ImGui::SetKeyboardFocusHere(); focusText = false; }
                        ImGui::InputText("Text", text, sizeof(text));
                        ImGui::End();
                    }
                    ui.navigate(0.05f);
                    renderer.drawScene(ui.camera(), scene);
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
                const auto cubeId = scene.cubes.front().id;
                ui.operations().select(scene, cubeId);
                editor::CubeProperties edited(*scene.findCube(cubeId));
                edited.color = {1, 0, 0};
                std::string error;
                expect(editor::Operations::update(scene, cubeId, edited, error), "Editor operation changes color");
                frame();
                expect(pixels(renderer.sceneTexture()) != original, "Edited properties reach viewport texture");
                expect(ui.operations().remove(scene, cubeId), "Delete inspected cube");
                frame();
                expect(ui.operations().selection() == engine::invalidCubeId, "Inspector survives selected deletion");
                // Exercise actual ImGui capture and focus, not just the isolated
                // navigation state. Read-only panels still enable keyboard nav.
                ImGui::SetWindowFocus("Hierarchy"); frame();
                const auto* viewport = ImGui::FindWindowByName("Viewport");
                const int x = static_cast<int>(viewport->Pos.x + viewport->Size.x * 0.5f);
                const int y = static_cast<int>(viewport->Pos.y + viewport->Size.y * 0.5f);
                injectPointer = true;
                pointer = {float(x), float(y)};
                frame();
                expect(ui.input().wantCaptureKeyboard, "Read-only hierarchy reports passive keyboard capture");
                ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Right, true);
                frame();
                expect(ui.input().viewportInteractionStarted && ui.input().viewportFocused,
                    "RMB transfers passive panel focus to viewport without scene selection");
                frame();
                expect(!ui.input().wantCaptureKeyboard, "Viewport does not capture camera keys for UI navigation");
                ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Right, false);
                frame();
                showTextEditor = focusText = true;
                frame(); frame(); frame();
                expect(ImGui::GetIO().WantTextInput && ImGui::IsAnyItemActive(), "Regression field is editing text");
                ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Right, true);
                ImGui::GetIO().AddKeyEvent(ImGuiKey_R, true);
                frame();
                expect(!ui.input().viewportInteractionStarted, "RMB cannot steal an active text editing session");
                ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Right, false);
                ImGui::GetIO().AddKeyEvent(ImGuiKey_R, false);
                showTextEditor = false;
                frame();
                ImGui::SetWindowSize("Viewport", {480, 360}); frame();
                expect(renderer.sceneTexture().width < initial.width, "Panel resizing resizes texture");
                ImGui::SetWindowCollapsed("Viewport", true); frame();
                expect(!ui.input().viewportVisible && !renderer.sceneTexture().handle, "Collapsed viewport skips scene");
                ImGui::SetWindowCollapsed("Viewport", false); frame();
                expect(ui.input().viewportVisible && renderer.sceneTexture().handle, "Restore resumes scene");
                const auto editableId = ui.operations().create(scene);
                frame();
                ImGui::SetWindowFocus("Inspector");
                auto* inspector = ImGui::FindWindowByName("Inspector");
                const auto scope = ImHashStr(std::to_string(editableId).c_str(), 0, inspector->ID);
                ImGui::ActivateItemByID(ImHashStr("Name", 0, scope));
                frame(); frame();
                expect(ImGui::GetIO().WantTextInput && ImGui::IsAnyItemActive(), "Actual inspector name is active");
                ImGui::GetIO().AddInputCharactersUTF8(" edited");
                frame();
                expect(scene.findCube(editableId)->name.find("edited") != std::string::npos, "Inspector typing commits name");
                ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Right, true);
                ImGui::GetIO().AddKeyEvent(ImGuiKey_W, true);
                frame();
                expect(!ui.input().viewportInteractionStarted, "Inspector typing blocks viewport navigation");
                ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Right, false);
                ImGui::GetIO().AddKeyEvent(ImGuiKey_W, false);
                frame();
                SendMessageW(window.handle(), WM_KEYDOWN, VK_ESCAPE, 0);
                frame(); frame();
                expect(!ImGui::IsAnyItemActive(), "Native Escape cancels name editing without closing the window");
                SendMessageW(window.handle(), WM_KEYUP, VK_ESCAPE, 0);
                frame();
                for (const auto size : {ImVec2{300, 180}, ImVec2{700, 400}, ImVec2{400, 240}}) {
                    ImGui::SetWindowSize("Viewport", size); frame();
                    expect(ui.input().viewportVisible, "Repeated panel resizing retains drawable");
                }
                ImGui::SetWindowSize("Viewport", {100, 40}); frame();
                expect(!ui.input().viewportVisible, "Expanded zero-content panel suspends scene drawing");
                ImGui::SetWindowSize("Viewport", {480, 360}); frame();
                SendMessageW(window.handle(), WM_SIZE, SIZE_MINIMIZED, 0);
                expect(window.minimized(), "Native minimize message suspends drawable");
                SendMessageW(window.handle(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(1280, 720));
                frame();
                expect(!window.minimized() && ui.input().viewportVisible, "Native restore message resumes drawable");
                SendMessageW(window.handle(), WM_KILLFOCUS, 0, 0);
                frame();
                ImGui::SetWindowFocus("Viewport"); frame(); frame();
                expect(!ImGui::GetIO().WantCaptureKeyboard, "Viewport releases keyboard capture");
                SendMessageW(window.handle(), WM_KEYDOWN, VK_ESCAPE, 0);
                expect(!window.poll(), "Uncaptured Escape still exits");
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
