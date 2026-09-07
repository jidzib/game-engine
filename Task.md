# Task: Add cube selection and property editing

## Goal

Create, select, edit, and delete scene cubes through the editor hierarchy and inspector.

## Context

- Engine: C++20, Windows, OpenGL 3.3, Dear ImGui.
- Prerequisites: tasks 1–5 provide stable IDs, editor panels, offscreen rendering, and independent navigation.
- GameObject contains name, position, scale, color, and optional BoxCollider.
- Cube dimensions are 2 * scale.
- worldMatrix() rejects nonfinite positions and nonpositive/nonfinite scales by throwing.
- BoxCollider has enabled, local offset, and local halfExtents; inspect actual validation rules.
- Constraints: follow repository instructions. Keep Scene::cubes; no ECS/reflection rewrite.

## Steps

1. Establish a baseline and inspect the current editor and scene lifecycle APIs.
2. Make hierarchy rows selectable using object IDs for both selection and UI identity, including duplicate names.
3. Add cube creation and deletion actions. Select newly created cubes and clear selection safely after selected-object deletion.
4. Add inspector fields for name, position, scale, and color. Clearly label scale as half dimensions or provide a dimensions field with explicit conversion.
5. Add collider attachment/removal and editing for enabled, offset, and halfExtents. Preserve the distinction between missing and disabled colliders.
6. Validate proposed values before committing. Reject nonfinite values and invalid sizes without allowing UI input to trigger a fatal render exception. Show understandable validation feedback.
7. Keep mutations behind small editor operations reusable by future undo support; do not build undo/redo yet.
8. Resolve selection by ID when used rather than retaining pointers across mutations.
9. Add focused tests for meaningful mutation/validation behavior and regressions.

## Acceptance Criteria

- [ ] Users can add, select, rename, edit, and delete cubes.
- [ ] Identically named cubes are independently selectable/editable.
- [ ] Deleting selection causes no stale access.
- [ ] Property changes appear in the viewport.
- [ ] Invalid transforms/collider values are rejected without crashing.
- [ ] Collider attachment, enabled state, and removal work distinctly.
- [ ] Typing and dragging inspector controls do not activate navigation.
- [ ] Existing tests, builds, and smoke-test pass, or blockers are reported.

## Non-Goals

- Player inspector, persistence, undo/redo, duplication, rotation, parenting, viewport picking, or gizmos.

## Verification Commands

Run from the repository root:

    cmake --preset windows -DBUILD_TESTING=ON
    cmake --build --preset debug
    ctest --test-dir build/windows -C Debug --output-on-failure
    cmake --build --preset release
    ctest --test-dir build/windows -C Release --output-on-failure
    & .\build\windows\bin\Debug\sandbox.exe --smoke-test

Manually exercise creation, duplicate names, editing, invalid values, collider toggles, and deletion. Report changed files, actual results, and whether visual verification occurred.