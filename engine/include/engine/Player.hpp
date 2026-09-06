#pragma once
#include <engine/GameObject.hpp>
#include <algorithm>
#include <cmath>

namespace engine {
struct Player {
    GameObject object{"Player", {0, -0.25f, 0}, {0.5f, 0.75f, 0.5f}, {1.0f, 0.58f, 0.12f}};
    float movementSpeed = 4.0f; // World units per second.

    void move(float horizontal, float forward, float vertical, float seconds) {
        const float length = std::sqrt(horizontal * horizontal + forward * forward + vertical * vertical);
        if (length == 0 || seconds <= 0) { return; }
        const float amount = std::max(movementSpeed, 0.0f) * seconds / std::max(length, 1.0f);
        object.position.x += horizontal * amount;
        object.position.y += vertical * amount;
        object.position.z += forward * amount;
    }
};
}
