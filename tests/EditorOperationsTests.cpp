#include "Operations.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

void expect(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    try {
        engine::Scene scene;
        editor::Operations operations;
        std::string error;
        const auto first = operations.create(scene), second = operations.create(scene);
        expect(first != second && operations.selection() == second, "Creation selects unique IDs");
        expect(scene.findCube(first)->name == scene.findCube(second)->name, "Duplicate names allowed");
        operations.select(scene, first);
        editor::CubeProperties properties(*scene.findCube(first));
        properties.name = "Renamed##literal";
        properties.position = {2, 3, 4}; properties.scale = {2, 1, 3}; properties.color = {1, 0, 0};
        expect(operations.update(scene, first, properties, error), "Valid properties commit");
        expect(scene.findCube(first)->position.x == 2 && scene.findCube(second)->position.x == 0
            && scene.findCube(second)->name == "Cube", "Only the selected identity changes");
        expect(!scene.findCube(first)->boxCollider, "Colliders are opt-in");
        properties.collider.emplace(); properties.collider->offset = {1, 0, 0};
        expect(operations.update(scene, first, properties, error), "Attach collider");
        expect(engine::worldAabb(*scene.findCube(first))->center.x == 4, "Edited collider uses scaled offset");
        properties.collider->enabled = false;
        expect(operations.update(scene, first, properties, error) && scene.findCube(first)->boxCollider
            && !engine::worldAabb(*scene.findCube(first)), "Disabled is distinct from absent");
        const auto valid = properties;
        const auto reject = [&](const editor::CubeProperties& invalid) {
            expect(!operations.update(scene, first, invalid, error) && !error.empty(), "Invalid edit has feedback");
            const auto* cube = scene.findCube(first);
            expect(cube->name == valid.name && cube->position.x == valid.position.x
                && cube->scale.x == valid.scale.x && cube->color.x == valid.color.x
                && cube->boxCollider->halfExtents.x == 1 && !cube->boxCollider->enabled,
                "Rejected edit is atomic");
            (void)cube->worldMatrix();
        };
        for (float invalid : {0.0f, -1.0f, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN()}) {
            properties = valid; properties.scale.x = invalid; properties.name = "Must not commit"; reject(properties);
            properties = valid; properties.collider->halfExtents.x = invalid; reject(properties);
        }
        properties = valid; properties.position.x = std::numeric_limits<float>::infinity(); reject(properties);
        properties = valid; properties.collider->offset.x = std::numeric_limits<float>::quiet_NaN(); reject(properties);
        properties = valid; properties.color.x = std::numeric_limits<float>::quiet_NaN(); reject(properties);
        properties = valid; properties.color.x = 2; reject(properties);
        properties = valid; properties.scale.x = std::numeric_limits<float>::max(); reject(properties);
        properties = valid; properties.collider->enabled = true;
        expect(operations.update(scene, first, properties, error) && error.empty(), "Re-enable clears feedback");
        properties.collider.reset();
        expect(operations.update(scene, first, properties, error) && !scene.findCube(first)->boxCollider, "Remove collider");
        expect(operations.remove(scene, second) && operations.selection() == first, "Other deletion preserves selection");
        expect(operations.remove(scene, first) && operations.selection() == engine::invalidCubeId, "Selected deletion clears selection");
        expect(!operations.update(scene, first, properties, error), "Deleted ID edits fail safely");
        const auto third = operations.create(scene);
        expect(third != first && third != second, "Deleted IDs never reused");
        scene.removeCube(third); operations.reconcile(scene);
        expect(operations.selection() == engine::invalidCubeId, "External deletion reconciles selection");
        std::cout << "Editor mutation and validation tests passed.\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
