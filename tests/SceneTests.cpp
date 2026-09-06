#include <engine/Scene.hpp>
#include <engine/Camera.hpp>
#include <numbers>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <set>
#include <type_traits>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}
bool near(float a, float b) { return std::abs(a - b) < 0.0001f; }
}
int main() {
    try {
        engine::GameObject object{"Test", {3, 4, 5}, {2, 3, 4}};
        const engine::Vec3 transformed = engine::transformPoint(object.worldMatrix(), {1, 1, 1});
        expect(near(transformed.x, 5) && near(transformed.y, 7) && near(transformed.z, 9), "Scale must precede translation");
        object.scale.x = 0;
        bool rejected = false;
        try { (void)object.worldMatrix(); } catch (const std::invalid_argument&) { rejected = true; }
        expect(rejected, "Zero scale must be rejected before inverse normal transform");

        const auto projection = engine::perspective(std::numbers::pi_v<float>/3, 2, 0.1f, 100);
        expect(near(engine::transformPoint(projection, {0,0,-0.1f}).z, -1), "OpenGL near plane must map to -1");
        expect(near(engine::transformPoint(projection, {0,0,-100}).z, 1), "OpenGL far plane must map to +1");
        expect(near(projection.values[5]/projection.values[0], 2), "Projection must account for aspect ratio");
        const auto view = engine::lookAt({0,0,5}, {0,0,0});
        expect(near(engine::transformPoint(view, {0,0,0}).z, -5), "Visible points must be in front of the RH camera");
        const auto combined = projection * view * engine::Mat4::transform({0,0,0}, {2,3,4});
        const auto center = engine::transformPoint(combined, {0,0,0});
        expect(near(center.x,0) && near(center.y,0) && center.z > -1 && center.z < 1, "MVP composition must keep the origin visible");
        engine::Camera camera;
        camera.setTarget({3,2,7});
        const auto target = engine::transformPoint(camera.viewProjection(16.0f/9), {3,2,7});
        expect(near(target.x,0) && near(target.y,0), "Camera must center its target after following the player");

        engine::Scene scene;
        auto& first = scene.addCube("First");
        for (int i = 0; i < 1000; ++i) { scene.addCube("Additional"); }
        first.position.x = 9;
        expect(near(scene.cubes.front().position.x, 9), "Adding cubes must preserve existing references");

        const auto firstId = first.id;
        std::set<engine::CubeId> ids;
        for (const auto& cube : scene.cubes) {
            expect(cube.id != engine::invalidCubeId && ids.insert(cube.id).second,
                "All cubes need unique nonzero IDs, including duplicate names");
        }
        expect(scene.findCube(firstId) == &first, "Mutable lookup resolves original cube after appends");
        const engine::Scene& constScene = scene;
        static_assert(std::is_same_v<decltype(constScene.findCube(firstId)), const engine::GameObject*>);
        expect(constScene.findCube(firstId) == &first, "Const lookup resolves the same cube");
        const auto removedId = scene.cubes[500].id;
        const auto lastId = scene.cubes.back().id;
        expect(scene.removeCube(removedId), "Middle cube deletion succeeds");
        // Do not use first or other borrowed references after deque erasure.
        expect(scene.findCube(removedId) == nullptr && !scene.removeCube(removedId), "Deleted ID is safely missing");
        expect(scene.findCube(firstId)->id == firstId && scene.findCube(lastId)->id == lastId,
            "Survivor IDs remain stable across deletion");
        expect(scene.findCube(0) == nullptr && constScene.findCube(0) == nullptr && !scene.removeCube(0),
            "Invalid ID safely fails lookup and deletion");
        const auto unknown = std::numeric_limits<engine::CubeId>::max();
        expect(scene.findCube(unknown) == nullptr && constScene.findCube(unknown) == nullptr && !scene.removeCube(unknown),
            "Unknown ID safely fails lookup and deletion");
        const auto newId = scene.addCube("Additional").id;
        expect(newId > lastId, "Creation does not recycle deleted IDs");
        expect(scene.removeCube(newId), "Highest ID can be removed");
        engine::Scene copied = scene;
        copied.findCube(firstId)->position.x = 42;
        expect(near(scene.findCube(firstId)->position.x, 9), "Copy owns independent object data");
        expect(copied.removeCube(firstId) && scene.findCube(firstId), "Copy deletion leaves source intact");
        expect(copied.addCube("Copy").id > newId, "Copy preserves allocation watermark after highest deletion");
        engine::Scene assigned;
        assigned.addCube("Replaced");
        assigned = scene;
        expect(assigned.findCube(lastId) && assigned.addCube("Assigned").id > newId,
            "Copy assignment preserves IDs and allocation watermark");
        expect(scene.removeCube(firstId) && scene.removeCube(lastId), "First and last original IDs can be deleted");

        engine::Scene loaded;
        expect(loaded.restoreCube(10, "Duplicate").id == 10, "Loading restores an explicit ID");
        expect(loaded.restoreCube(50, "Duplicate").id == 50 && loaded.addCube("New").id == 51,
            "Restoration advances normal allocation past loaded IDs");
        expect(loaded.removeCube(50), "Restored cube can be deleted");
        for (engine::CubeId invalid : {engine::CubeId{0}, engine::CubeId{10}, engine::CubeId{50}}) {
            bool rejectedId = false;
            try { loaded.restoreCube(invalid, "Invalid"); }
            catch (const std::invalid_argument&) { rejectedId = true; }
            expect(rejectedId, "Restoration rejects invalid, duplicate, and retired IDs");
        }
        expect(loaded.addCube("After rejection").id == 52, "Rejected restore leaves allocator unchanged");
        engine::Scene exhausted;
        exhausted.restoreCube(unknown, "Last possible ID");
        exhausted.removeCube(unknown);
        bool exhaustedId = false;
        try { exhausted.addCube("Overflow"); }
        catch (const std::overflow_error&) { exhaustedId = true; }
        expect(exhaustedId && exhausted.cubes.empty(), "Exhaustion never wraps or reuses deleted IDs");
        engine::GameObject standalone;
        expect(standalone.id == engine::invalidCubeId && scene.player.object.id == engine::invalidCubeId,
            "Standalone objects and player remain outside scene cube identity");

        engine::Player straight, diagonal, subdivided;
        straight.move(1, 0, 0, 1);
        diagonal.move(1, 1, 0, 1);
        for (int i = 0; i < 100; ++i) { subdivided.move(1, 0, 0, 0.01f); }
        const auto& p = diagonal.object.position;
        expect(near(straight.object.position.x, 4), "Movement speed should use seconds");
        expect(near(std::sqrt(p.x*p.x + p.z*p.z), 4), "Diagonal movement must not be faster");
        expect(near(subdivided.object.position.x, straight.object.position.x), "Movement must be frame-rate independent");
        expect(near(p.y, -0.25f), "Movement must preserve player height");
        straight.move(-1, 0, 0, 1);
        straight.move(0, 1, 0, 0.5f);
        straight.move(0, -1, 0, 0.5f);
        expect(near(straight.object.position.x, 0) && near(straight.object.position.z, 0), "Opposing directions must cancel");
        straight.move(0, 0, 0, 10);
        expect(near(straight.object.position.x, 0), "No input must not move the player");
        std::cout << "Scene and player checks passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
