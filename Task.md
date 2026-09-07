# Task: Separate editor navigation from player simulation

## Goal

Navigate the scene with an independent editor camera while leaving the authored player and cubes stationary.

## Context

- Engine: C++20, Windows, OpenGL 3.3, Dear ImGui.
- Prerequisites: tasks 1–4 provide identity, offscreen rendering, separate presentation, and editor panels.
- Original Engine::run polls GetAsyncKeyState, moves scene.player, orbits/zooms Camera, and targets scene.player.object.position every frame.
- Original controls include WASD, Space/Shift, arrow keys, mouse wheel, and R.
- Inspect Camera, Window, Player, and editor integration before choosing a minimal navigation API.
- Constraints: follow repository instructions. Keep gameplay movement/collision code intact for future Play mode.

## Steps

1. Establish a baseline and identify current input ownership and camera capabilities.
2. Add editor-owned camera/navigation state that does not reference the player's transform as its continuously updated target.
3. Start in Edit mode and stop calling player movement/simulation from that mode.
4. Implement orbit, zoom, reset, and target translation/panning so users can inspect the whole scene. Reuse current camera math where practical and document bindings.
5. Route navigation only during explicit viewport interaction. Account for text editing, UI keyboard/mouse capture, window deactivation, and focus loss.
6. Ensure wheel events over other panels do not zoom the scene and consumed events do not cause delayed navigation.
7. Make reset restore an editor view without changing scene data.
8. Preserve the bounded smoke-test path. Add focused tests only for nontrivial separable input/state logic.

## Acceptance Criteria

- [ ] Camera navigation leaves player and cube data unchanged.
- [ ] The editor can inspect locations away from the player.
- [ ] Typing in UI fields does not navigate or reset the camera.
- [ ] Scrolling other panels does not zoom the viewport.
- [ ] Losing focus stops navigation without stuck controls.
- [ ] Frame-rate-scaled movement and safe camera limits are maintained.
- [ ] Existing collision tests remain valid and builds/smoke-test pass.

## Non-Goals

- Play/Stop, gameplay redesign, gravity changes, object picking, or transform handles.

## Verification Commands

Run from the repository root:

    cmake --preset windows -DBUILD_TESTING=ON
    cmake --build --preset debug
    ctest --test-dir build/windows -C Debug --output-on-failure
    cmake --build --preset release
    ctest --test-dir build/windows -C Release --output-on-failure
    & .\build\windows\bin\Debug\sandbox.exe --smoke-test

Manually exercise viewport interaction, other panels, focus loss, and camera reset when possible. Repeat text-field checks once task 6 adds editable fields. Report bindings, actual results, and verification limitations.