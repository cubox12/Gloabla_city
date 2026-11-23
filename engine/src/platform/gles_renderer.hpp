#pragma once
#include "global_engine/world.hpp"
#include <GLES3/gl3.h>
#include <vector>
namespace global_engine::platform {
struct CameraState { float yaw{0.72f}; float pitch{0.58f}; float distance{78.0f}; };
class GlesRenderer {
public:
    bool initialize();
    void resize(int width,int height);
    void render(const CityWorld&world,const CameraState&camera);
    void shutdown();
private:
    struct GpuChunk {
        int x{0},z{0}; GLuint high_buffer{0},low_buffer{0};
        GLsizei high_count{0},low_count{0};
    };
    void upload_chunk(const WorldChunk&source,GpuChunk&target);
    GLuint program_{0},dynamic_vertex_buffer_{0};
    GLint mvp_location_{-1};
    int width_{1},height_{1};
    std::vector<GpuChunk> gpu_chunks_;
    std::vector<Box> dynamic_boxes_;
    std::vector<float> dynamic_vertices_;
};
} // namespace global_engine::platform
