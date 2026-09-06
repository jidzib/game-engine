# GameEngine

C++20 engine foundation with a native Windows window, OpenGL 3.3 Core renderer,
and an orbit camera following a movable orange player among configurable cubes.
The engine is a static library; the sandbox is a standalone desktop executable.
No third-party downloads are required. WGL creates the context, the graphics
driver supplies OpenGL, and the engine's Vec3/Mat4 types provide the math.

## Camera controls

- WASD: move the orange player on the world X/Z plane (W = +Z, S = -Z, A = -X, D = +X).
- Arrow keys: orbit around the player (hold to move).
- Mouse wheel: zoom in/out.
- R: reset the camera.
- Escape or the close button: exit.

Each cube mesh is centered on its GameObject position and measures two units per side before scaling. The camera
uses a 60-degree vertical field of view, with near/far planes of 0.1 and 100.
The window starts at 1280 x 720, supports resizing, and pauses rendering while
minimized. Rendering uses a depth buffer, directional face lighting, and VSync
when the driver supports WGL_EXT_swap_control.

## Offscreen scene target

The renderer draws the grid, cubes, and player into an RGBA8 texture with a
24-bit depth renderbuffer, then blits its color to the window before the single
presentation. Existing shaders and geometry are shared by this path.
`Renderer::sceneTexture()` exposes a borrowed texture handle and dimensions for
a future editor viewport; texture coordinates use OpenGL's bottom-left origin.
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
Movement is normalized diagonally and uses elapsed frame time. The camera
follows the player; R resets the camera without resetting player position.
This is movement only: collision, gravity, and jumping are not implemented yet.

## BoxCollider component (step 1)

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
This step stores configuration only: bounds, overlap checks, and movement
blocking are not implemented yet, and `enabled` has no gameplay effect yet.

## Build on this Windows machine

Installed tools: CMake 4.2 and Visual Studio Community 2026 with C++ tools and Windows SDK.
The Windows preset requires CMake 4.2+ for the Visual Studio 2026 generator.
Run these commands in PowerShell from this folder:

```powershell
cmake --preset windows
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

## Verification

Run `ctest --test-dir build/windows -C Debug --output-on-failure` after building.
The tests check transforms, stable object references, movement speed and diagonal
normalization, projection depth/aspect, camera targeting, and a ten-frame OpenGL
rendering smoke test. Use `-C Release` for Release.
The `renderer_target` integration test reads actual GL pixels to check depth
occlusion, draw-order independence, preview blitting, and resize aspect ratio.
It also checks unchanged storage, zero-area/restore behavior, size-limit errors,
repeated resizing, and context teardown.

Offscreen task verification (2026-09-06): configuration and Debug/Release builds
succeeded, and all four CTest tests passed in each configuration. The standalone
Debug sandbox smoke test also exited successfully. Manual visual inspection and
interactive minimize/restore were not performed; zero-area/restore was exercised
through the renderer API. GPU out-of-memory and incomplete-framebuffer failures
were not forced; size-limit rejection was tested.

Run the sandbox with `--smoke-test` to create the window, compile the shaders,
render ten frames, and exit. Exit code zero indicates success; errors return one.
Normal runs display an error dialog on startup or rendering failure.

Use the executables under `build/windows/bin/Debug` or `build/windows/bin/Release`.
The older `build/verify` directory is not used by the Windows presets and may
contain stale files from a previous project location.
