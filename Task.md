# Task: Save and load authored scenes

## Goal

Persist scenes edited in the application and reload them without losing object identity or authored properties.

## Context

- Engine: C++20, Windows, OpenGL 3.3, Dear ImGui.
- Prerequisites: tasks 1–6 provide stable IDs and working cube authoring.
- Scene contains player and cubes. Cube data includes ID, name, position, scale, color, and optional BoxCollider.
- Inspect Player and BoxCollider to determine authored fields and existing validation.
- Use versioned JSON. Reuse an existing JSON library; if absent, this task authorizes one small, pinned JSON dependency following repository conventions.
- Constraints: inspect repository instructions. Do not serialize pointers, OpenGL resources, UI selection, or transient collision state.

## Steps

1. Establish a baseline and define a documented version-1 schema.
2. Persist cube order, IDs, names, transforms, colors, and complete optional collider configuration. Preserve missing versus disabled colliders.
3. Persist player spawn/transform, appearance, collider configuration, movementSpeed, and other existing authored settings where applicable. Exclude runtime-only state and document the field mapping.
4. Implement serialization separately from editor widgets.
5. Parse into a temporary scene and validate schema version, required fields, numeric bounds, object IDs, and collider data. Reject duplicate/invalid IDs and unsupported versions with actionable errors.
6. Restore IDs without precision loss and update ID allocation so later additions cannot collide.
7. Replace the active scene only after a successful load. Clear selection and stale editor references after replacement; leave the current scene untouched on failure.
8. Add Save and Load actions with an editable path and visible success/error feedback. Native dialogs are optional.
9. Save through a temporary sibling file and an appropriate replacement operation so a failed write does not truncate an existing valid scene. Handle filesystem failures explicitly.
10. Add automated round-trip, invalid-input, identity-continuation, and failed-load preservation tests using temporary test files.

## Acceptance Criteria

- [ ] Save/load preserves authored cube and player data.
- [ ] Duplicate names and missing/disabled colliders round-trip correctly.
- [ ] Loaded IDs remain stable and subsequent creation gets a fresh ID.
- [ ] Invalid, unsupported, or unreadable scenes do not replace the current scene.
- [ ] Failed saves report errors and preserve existing valid destination contents.
- [ ] Editor selection is safe after loading.
- [ ] Schema/dependency choices are documented and relevant checks pass.

## Non-Goals

- Asset database, external mesh loading, undo history persistence, autosave, schema migration, or Play mode.
- Unsaved-change prompts, which are a later feature.

## Verification Commands

Run from the repository root:

    cmake --preset windows -DBUILD_TESTING=ON
    cmake --build --preset debug
    ctest --test-dir build/windows -C Debug --output-on-failure
    cmake --build --preset release
    ctest --test-dir build/windows -C Release --output-on-failure
    & .\build\windows\bin\Debug\sandbox.exe --smoke-test

Manually save an edited scene, restart, and load it. Exercise malformed input and an unwritable path where possible. Report actual results, field mapping, and limitations. Do not claim unrun checks passed.