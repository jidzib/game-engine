#include "Operations.hpp"
#include "Navigation.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void expect(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
bool equal(engine::Vec3 a, engine::Vec3 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
std::string read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    expect(bool(input), "Read workflow snapshot");
    return {std::istreambuf_iterator<char>(input), {}};
}
void verify(const engine::Scene& scene) {
    expect(scene.cubes.size() == 3, "Three surviving cubes after restart");
    for (engine::CubeId id = 1; id <= 3; ++id) {
        const auto* cube = scene.findCube(id);
        expect(cube && cube->name == "wasd R ## duplicate", "Duplicate names retain separate IDs");
        const float n = float(id);
        expect(equal(cube->position, {n, -n, n * 2}), "Position survives restart");
        expect(equal(cube->scale, {n, 0.5f, 2}), "Scale survives restart");
        expect(equal(cube->color, {0.25f * n, 0.5f, 1}), "Color survives restart");
        expect(bool(cube->boxCollider) == (id != 1), "Collider attachment survives restart");
        if (cube->boxCollider) {
            expect(cube->boxCollider->enabled == (id == 3), "Collider enabled state survives restart");
            expect(equal(cube->boxCollider->offset, {n, -1, 0.5f}) &&
                equal(cube->boxCollider->halfExtents, {0.5f, n, 2}), "Collider geometry survives restart");
        }
    }
    expect(!scene.findCube(4), "Deleted selected cube stays deleted");
}
}
int main(int argc, char** argv) {
    try {
        expect(argc == 3, "Expected author/load and scene path");
        const std::filesystem::path path(argv[2]);
        engine::Scene scene;
        editor::Operations operations;
        std::string error;
        if (std::string(argv[1]) == "author") {
            for (int i = 1; i <= 4; ++i) operations.create(scene);
            for (engine::CubeId id : {3ULL, 1ULL, 2ULL}) {
                operations.select(scene, id);
                expect(operations.selection() == id, "Select duplicate by identity");
                editor::CubeProperties proposed(*scene.findCube(id));
                const float n = float(id);
                proposed.name = "wasd R ## duplicate";
                proposed.position = {n, -n, n * 2};
                proposed.scale = {n, 0.5f, 2};
                proposed.color = {0.25f * n, 0.5f, 1};
                if (id != 1) proposed.collider = engine::BoxCollider{id == 3, {n, -1, 0.5f}, {0.5f, n, 2}};
                expect(operations.update(scene, id, proposed, error), "Commit complete inspector properties");
                proposed.scale.x = 0;
                expect(!operations.update(scene, id, proposed, error) && !error.empty(), "Invalid edit gives recoverable feedback");
            }
            operations.select(scene, 4);
            expect(operations.remove(scene, 4) && operations.selection() == 0, "Delete selected cube clears selection");
            verify(scene);
            engine::ScenePersistence::save(scene, path);
        } else {
            expect(std::string(argv[1]) == "load", "Unknown workflow phase");
            operations.create(scene);
            operations.load(scene, path);
            expect(operations.selection() == 0, "Replacement clears colliding old selection");
            verify(scene);
            auto copy = path; copy += ".copy";
            engine::ScenePersistence::save(scene, copy);
            const auto snapshot = read(path);
            expect(read(copy) == snapshot, "All scene and player fields round trip across process restart");
            editor::Navigation navigation;
            editor::Navigation::Input input;
            input.allowed = input.pressed = input.held = true;
            input.horizontal = input.forward = input.vertical = input.yaw = input.wheel = 1;
            navigation.update(input, 0.05f);
            engine::ScenePersistence::save(scene, copy);
            expect(read(copy) == snapshot, "Camera navigation leaves all authored data unchanged");
            operations.select(scene, 2);
            auto bad = path; bad += ".bad";
            { std::ofstream output(bad); output << "{ invalid scene"; }
            bool rejected = false;
            try { operations.load(scene, bad); }
            catch (const std::exception& e) { rejected = *e.what() != 0; }
            expect(rejected && operations.selection() == 2, "Failed load reports error and preserves selection");
            engine::ScenePersistence::save(scene, copy);
            expect(read(copy) == snapshot, "Failed load preserves all active values and allocator");
            expect(operations.create(scene) == 5, "New ID does not reuse deleted or loaded identities");
        }
        std::cout << "Editor workflow phase " << argv[1] << " passed.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
