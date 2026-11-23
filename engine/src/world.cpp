#include "global_engine/world.hpp"

#include <algorithm>
#include <cmath>

namespace global_engine {
namespace {
constexpr float kWorldHalfSize = 70.0f;
constexpr float kRoadSpacing = 34.0f;
constexpr float kPlayerRadius = 0.36f;
constexpr float kPlayerHeight = 1.8f;
constexpr float kMoveSpeed = 6.5f;
constexpr float kAcceleration = 22.0f;

float clamp_input(float v) noexcept { return std::max(-1.0f, std::min(1.0f, v)); }
bool overlap(float a0, float a1, float b0, float b1) noexcept {
    return a0 < b1 && a1 > b0;
}
} // namespace

Vec3 Vec3::operator+(const Vec3& r) const noexcept { return {x+r.x,y+r.y,z+r.z}; }
Vec3 Vec3::operator-(const Vec3& r) const noexcept { return {x-r.x,y-r.y,z-r.z}; }
Vec3 Vec3::operator*(float s) const noexcept { return {x*s,y*s,z*s}; }

CityWorld::CityWorld() { reset(); }

void CityWorld::add_box(const Vec3& c, const Vec3& h, const Color& color, bool solid) {
    static_boxes_.push_back({c,h,color,solid});
}

void CityWorld::reset() {
    ++static_geometry_revision_;
    if (static_geometry_revision_ == 0) ++static_geometry_revision_;
    static_boxes_.clear();
    buildings_.clear();
    player_position_ = {};
    player_velocity_ = {};
    npc_elapsed_ = 0.0f;
    building_count_ = 0;

    add_box({0,-0.18f,0},{kWorldHalfSize,0.18f,kWorldHalfSize},{0.34f,0.48f,0.30f,1});
    for (int i=-2;i<=2;++i) {
        const float offset = static_cast<float>(i)*kRoadSpacing;
        add_box({offset,0.015f,0},{5,0.035f,kWorldHalfSize},{0.20f,0.22f,0.23f,1});
        add_box({0,0.016f,offset},{kWorldHalfSize,0.035f,5},{0.20f,0.22f,0.23f,1});
        for (float side : {-1.0f,1.0f}) {
            const float sidewalk = offset + side*6.35f;
            add_box({sidewalk,0.055f,0},{1.1f,0.07f,kWorldHalfSize},{0.66f,0.66f,0.61f,1});
            add_box({0,0.056f,sidewalk},{kWorldHalfSize,0.07f,1.1f},{0.66f,0.66f,0.61f,1});
        }
    }

    const Color facades[] = {
        {0.78f,0.68f,0.53f,1},{0.56f,0.68f,0.70f,1},{0.75f,0.49f,0.37f,1}
    };
    int lot=0;
    for (int xi=-2;xi<2;++xi) for (int zi=-2;zi<2;++zi) {
        const float x=(static_cast<float>(xi)+0.5f)*kRoadSpacing;
        const float z=(static_cast<float>(zi)+0.5f)*kRoadSpacing;
        const float w=5.75f+static_cast<float>((lot*3)%4)*0.6f;
        const float d=5.5f+static_cast<float>((lot*5)%3)*0.65f;
        const float h=7.5f+static_cast<float>((lot*7)%5)*1.45f;
        Box b{{x,h*0.5f,z},{w,h*0.5f,d},facades[lot%3],true};
        buildings_.push_back(b);
        static_boxes_.push_back(b);
        ++building_count_;
        add_box({x,h+0.14f,z},{w+0.18f,0.14f,d+0.18f},{0.39f,0.42f,0.40f,1});

        const float front=z-d-0.055f;
        for (int floor=0;floor<static_cast<int>(h/3.0f);++floor) {
            const float y=2.0f+static_cast<float>(floor)*2.5f;
            if (y>h-1.0f) continue;
            for (int col=0;col<3;++col) {
                const float wx=x+(static_cast<float>(col)-1.0f)*w*0.27f;
                add_box({wx,y,front},{0.58f,0.62f,0.055f},{0.23f,0.43f,0.53f,1});
            }
        }
        add_box({x,1.05f,front-0.015f},{0.68f,1.05f,0.065f},{0.25f,0.29f,0.28f,1});
        ++lot;
    }

    for (int side : {-1,1}) for (int segment=-2;segment<=2;++segment) {
        const float z=static_cast<float>(segment)*kRoadSpacing+10.0f;
        const float x=static_cast<float>(side)*8.0f;
        add_box({x,0.85f,z},{0.20f,0.85f,0.20f},{0.35f,0.25f,0.16f,1});
        add_box({x,2.05f,z},{0.85f,0.85f,0.85f},{0.22f,0.43f,0.22f,1});
        add_box({x-1,2,z+7},{0.08f,2,0.08f},{0.24f,0.26f,0.27f,1});
        add_box({x-1,4,z+7},{0.32f,0.10f,0.32f},{0.88f,0.77f,0.43f,1});
    }
}

bool CityWorld::collides_with_building(const Vec3& p) const noexcept {
    for (const Box& b : buildings_) {
        if (!overlap(p.x-kPlayerRadius,p.x+kPlayerRadius,b.center.x-b.half_extent.x,b.center.x+b.half_extent.x)) continue;
        if (!overlap(p.z-kPlayerRadius,p.z+kPlayerRadius,b.center.z-b.half_extent.z,b.center.z+b.half_extent.z)) continue;
        if (overlap(p.y,p.y+kPlayerHeight,b.center.y-b.half_extent.y,b.center.y+b.half_extent.y)) return true;
    }
    return false;
}

void CityWorld::update(float delta, const InputState& input) {
    const float dt=std::max(0.0f,std::min(delta,0.05f));
    npc_elapsed_+=dt;
    float ix=clamp_input(input.move_x), iz=clamp_input(input.move_z);
    const float length=std::sqrt(ix*ix+iz*iz);
    if (length>1.0f) { ix/=length; iz/=length; }
    const float max_delta=kAcceleration*dt;
    const auto approach=[max_delta](float v,float target) {
        return v+std::max(-max_delta,std::min(max_delta,target-v));
    };
    player_velocity_.x=approach(player_velocity_.x,ix*kMoveSpeed);
    player_velocity_.z=approach(player_velocity_.z,iz*kMoveSpeed);

    Vec3 next=player_position_;
    next.x=std::max(-kWorldHalfSize+kPlayerRadius,
                    std::min(kWorldHalfSize-kPlayerRadius,next.x+player_velocity_.x*dt));
    if (!collides_with_building(next)) player_position_.x=next.x;
    else player_velocity_.x=0;

    next=player_position_;
    next.z=std::max(-kWorldHalfSize+kPlayerRadius,
                    std::min(kWorldHalfSize-kPlayerRadius,next.z+player_velocity_.z*dt));
    if (!collides_with_building(next)) player_position_.z=next.z;
    else player_velocity_.z=0;
}

void CityWorld::gather_dynamic_draw_boxes(std::vector<Box>& out) const {
    out.clear();
    out.reserve(12);
    out.push_back({{player_position_.x,1.02f,player_position_.z},
                   {0.34f,0.56f,0.24f},{0.15f,0.42f,0.67f,1},false});
    out.push_back({{player_position_.x,1.78f,player_position_.z},
                   {0.22f,0.22f,0.22f},{0.78f,0.57f,0.42f,1},false});
    for (int i=0;i<5;++i) {
        const float phase=npc_elapsed_*0.18f+static_cast<float>(i)*1.25663706f;
        const float radius=23.0f+static_cast<float>(i%2)*3.0f;
        const float x=std::cos(phase)*radius, z=std::sin(phase)*radius;
        const Color shirt=(i%2==0)?Color{0.70f,0.35f,0.22f,1}:Color{0.15f,0.42f,0.67f,1};
        out.push_back({{x,0.92f,z},{0.24f,0.46f,0.24f},shirt,false});
        out.push_back({{x,1.55f,z},{0.18f,0.18f,0.18f},{0.78f,0.57f,0.42f,1},false});
    }
}

void CityWorld::gather_draw_boxes(std::vector<Box>& out) const {
    out = static_boxes_;
    std::vector<Box> dynamic_boxes;
    gather_dynamic_draw_boxes(dynamic_boxes);
    out.insert(out.end(), dynamic_boxes.begin(), dynamic_boxes.end());
}

bool CityWorld::player_intersects_building() const noexcept {
    return collides_with_building(player_position_);
}
} // namespace global_engine
