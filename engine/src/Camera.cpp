#include <engine/Camera.hpp>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace engine {
void Camera::orbit(float yawDelta, float pitchDelta) {
    yaw_ = std::remainder(yaw_ + yawDelta, 2.0f * std::numbers::pi_v<float>);
    pitch_ = std::clamp(pitch_ + pitchDelta, 0.08f, 1.45f);
}
void Camera::zoom(float delta) { distance_ = std::clamp(distance_ + delta, 3.0f, 25.0f); }
void Camera::reset() { *this = Camera{}; }
Mat4 Camera::viewProjection(float aspectRatio) const {
    const float horizontal = distance_ * std::cos(pitch_);
    const Vec3 eye = target_ + Vec3{horizontal * std::sin(yaw_),
        distance_ * std::sin(pitch_), horizontal * std::cos(yaw_)};
    return perspective(std::numbers::pi_v<float> / 3, aspectRatio, 0.1f, 100.0f) * lookAt(eye, target_);
}
}
