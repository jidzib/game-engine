# Task: Swept box collision and sliding for player movement

## Goal
Make the player stop at solid BoxCollider surfaces and slide along them in 3D using swept collision detection. Preserve existing controls and diagonal speed while preventing movement through static obstacles, including at high speed.

## Context
- Relevant files:
  - `engine/include/engine/BoxCollider.hpp`: existing enabled flag, local offset, and local halfExtents.
  - `engine/include/engine/GameObject.hpp`: optional boxCollider, position, and positive scale.
  - `engine/include/engine/Player.hpp`: currently moves directly, normalizes input, and supports vertical movement.
  - `engine/include/engine/Scene.hpp`: player and static cubes.
  - `engine/src/Engine.cpp`: input and delta-time integration.
  - `engine/include/engine/Math.hpp`: existing math types.
  - `sandbox/main.cpp`: demonstration scene.
  - `tests/SceneTests.cpp`, `CMakeLists.txt`, `CMakePresets.json`: tests and builds.
- Existing patterns to follow: reuse the existing optional BoxCollider and math types. Keep collider configuration separate from collision queries and movement resolution. Choose a small API suited to this engine.
- Constraints: C++20, current Windows build, no new dependencies or unrelated refactoring. Inspect current code and repository instructions before implementing.
- Background decisions already made (don't re-litigate these):
  - Only the player moves through collision resolution; obstacles remain static.
  - Use axis-aligned boxes (AABBs). Rotated colliders are out of scope.
  - Sweep the complete desired displacement, find the earliest hit, and slide with the remaining movement. Do not resolve movement one axis at a time.
  - Preserve movement speed, normalized input, current key mapping, and vertical controls. Do not add gravity or jumping.
  - Missing or disabled obstacle colliders do not block. Exclude the player from its own candidates. A missing or disabled player collider permits unrestricted movement.
  - Keep general GameObject colliders opt-in; explicitly enable the player collider and demonstration obstacle colliders.

## Steps
1. Inspect the implementation and establish a build/test baseline. Existing SceneTests.cpp calls appear to use an older Player::move signature; update relevant tests without discarding their behavioral coverage. Report pre-existing failures separately.
2. Implement world AABB calculation: center = position + component-wise(scale * collider.offset); half size = component-wise(scale * collider.halfExtents). Follow existing finite-value and positive-size validation conventions and handle invalid collider data explicitly.
3. Implement a testable swept AABB query. Expand the static box by the moving box's world half size, then intersect the moving center's displacement segment with the expanded box using slab intervals. Return hit time in [0, 1] and outward contact normal(s). Handle zero displacement components, parallel movement, and initially touching faces moving inward, outward, or tangentially.
4. Implement move-and-slide over enabled static colliders using a simple linear scan. Select the earliest hit and move safely to contact using a small documented world-space clearance. Remove only the inward normal component from the unconsumed displacement: if dot(remaining, normal) < 0, subtract normal * dot(remaining, normal). Never renormalize the slide vector. Sweep the remainder again to handle multiple contacts. Retain applicable contact constraints and handle simultaneous hits consistently so corners cannot cause penetration, oscillation, or obstacle-order-dependent results. Bound iterations, detect lack of progress, and never apply unchecked leftover movement after the limit.
5. Add bounded recovery for pre-existing overlaps, separate from sweeping. Use minimum-axis separation with deterministic ties and recompute overlaps after corrections. Attempt recovery even with zero desired movement. If a trapped configuration cannot be resolved within the limit, return an explicit unresolved status without hanging or claiming success.
6. Integrate collision-aware movement into the actual engine input path. Separate desired movement calculation from resolution. Return at least resolved position or applied displacement, contact normals, and overlap-recovery status. Do not introduce a velocity/gravity system; the normals can support a future controller.
7. Add a reachable wall/corner arrangement with enabled colliders to the sandbox and keep the player spawn outside obstacles. Add focused automated tests for the acceptance criteria and any necessary build registration.

## Acceptance Criteria
- [ ] Unobstructed movement retains existing speed, diagonal normalization, input mapping, and vertical controls.
- [ ] Local offsets and nonuniform positive object scales produce correct world collision bounds.
- [ ] Missing/disabled obstacle colliders are ignored; a missing/disabled player collider permits unrestricted movement.
- [ ] Walls, floors, and ceilings block approaches from either direction on all three axes within documented numerical tolerance.
- [ ] A displacement crossing an entire thin obstacle is blocked even when its endpoint lies beyond the obstacle.
- [ ] Diagonal contact preserves tangential displacement without a speed boost.
- [ ] Sliding can hit another obstacle in the same move. Corners and simultaneous contacts constrain the correct components without jitter, penetration, or dependence on obstacle insertion order.
- [ ] Starting in contact allows moving away from or along the surface and blocks moving into it. Zero input and parallel movement are safe.
- [ ] Recoverable initial overlap is separated even with zero input; unresolvable configurations terminate with an explicit status.
- [ ] One large step and equivalent smaller steps produce consistent results within documented tolerance for representative static wall/sliding cases.
- [ ] Focused automated tests cover these behaviors, existing relevant tests remain valid, Debug and Release builds pass, and actual CTest results are reported.
- [ ] The sandbox demonstrates blocking and sliding with existing controls. The completion report states whether manual visual verification was performed.

## Non-Goals
- Dynamic rigid bodies, pushing objects, mass, impulses, friction, restitution, or a general physics engine.
- Gravity, jumping, stair stepping, slopes, or grounded-state gameplay.
- Rotated boxes, mesh collision, capsules, or other collider shapes.
- Triggers, collision layers, spatial trees, or broad-phase optimization beyond a simple scan.
- Unrelated camera, rendering, input, or architectural changes.

## Verification Commands
Run from the repository root using the installed CMake/Visual Studio toolchain and existing Windows preset. If tooling is unavailable, report the precise blocker and actual fallback commands used. Do not claim unrun checks passed.

```powershell
cmake --preset windows -DBUILD_TESTING=ON
cmake --build --preset debug
ctest --test-dir build/windows -C Debug --output-on-failure
cmake --build --preset release
ctest --test-dir build/windows -C Release --output-on-failure
```

For manual verification, launch `build/windows/bin/Debug/sandbox.exe` and exercise wall approach, diagonal sliding, vertical blocking, moving away from contact, and inside corners. Report changed files, implementation choices and tolerances, actual build/test output, and remaining limitations.
