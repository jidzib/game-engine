#pragma once
#include <engine/GameObject.hpp>
#include <engine/Player.hpp>
#include <deque>
#include <utility>

namespace engine {
struct Scene {
    Player player;
    // deque keeps object references valid when more cubes are appended.
    std::deque<GameObject> cubes;

    GameObject& addCube(std::string name, Vec3 position = {0, 0, 0},
        Vec3 scale = {1, 1, 1}, Vec3 color = {0.12f, 0.78f, 0.90f}) {
        cubes.push_back({std::move(name), position, scale, color});
        return cubes.back();
    }
};
}

