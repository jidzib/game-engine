#pragma once
#include <engine/GameObject.hpp>
#include <engine/Player.hpp>
#include <deque>
#include <utility>
#include <algorithm>
#include <limits>

namespace engine {
struct Scene {
    Player player;
    // Appending preserves references; arbitrary deletion does not. Store IDs
    // for selection and resolve them when needed. Structural changes must use
    // the Scene API; direct container edits/identity assignment bypass it.
    std::deque<GameObject> cubes;

    GameObject& addCube(std::string name, Vec3 position = {0, 0, 0},
        Vec3 scale = {1, 1, 1}, Vec3 color = {0.12f, 0.78f, 0.90f}) {
        return restoreCube(nextCubeId_, std::move(name), position, scale, color);
    }

    [[nodiscard]] GameObject* findCube(CubeId id) {
        if (id == invalidCubeId) return nullptr;
        const auto found = std::find_if(cubes.begin(), cubes.end(),
            [id](const GameObject& cube) { return cube.id == id; });
        return found == cubes.end() ? nullptr : &*found;
    }

    [[nodiscard]] const GameObject* findCube(CubeId id) const {
        if (id == invalidCubeId) return nullptr;
        const auto found = std::find_if(cubes.begin(), cubes.end(),
            [id](const GameObject& cube) { return cube.id == id; });
        return found == cubes.end() ? nullptr : &*found;
    }

    bool removeCube(CubeId id) {
        if (id == invalidCubeId) return false;
        const auto found = std::find_if(cubes.begin(), cubes.end(),
            [id](const GameObject& cube) { return cube.id == id; });
        if (found == cubes.end()) return false;
        cubes.erase(found);
        return true;
    }

    // Loading restores cubes in ascending ID order into a fresh Scene. IDs
    // below the allocation watermark are rejected even if already deleted.
    // Failed insertion does not consume an ID; exhaustion never wraps to zero.
    GameObject& restoreCube(CubeId id, std::string name, Vec3 position = {0, 0, 0},
        Vec3 scale = {1, 1, 1}, Vec3 color = {0.12f, 0.78f, 0.90f}) {
        if (nextCubeId_ == invalidCubeId)
            throw std::overflow_error("Scene cube IDs exhausted.");
        if (id == invalidCubeId || id < nextCubeId_)
            throw std::invalid_argument("Restored cube IDs must be nonzero and increasing.");
        cubes.push_back({std::move(name), position, scale, color, std::nullopt, id});
        nextCubeId_ = id == std::numeric_limits<CubeId>::max() ? invalidCubeId : id + 1;
        return cubes.back();
    }

private:
    // Default copying preserves IDs and this watermark (including deleted IDs).
    // Copies are independent identity namespaces; IDs are not globally unique.
    CubeId nextCubeId_ = 1;
};
}

