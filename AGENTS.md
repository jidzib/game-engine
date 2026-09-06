# Project instructions

## Build and verification
- C++20, Windows WGL/OpenGL 3.3 Core; no additional dependencies.
- Configure: `cmake --preset windows -DBUILD_TESTING=ON`.
- Build: `cmake --build --preset debug` and `cmake --build --preset release`.
- Test: `ctest --test-dir build/windows -C Debug --output-on-failure` and the equivalent `-C Release`.
- CTest registers scene_behavior, collision_behavior, and render_smoke. Keep existing scene/math/movement coverage when changing APIs.
- The sandboxed MSBuild process may be denied access to the installed Windows SDK metadata under AppData/Local/Microsoft SDKs. Use approved build escalation when necessary; do not change the toolchain or global Git settings to bypass environment restrictions.

## Collision and player movement
- BoxCollider.hpp is configuration only; GameObject colliders remain opt-in. Player and sandbox obstacles explicitly attach enabled colliders.
- Collision.hpp/Collision.cpp own worldAabb, sweepAabb, and moveAndSlide. Use existing Vec3 and GameObject types, with no gravity or velocity system.
- World center is position + scale * local offset; world half size is scale * local halfExtents, component-wise. Active invalid or overflowing bounds and nonfinite movement throw invalid_argument. Missing/disabled colliders are ignored before geometry validation.
- Queries use unrotated static AABBs. Only the player resolves movement; exclude its own object pointer. Candidate pointers are borrowed for the duration of a call.
- Player::desiredDisplacement preserves 3D input normalization and speed. Player::move accepts horizontal, forward, vertical, seconds, and optional obstacle pointers, applies resolution, and returns MoveResult. Engine.cpp supplies scene cubes on the actual input path.
- Preserve controls: A/D = +X/-X, W/S = +Z/-Z, Space/left Shift = +Y/-Y. Camera controls are unchanged.
- Sweep the complete displacement with double-precision slab intervals. Face tangency and outward motion are allowed; inward contact blocks. Strict initial overlap belongs to recovery, not sweepAabb.
- Clearance is 0.0001 world units, achieved by retreating along the sweep before contact. Unconsumed tangential movement is preserved without normalization. Tests allow 0.0005 world units for representative positions and timestep comparisons.
- Slab normal ties use 1e-7 world units. Equal obstacle hit times combine normals; later contacts are found by re-sweeping. Contact constraints remain for the current move, making corner behavior conservative and deterministic.
- Recovery recomputes strict overlaps after each minimum-axis correction, capped at 32 corrections. Sort obstacle geometry for deterministic ties; axis ties prefer X, then Y, then Z; coincident centers prefer positive separation. Recovery runs even with zero input.
- MoveResult exposes position, appliedDisplacement (including recovery), unique contact normals, RecoveryStatus, and iterationLimitReached. Unresolved recovery stops before desired movement. Sliding is capped at 8 iterations, stops on lack of progress, and never applies unchecked remainder.
- Limitations: static axis-aligned boxes only; float world storage means clearance is intended for ordinary scene coordinates, not arbitrary extreme magnitudes. Bounded local recovery can report Unresolved even if a global escape path exists. Normals describe sweep contacts, not grounding or persistent velocity.

## Last task verification (2026-09-06)
- Implemented Task.md swept collision/sliding, bounded overlap recovery, engine integration, enabled sandbox walls/corner/floor/ceiling, and focused CollisionTests.cpp.
- Baseline configuration succeeded; baseline Debug build failed with C2660 because SceneTests.cpp used the obsolete three-argument Player::move. Updated those calls with zero vertical input while retaining their assertions.
- Final Debug and Release builds succeeded with the installed Visual Studio 2026 preset and approved SDK access.
- Debug CTest: 3/3 passed, 0 failed (scene_behavior, collision_behavior, render_smoke), 0.51 seconds.
- Release CTest: 3/3 passed, 0 failed, 0.47 seconds.
- Manual interactive visual verification was not performed. Automated render_smoke launched and rendered successfully in both configurations. For manual checks launch build/windows/bin/Debug/sandbox.exe and exercise wall approach, diagonal sliding, vertical blocking, moving away, and inside corners.
- Preserve the pre-existing user change to .gitignore.
