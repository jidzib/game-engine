# Task: Integrate a minimal editor interface

## Goal

Show the scene inside an editor application with hierarchy, inspector, and viewport panels.

## Context

- Engine: C++20, Windows, OpenGL 3.3.
- Prerequisites: tasks 1–3 provide cube IDs, separate presentation, and an offscreen scene texture.
- The window uses native Windows handling; inspect its event procedure and OpenGL context setup.
- Use Dear ImGui with its Win32 and OpenGL 3 backends. This task authorizes that dependency.
- Keep editor code in a separate module or target with no editor dependency in Scene/GameObject.
- This task builds the interface shell. Dedicated navigation is task 5; object selection and editing are task 6.

## Steps

1. Inspect repository instructions and establish a baseline.
2. Integrate a pinned Dear ImGui revision using repository dependency conventions. Avoid floating branches and document acquisition/build requirements.
3. Initialize and shut down UI backends in the correct window/context lifetime order.
4. Forward necessary Windows events to the UI while preserving close, resize, and other window lifecycle handling.
5. Add hierarchy, inspector, and viewport panels. A simple resizable layout is sufficient; docking is optional.
6. Show cube names in a read-only hierarchy and placeholder inspector guidance.
7. Replace the previous full-window blit with the scene texture displayed inside the viewport panel. Handle texture orientation and display/DPI scaling.
8. Derive offscreen drawable dimensions from available panel content, skip collapsed/zero-area viewports, and render scene then UI before one presentation.
9. Expose viewport focus/hover and UI input-capture information for task 5. Suppress gameplay keyboard input during UI keyboard capture and prevent UI scrolling from zooming the scene.
10. Preserve bounded smoke-test startup and shutdown.

## Acceptance Criteria

- [ ] The application shows all three panels and the correctly oriented scene.
- [ ] Viewport resizing maintains correct proportions and depth.
- [ ] UI initialization, rendering, and shutdown work without OpenGL errors.
- [ ] Scene drawing establishes its needed state after UI rendering on previous frames.
- [ ] Core scene types do not depend on Dear ImGui.
- [ ] Dependency revision is reproducible and documented.
- [ ] Builds/tests and smoke-test succeed, or blockers are reported.

## Non-Goals

- Object editing, persistence, Play mode, multi-window viewports, or a custom UI toolkit.

## Verification Commands

Run from the repository root:

    cmake --preset windows -DBUILD_TESTING=ON
    cmake --build --preset debug
    ctest --test-dir build/windows -C Debug --output-on-failure
    cmake --build --preset release
    ctest --test-dir build/windows -C Release --output-on-failure
    & .\build\windows\bin\Debug\sandbox.exe --smoke-test

Manually inspect panel sizing, texture orientation, DPI behavior where possible, and shutdown. Report actual results and visual verification status. Do not claim unrun checks passed.