#include "Editor.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <windowsx.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
namespace editor {
Editor::Editor(engine::Window& window) : window_(window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().UserData = this;
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
    auto* self = ImGui::GetCurrentContext() ? static_cast<Editor*>(ImGui::GetIO().UserData) : nullptr;
    // The Win32 backend queues keyboard input but normally returns zero for it.
    // Preserve Escape for cancelling UI edits/popups before the window shortcut.
    const bool captureEscape = self && msg == WM_KEYDOWN && wp == VK_ESCAPE &&
        (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive());
    if (self) {
        if (msg == WM_KILLFOCUS || (msg == WM_ACTIVATEAPP && !wp) ||
            (msg == WM_SIZE && wp == SIZE_MINIMIZED) || msg == WM_CAPTURECHANGED ||
            msg == WM_CANCELMODE || msg == WM_RBUTTONUP) {
            self->navigation_.cancel();
            self->wheel_ = 0;
            self->interrupted_ = true;
        }
        if (msg == WM_MOUSEWHEEL) {
            const POINT point{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            POINT client = point;
            ScreenToClient(hwnd, &client);
            ImGuiWindow* hovered = nullptr;
            // Use the pinned ImGui hit test so overlapping panels and resize
            // borders are respected even if the pointer moves again this frame.
            ImGui::FindHoveredWindowEx({float(client.x), float(client.y)}, false, &hovered, nullptr);
            // Route at event time, never from the cursor's later frame position.
            if (self->navigation_.engaged() && self->window_.active() &&
                (wp & MK_RBUTTON) && self->input_.viewportHovered &&
                hovered == ImGui::FindWindowByName("Viewport") &&
                PtInRect(&self->viewportRect_, point) && WindowFromPoint(point) == hwnd &&
                !ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantTextInput)
                self->wheel_ += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / WHEEL_DELTA;
        }
    }
    const auto result = ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
    return captureEscape ? 1 : result;
}
void Editor::beginFrame(engine::Scene& scene, engine::Renderer& renderer) {
    operations_.reconcile(scene);
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
        const auto resizePath = [](ImGuiInputTextCallbackData* data) -> int {
            auto& path = *static_cast<std::string*>(data->UserData);
            path.resize(data->BufTextLen);
            data->Buf = path.data();
            return 0;
        };
        ImGui::InputText("Scene path", scenePath_.data(), scenePath_.capacity() + 1,
            ImGuiInputTextFlags_CallbackResize, resizePath, &scenePath_);
        if (ImGui::Button("Save")) {
            try {
                engine::ScenePersistence::save(scene, std::filesystem::path(std::u8string(scenePath_.begin(), scenePath_.end())));
                persistenceFeedback_ = "Scene saved.";
            } catch (const std::exception& e) { persistenceFeedback_ = e.what(); }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            try {
                operations_.load(scene, std::filesystem::path(std::u8string(scenePath_.begin(), scenePath_.end())));
                validationError_.clear();
                navigation_.cancel();
                wheel_ = 0;
                ImGui::ClearActiveID();
                persistenceFeedback_ = "Scene loaded.";
            } catch (const std::exception& e) { persistenceFeedback_ = e.what(); }
        }
        if (!persistenceFeedback_.empty()) ImGui::TextWrapped("%s", persistenceFeedback_.c_str());
        ImGui::Separator();
        ImGui::TextDisabled("Scene cubes (%d)", static_cast<int>(scene.cubes.size()));
        ImGui::Separator();
        if (ImGui::Button("Add cube")) {
            try { operations_.create(scene); validationError_.clear(); }
            catch (const std::overflow_error& e) { validationError_ = e.what(); }
        }
        for (const auto& cube : scene.cubes) {
            const auto identity = std::to_string(cube.id);
            ImGui::PushID(identity.c_str());
            // A fixed hidden label avoids name-based identity and ## interpretation.
            const auto start = ImGui::GetCursorScreenPos();
            if (ImGui::Selectable("##cube", operations_.selection() == cube.id)) {
                operations_.select(scene, cube.id);
                validationError_.clear();
            }
            ImGui::GetWindowDrawList()->AddText(start, ImGui::GetColorU32(ImGuiCol_Text),
                cube.name.empty() ? "(unnamed)" : cube.name.c_str());
            ImGui::PopID();
        }
    }
    ImGui::End();
    ImGui::SetNextWindowPos({0, size.y * 0.55f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({sidebar, size.y * 0.45f}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Inspector")) {
        inspector(scene);
    }
    ImGui::End();
    ImGui::SetNextWindowPos({sidebar, 0}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({size.x - sidebar, size.y}, ImGuiCond_FirstUseEver);
    input_ = {};
    unsigned int width = 0, height = 0;
    if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs)) {
        ImGui::TextUnformatted("Edit | Hold RMB on image: WASD + Space/Shift pan | Arrows orbit | Wheel zoom | R reset");
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
            // Passive keyboard navigation in a read-only panel is not a widget
            // editing session. A fresh image click may transfer that focus.
            const bool passiveNavigationCapture = io.NavActive &&
                GImGui->OpenPopupStack.empty() && GImGui->WantCaptureKeyboardNextFrame != 1;
            if (input_.viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
                (!io.WantCaptureKeyboard || passiveNavigationCapture) &&
                !io.WantTextInput && !ImGui::IsAnyItemActive()) {
                ImGui::SetWindowFocus();
                input_.viewportInteractionStarted = true;
            }
            input_.viewportFocused = ImGui::IsWindowFocused();
            const auto min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
            POINT origin{0, 0};
            ClientToScreen(window_.handle(), &origin);
            viewportRect_ = {origin.x + static_cast<LONG>(min.x), origin.y + static_cast<LONG>(min.y),
                origin.x + static_cast<LONG>(max.x), origin.y + static_cast<LONG>(max.y)};
        }
    } else renderer.resizeScene(0, 0);
    ImGui::End();
    input_.wantCaptureKeyboard = io.WantCaptureKeyboard;
    input_.wantCaptureMouse = io.WantCaptureMouse;
}
void Editor::inspector(engine::Scene& scene) {
    const auto id = operations_.selection();
    const auto* cube = scene.findCube(id);
    if (!cube) { ImGui::TextWrapped("Select a cube in the hierarchy to edit it."); return; }
    const auto identity = std::to_string(id);
    ImGui::PushID(identity.c_str());
    if (ImGui::Button("Delete cube")) {
        operations_.remove(scene, id);
        validationError_.clear();
        ImGui::PopID();
        return;
    }
    CubeProperties proposed(*cube);
    // Resize callback keeps arbitrary-length names editable without truncation.
    const auto resizeText = [](ImGuiInputTextCallbackData* data) -> int {
        auto& text = *static_cast<std::string*>(data->UserData);
        text.resize(data->BufTextLen);
        data->Buf = text.data();
        return 0;
    };
    bool changed = ImGui::InputText("Name", proposed.name.data(), proposed.name.capacity() + 1,
        ImGuiInputTextFlags_CallbackResize, resizeText, &proposed.name);
    const auto vectorField = [](const char* label, engine::Vec3& value) {
        float components[]{value.x, value.y, value.z};
        const bool edited = ImGui::DragFloat3(label, components, 0.05f, 0, 0, "%.4f");
        if (edited) value = {components[0], components[1], components[2]};
        return edited;
    };
    changed |= vectorField("Position", proposed.position);
    changed |= vectorField("Scale (half dimensions)", proposed.scale);
    ImGui::TextWrapped("Cube dimensions = 2 x scale. Ctrl+click a number to type.");
    float color[]{proposed.color.x, proposed.color.y, proposed.color.z};
    if (ImGui::ColorEdit3("Color", color, ImGuiColorEditFlags_Float)) {
        proposed.color = {color[0], color[1], color[2]}; changed = true;
    }
    ImGui::Separator();
    if (!proposed.collider) {
        ImGui::TextUnformatted("No box collider attached");
        if (ImGui::Button("Attach box collider")) { proposed.collider.emplace(); changed = true; }
    } else {
        if (ImGui::Button("Remove box collider")) { proposed.collider.reset(); changed = true; }
        else {
            changed |= ImGui::Checkbox("Collider enabled", &proposed.collider->enabled);
            changed |= vectorField("Local offset", proposed.collider->offset);
            changed |= vectorField("Local half extents", proposed.collider->halfExtents);
            ImGui::TextWrapped("Collider sizes must be positive. Offset and half extents are scaled by the cube scale.");
        }
    }
    if (changed) Operations::update(scene, id, proposed, validationError_);
    if (!validationError_.empty()) {
        ImGui::TextWrapped("Edit rejected: %s", validationError_.c_str());
    }
    ImGui::PopID();
}
void Editor::navigate(float seconds) {
    const auto& io = ImGui::GetIO();
    Navigation::Input command;
    // WantCaptureMouse also covers our own viewport. Explicit image hover and
    // no active UI item grant that image ownership; popups/other panels block it.
    command.allowed = !std::exchange(interrupted_, false) && window_.active() &&
        input_.viewportVisible && input_.viewportFocused && input_.viewportHovered &&
        (!input_.wantCaptureKeyboard || input_.viewportInteractionStarted) &&
        !io.WantTextInput && !ImGui::IsAnyItemActive();
    command.pressed = input_.viewportInteractionStarted;
    command.held = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    command.reset = ImGui::IsKeyPressed(ImGuiKey_R, false);
    const auto axis = [](ImGuiKey positive, ImGuiKey negative) {
        return float(ImGui::IsKeyDown(positive)) - float(ImGui::IsKeyDown(negative));
    };
    command.horizontal = axis(ImGuiKey_A, ImGuiKey_D);
    command.forward = axis(ImGuiKey_W, ImGuiKey_S);
    command.vertical = axis(ImGuiKey_Space, ImGuiKey_LeftShift);
    command.yaw = axis(ImGuiKey_LeftArrow, ImGuiKey_RightArrow);
    command.pitch = axis(ImGuiKey_UpArrow, ImGuiKey_DownArrow);
    command.wheel = std::exchange(wheel_, 0.0f);
    navigation_.update(command, seconds);
}
void Editor::render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (glGetError() != GL_NO_ERROR) throw std::runtime_error("OpenGL error during editor rendering.");
}
}
