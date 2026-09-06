#pragma once
#include <engine/Math.hpp>

namespace engine {
class Camera {
public:
    void orbit(float yawDelta, float pitchDelta);
    void zoom(float delta);
    void reset();
    void setTarget(Vec3 target) { target_ = target; }
    [[nodiscard]] Mat4 viewProjection(float aspectRatio) const;

private:
    float yaw_ = 2.55f;
    float pitch_ = 0.40f;
    float distance_ = 12.0f;
    Vec3 target_{0, 0, 0};
};
}

