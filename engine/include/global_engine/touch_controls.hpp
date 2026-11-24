#pragma once
#include <algorithm>
#include <cmath>

namespace global_engine {
struct VirtualStickLayout {
    float center_x{0.0f};
    float center_y{0.0f};
    float radius{80.0f};
};

struct VirtualStickOutput {
    float move_x{0.0f};
    float move_z{0.0f};
    float knob_x{0.0f};
    float knob_y{0.0f};
    float magnitude{0.0f};
};

inline VirtualStickLayout virtual_stick_layout(int width, int height) noexcept {
    const float w = static_cast<float>(std::max(1, width));
    const float h = static_cast<float>(std::max(1, height));
    const float short_side = std::min(w, h);
    const float radius = std::max(76.0f, std::min(102.0f, short_side * 0.10f));
    return {w * 0.16f, h * 0.78f, radius};
}

inline bool is_virtual_stick_touch(float x, float y, int width, int height) noexcept {
    const VirtualStickLayout layout = virtual_stick_layout(width, height);
    const float dx = x - layout.center_x;
    const float dy = y - layout.center_y;
    const float hit_radius = layout.radius * 1.55f;
    return dx * dx + dy * dy <= hit_radius * hit_radius;
}

inline VirtualStickOutput map_virtual_stick(float x, float y,
                                             const VirtualStickLayout& layout) noexcept {
    constexpr float deadzone = 0.12f;
    const float dx = x - layout.center_x;
    const float dy = y - layout.center_y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    if (distance <= 1.0e-4f || layout.radius <= 1.0f) return {};

    const float raw_magnitude = std::min(1.0f, distance / layout.radius);
    const float direction_x = dx / distance;
    const float direction_y = dy / distance;
    const float strength = raw_magnitude <= deadzone ? 0.0f :
        (raw_magnitude - deadzone) / (1.0f - deadzone);

    VirtualStickOutput out;
    out.move_x = direction_x * strength;
    // World forward is -Z (camera starts behind the player on +Z).
    out.move_z = direction_y * strength;
    out.knob_x = direction_x * raw_magnitude;
    out.knob_y = direction_y * raw_magnitude;
    out.magnitude = raw_magnitude;
    return out;
}
struct WorldMovement {
    float move_x{0.0f};
    float move_z{0.0f};
};

// Convert screen-relative stick input into world axes using the camera heading.
// move_z < 0 means forward; camera yaw defines the view direction.
inline WorldMovement camera_relative_movement(float move_x, float move_z,
                                               float camera_yaw) noexcept {
    const float c = std::cos(camera_yaw);
    const float s = std::sin(camera_yaw);
    return {c * move_x + s * move_z, -s * move_x + c * move_z};
}
} // namespace global_engine
