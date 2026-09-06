#pragma once
#include <engine/Scene.hpp>

namespace engine {
class Engine {
public:
    Scene scene;
    // A nonzero frame limit supports automated startup/render/shutdown checks.
    void run(unsigned int frameLimit = 0);
};
}
