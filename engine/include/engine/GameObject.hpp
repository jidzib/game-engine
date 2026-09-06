#pragma once
#include <engine/Math.hpp>
#include <engine/BoxCollider.hpp>
#include <optional>
#include <cmath>
#include <stdexcept>
#include <string>
#include <cstdint>

namespace engine {
using CubeId = std::uint64_t;
inline constexpr CubeId invalidCubeId = 0;
// Cube mesh is centered on position and has dimensions 2 * scale.
struct GameObject {
    std::string name = "Cube";
    Vec3 position{0, 0, 0};
    Vec3 scale{1, 1, 1};
    Vec3 color{0.12f, 0.78f, 0.90f};
    // No collider by default. Use boxCollider.emplace() to attach one.
    std::optional<BoxCollider> boxCollider = std::nullopt;
    // Scene-managed identity. Do not assign this field on a live scene cube.
    // Standalone objects (including the player) have no cube identity.
    CubeId id = invalidCubeId;

    [[nodiscard]] Mat4 worldMatrix() const {
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)
            || !std::isfinite(scale.x) || !std::isfinite(scale.y) || !std::isfinite(scale.z)
            || scale.x <= 0 || scale.y <= 0 || scale.z <= 0) {
            throw std::invalid_argument("GameObject '" + name + "' needs finite position and positive finite scale.");
        }
        return Mat4::transform(position, scale);
    }
};
}

