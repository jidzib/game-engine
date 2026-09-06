# Task: Separate scene drawing from window presentation

## Goal

Allow scene rendering and a future editor interface to compose one frame before the window presents it.

## Context

- Engine: C++20, Windows, OpenGL 3.3.
- Prerequisite: stable cube IDs and lookup/deletion operations from task 1.
- Inspect engine/src/Engine.cpp and the Renderer implementation/header; discover their exact paths.
- Engine::run currently creates Window, Renderer, and Camera, then updates input, resizes, and calls renderer.render(camera, scene).
- Renderer::render draws the grid, cubes, and player, then calls SwapBuffers and optionally Sleep(1).
- Renderer::resize updates stored dimensions and glViewport.
- Constraints: inspect repository instructions and current code. Preserve current gameplay and the bounded --smoke-test path. No new dependencies.

## Steps

1. Establish a build/test baseline and identify OpenGL context and resource lifetime ownership.
2. Separate scene drawing from presentation using a small renderer API.
3. Move SwapBuffers and existing frame pacing into the presentation operation.
4. Update Engine::run to draw and then present exactly once per completed frame.
5. Preserve error handling, camera aspect ratio, resize behavior, and minimized-window behavior.
6. Ensure shutdown still destroys OpenGL resources while the required context is valid.
7. Keep changes limited to enabling later editor composition; preserve the current default framebuffer rendering path.

## Acceptance Criteria

- [ ] Drawing the scene does not swap buffers or sleep.
- [ ] A separate presentation operation swaps once and retains existing pacing behavior.
- [ ] Existing scene visuals and controls remain unchanged.
- [ ] Resize and minimize/restore work without invalid dimensions.
- [ ] The bounded smoke-test path still exits.
- [ ] Debug/Release builds and relevant tests pass, or blockers are reported.

## Non-Goals

- Editor UI, offscreen framebuffers, new cameras, scene editing, or gameplay changes.
- A general rendering architecture rewrite.

## Verification Commands

Run from the repository root:

    cmake --preset windows -DBUILD_TESTING=ON
    cmake --build --preset debug
    ctest --test-dir build/windows -C Debug --output-on-failure
    cmake --build --preset release
    ctest --test-dir build/windows -C Release --output-on-failure
    & .\build\windows\bin\Debug\sandbox.exe --smoke-test

Use actual repository paths if different. Manually check normal rendering and resize/minimize behavior when a graphical session is available. Report changed files, actual results, and whether visual verification occurred. Do not claim unrun checks passed.