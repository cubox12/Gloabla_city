#pragma once
#include "global_engine/world.hpp"
#include <GLES3/gl3.h>

namespace global_engine::platform {
struct CameraState {
    float yaw{0.72f};
    float pitch{0.58f};
    float distance{78.0f};
};
class GlesRenderer {
public:
    bool initialize();
    void resize(int width, int height);
    void render(const CityWorld& world, const CameraState& camera);
    void shutdown();
private:
    GLuint program_{0};
    GLuint vertex_buffer_{0};
    GLint mvp_location_{-1};
    int width_{1};
    int height_{1};
};
} // namespace global_engine::platform
