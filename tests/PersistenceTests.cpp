#include "Operations.hpp"
#include <json.hpp>
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <functional>
using engine::ScenePersistence;
using Json = nlohmann::json;
void expect(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::string read(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
void write(const std::filesystem::path& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary); out << bytes;
    if (!out) throw std::runtime_error("Fixture write failed");
}
void rejected(const std::function<void()>& action) {
    bool caught = false;
    try { action(); } catch (const std::exception& e) { caught = *e.what() != 0; }
    expect(caught, "Expected actionable failure");
}
struct Files {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("engine-persistence-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
    Files() { std::filesystem::create_directory(root); }
    ~Files() { std::error_code ec; std::filesystem::remove_all(root, ec); }
};
int main() {
    try {
        Files files;
        const auto path = files.root / L"scene-\u00e9.json", copy = files.root / "copy.json", bad = files.root / "bad.json";
        engine::Scene authored;
        auto& first = authored.restoreCube(9007199254740993ULL, "duplicate\n\"##");
        first.position = {1.25f, -2.5f, 3.75f};
        first.scale = {0.5f, 2, 3}; first.color = {0, 0.25f, 1};
        auto& second = authored.addCube(first.name);
        second.boxCollider = engine::BoxCollider{false, {1, 2, 3}, {0.25f, 2, 3}};
        authored.addCube("").boxCollider = engine::BoxCollider{true, {-1, 0, 2}, {2, 3, 4}};
        const auto deleted = authored.addCube("deleted").id;
        authored.removeCube(deleted);
        std::swap(authored.cubes[0], authored.cubes[2]);
        authored.player.object.name = "Custom player";
        authored.player.object.position = {-7, 8, 9};
        authored.player.object.scale = {2, 3, 4};
        authored.player.object.color = {0.1f, 0.2f, 0.3f};
        authored.player.object.boxCollider = engine::BoxCollider{false, {1, -2, 3}, {4, 5, 6}};
        authored.player.movementSpeed = -2.5f;
        ScenePersistence::save(authored, path);
        const auto original = read(path);
        engine::Scene loaded;
        editor::Operations operations;
        operations.create(loaded);
        operations.load(loaded, path);
        expect(operations.selection() == 0, "Successful load clears selection");
        ScenePersistence::save(loaded, copy);
        expect(read(copy) == original, "All fields and array order round trip exactly");
        expect(loaded.addCube("fresh").id == deleted + 1, "Deleted IDs stay consumed");
        ScenePersistence::load(loaded, path);
        const auto selected = loaded.cubes[0].id;
        operations.select(loaded, selected);
        const auto base = Json::parse(original);
        auto invalid = [&](Json document) {
            write(bad, document.dump());
            rejected([&] { operations.load(loaded, bad); });
            expect(operations.selection() == selected, "Failed load preserves selection");
            ScenePersistence::save(loaded, copy);
            expect(read(copy) == original, "Failed load preserves scene and allocator");
        };
        for (Json version : {Json(2), Json("1"), Json(1.0)}) { auto j = base; j["version"] = version; invalid(j); }
        for (Json id : {Json("0"), Json("-1"), Json("01"), Json("18446744073709551616"), Json(9007199254740993ULL)}) {
            auto j = base; j["cubes"][0]["id"] = id; invalid(j);
        }
        auto j = base; j["cubes"][1]["id"] = j["cubes"][0]["id"]; invalid(j);
        j = base; j["nextCubeId"] = "1"; invalid(j);
        j = base; j["player"].erase("movementSpeed"); invalid(j);
        j = base; j["cubes"][0].erase("collider"); invalid(j);
        j = base; j["cubes"] = Json::object(); invalid(j);
        j = base; j["player"]["movementSpeed"] = 1e100; invalid(j);
        j = base; j["cubes"][0]["position"] = {0, 1}; invalid(j);
        j = base; j["cubes"][0]["scale"][0] = 0; invalid(j);
        j = base; j["cubes"][0]["color"][1] = 2; invalid(j);
        j = base; j["player"]["collider"]["halfExtents"][0] = -1; invalid(j);
        j = base; j["player"]["collider"]["offset"][0] = 3e38; invalid(j);
        j = base; j["player"]["collider"]["enabled"] = 1; invalid(j);
        for (const auto* malformed : {"{", "null", "{} trailing", "{\"version\":NaN}", "{\"version\":1,\"version\":1}"}) {
            write(bad, malformed); rejected([&] { operations.load(loaded, bad); });
        }
        rejected([&] { operations.load(loaded, files.root / "missing.json"); });
        ScenePersistence::save(loaded, copy);
        expect(read(copy) == original && operations.selection() == selected, "Unreadable loads preserve state");
        rejected([&] { ScenePersistence::save(loaded, files.root / "missing" / "scene.json"); });
        loaded.player.movementSpeed = std::numeric_limits<float>::infinity();
        rejected([&] { ScenePersistence::save(loaded, path); });
        expect(read(path) == original, "Invalid save preserves destination");
        loaded.player.movementSpeed = 5;
        HANDLE lock = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        expect(lock != INVALID_HANDLE_VALUE, "Lock fixture destination");
        bool failed = false;
        try { ScenePersistence::save(loaded, path); } catch (const std::exception&) { failed = true; }
        CloseHandle(lock);
        expect(failed && read(path) == original, "Replacement failure preserves destination");
        for (const auto& entry : std::filesystem::directory_iterator(files.root))
            expect(entry.path().filename().wstring().find(L".tmp.") == std::wstring::npos, "Temporary files cleaned");
        ScenePersistence::save(loaded, path);
        expect(read(path) != original, "Replacement updates destination");
        engine::Scene exhausted;
        exhausted.restoreCube(std::numeric_limits<engine::CubeId>::max(), "last");
        exhausted.player.object.boxCollider.reset();
        ScenePersistence::save(exhausted, path);
        ScenePersistence::load(loaded, path);
        expect(!loaded.player.object.boxCollider && loaded.cubes[0].id == std::numeric_limits<engine::CubeId>::max(), "Max ID and absent player collider round trip");
        rejected([&] { loaded.addCube("overflow"); });
        std::cout << "Scene persistence tests passed.\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

