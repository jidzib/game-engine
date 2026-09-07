#pragma once
#include <engine/Scene.hpp>
#include <engine/Collision.hpp>

namespace editor {
// Value snapshots contain editable properties only; identity stays Scene-owned.
struct CubeProperties {
    std::string name;
    engine::Vec3 position, scale, color;
    std::optional<engine::BoxCollider> collider;
    explicit CubeProperties(const engine::GameObject& cube)
        : name(cube.name), position(cube.position), scale(cube.scale),
          color(cube.color), collider(cube.boxCollider) {}
};

class Operations {
public:
    engine::CubeId selection() const { return selected_; }
    void select(const engine::Scene& scene, engine::CubeId id) {
        selected_ = scene.findCube(id) ? id : engine::invalidCubeId;
    }
    void reconcile(const engine::Scene& scene) { select(scene, selected_); }
    engine::CubeId create(engine::Scene& scene) {
        selected_ = scene.addCube("Cube").id;
        return selected_;
    }
    bool remove(engine::Scene& scene, engine::CubeId id) {
        const bool removed = scene.removeCube(id);
        reconcile(scene);
        return removed;
    }
    // Validate the complete proposed object before committing any properties.
    // Disabled colliders are validated too, so enabling them is always safe.
    static bool update(engine::Scene& scene, engine::CubeId id,
        const CubeProperties& properties, std::string& error) {
        error.clear();
        auto* cube = scene.findCube(id);
        if (!cube) { error = "This cube no longer exists."; return false; }
        auto proposed = *cube;
        proposed.name = properties.name;
        proposed.position = properties.position;
        proposed.scale = properties.scale;
        proposed.color = properties.color;
        proposed.boxCollider = properties.collider;
        try {
            (void)proposed.worldMatrix();
            const auto c = proposed.color;
            if (!std::isfinite(c.x) || !std::isfinite(c.y) || !std::isfinite(c.z)
                || c.x < 0 || c.x > 1 || c.y < 0 || c.y > 1 || c.z < 0 || c.z > 1)
                throw std::invalid_argument("Color components must be finite and between 0 and 1.");
            auto checked = proposed;
            if (checked.boxCollider) {
                checked.boxCollider->enabled = true;
                (void)engine::worldAabb(checked);
            }
        } catch (const std::invalid_argument& e) {
            error = e.what();
            return false;
        }
        *cube = std::move(proposed);
        return true;
    }
private:
    engine::CubeId selected_ = engine::invalidCubeId;
};
}
