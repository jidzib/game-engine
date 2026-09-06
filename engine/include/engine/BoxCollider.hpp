#pragma once
#include <engine/Math.hpp>

namespace engine {
// Collider configuration only. Bounds and collision behavior are added separately.
struct BoxCollider {
    bool enabled = true;
    // Local-space center relative to the GameObject's origin, before its scale.
    Vec3 offset{0, 0, 0};
    // Positive local-space half sizes. {1,1,1} matches the default cube mesh.
    // The GameObject's scale will also scale the collider.
    Vec3 halfExtents{1, 1, 1};
};
}
