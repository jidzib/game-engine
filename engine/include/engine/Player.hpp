#pragma once
#include <engine/Collision.hpp>
#include <algorithm>
#include <cmath>

namespace engine {
struct Player {
    GameObject object{"Player", {0, -0.25f, 0}, {0.5f, 0.75f, 0.5f}, {1.0f, 0.58f, 0.12f}, BoxCollider{}};
    float movementSpeed = 4.0f; // World units per second.

    [[nodiscard]] Vec3 desiredDisplacement(float horizontal, float forward, float vertical, float seconds) const {
        if (!std::isfinite(horizontal) || !std::isfinite(forward) || !std::isfinite(vertical)
            || !std::isfinite(seconds) || !std::isfinite(movementSpeed))
            throw std::invalid_argument("Player input and speed must be finite.");
        const double length = std::hypot(double(horizontal), double(forward), double(vertical));
        if (length == 0 || seconds <= 0) return {};
        const double amount = double(std::max(movementSpeed, 0.0f)) * seconds / std::max(length, 1.0);
        return {float(horizontal*amount), float(vertical*amount), float(forward*amount)};
    }
    MoveResult move(float horizontal, float forward, float vertical, float seconds,
        std::span<const GameObject* const> obstacles = {}) {
        auto result = moveAndSlide(object, desiredDisplacement(horizontal, forward, vertical, seconds), obstacles);
        object.position = result.position;
        return result;
    }
};
}
