#pragma once
#include <engine/Camera.hpp>
#include <algorithm>
#include <cmath>

namespace editor {
// Edit-only state: no scene or player references, including during reset.
class Navigation {
public:
    struct Input {
        bool allowed = false;
        bool pressed = false;
        bool held = false;
        bool reset = false;
        float horizontal = 0, forward = 0, vertical = 0;
        float yaw = 0, pitch = 0, wheel = 0;
    };
    void cancel() { engaged_ = false; }
    bool engaged() const { return engaged_; }
    const engine::Camera& camera() const { return camera_; }
    void update(const Input& input, float seconds) {
        if (!input.allowed || !input.held) { cancel(); return; }
        if (input.pressed) engaged_ = true;
        if (!engaged_) return;
        if (input.reset) { camera_.reset(); target_ = {}; return; }
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.0f, 0.05f) : 0.0f;
        engine::Vec3 direction{input.horizontal, input.vertical, input.forward};
        const float length = std::sqrt(engine::dot(direction, direction));
        if (length > 0) {
            const float step = 5.0f * dt / std::max(1.0f, length);
            target_ = target_ + engine::Vec3{direction.x * step, direction.y * step, direction.z * step};
            camera_.setTarget(target_);
        }
        camera_.orbit(input.yaw * dt * 1.5f, input.pitch * dt * 1.2f);
        camera_.zoom(-input.wheel * 0.6f);
    }
private:
    engine::Camera camera_;
    engine::Vec3 target_{};
    bool engaged_ = false;
};
}
