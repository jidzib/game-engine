#pragma once
#include <engine/Scene.hpp>
#include <filesystem>

namespace engine {
// Throws descriptive exceptions on invalid data or filesystem errors.
// Loading commits via noexcept swap only after the complete file validates.
struct ScenePersistence {
    static void save(const Scene& scene, const std::filesystem::path& path);
    static void load(Scene& scene, const std::filesystem::path& path);
};
}
