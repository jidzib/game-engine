#include "Navigation.hpp"
#include <iostream>
#include <stdexcept>

void expect(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
bool near(const engine::Mat4& a, const engine::Mat4& b) {
    for (size_t i = 0; i < a.values.size(); ++i)
        if (std::abs(a.values[i] - b.values[i]) > 0.0005f) return false;
    return true;
}
engine::Mat4 view(const editor::Navigation& nav) { return nav.camera().viewProjection(1.5f); }
int main() {
    try {
        editor::Navigation nav;
        const auto initial = view(nav);
        editor::Navigation::Input input;
        input.allowed = input.held = true;
        input.forward = 1;
        nav.update(input, 0.05f);
        expect(near(view(nav), initial), "Held button without explicit press does not start navigation");
        input.pressed = true;
        nav.update(input, 0.05f);
        expect(!near(view(nav), initial), "Explicit interaction translates the editor target");
        const auto moved = view(nav);
        input.allowed = false; input.reset = true; input.wheel = 10;
        nav.update(input, 0.05f);
        expect(!nav.engaged() && near(view(nav), moved), "UI capture/focus loss blocks reset, movement and wheel");
        input.allowed = true; input.pressed = false; input.reset = false; input.wheel = 0;
        nav.update(input, 0.05f);
        expect(near(view(nav), moved), "Returning focus while held requires a fresh press");
        input.pressed = true; input.reset = true;
        nav.update(input, 0.05f);
        expect(near(view(nav), initial), "Reset restores complete editor view");
        input.reset = false; input.pressed = false; input.horizontal = 1; input.vertical = 1;
        editor::Navigation slow, fast;
        input.pressed = true;
        for (int i = 0; i < 20; ++i) slow.update(input, 0.05f);
        for (int i = 0; i < 100; ++i) fast.update(input, 0.01f);
        expect(near(view(slow), view(fast)), "Translation independent of frame rate");
        engine::Camera expected;
        const float component = 5.0f / std::sqrt(3.0f);
        expected.setTarget({component, component, component});
        expect(near(view(slow), expected.viewProjection(1.5f)), "Three-axis panning normalized at five units per second");
        input = {}; input.allowed = input.held = input.pressed = true;
        input.yaw = 1; input.pitch = 1;
        slow = {}; fast = {};
        for (int i = 0; i < 20; ++i) slow.update(input, 0.05f);
        for (int i = 0; i < 100; ++i) fast.update(input, 0.01f);
        expect(near(view(slow), view(fast)), "Orbit independent of frame rate");
        input.wheel = 10000;
        for (int i = 0; i < 100; ++i) nav.update(input, 0.05f);
        for (float value : view(nav).values) expect(std::isfinite(value), "Clamped camera view remains finite");
        input = {}; input.allowed = input.held = input.pressed = true; input.forward = 1;
        slow = {}; fast = {};
        slow.update(input, 10); fast.update(input, 0.05f);
        expect(near(view(slow), view(fast)), "Long frames capped");
        const auto before = view(slow);
        input.held = false; input.wheel = 50;
        slow.update(input, 0.05f);
        expect(!slow.engaged() && near(view(slow), before), "Release stops input immediately");
        slow.cancel(); input.held = true; input.pressed = false;
        slow.update(input, 0.05f);
        expect(near(view(slow), before), "Native cancellation cannot resume held controls");
        std::cout << "Editor navigation ownership, reset, cancellation and camera limits passed.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
