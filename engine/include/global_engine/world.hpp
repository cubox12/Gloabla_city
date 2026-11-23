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

struct Box {
    Vec3 center;
    Vec3 half_extent;
    Color color;
    bool solid{false};
};

struct InputState { float move_x{0.0f}, move_z{0.0f}; };

class CityWorld {
public:
    CityWorld();
    void reset();
    void update(float delta_seconds, const InputState& input);

    // Static geometry is immutable between resets and can remain resident on the GPU.
    const std::vector<Box>& static_draw_boxes() const noexcept { return static_boxes_; }
    std::uint64_t static_geometry_revision() const noexcept { return static_geometry_revision_; }

    // Dynamic geometry is regenerated into caller-owned storage to reuse its capacity.
    void gather_dynamic_draw_boxes(std::vector<Box>& output) const;
    void gather_draw_boxes(std::vector<Box>& output) const;

    const Vec3& player_position() const noexcept { return player_position_; }
    std::size_t static_box_count() const noexcept { return static_boxes_.size(); }
    std::size_t building_count() const noexcept { return building_count_; }
    bool player_intersects_building() const noexcept;

private:
    bool collides_with_building(const Vec3& candidate) const noexcept;
    void add_box(const Vec3& center, const Vec3& half_extent,
                 const Color& color, bool solid = false);

    std::vector<Box> static_boxes_;
    std::vector<Box> buildings_;
    Vec3 player_position_{0.0f, 0.0f, 0.0f};
    Vec3 player_velocity_{0.0f, 0.0f, 0.0f};
    float npc_elapsed_{0.0f};
    std::size_t building_count_{0};
    std::uint64_t static_geometry_revision_{0};
};

} // namespace global_engine
