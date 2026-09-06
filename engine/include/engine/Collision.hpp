#pragma once
#include <engine/GameObject.hpp>
#include <span>
#include <vector>

namespace engine {
struct Aabb { Vec3 center; Vec3 halfSize; };
struct SweepHit { double time; std::vector<Vec3> normals; };
enum class RecoveryStatus { NotNeeded, Recovered, Unresolved };
struct MoveResult {
    Vec3 position;
    Vec3 appliedDisplacement;
    std::vector<Vec3> normals;
    RecoveryStatus recovery = RecoveryStatus::NotNeeded;
    bool iterationLimitReached = false;
};
// World units. Slab calculations use doubles; geometry/storage remain floats.
inline constexpr float collisionClearance = 0.0001f;
// Disabled/missing colliders return nullopt. Invalid active geometry throws.
std::optional<Aabb> worldAabb(const GameObject& object);
// Strict initial overlaps are handled by moveAndSlide recovery, not this query.
std::optional<SweepHit> sweepAabb(Aabb moving, Vec3 displacement, Aabb obstacle);
MoveResult moveAndSlide(const GameObject& moving, Vec3 displacement,
    std::span<const GameObject* const> obstacles);
}
