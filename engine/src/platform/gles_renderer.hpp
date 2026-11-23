#pragma once
#include "global_engine/world.hpp"
#include <GLES3/gl3.h>
#include <cstdint>
#include <vector>

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
    GLuint static_vertex_buffer_{0};
    GLuint dynamic_vertex_buffer_{0};
    GLint mvp_location_{-1};
    GLsizei static_vertex_count_{0};
    std::uint64_t uploaded_static_revision_{0};
    int width_{1};
    int height_{1};
    std::vector<Box> dynamic_boxes_;
    std::vector<float> static_vertices_;
    std::vector<float> dynamic_vertices_;
};
} // namespace global_engine::platform
