#pragma once
#include "global_engine/world.hpp"
#include <GLES3/gl3.h>
#include <vector>

namespace global_engine::platform {
struct CameraState {
    float yaw{-1.45f}; float pitch{0.46f}; float distance{14.0f};
    float joystick_x{0.0f},joystick_y{0.0f},move_x{0.0f},move_y{0.0f};
    float stick_knob_x{0.0f},stick_knob_y{0.0f},stick_magnitude{0.0f};
    bool joystick_active{false};
};
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
    void upload_chunk(const WorldChunk&source,GpuChunk&target,bool high_detail);
    void draw_hud(const CityWorld&world,const CameraState&camera);
    GLuint program_{0},shadow_program_{0};
    GLuint dynamic_vertex_buffer_{0},shadow_vertex_buffer_{0},hud_vertex_buffer_{0},control_vertex_buffer_{0};
    GLuint shadow_framebuffer_{0},shadow_depth_texture_{0};
    GLint mvp_location_{-1},view_location_{-1},light_mvp_location_{-1};
    GLint lighting_location_{-1},camera_position_location_{-1};
    GLint shadow_mvp_location_{-1},shadow_map_location_{-1},shadows_enabled_location_{-1};
    int width_{1},height_{1};
    int hud_cached_width_{0},hud_cached_height_{0},hud_cached_distance_{-1},hud_cached_stage_{-1};
    bool hud_cached_complete_{false};
    GLsizei hud_vertex_count_{0};
    std::vector<GpuChunk> gpu_chunks_;
    std::vector<Box> dynamic_boxes_;
    std::vector<float> dynamic_vertices_;
    std::vector<float> shadow_dynamic_vertices_;
    std::vector<float> hud_vertices_;
    std::vector<float> control_vertices_;
};
} // namespace global_engine::platform
