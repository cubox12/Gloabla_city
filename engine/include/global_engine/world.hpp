#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace global_engine {
struct Vec3 {
    float x{0.0f}, y{0.0f}, z{0.0f};
    Vec3 operator+(const Vec3& rhs) const noexcept;
    Vec3 operator-(const Vec3& rhs) const noexcept;
    Vec3 operator*(float scalar) const noexcept;
};
struct Color { float r{1.0f}, g{1.0f}, b{1.0f}, a{1.0f}; };
struct Box { Vec3 center; Vec3 half_extent; Color color; bool solid{false}; };
struct InputState { float move_x{0.0f}, move_z{0.0f}; };

enum class MissionState : std::uint8_t { InProgress, Completed };
enum class MissionStage : std::uint8_t { FindVehicle, DeliverPackage, Completed };

struct WorldChunk {
    int x{0}, z{0};
    float center_x{0.0f}, center_z{0.0f}, bounds_radius{48.0f};
    std::vector<Box> high_detail_boxes;
    std::vector<Box> low_detail_boxes;
    std::vector<Box> collision_boxes;
};
struct StreamingStats {
    std::size_t active_chunks{0};
    std::size_t resident_boxes{0};
    std::uint64_t generated_chunks{0};
    std::uint64_t evicted_chunks{0};
    int world_chunks_per_axis{64};
    int active_radius{2};
};

class CityWorld {
public:
    CityWorld();
    void reset();
    void update(float delta_seconds, const InputState& input);
    const std::vector<WorldChunk>& active_chunks() const noexcept { return active_chunks_; }
    const StreamingStats& streaming_stats() const noexcept { return streaming_stats_; }
    std::uint64_t static_geometry_revision() const noexcept { return static_geometry_revision_; }
    void gather_dynamic_draw_boxes(std::vector<Box>& output) const;
    void gather_draw_boxes(std::vector<Box>& output) const;
    const Vec3& player_position() const noexcept { return player_position_; }
    std::size_t static_box_count() const noexcept { return streaming_stats_.resident_boxes; }
    std::size_t building_count() const noexcept { return active_building_count_; }
    bool player_intersects_building() const noexcept;
    MissionState mission_state() const noexcept { return mission_state_; }
    MissionStage mission_stage() const noexcept { return mission_stage_; }
    bool mission_completed() const noexcept { return mission_state_ == MissionState::Completed; }
    const Vec3& mission_target() const noexcept { return mission_target_; }
    float mission_distance() const noexcept { return mission_distance_; }
private:
    WorldChunk generate_chunk(int chunk_x, int chunk_z) const;
    void sync_chunks();
    bool collides_with_building(const Vec3& candidate) const noexcept;
    void append_dynamic_draw_boxes(std::vector<Box>& output) const;
    std::vector<WorldChunk> active_chunks_;
    int stream_center_x_{0},stream_center_z_{0};
    bool stream_center_initialized_{false};
    Vec3 player_position_{};
    Vec3 player_velocity_{};
    Vec3 player_facing_{0.0f,0.0f,-1.0f};
    float player_animation_phase_{0.0f};
    Vec3 vehicle_position_{96.0f, 0.0f, -6.0f};
    Vec3 mission_target_{96.0f, 0.0f, -6.0f};
    float mission_distance_{96.0f};
    MissionState mission_state_{MissionState::InProgress};
    MissionStage mission_stage_{MissionStage::FindVehicle};
    float npc_elapsed_{0.0f};
    std::size_t active_building_count_{0};
    std::uint64_t static_geometry_revision_{0};
    StreamingStats streaming_stats_{};
};
} // namespace global_engine
