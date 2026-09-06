#include "Editor.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
namespace editor {
Editor::Editor(engine::Window& window) : window_(window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplWin32_InitForOpenGL(window.handle())) {
        ImGui::DestroyContext();
        throw std::runtime_error("Dear ImGui Win32 initialization failed.");
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        throw std::runtime_error("Dear ImGui OpenGL initialization failed.");
    }
    window_.setEventHandler(handleEvent);
}
Editor::~Editor() {
    window_.setEventHandler(nullptr);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
LRESULT Editor::handleEvent(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
}
void Editor::beginFrame(const engine::Scene& scene, engine::Renderer& renderer) {
    const float dpi = ImGui_ImplWin32_GetDpiScaleForHwnd(window_.handle());
    if (dpiScale_ != dpi) {
        dpiScale_ = dpi;
        ImGui::GetStyle() = ImGuiStyle{};
        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(dpi);
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        auto& io = ImGui::GetIO();
        io.Fonts->Clear();
        ImFontConfig config;
        config.SizePixels = 13.0f * dpi;
        io.Fonts->AddFontDefault(&config);
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    const auto& io = ImGui::GetIO();
    const auto size = io.DisplaySize;
    // Win32 reports physical client pixels under per-monitor DPI awareness.
    // FramebufferScale is 1; account for it explicitly for drawable sizing.
    const float sidebar = std::min(260.0f * dpi, size.x * 0.3f);
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({sidebar, size.y * 0.55f}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Hierarchy")) {
        ImGui::TextDisabled("Scene cubes (%d)", static_cast<int>(scene.cubes.size()));
        ImGui::Separator();
        for (const auto& cube : scene.cubes) ImGui::TextUnformatted(cube.name.c_str());
    }
    ImGui::End();
    ImGui::SetNextWindowPos({0, size.y * 0.55f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({sidebar, size.y * 0.45f}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Inspector")) {
        ImGui::TextWrapped("Object selection and editing will be available in task 6.");
        ImGui::Separator();
        ImGui::TextWrapped("Resize or move panels using their borders and title bars.");
    }
    ImGui::End();
    ImGui::SetNextWindowPos({sidebar, 0}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({size.x - sidebar, size.y}, ImGuiCond_FirstUseEver);
    input_ = {};
    unsigned int width = 0, height = 0;
    if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        const auto available = ImGui::GetContentRegionAvail();
        input_.viewportFocused = ImGui::IsWindowFocused();
        if (available.x >= 1 && available.y >= 1) {
            width = static_cast<unsigned int>(std::floor(available.x * io.DisplayFramebufferScale.x));
            height = static_cast<unsigned int>(std::floor(available.y * io.DisplayFramebufferScale.y));
            input_.viewportVisible = width > 0 && height > 0;
        }
        renderer.resizeScene(width, height);
        if (input_.viewportVisible) {
            const auto texture = renderer.sceneTexture();
            ImGui::Image(static_cast<ImTextureID>(texture.handle), available, {0, 1}, {1, 0});
            input_.viewportHovered = ImGui::IsItemHovered();
        }
    } else renderer.resizeScene(0, 0);
    ImGui::End();
    input_.wantCaptureKeyboard = io.WantCaptureKeyboard;
    input_.wantCaptureMouse = io.WantCaptureMouse;
}
void Editor::render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (glGetError() != GL_NO_ERROR) throw std::runtime_error("OpenGL error during editor rendering.");
}
}
