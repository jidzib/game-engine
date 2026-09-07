# GameEngine

C++20 engine foundation with a native Windows window, OpenGL 3.3 Core renderer,
and an independent editor camera inspecting an authored player and configurable cubes.
The engine is a static library; the sandbox is a standalone desktop executable.
Dear ImGui is vendored at a pinned revision; no build-time downloads are required. WGL creates the context, the graphics
driver supplies OpenGL, and the engine's Vec3/Mat4 types provide the math.

## Camera controls

The sandbox starts in Edit mode. Player and cube data stay stationary; gameplay
movement and collision APIs remain intact for a future Play mode.

Hold the right mouse button **over the viewport image** to use these controls:

- WASD: translate the editor target on world X/Z (W = +Z, S = -Z, A = +X, D = -X).
- Space / left Shift: translate the target up / down.
- Arrow keys: orbit the editor target.
- Mouse wheel: zoom in / out.
- R: reset the entire editor view to its initial origin target, orbit and distance.

Panning is normalized in 3D at 5 world units/second. Translation and orbit use
frame time capped at 0.05 seconds; existing pitch (0.08–1.45 radians) and zoom
(3–25 units) limits remain. Escape cancels active UI editing; when the UI does
not capture the keyboard, Escape exits. The close button always exits.

Releasing RMB, leaving the image, UI capture, deactivation, minimization, or
focus loss cancels navigation. Press RMB again on the image to resume. Text
editing blocks all navigation, including reset. Wheel events are checked at their
original screen position against the image and ImGui panel stacking; other panels
keep scrolling and rejected events are never replayed. Wheel navigation begins
after the RMB interaction has been established by a frame.

Each cube mesh is centered on its GameObject position and measures two units per side before scaling. The camera
uses a 60-degree vertical field of view, with near/far planes of 0.1 and 100.
The window starts at 1280 x 720, supports resizing, and pauses rendering while
minimized. Rendering uses a depth buffer, directional face lighting, and VSync
when the driver supports WGL_EXT_swap_control.

## Offscreen scene target

The renderer draws the grid, cubes, and player into an RGBA8 texture with a
24-bit depth renderbuffer, then displays it in the editor viewport before the single
presentation. Existing shaders and geometry are shared by this path.
`Renderer::sceneTexture()` exposes a borrowed texture handle and dimensions for
the editor viewport; texture coordinates use OpenGL's bottom-left origin and the UI flips the vertical UVs.
The handle changes on successful storage resize and expires at renderer destruction.

Unchanged dimensions reuse storage. Zero-area drawables skip drawing, preview,
and presentation and expose an empty texture descriptor, retaining storage for
restore. Sizes exceeding texture, renderbuffer, or viewport limits throw a
descriptive error. Allocation and framebuffer completeness errors include the
requested dimensions; failed allocation releases temporary resources and keeps
the previous target. The sandbox reports the error and exits. All owned GL
resources are deleted before the context and window are destroyed.

Scene drawing establishes its depth, culling, masks, viewport, and framebuffer
state. Drawing and `blitSceneToWindow()` finish with the default framebuffer,
window viewport, program 0 and VAO 0, depth/culling/blending/scissor/stencil/sRGB
disabled, and color/depth writes enabled. Later UI drawing must set its own
program, geometry, texture, and blend state. These passes do not preserve caller
GL state; calls require the renderer's context to remain current on its thread.

## Add your own cubes

Edit `sandbox/main.cpp`, before `application.run(...)`, then rebuild:

```cpp
auto& cube = application.scene.addCube("My cube");
cube.position = {4.0f, 0.0f, 2.0f};
cube.scale = {1.0f, 1.0f, 1.0f};
cube.color = {0.2f, 0.8f, 0.4f};
```

Or use `application.scene.addCube("Tall cube", {3, 1, 0}, {1, 2, 1});`.
Scale must be finite and positive on every axis; actual dimensions are twice
the scale. The grid is at Y = -1.01, so use `position.y = scale.y - 1.0f`
to place a cube just above it. Color components use the range 0 to 1.
References returned by `addCube` remain valid when you append more cubes.

The player is also drawn as a GameObject, available through
`application.scene.player.object`. Configure its position, scale, and color
the same way, and set `application.scene.player.movementSpeed` in units/second.
The player movement API normalizes diagonally and uses elapsed frame time,
with swept AABB collision and sliding against enabled scene colliders. Edit mode
does not call it; its camera is independent of the player. Gravity and jumping
are not implemented.

## BoxCollider component

Each GameObject has an optional `boxCollider`, absent by default. Attach and
configure it before `application.run(...)`:

```cpp
auto& cube = application.scene.addCube("Collidable cube", {4, 0, 2});
auto& collider = cube.boxCollider.emplace();
collider.enabled = true;
collider.offset = {0, 0, 0};
collider.halfExtents = {1, 1, 1};
```

`offset` and `halfExtents` are in the object's local space, before its scale.
Half extents should be positive; `{1,1,1}` matches the existing cube mesh.
Use `cube.boxCollider->enabled = false` to disable an attached component, or
`cube.boxCollider.reset()` to remove it. The player's GameObject supports the
same attachment through `application.scene.player.object.boxCollider.emplace()`.
The component stores configuration; Collision.cpp computes bounds, resolves
overlap, and sweeps player movement against enabled static boxes. Editor mode
stores these settings without running player movement.

## Build on this Windows machine

Installed tools: CMake 4.2 and Visual Studio Community 2026 with C++ tools and Windows SDK.
The Windows preset requires CMake 4.2+ for the Visual Studio 2026 generator.
Run these commands in PowerShell from this folder:

```powershell
cmake --preset windows -DBUILD_TESTING=ON
cmake --build --preset debug
.\build\windows\bin\Debug\sandbox.exe
```

For an optimized build:

```powershell
cmake --build --preset release
.\build\windows\bin\Release\sandbox.exe
```

Open this folder in Visual Studio to use its CMake integration, or open the generated solution under build/windows after configuring. Select sandbox as the startup target.

After moving or renaming the project folder, regenerate CMake's cached absolute
paths and rebuild from the new location:

```powershell
cmake --preset windows --fresh -DBUILD_TESTING=ON
cmake --build --preset debug
.\build\windows\bin\Debug\sandbox.exe
```

Reopen the folder in Visual Studio if it was open during the move. Generated
build files retain the old location until CMake regenerates them.

## Structure

- CMakeLists.txt: targets, include paths, C++ standard, and compiler warnings.
- CMakePresets.json: reproducible x64 configuration and Debug/Release builds.
- engine/include/engine: public engine headers.
- engine/src: engine implementation.
- engine/src/Window.*: window lifetime, events, and wheel input.
- engine/src/Renderer.*: WGL context, OpenGL function loading, VAOs/VBOs, embedded GLSL shaders, and drawing.
- engine/include/engine/Math.hpp: Vec3, column-major Mat4, transforms, and OpenGL projection math.
- engine/include/engine/Camera.hpp and engine/src/Camera.cpp: orbit controls and view/projection matrices.
- sandbox: executable for trying engine features.
- build: generated files, ignored by Git.

Add each new source file explicitly to its target in CMakeLists.txt. Link future libraries to engine with target_link_libraries; use PUBLIC when public engine headers expose that dependency and PRIVATE otherwise.

The current window/context layer requires Windows 10+ and a graphics driver
supporting OpenGL 3.3 Core. Context creation fails with a clear error if this
requirement is unmet. There is no legacy rendering fallback.
CMake links OpenGL::GL, user32, and gdi32. No DirectX libraries or math types remain.
The native Win32/WGL window and input layer still needs porting for other platforms.

Matrices use column-major storage and column vectors, with
`clip = projection * view * model * position`. The camera is right-handed and
the near/far planes map to OpenGL's -1/+1 depth range. GLSL transforms normals
with the inverse transpose of the model matrix for nonuniform scales.
See the [Khronos WGL context guide](https://wikis.khronos.org/opengl/Creating_an_OpenGL_Context)
and [uniform matrix reference](https://wikis.khronos.org/opengl/GLAPI/glUniform).

## Scene editor workflow

In Hierarchy, use **Add cube** and select a row. Selection uses stable cube IDs,
so duplicate names and names containing `##` are safe. Inspector edits the name,
position, positive scale (half dimensions), color, and optional box collider.
Drag numbers or Ctrl+click to type. Collider offsets and half extents use local
space and are scaled by the object. Disabled colliders retain their geometry.
Invalid numeric edits leave the object unchanged and show an error. **Delete
cube** removes the selected cube and clears selection.

Enter a file in **Scene path**, then **Save**. Paths are UTF-8; relative paths
resolve from the process working directory. Prefer an absolute path when
restarting from another launcher. Restart sandbox, enter the same path, then
**Load**. Loading replaces the active scene, clears selection and navigation,
and preserves IDs, deleted-ID allocation history, player data and cube properties.
Failed loads preserve the scene and selection and report an error in Hierarchy.
Save replaces an existing destination through a temporary sibling file. Missing
parent directories must be created separately. See [SCENE_FORMAT.md](SCENE_FORMAT.md)
for the version 1 schema, validation, and filesystem limitations.

Panels can be moved, resized, and collapsed. Layout resets each launch. The
viewport flips OpenGL texture UVs, sizes storage to its drawable area and updates
projection aspect. Collapsing or shrinking it to zero content suspends scene
drawing while other panels render. Native minimization pauses rendering.
Per-monitor DPI awareness uses physical client pixels; fonts/style rebuild on
DPI changes and the window applies the suggested bounds.

The separate editor target owns Dear ImGui and its Win32/OpenGL backends.
Vendored dependencies are Dear ImGui v1.91.9b and nlohmann/json 3.11.3; see
[ImGui provenance](third_party/imgui/README.vendor.md) and
[JSON provenance](third_party/json/README.vendor.md) for pins and licenses.
No package installation or network download is needed. ImGui also links dwmapi.
The wheel routing uses the pinned ImGui internal hit test; review it when upgrading.

Known scope limits: no undo/redo, unsaved-change warning, automatic scene loading,
Play/Stop, rotation, parenting, picking, transform handles, docking, or additional
object types. The player is saved and rendered but has no Inspector row. Collision
is limited to static unrotated AABBs. GL allocation failures remain fatal and
report an error; they are not recoverable Inspector validation failures.

## Verification

Configure with BUILD_TESTING=ON, build both presets, and run:

```powershell
ctest --test-dir build/windows -C Debug --output-on-failure
ctest --test-dir build/windows -C Release --output-on-failure
& .\build\windows\bin\Debug\sandbox.exe --smoke-test
& .\build\windows\bin\Release\sandbox.exe --smoke-test
```

Smoke mode renders ten frames and exits; its CTest timeout is 20 seconds.
The minimized branch also consumes its frame budget rather than waiting forever.
A normal close preserves the HWND until editor and renderer cleanup completes.
Source inspection confirms one presentation call per rendered Engine frame and
one SwapBuffers call in Renderer::present; no gameplay update runs in Edit mode.

The nine CTest entries cover:

- `editor_workflow`: author through editor operations, edit all cube properties,
  reject invalid edits, delete selection, save, exit, and load in a separate
  process. Compare complete saved state, IDs, failed-load preservation and
  camera independence. This is an API integration test, not a UI click script.
- `scene_persistence`: schema, numeric/ID validation, all authored fields,
  allocator exhaustion, failed transactions and destination replacement.
- `editor_operations`: selection, deletion, validation and collider authoring.
- `scene_behavior` and `collision_behavior`: existing scene/math/movement and
  swept collision/recovery behavior.
- `renderer_target`: real GPU depth, draw order, blit pixels, aspect, repeated
  sizing, zero-area restoration, allocation limit rejection and context cleanup.
- `editor_navigation`: ownership, capture cancellation, timing and camera limits.
- `editor_interface`: actual ImGui name input/capture, rendered edits, selection
  deletion, resize/collapse/zero-content, synthetic native minimize/restore and
  focus-loss messages, Escape cancellation/exit, and GL/UI cleanup.
- `render_smoke`: bounded sandbox startup, rendering and shutdown.

### Milestone verification (2026-09-07)

Baseline configuration and Debug build succeeded; Debug CTest passed 8/8.
The baseline Release build initially hit the documented SDK metadata access
restriction; approved escalation succeeded and Release CTest then passed 8/8.
No baseline test failures were found. The input review found that native Escape
unconditionally closed the application during text editing. The fix preserves
Escape for UI cancellation while retaining the uncaptured exit shortcut.

Final Debug and Release builds succeeded. Debug CTest passed 9/9 (1.48 seconds)
and Release CTest passed 9/9 (1.69 seconds). Both standalone smoke runs exited 0
in 0.48 seconds. An intermediate Debug link was blocked by an already-running
sandbox; after it closed, the full Debug build, CTest and standalone smoke were
rerun successfully. No toolchain or global Git settings were changed.

Manual checks: **unavailable**, not passed. This session has no native desktop
control API (browser control is available, native APIs are disabled). No manual
create/save/restart/load, visual orientation, dragging, panel scrolling, camera
navigation, Alt-Tab, minimize/restore or multi-monitor DPI check was performed.
Automated GL/ImGui tests run in this environment, but synthetic messages do not
prove foreground interaction or real monitor DPI transitions. UV orientation is
reviewed in source; no screen-level orientation assertion is claimed. Neither
GPU out-of-memory nor minimized smoke execution was forced.

For manual follow-up, launch the Debug sandbox, create duplicate-name cubes,
edit every property, delete the selection, save to an absolute path, restart and
load. Try invalid numeric edits and malformed JSON. Type `wasd R` in Name while
holding navigation keys, drag numbers, scroll Hierarchy under the pointer, and
navigate only with a fresh RMB press over the image. Alt-Tab while navigating,
then verify a fresh RMB press is required. Resize/collapse/restore the viewport,
minimize/restore the window and move between differing-DPI monitors; inspect
orientation, occlusion and proportions. Confirm authored data stays fixed.
