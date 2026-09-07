# Task: Verify and stabilize the complete scene editor workflow

## Goal

Validate the first editor milestone end to end and fix integration defects within its existing scope.

## Context

- Engine: C++20, Windows, OpenGL 3.3.
- Prerequisites: tasks 1–7 are complete.
- Expected features: stable cube IDs, separate presentation, offscreen viewport, Dear ImGui panels, independent editor camera, cube authoring, and versioned scene save/load.
- Inspect repository instructions and current implementation; do not assume earlier completion reports prove behavior.
- Constraints: fix defects in these features without redesigning the engine or adding later milestones.

## Steps

1. Establish Debug/Release build and test results. Separate pre-existing failures from editor regressions.
2. Exercise the full workflow: create multiple cubes, use duplicate names, select each, edit transforms/colors/colliders, delete the selected cube, save, restart, and load.
3. Confirm loaded values and IDs match, new IDs remain unique, and failed loading preserves the active scene.
4. Check viewport orientation, depth, aspect ratio, repeated resizing, collapsed/zero-area panels, minimize/restore, and DPI behavior where available.
5. Check typing names containing navigation keys, dragging inspector controls, scrolling other panels, viewport navigation, and window focus loss.
6. Confirm camera movement does not mutate player/cube data and player simulation remains stopped in Edit mode.
7. Check one presentation per frame, resource cleanup, and bounded smoke-test termination.
8. Add a focused automated integration test for create/edit/delete/save/load and regression tests for defects actually found. Avoid tests that merely mirror implementation.
9. Fix identified integration defects and rerun affected checks.
10. Document editor controls, scene-file usage, dependency/build requirements, and known limitations in the existing documentation.

## Acceptance Criteria

- [ ] The complete authoring/save/restart/load workflow works.
- [ ] Selection remains safe across deletion and scene replacement.
- [ ] Input capture prevents editing from triggering navigation.
- [ ] Viewport resize and window lifecycle behavior are correct.
- [ ] Invalid edits/files produce recoverable feedback.
- [ ] Existing collision behavior remains covered by relevant tests.
- [ ] Debug and Release builds and actual CTest results are reported.
- [ ] Smoke-test termination is verified.
- [ ] Manual checks are explicitly marked performed, failed, or unavailable.
- [ ] Remaining limitations are stated without presenting unverified behavior as passed.

## Non-Goals

- Undo/redo, unsaved-change tracking, Play/Stop, rotation, parenting, picking, transform handles, or additional object types.
- Performance optimization without evidence of a milestone-blocking issue.

## Verification Commands

Run from the repository root:

    cmake --preset windows -DBUILD_TESTING=ON
    cmake --build --preset debug
    ctest --test-dir build/windows -C Debug --output-on-failure
    cmake --build --preset release
    ctest --test-dir build/windows -C Release --output-on-failure
    & .\build\windows\bin\Debug\sandbox.exe --smoke-test
    & .\build\windows\bin\Release\sandbox.exe --smoke-test

Launch build/windows/bin/Debug/sandbox.exe for manual verification when available. If tooling or a graphical session is unavailable, report the precise blocker and actual fallback checks. Report defects fixed, changed files, test results, manual verification status, and remaining limitations.