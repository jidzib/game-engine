# Task: Add an offscreen scene rendering target

## Goal

Render the existing scene into a resizable color texture with a depth attachment, ready for an editor viewport.

## Context

- Engine: C++20, Windows, OpenGL 3.3.
- Prerequisites: task 1 object identity and task 2 separate drawing/presentation.
- Renderer uses a custom GLApi function-pointer loader populated through extension().
- Existing rendering draws a grid, scene cubes, and the player with GLSL 330 shaders.
- Constraints: inspect repository instructions, renderer/context ownership, and loader conventions. Reuse current shaders and geometry. No new dependencies.

## Steps

1. Establish a baseline and inspect extension() address lookup and existing OpenGL declarations.
2. Extend GLApi with the framebuffer/renderbuffer entry points required for a color texture and depth renderbuffer. Follow existing loading/error conventions.
3. Add owned viewport resources with cleanup while the OpenGL context remains valid.
4. Allocate suitable OpenGL 3.3 color/depth storage and verify framebuffer completeness.
5. Resize attachments only when drawable dimensions change. Skip zero-area rendering safely and respect OpenGL size limits.
6. Render using the offscreen target's viewport and aspect ratio. Expose its texture handle and dimensions through a small internal API for the future editor.
7. Make framebuffer and relevant rendering-state ownership explicit for scene drawing and later window/UI drawing.
8. Exercise the new target through the sandbox: render offscreen and blit its color output to the window before presentation. Load any additional required blit function. The next task will replace this full-window preview with an editor panel.

## Acceptance Criteria

- [ ] The sandbox visibly renders through the offscreen target with correct depth and aspect ratio.
- [ ] Resize does not allocate new storage every unchanged frame.
- [ ] Zero-size/minimized states and allocation failures are handled safely.
- [ ] Framebuffer completeness is checked and failures are actionable.
- [ ] Resources are released with valid context lifetime.
- [ ] There is still one presentation per frame.
- [ ] Builds, tests, and the smoke-test path succeed, or blockers are reported.

## Non-Goals

- Editor UI, multisampling, post-processing, picking, or shader redesign.

## Verification Commands

Run from the repository root:

    cmake --preset windows -DBUILD_TESTING=ON
    cmake --build --preset debug
    ctest --test-dir build/windows -C Debug --output-on-failure
    cmake --build --preset release
    ctest --test-dir build/windows -C Release --output-on-failure
    & .\build\windows\bin\Debug\sandbox.exe --smoke-test

Manually inspect depth, aspect ratio, repeated resizing, and minimize/restore if available. Report actual results, visual verification status, and limitations. Do not claim unrun checks passed.