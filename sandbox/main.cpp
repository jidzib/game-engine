#include <engine/Engine.hpp>
#include <Windows.h>
#include <exception>
#include <string_view>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR commandLine, int) {
    const bool smokeTest = std::wstring_view(commandLine).find(L"--smoke-test") != std::wstring_view::npos;
    try {
        engine::Engine application;
        // Add your cubes here. Position is the center; dimensions are 2 * scale.
        application.scene.addCube("Left cube", {-3, 0, 2});
        application.scene.addCube("Tall cube", {3, 0.5f, 3}, {0.75f, 1.5f, 0.75f}, {0.50f, 0.35f, 0.95f});
        application.scene.addCube("Wide cube", {0, -0.5f, 5}, {1.5f, 0.5f, 0.75f}, {0.25f, 0.80f, 0.40f});
        application.scene.addCube("Wide cube", { 5, 2.0f, 5 }, { 1.5f, 0.5f, 0.75f }, { 1.0f, 0.0f, 0.0f });
        application.scene.addCube("Collision wall", {-4, 1, 0}, {0.25f, 3, 4});
        application.scene.addCube("Corner wall", {0, 1, 4}, {4, 3, 0.25f});
        application.scene.addCube("Floor", {0, -2, 0}, {4, 0.25f, 4});
        application.scene.addCube("Ceiling", {0, 4, 0}, {4, 0.25f, 4});
        for (auto& cube : application.scene.cubes) cube.boxCollider.emplace();
        application.scene.player.movementSpeed = 4.0f;
        application.run(smokeTest ? 10u : 0u);
        return 0;
    } catch (const std::exception& error) {
        OutputDebugStringA(error.what());
        if (!smokeTest) { MessageBoxA(nullptr, error.what(), "C++ Game Engine - Startup or rendering error", MB_OK | MB_ICONERROR); }
        return 1;
    }
}
