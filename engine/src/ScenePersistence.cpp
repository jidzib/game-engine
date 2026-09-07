#include <engine/ScenePersistence.hpp>
#include <engine/Collision.hpp>
#include <json.hpp>
#include <Windows.h>
#include <charconv>
#include <fstream>
#include <set>
#include <atomic>
#include <type_traits>

namespace engine {
namespace {
using Json = nlohmann::json;
[[noreturn]] void fail(const std::string& message) { throw std::runtime_error(message); }
CubeId identity(const Json& j, const std::string& field, bool allowZero = false) {
    if (!j.is_string()) fail(field + " must be a decimal string.");
    const auto s = j.get<std::string>();
    CubeId value = 0;
    const auto [end, error] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (s.empty() || (s.size() > 1 && s[0] == '0') || error != std::errc{} || end != s.data() + s.size()
        || (!allowZero && value == 0)) fail(field + " must be a canonical uint64 decimal ID (nonzero for cubes).");
    return value;
}
float number(const Json& j) {
    if (!j.is_number()) fail("Expected a finite float number.");
    const double d = j.get<double>();
    if (!std::isfinite(d) || std::abs(d) > std::numeric_limits<float>::max()) fail("Number exceeds finite float bounds.");
    const float value = static_cast<float>(d);
    if (d != 0 && value == 0) fail("Number underflows float storage.");
    return value;
}
Vec3 vector(const Json& j) {
    if (!j.is_array() || j.size() != 3) fail("Vector must contain exactly three numbers.");
    return {number(j[0]), number(j[1]), number(j[2])};
}
Json vector(Vec3 v) { return Json::array({v.x, v.y, v.z}); }
void validate(const GameObject& object) {
    (void)object.worldMatrix();
    for (float c : {object.color.x, object.color.y, object.color.z})
        if (!std::isfinite(c) || c < 0 || c > 1) fail("Color components must be between 0 and 1.");
    auto checked = object;
    if (checked.boxCollider) {
        checked.boxCollider->enabled = true;
        (void)worldAabb(checked);
    }
}
GameObject object(const Json& j) {
    GameObject result;
    result.name = j.at("name").get<std::string>();
    result.position = vector(j.at("position"));
    result.scale = vector(j.at("scale"));
    result.color = vector(j.at("color"));
    const auto& c = j.at("collider");
    if (!c.is_null()) result.boxCollider = BoxCollider{c.at("enabled").get<bool>(),
        vector(c.at("offset")), vector(c.at("halfExtents"))};
    validate(result);
    return result;
}
Json object(const GameObject& o) {
    validate(o);
    Json c = nullptr;
    if (o.boxCollider) c = {{"enabled", o.boxCollider->enabled}, {"offset", vector(o.boxCollider->offset)},
        {"halfExtents", vector(o.boxCollider->halfExtents)}};
    return {{"name", o.name}, {"position", vector(o.position)}, {"scale", vector(o.scale)},
        {"color", vector(o.color)}, {"collider", c}};
}
void watermark(const Scene& scene, CubeId next) {
    std::set<CubeId> ids;
    for (const auto& cube : scene.cubes) {
        if (!cube.id || !ids.insert(cube.id).second) fail("Cube IDs must be nonzero and unique.");
        if (next && cube.id >= next) fail("nextCubeId must exceed every cube ID, or be 0 (exhausted).");
    }
}
std::string windowsError(const char* operation) {
    return std::string(operation) + ": " + std::system_category().message(static_cast<int>(GetLastError()));
}
// CREATE_NEW owns a unique sibling; all exits close and remove only our file.
struct Temporary {
    std::filesystem::path path;
    HANDLE handle = INVALID_HANDLE_VALUE;
    ~Temporary() {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
        if (!path.empty()) DeleteFileW(path.c_str());
    }
};
}
void ScenePersistence::save(const Scene& scene, const std::filesystem::path& path) {
    try {
        watermark(scene, scene.nextCubeId_);
        if (!std::isfinite(scene.player.movementSpeed)) fail("player.movementSpeed must be finite.");
        Json document{{"version", 1}, {"nextCubeId", std::to_string(scene.nextCubeId_)},
            {"player", object(scene.player.object)}, {"cubes", Json::array()}};
        document["player"]["movementSpeed"] = scene.player.movementSpeed;
        for (const auto& cube : scene.cubes) {
            auto encoded = object(cube);
            encoded["id"] = std::to_string(cube.id);
            document["cubes"].push_back(std::move(encoded));
        }
        const auto bytes = document.dump(2) + "\n";
        if (path.empty() || path.filename().empty()) fail("Choose a destination file path.");
        Temporary temporary;
        static std::atomic<unsigned long long> sequence{0};
        for (int attempt = 0; attempt < 100; ++attempt) {
            auto candidate = path;
            candidate += L".tmp." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(sequence++);
            temporary.handle = CreateFileW(candidate.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (temporary.handle != INVALID_HANDLE_VALUE) { temporary.path = std::move(candidate); break; }
            if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS) fail(windowsError("Create temporary scene file"));
        }
        if (temporary.handle == INVALID_HANDLE_VALUE) fail("Could not allocate a unique temporary scene file.");
        size_t offset = 0;
        while (offset < bytes.size()) {
            const auto count = static_cast<DWORD>(std::min<size_t>(bytes.size() - offset, 1024 * 1024));
            DWORD written = 0;
            if (!WriteFile(temporary.handle, bytes.data() + offset, count, &written, nullptr)) fail(windowsError("Write scene"));
            if (!written) fail("Write scene made no progress.");
            offset += written;
        }
        if (!FlushFileBuffers(temporary.handle)) fail(windowsError("Flush scene"));
        const auto handle = std::exchange(temporary.handle, INVALID_HANDLE_VALUE);
        if (!CloseHandle(handle)) fail(windowsError("Close scene"));
        // Same-directory rename/replacement: never open or truncate the destination.
        if (!MoveFileExW(temporary.path.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            fail(windowsError("Replace scene destination"));
        temporary.path.clear();
    } catch (const std::exception& e) { fail("Save '" + path.string() + "': " + e.what()); }
}
void ScenePersistence::load(Scene& scene, const std::filesystem::path& path) {
    try {
        std::ifstream input(path, std::ios::binary);
        if (!input) fail("Cannot open scene for reading.");
        std::vector<std::set<std::string>> keys;
        const auto document = Json::parse(input, [&](int, Json::parse_event_t event, Json& parsed) {
            if (event == Json::parse_event_t::object_start) keys.emplace_back();
            else if (event == Json::parse_event_t::object_end) keys.pop_back();
            else if (event == Json::parse_event_t::key && !keys.back().insert(parsed.get<std::string>()).second)
                fail("Duplicate JSON field: " + parsed.get<std::string>());
            return true;
        });
        if (input.bad()) fail("Failed while reading scene.");
        if (!document.at("version").is_number_integer() || document.at("version") != 1)
            fail("Unsupported schema version; expected integer version 1.");
        Scene temporary;
        const auto& player = document.at("player");
        try {
            temporary.player.object = object(player);
            temporary.player.movementSpeed = number(player.at("movementSpeed"));
        } catch (const std::exception& e) { fail(std::string("player: ") + e.what()); }
        const auto& cubes = document.at("cubes");
        if (!cubes.is_array()) fail("cubes must be an array.");
        for (size_t i = 0; i < cubes.size(); ++i) {
            try {
                auto cube = object(cubes[i]);
                cube.id = identity(cubes[i].at("id"), "id");
                temporary.cubes.push_back(std::move(cube));
            } catch (const std::exception& e) { fail("cubes[" + std::to_string(i) + "]: " + e.what()); }
        }
        temporary.nextCubeId_ = identity(document.at("nextCubeId"), "nextCubeId", true);
        watermark(temporary, temporary.nextCubeId_);
        static_assert(std::is_nothrow_swappable_v<Player>);
        static_assert(noexcept(scene.cubes.swap(temporary.cubes)));
        std::swap(scene.player, temporary.player);
        scene.cubes.swap(temporary.cubes);
        std::swap(scene.nextCubeId_, temporary.nextCubeId_);
    } catch (const std::exception& e) { fail("Load '" + path.string() + "': " + e.what()); }
}
}
