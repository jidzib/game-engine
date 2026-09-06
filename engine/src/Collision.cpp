#include <engine/Collision.hpp>
#include <algorithm>
#include <array>
#include <limits>
#include <tuple>

namespace engine {
namespace {
float axis(Vec3 v, int i) { return i == 0 ? v.x : i == 1 ? v.y : v.z; }
Vec3 normal(int i, float sign) { return {i == 0 ? sign : 0, i == 1 ? sign : 0, i == 2 ? sign : 0}; }
Vec3 times(Vec3 v, double s) { return {float(v.x*s), float(v.y*s), float(v.z*s)}; }
bool finite(Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
void validate(Aabb b) {
    if (!finite(b.center) || !finite(b.halfSize) || b.halfSize.x <= 0 || b.halfSize.y <= 0 || b.halfSize.z <= 0
        || !finite(b.center+b.halfSize) || !finite(b.center-b.halfSize))
        throw std::invalid_argument("Collision bounds require finite centers and positive finite sizes.");
}
void addNormal(std::vector<Vec3>& normals, Vec3 n) {
    if (std::none_of(normals.begin(), normals.end(), [n](Vec3 v) { return dot(v,n) == 1; })) normals.push_back(n);
    std::sort(normals.begin(), normals.end(), [](Vec3 a, Vec3 b) { return std::tie(a.x,a.y,a.z) < std::tie(b.x,b.y,b.z); });
}
bool overlaps(Aabb a, Aabb b) {
    for (int i=0; i<3; ++i)
        if (std::abs(double(axis(a.center,i))-axis(b.center,i)) >= double(axis(a.halfSize,i))+axis(b.halfSize,i)) return false;
    return true;
}
}
std::optional<Aabb> worldAabb(const GameObject& object) {
    if (!object.boxCollider || !object.boxCollider->enabled) return std::nullopt;
    (void)object.worldMatrix();
    const auto& c = *object.boxCollider;
    validate({c.offset,c.halfExtents});
    Aabb b{object.position + Vec3{object.scale.x*c.offset.x,object.scale.y*c.offset.y,object.scale.z*c.offset.z},
        {object.scale.x*c.halfExtents.x,object.scale.y*c.halfExtents.y,object.scale.z*c.halfExtents.z}};
    validate(b);
    return b;
}
std::optional<SweepHit> sweepAabb(Aabb moving, Vec3 displacement, Aabb obstacle) {
    validate(moving); validate(obstacle);
    if (!finite(displacement)) throw std::invalid_argument("Displacement must be finite.");
    double enter = -std::numeric_limits<double>::infinity(), leave = std::numeric_limits<double>::infinity();
    std::array<double,3> entries{};
    std::array<Vec3,3> normals{};
    for (int i=0; i<3; ++i) {
        const double p = double(axis(moving.center,i))-axis(obstacle.center,i);
        const double h = double(axis(moving.halfSize,i))+axis(obstacle.halfSize,i);
        const double d = axis(displacement,i);
        // A stationary boundary axis cannot enter the box interior: allow tangency.
        if (d == 0) {
            if (p <= -h || p >= h) return std::nullopt;
            entries[i] = -std::numeric_limits<double>::infinity();
            continue;
        }
        double a = (-h-p)/d, b = (h-p)/d;
        normals[i] = normal(i, d > 0 ? -1.0f : 1.0f);
        if (a>b) std::swap(a,b);
        entries[i]=a; enter=std::max(enter,a); leave=std::min(leave,b);
    }
    if (enter < 0 || enter > 1 || leave <= enter) return std::nullopt;
    SweepHit hit{enter,{}};
    for (int i=0; i<3; ++i) {
        // Compare in world distance so long sweeps do not merge distant contacts.
        if (std::isfinite(entries[i]) && std::abs(entries[i]-enter)*std::abs(double(axis(displacement,i))) <= 1e-7)
            addNormal(hit.normals,normals[i]);
    }
    return hit;
}
MoveResult moveAndSlide(const GameObject& moving, Vec3 displacement, std::span<const GameObject* const> obstacles) {
    if (!finite(displacement) || !finite(moving.position) || !finite(moving.position+displacement))
        throw std::invalid_argument("Movement requires finite position and displacement.");
    MoveResult result{moving.position,{},{}};
    auto bounds = worldAabb(moving);
    if (!bounds) { result.position=moving.position+displacement; result.appliedDisplacement=displacement; return result; }
    std::vector<Aabb> boxes;
    for (const auto* object : obstacles) {
        if (object && object != &moving) if (auto b=worldAabb(*object)) boxes.push_back(*b);
    }
    // Stable geometric ordering makes recovery and equal-time hits insertion independent.
    std::sort(boxes.begin(),boxes.end(),[](Aabb a,Aabb b) {
        return std::tie(a.center.x,a.center.y,a.center.z,a.halfSize.x,a.halfSize.y,a.halfSize.z)
             < std::tie(b.center.x,b.center.y,b.center.z,b.halfSize.x,b.halfSize.y,b.halfSize.z);
    });
    const Vec3 initialCenter=bounds->center;
    auto finish = [&]() {
        result.appliedDisplacement=bounds->center-initialCenter;
        result.position=moving.position+result.appliedDisplacement;
        return result;
    };
    constexpr int recoveryLimit=32;
    for (int iteration=0; ; ++iteration) {
        double best=std::numeric_limits<double>::infinity(); Vec3 correctionNormal{};
        for (auto b:boxes) if (overlaps(*bounds,b)) {
            for (int i=0;i<3;++i) {
                double delta=double(axis(bounds->center,i))-axis(b.center,i);
                double depth=double(axis(bounds->halfSize,i))+axis(b.halfSize,i)-std::abs(delta);
                if (depth<best) { best=depth; correctionNormal=normal(i,delta<0 ? -1.0f : 1.0f); }
            }
        }
        if (!std::isfinite(best)) break;
        if (iteration==recoveryLimit) { result.recovery=RecoveryStatus::Unresolved; return finish(); }
        result.recovery=RecoveryStatus::Recovered;
        bounds->center=bounds->center+times(correctionNormal,best+collisionClearance);
    }
    Vec3 remaining=displacement;
    constexpr int slideLimit=8;
    for (int iteration=0; iteration<slideLimit; ++iteration) {
        if (remaining.x==0 && remaining.y==0 && remaining.z==0) return finish();
        double earliest=2;
        std::vector<Vec3> contacts;
        for (auto b:boxes) if (auto hit=sweepAabb(*bounds,remaining,b)) {
            if (hit->time<earliest) { earliest=hit->time; contacts=hit->normals; }
            else if (hit->time==earliest) for (auto n:hit->normals) addNormal(contacts,n);
        }
        if (earliest>1) { bounds->center=bounds->center+remaining; return finish(); }
        double retreat=0;
        for (auto n:contacts) {
            retreat=std::max(retreat,double(collisionClearance)/-double(dot(remaining,n)));
            addNormal(result.normals,n);
        }
        const double advance=std::max(0.0,earliest-retreat);
        bounds->center=bounds->center+times(remaining,advance);
        Vec3 next=times(remaining,1-advance);
        // Axis normals commute. Retain constraints for this move, never normalize slides.
        for (auto n:result.normals) if (const float inward=dot(next,n); inward<0) next=next-times(n,inward);
        if (advance==0 && next.x==remaining.x && next.y==remaining.y && next.z==remaining.z) {
            result.iterationLimitReached=true; return finish();
        }
        remaining=next;
    }
    result.iterationLimitReached=true;
    return finish();
}
}
