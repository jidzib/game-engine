# Task: Stable identity and lifecycle operations for scene cubes

## Goal

Give scene cubes stable IDs and safe lookup/deletion operations as the foundation for editor selection and scene persistence.

## Context

- Engine: C++20, Windows, OpenGL 3.3.
- Relevant files: engine/include/engine/Scene.hpp, engine/include/engine/GameObject.hpp, sandbox/main.cpp, tests/SceneTests.cpp, CMakeLists.txt, CMakePresets.json.
- Scene currently contains Player player and std::deque<GameObject> cubes.
- addCube() appends a cube and returns GameObject&.
- GameObject contains name, position, positive scale, color, and an optional BoxCollider. Names may be duplicated.
- Constraints: inspect current code and repository instructions before implementing. Preserve existing rendering, collision behavior, and addCube() call sites where practical. No new dependency or ECS rewrite.
- This is task 1 of 8. Later tasks add an editor and save/load.

## Steps

1. Establish the current build/test baseline and report pre-existing failures separately.
2. Add a stable cube ID using a small API appropriate to this engine. Define invalid-ID, allocation, and scene-copy semantics.
3. Ensure normal creation allocates unique IDs that remain unchanged for the object's lifetime. Avoid reusing deleted IDs during the same scene lifetime.
4. Add const and mutable lookup plus deletion by ID. Define missing-ID behavior without dangling-reference access.
5. Preserve Scene::cubes and addCube(). Inspect aggregate initialization and update affected construction sites.
6. Document that editor selection must use IDs and resolve them when needed; deque storage does not make references safe after arbitrary deletion.
7. Add focused tests for uniqueness, duplicate names, lookup, deletion, missing IDs, and the chosen copy behavior. Establish how later loading can restore IDs and advance allocation safely.

## Acceptance Criteria

- [ ] Duplicate names do not interfere with identifying objects.
- [ ] IDs remain stable when other cubes are added or removed.
- [ ] Lookup and deletion handle missing IDs safely.
- [ ] Scene copies retain valid internal identity and independent lifecycle operations.
- [ ] Existing creation, rendering, and collision behavior remains valid.
- [ ] Relevant tests and Debug/Release builds pass, or precise blockers are reported.

## Non-Goals

- UI, serialization, undo/redo, parenting, rotation, generic components, or changing Player ownership.
- A claim that raw GameObject references remain valid after deletion.

## Verification Commands

Run from the repository root:

    cmake --preset windows -DBUILD_TESTING=ON
    cmake --build --preset debug
    ctest --test-dir build/windows -C Debug --output-on-failure
    cmake --build --preset release
    ctest --test-dir build/windows -C Release --output-on-failure

Use existing repository conventions if paths or presets have changed. Report changed files, identity semantics, actual results, and blockers. Do not claim unrun checks passed.